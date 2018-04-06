#include "coreinterfaces.h"
#include "ascriptmanager.h"
#include "afiletools.h"

#include "TRandom2.h"

#include <QScriptEngine>
#include <QDateTime>
#include <QFileInfo>
#include <QFile>
#include <QDebug>
#include <QApplication>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

// ------------------- CORE ----------------------

AInterfaceToCore::AInterfaceToCore(AScriptManager* ScriptManager) :
    ScriptManager(ScriptManager)
{
  Description = "General-purpose opeartions: abort script, basic text output and file save/load";

  H["str"] = "Converts double value to string with a given precision";
  H["print"] = "Print the argument (string) on the script output text field";
  H["clearText"] = "Clear the script output text field";
  H["abort"] = "Abort skript execution.\nOptional string argument is a message to be shown on the script output text field";
  H["save"] = "Add string (second argument) to the file with the name given by the first argument.\n"
              "Save is not performed (and false is returned) if the file does not exist\n"
              "It is a very slow method!\n"
              "Use \"<br>\" or \"\\n\" to add a line break.\n"
              "For Windows users: pathes have to use \"/\" character, e.g. c:/tmp/file.txt\n";
  H["saveArray"] = "Appends an array (or array of arrays) with numeric data to the file.\n"
                   "Save is not performed (and false is returned) if the file does not exist.\n"
                   "For Windows users: pathes have to use \"/\" character, e.g. c:/tmp/file.txt\n";
  H["createFile"] = "Create new or clear an existent file.\n"
                    "The file is NOT rewritten if the second argument is true (or absent) and the file already exists\n"
                    "For Windows users: pathes have to use \"/\" character, e.g. c:/tmp/file.txt\n";
  H["isFileExists"] = "Return true if file exists";
  H["loadColumn"] = "Load a column with ascii numeric data from the file.\nSecond argument is the column number starting from 0.";
  H["loadArray"] = "Load an array of numerics (or an array of numeric arrays if columns>1).\nColumns parameter can be from 1 to 3.";
  H["evaluate"] = "Evaluate script during another script evaluation. See example ScriptInsideScript.txt";

  H["SetNewFileFinder"] = "Configurer for GetNewFiles() function. dir is the search directory, fileNamePattern: *.* for all files. Function return all filenames found.";
  H["GetNewFiles"] = "Get list (array) of names of new files appeared in the directory configured with SetNewFileFinder()";
}

AInterfaceToCore::AInterfaceToCore(const AInterfaceToCore &other) :
  AScriptInterface(other)
{
   ScriptManager = 0; //to be set after copy!!!
}

void AInterfaceToCore::abort(QString message)
{
  qDebug() << ">Core module: abort triggered!";
  ScriptManager->AbortEvaluation(message);
}

QVariant AInterfaceToCore::evaluate(QString script)
{
    return ScriptManager->EvaluateScriptInScript(script);
}

void AInterfaceToCore::sleep(int ms)
{
  if (ms == 0) return;
  QTime t;
  t.restart();
  do
    {
      qApp->processEvents();
      if (ScriptManager->isEvalAborted()) break;
    }
  while (t.elapsed()<ms);
}

int AInterfaceToCore::elapsedTimeInMilliseconds()
{
    return ScriptManager->getElapsedTime();
}

void AInterfaceToCore::print(QString text)
{
    emit ScriptManager->showMessage(text);
}

void AInterfaceToCore::clearText()
{
    emit ScriptManager->clearText();
}

QString AInterfaceToCore::str(double value, int precision)
{
    return QString::number(value, 'g', precision);
}

QString AInterfaceToCore::GetTimeStamp()
{
    return QDateTime::currentDateTime().toString("H:m:s");
}

QString AInterfaceToCore::GetDateTimeStamp()
{
    return QDateTime::currentDateTime().toString("d/M/yyyy H:m:s");
}

bool AInterfaceToCore::save(QString fileName, QString str)
{
  if (!QFileInfo(fileName).exists())
    {
      //abort("File does not exist: " + fileName);
      qDebug() << "File does not exist: " << fileName;
      return false;
    }

  QFile file(fileName);
  if ( !file.open(QIODevice::Append ) )
  {
      //abort("Cannot open file for appending:" + fileName);
      qDebug() << "Cannot open file for appending:" << fileName;
      return false;
  }

  //qDebug() << str;
  str.replace("<br>", "\n");
  QTextStream outstream(&file);
  outstream << str;
  return true;
}

bool AInterfaceToCore::saveArray(QString fileName, QVariant array)
{
    QString type = array.typeName();
    if (type != "QVariantList")
    {
        qDebug() << "Cannot extract array in saveColumns function";
        //abort("Cannot extract array in saveColumns function");
        return false;
    }

    if (!QFileInfo(fileName).exists())
      {
        //abort("File does not exist: " + fileName);
        qDebug() << "File does not exist: " << fileName;
        return false;
      }

    QFile file(fileName);
    if ( !file.open(QIODevice::Append ) )
    {
        //abort("Cannot open file for appending:" + fileName);
        qDebug() << "Cannot open file for appending:" << fileName;
        return false;
    }

    QTextStream s(&file);

    QVariantList vl = array.toList();
    QJsonArray ar = QJsonArray::fromVariantList(vl);
    //qDebug() << ar.size();

    for (int i=0; i<ar.size(); i++)
    {
        //qDebug() << ar[i];
        if (ar[i].isArray())
        {
            QJsonArray el = ar[i].toArray();
            //qDebug() << "   "<<el;
            for (int j=0; j<el.size(); j++) s << el[j].toDouble(1e10) << " ";
        }
        else
        {
            //qDebug() << "   "<< ar[i].toDouble();
            s << ar[i].toDouble(1e10);
        }
        s << "\n";
    }

    return true;
}

bool AInterfaceToCore::saveObject(QString FileName, QVariant Object, bool CanOverride)
{
    QString type = Object.typeName();
    if (type != "QVariantMap")
    {
        qDebug() << "Not an object - cannt use saveObject function";
        //abort("Cannot extract array in saveColumns function");
        return false;
    }

    if (QFileInfo(FileName).exists() && !CanOverride)
    {
        //abort("File already exists: " + fileName);
        qDebug() << "File already exists: " << FileName << " Skipping!";
        return false;
    }

    QVariantMap mp = Object.toMap();
    QJsonObject json = QJsonObject::fromVariantMap(mp);
    QJsonDocument saveDoc(json);

    QFile saveFile(FileName);
    if (saveFile.open(QIODevice::WriteOnly))
    {
        saveFile.write(saveDoc.toJson());
        saveFile.close();
    }
    else
    {
        qDebug() << "Cannot open file for writing: " << FileName;
        return false;
    }
    return true;
}

QVariant AInterfaceToCore::loadColumn(QString fileName, int column)
{
  if (column<0 || column>2)
    {
      abort ("Supported loadColumn with column # 0, 1 and 2");
      return QVariant();
    }

  if (!QFileInfo(fileName).exists())
  {
    //abort("File does not exist: " + fileName);
    qWarning() << "File does not exist: " << fileName;
    return QVariant();
  }

  QVector<double> v1, v2, v3;
  int res;
  if (column == 0)
     res = LoadDoubleVectorsFromFile(fileName, &v1);
  else if (column == 1)
     res = LoadDoubleVectorsFromFile(fileName, &v1, &v2);
  else if (column == 2)
     res = LoadDoubleVectorsFromFile(fileName, &v1, &v2, &v3);

  if (res != 0)
      {
        abort("Error reading from file: "+fileName);
        return QVariant();
      }
  QList<QVariant> l;
  for (int i=0; i<v1.size(); i++)
    {
      if (column == 0) l.append(v1[i]);
      else if (column == 1) l.append(v2[i]);
      else if (column == 2) l.append(v3[i]);
    }

  return l;
}

QVariant AInterfaceToCore::loadArray(QString fileName, int columns)
{
  if (columns<0 || columns>2)
    {
      abort ("Supported 1, 2 and 3 columns");
      return QVariant();
    }

  if (!QFileInfo(fileName).exists())
  {
    //abort("File does not exist: " + fileName);
    qWarning() << "File does not exist: " << fileName;
    return QVariant();
  }

  QVector<double> v1, v2, v3;
  int res;
  if (columns == 1)
     res = LoadDoubleVectorsFromFile(fileName, &v1);
  else if (columns == 2)
     res = LoadDoubleVectorsFromFile(fileName, &v1, &v2);
  else if (columns == 3)
     res = LoadDoubleVectorsFromFile(fileName, &v1, &v2, &v3);

  if (res != 0)
      {
        abort("Error reading from file: "+fileName);
        return QVariant();
      }

  QList< QVariant > l;
  for (int i=0; i<v1.size(); i++)
    {
      QList<QVariant> ll;
      ll.append(v1[i]);
      if (columns > 1) ll.append(v2[i]);
      if (columns == 3) ll.append(v3[i]);

      QVariant r = ll;
      l << r;
    }
  return l;
}

QString AInterfaceToCore::loadText(QString fileName)
{
  if (!QFileInfo(fileName).exists())
  {
    //abort("File does not exist: " + fileName);
    qWarning() << "File does not exist: " << fileName;
    return "";
  }

  QString str;
  bool bOK = LoadTextFromFile(fileName, str);
  if (!bOK)
    {
      qWarning() << "Error reading file: " << fileName;
      return "";
    }
  return str;
}

QString AInterfaceToCore::GetWorkDir()
{
  return ScriptManager->LastOpenDir;
}

QString AInterfaceToCore::GetScriptDir()
{
  return ScriptManager->LibScripts;
}

QString AInterfaceToCore::GetExamplesDir()
{
  return ScriptManager->ExamplesDir;
}

QVariant AInterfaceToCore::SetNewFileFinder(const QString dir, const QString fileNamePattern)
{
    Finder_Dir = dir;
    Finder_NamePattern = fileNamePattern;

    QDir d(dir);
    QStringList files = d.entryList( QStringList(fileNamePattern), QDir::Files);
    //  qDebug() << files;

    QVariantList res;
    for (auto& n : files)
    {
        Finder_FileNames << n;
        res << n;
    }
    return res;
}

QVariant AInterfaceToCore::GetNewFiles()
{
    QVariantList newFiles;
    QDir d(Finder_Dir);
    QStringList files = d.entryList( QStringList(Finder_NamePattern), QDir::Files);

    for (auto& n : files)
    {
        if (!Finder_FileNames.contains(n)) newFiles << QVariant(n);
        Finder_FileNames << n;
    }
    return newFiles;
}

bool AInterfaceToCore::createFile(QString fileName, bool AbortIfExists)
{
  if (QFileInfo(fileName).exists())
    {
      if (AbortIfExists)
        {
          //abort("File already exists: " + fileName);
          qDebug() << "File already exists: " << fileName;
          return false;
        }
    }

  //create or clear content of the file
  QFile file(fileName);
  if ( !file.open(QIODevice::WriteOnly) )
  {
      //abort("Cannot open file: "+fileName);
      qDebug() << "Cannot open file: " << fileName;
      return false;
  }
  return true;
}

bool AInterfaceToCore::isFileExists(QString fileName)
{
    return QFileInfo(fileName).exists();
}

bool AInterfaceToCore::deleteFile(QString fileName)
{
    return QFile(fileName).remove();
}

bool AInterfaceToCore::createDir(QString path)
{
    QDir dir(path);
    return dir.mkdir(".");
}

QString AInterfaceToCore::getCurrentDir()
{
    return QDir::currentPath();
}

bool AInterfaceToCore::setCirrentDir(QString path)
{
    return QDir::setCurrent(path);
}
// ------------- End of CORE --------------

// ------------- MATH ------------

AInterfaceToMath::AInterfaceToMath(TRandom2* RandGen)
{
  Description = "Expanded math module; implemented using std and CERN ROOT functions";

  //srand (time(NULL));
    this->RandGen = RandGen;

  H["random"] = "Returns a random number between 0 and 1.\nGenerator respects the seed set by SetSeed method of the sim module!";
  H["gauss"] = "Returns a random value sampled from Gaussian distribution with mean and sigma given by the user";
  H["poisson"] = "Returns a random value sampled from Poisson distribution with mean given by the user";
  H["maxwell"] = "Returns a random value sampled from maxwell distribution with Sqrt(kT/M) given by the user";
  H["exponential"] = "Returns a random value sampled from exponential decay with decay time given by the user";
}

void AInterfaceToMath::setRandomGen(TRandom2 *RandGen)
{
  this->RandGen = RandGen;
}

double AInterfaceToMath::abs(double val)
{
  return std::abs(val);
}

double AInterfaceToMath::acos(double val)
{
  return std::acos(val);
}

double AInterfaceToMath::asin(double val)
{
  return std::asin(val);
}

double AInterfaceToMath::atan(double val)
{
  return std::atan(val);
}

double AInterfaceToMath::atan2(double y, double x)
{
  return std::atan2(y, x);
}

double AInterfaceToMath::ceil(double val)
{
  return std::ceil(val);
}

double AInterfaceToMath::cos(double val)
{
  return std::cos(val);
}

double AInterfaceToMath::exp(double val)
{
  return std::exp(val);
}

double AInterfaceToMath::floor(double val)
{
  return std::floor(val);
}

double AInterfaceToMath::log(double val)
{
  return std::log(val);
}

double AInterfaceToMath::max(double val1, double val2)
{
  return std::max(val1, val2);
}

double AInterfaceToMath::min(double val1, double val2)
{
  return std::min(val1, val2);
}

double AInterfaceToMath::pow(double val, double power)
{
  return std::pow(val, power);
}

double AInterfaceToMath::sin(double val)
{
  return std::sin(val);
}

double AInterfaceToMath::sqrt(double val)
{
  return std::sqrt(val);
}

double AInterfaceToMath::tan(double val)
{
    return std::tan(val);
}

double AInterfaceToMath::round(double val)
{
  int f = std::floor(val);
  if (val>0)
    {
      if (val - f < 0.5) return f;
      else return f+1;
    }
  else
    {
      if (val - f < 0.5 ) return f;
      else return f+1;
    }
}

double AInterfaceToMath::random()
{
  //return rand()/(double)RAND_MAX;

  if (!RandGen) return 0;
  return RandGen->Rndm();
}

double AInterfaceToMath::gauss(double mean, double sigma)
{
  if (!RandGen) return 0;
  return RandGen->Gaus(mean, sigma);
}

double AInterfaceToMath::poisson(double mean)
{
  if (!RandGen) return 0;
  return RandGen->Poisson(mean);
}

double AInterfaceToMath::maxwell(double a)
{
  if (!RandGen) return 0;

  double v2 = 0;
  for (int i=0; i<3; i++)
    {
      double v = RandGen->Gaus(0, a);
      v *= v;
      v2 += v;
    }
  return std::sqrt(v2);
}

double AInterfaceToMath::exponential(double tau)
{
    if (!RandGen) return 0;
    return RandGen->Exp(tau);
}

// ------------- End of MATH -------------
