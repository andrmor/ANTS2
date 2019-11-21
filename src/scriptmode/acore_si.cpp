#include "acore_si.h"
#include "ascriptmanager.h"
#include "afiletools.h"

#ifdef USE_EIGEN
#include "curvefit.h"
#endif

#ifdef _ALLOW_LAUNCH_EXTERNAL_PROCESS_
#include <QProcess>
#endif

#include <QScriptEngine>
#include <QDateTime>
#include <QFileInfo>
#include <QFile>
#include <QDebug>
#include <QtWidgets/QApplication>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QThread>
#include <QRegularExpression>

ACore_SI::ACore_SI(AScriptManager* ScriptManager) :
    ScriptManager(ScriptManager)
{
  Description = "General-purpose opeartions: abort script, basic text output and file save/load";

  H["str"] = "Converts double value to string with a given precision";
  H["print"] = "Print the argument (string) on the script output text field";
  H["clearText"] = "Clear the script output text field";
  H["abort"] = "Abort script execution.\nOptional string argument is a message to be shown on the script output text field";
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
  H["loadArray"] = "Load an array of numerics (or an array of numeric arrays).\nSecond argument is used to limit the number of columns to read";
  H["evaluate"] = "Evaluate script during another script evaluation. See example ScriptInsideScript.txt";

  H["SetNewFileFinder"] = "Configurer for GetNewFiles() function. dir is the search directory, fileNamePattern: *.* for all files. Function return all filenames found.";
  H["GetNewFiles"] = "Get list (array) of names of new files appeared in the directory configured with SetNewFileFinder()";

  H["setCurveFitter"] = "Create splie fitter for curve y(x) in the range x from min to max and nInt nodes";
  H["getFitted"] = "Used with setCurveFitter. Extracts the fitted value of y for x";
  H["getFittedArr"] = "Used with setCurveFitter. Extracts the fitted values of y for an array of x";

  //DepRem["isFileExists"] = "Deprecated. Use file.isFileExists method";
  DepRem["str"] = "Deprecated. Use .toFixed(n) javaScript method. E.g.: 'var i=123.456; i.toFixed(2)'";
}

ACore_SI::ACore_SI(const ACore_SI &other) :
  AScriptInterface(other)
{
   ScriptManager = 0; //to be set after copy!!!
}

void ACore_SI::abort(QString message)
{
  qDebug() << ">Core module: abort triggered!";
  ScriptManager->AbortEvaluation(message);
}

QVariant ACore_SI::evaluate(QString script)
{
    return ScriptManager->EvaluateScriptInScript(script);
}

void ACore_SI::sleep(int ms)
{
  if (ms == 0) return;
  QTime t;
  t.restart();
  do
    {
      QThread::usleep(100);
      qApp->processEvents();
      if (ScriptManager->isEvalAborted()) break;
    }
  while (t.elapsed()<ms);
}

int ACore_SI::elapsedTimeInMilliseconds()
{
    return ScriptManager->getElapsedTime();
}

/*
void ACore_SI::print(QString text)
{
    emit ScriptManager->showMessage(text);
}
*/

void ACore_SI::addQVariantToString(const QVariant & var, QString & string) const
{
    switch (var.type())
    {
    case QVariant::Map:
      {
        string += '{';
        const QMap<QString, QVariant> map = var.toMap();
        for (const QString & k : map.keys())
        {
            string += QString("\"%1\":").arg(k);
            addQVariantToString(map.value(k), string);
            string += ", ";
        }
        if (string.endsWith(", ")) string.chop(2);
        string += '}';
        break;
      }
    case QVariant::List:
        string += '[';
        for (const QVariant & v : var.toList())
        {
            addQVariantToString(v, string);
            string += ", ";
        }
        if (string.endsWith(", ")) string.chop(2);
        string += ']';
        break;
    default:
        // implicit convertion to string
        string += var.toString();
    }
}

void ACore_SI::print(QVariant message)
{
    QString s;
    addQVariantToString(message, s);
    emit ScriptManager->showPlainTextMessage(s);
}

void ACore_SI::printHTML(QVariant message)
{
    QString s;
    addQVariantToString(message, s);
    emit ScriptManager->showMessage(s);
}

void ACore_SI::clearText()
{
    emit ScriptManager->clearText();
}

QString ACore_SI::str(double value, int precision)
{
    return QString::number(value, 'g', precision);
}

bool ACore_SI::strIncludes(QString str, QString pattern)
{
    return (str.indexOf(pattern) >= 0);
}

QString ACore_SI::GetTimeStamp()
{
    return QDateTime::currentDateTime().toString("H:m:s");
}

QString ACore_SI::GetDateTimeStamp()
{
    return QDateTime::currentDateTime().toString("d/M/yyyy H:m:s");
}

bool ACore_SI::save(QString fileName, QString str)
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

bool ACore_SI::saveArray(QString fileName, QVariant array)
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

bool ACore_SI::saveObject(QString FileName, QVariant Object, bool CanOverride)
{
    QString type = Object.typeName();
    if (type != "QVariantMap")
    {
        qDebug() << "Not an object - cannot use saveObject function";
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

QVariant ACore_SI::loadColumn(QString fileName, int column)
{
  QVector< QVector<double>* > vec;
  for (int i=0; i<column+1; i++)
      vec << new QVector<double>();

  QVariantList l;

  QString err = LoadDoubleVectorsFromFile(fileName, vec);
  if (err.isEmpty())
  {
      for (int i=0; i < vec.at(column)->size(); i++)
          l << vec.at(column)->at(i);
  }

  for (QVector<double>* v : vec) delete v;

  if (!err.isEmpty()) abort(err);

  return l;


    /*
  if (column<0 || column>2)
    {
      abort ("Supported loadColumn with column # 0, 1 and 2");
      return QVariant();
    }

    if (!QFileInfo(fileName).exists())
    {
      abort("File does not exist: " + fileName);
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
  */
}

QVariant ACore_SI::loadArray(QString fileName, int columns)
{
    QVariantList l;
    if (columns == 0) return l;

    QVector< QVector<double>* > vec;
    for (int i=0; i<columns; i++)
        vec << new QVector<double>();

    QString err = LoadDoubleVectorsFromFile(fileName, vec);
    if (err.isEmpty())
    {
        for (int irow = 0; irow < vec.at(0)->size(); irow++)
        {
            QVariantList el;
            for (int icol = 0; icol < columns; icol++)
                el << vec.at(icol)->at(irow);
            l.push_back(el);
        }
    }

    for (QVector<double>* v : vec) delete v;

    if (!err.isEmpty()) abort(err);

    return l;

    /*
  if (columns<0 || columns>2)
    {
      abort ("Supported 1, 2 and 3 columns");
      return QVariant();
    }

  if (!QFileInfo(fileName).exists())
  {
    abort("File does not exist: " + fileName);
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
  */
}

QVariant ACore_SI::loadArray(QString fileName)
{
    if (!QFileInfo(fileName).exists())
    {
        abort("File does not exist: " + fileName);
        return QVariant();
    }

    QFile file(fileName);
    if(!file.open(QIODevice::ReadOnly | QFile::Text))
    {
        abort("Cannot open file: "+fileName);
        return QVariant();
    }

    QTextStream in(&file);
    QRegularExpression rx("(\\ |\\,|\\:|\\t)"); //separators: ' ' or ',' or ':' or '\t'

    QVariantList vl;

    while(!in.atEnd())
    {
        QString line = in.readLine();

        QStringList fields = line.split(rx, QString::SkipEmptyParts);

        if (fields.isEmpty()) continue;
        bool bOK;
        double first = fields.at(0).toDouble(&bOK);
        if (!bOK) continue;

        if (fields.size() == 1)
            vl.append(first);
        else
        {
            QVariantList el;
            el << first;
            for (int i=1; i<fields.size(); i++)
                el << fields.at(i).toDouble();
            vl.push_back(el);
        }
    }

    file.close();
    return vl;
}

enum AArrayFormatEnum {StringFormat, IntFormat, DoubleFormat};

QVariantList ACore_SI::loadArrayExtended(const QString & fileName, const QVariantList & format, bool bSkipComments, int fromLine, int toLine)
{
    QVariantList vl;

    QVector<AArrayFormatEnum> Format;

    int numEl = format.size();
    bool bErr = (numEl==0);
    for (int i=0; i<format.size(); i++)
    {
        QString f = format.at(i).toString();
        AArrayFormatEnum   Option;
        if      (f == "s") Option = StringFormat;
        else if (f == "i") Option = IntFormat;
        else if (f == "d") Option = DoubleFormat;
        else
        {
            bErr = true;
            break;
        }
        Format << Option;
    }
    if (bErr)
    {
        abort("'format' parameter should be an array of 's', 'i' or 'd' markers (string, int and double, respectively)");
        return vl;
    }

    if (!QFileInfo(fileName).exists())
    {
        abort("File does not exist: " + fileName);
        return vl;
    }

    QFile file(fileName);
    if(!file.open(QIODevice::ReadOnly | QFile::Text))
    {
        abort("Cannot open file: "+fileName);
        return vl;
    }

    QTextStream in(&file);
    QRegularExpression rx("(\\ |\\,|\\:|\\t)"); //separators: ' ' or ',' or ':' or '\t'

    int iLine = 0;
    while(!in.atEnd())
    {
        QString line = in.readLine();
        if (iLine < fromLine) continue;
        if (iLine > toLine)   break;

        QStringList fields = line.split(rx, QString::SkipEmptyParts);
        if (bSkipComments)
        {
            if (fields.isEmpty()) continue;
            const QString & first = fields.first();
            if (first.startsWith('#') || first.startsWith("//")) continue;
        }

        if (fields.size() < numEl) continue;

        QVariantList el;

        for (int i=0; i<fields.size(); i++)
        {
            const QString & txt = fields.at(i);

            switch (Format.at(i))
            {
            case StringFormat:
                el.push_back(txt);
                break;
            case IntFormat:
                el.push_back(txt.toInt());
                break;
            case DoubleFormat:
                el.push_back(txt.toDouble());
                break;
            }
        }
        vl.push_back(el);
    }

    file.close();
    return vl;
}

QString ACore_SI::loadText(QString fileName)
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

QVariant ACore_SI::loadObject(QString fileName)
{
    QFile loadFile(fileName);
    if (!loadFile.open(QIODevice::ReadOnly))
      {
        qWarning() << "Cannot open file " + fileName;
        return QVariant();
      }

    QByteArray saveData = loadFile.readAll();
    QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));
    QJsonObject json = loadDoc.object();

    QVariantMap v = json.toVariantMap();
    return v;
}

#include "ainternetbrowser.h"
QVariant ACore_SI::loadArrayFromWeb(QString url, int msTimeout)
{
    AInternetBrowser b(msTimeout);
    QString Reply;
    bool fOK = b.Post(url, "", Reply);
    //  qDebug() << "Post result:"<<fOK;

    if (!fOK)
    {
        abort("Error:\n" + b.GetLastError());
        return 0;
    }
    //  qDebug() << Reply;

    QRegularExpression rx("(\\ |\\,|\\:|\\t)"); //separators: ' ' or ',' or ':' or '\t'
    QVariantList vl;

    QStringList sl = Reply.split(QRegExp("[\r\n]"), QString::SkipEmptyParts);
    for (const QString& line : sl)
    {
        QStringList fields = line.split(rx, QString::SkipEmptyParts);

        if (fields.isEmpty()) continue;
        bool bOK;
        double first = fields.at(0).toDouble(&bOK);
        if (!bOK) continue;

        if (fields.size() == 1)
            vl.append(first);
        else
        {
            QVariantList el;
            el << first;
            for (int i=1; i<fields.size(); i++)
                el << fields.at(i).toDouble();
            vl.push_back(el);
        }
    }

    return vl;
}

QString ACore_SI::GetWorkDir()
{
    if (!ScriptManager->LastOpenDir) return QString();
    else return *ScriptManager->LastOpenDir;
}

QString ACore_SI::GetScriptDir()
{
    if (!ScriptManager->LibScripts) return QString();
    else return *ScriptManager->LibScripts;
}

QString ACore_SI::GetExamplesDir()
{
    if (!ScriptManager->ExamplesDir) return QString();
    else return *ScriptManager->ExamplesDir;
}

QVariant ACore_SI::SetNewFileFinder(const QString dir, const QString fileNamePattern)
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

QVariant ACore_SI::GetNewFiles()
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

void ACore_SI::processEvents()
{
    qApp->processEvents();
}

void ACore_SI::reportProgress(int percents)
{
    emit ScriptManager->reportProgress(percents);
    qApp->processEvents();
}

void ACore_SI::setCurveFitter(double min, double max, int nInt, QVariant x, QVariant y)
{
#ifdef USE_EIGEN
    QVariantList vlX = x.toList();
    QVariantList vlY = y.toList();

    QVector<double> xx, yy;
    for (int i=0; i<vlX.size() && i<vlY.size(); i++)
    {
        xx << vlX.at(i).toDouble();
        yy << vlY.at(i).toDouble();
    }

    CurF = new CurveFit(min, max, nInt, xx, yy);
#else
    abort("CurveFitter is supported only if ANTS2 is compliled with Eigen library enabled");
#endif
}

double ACore_SI::getFitted(double x)
{
#ifdef USE_EIGEN
    if (!CurF) return 0;

    return CurF->eval(x);
#else
    abort("CurveFitter is supported only if ANTS2 is compliled with Eigen library enabled");
    return 0;
#endif
}

const QVariant ACore_SI::getFittedArr(const QVariant array)
{
#ifdef USE_EIGEN
    if (!CurF) return 0;

    QVariantList vl = array.toList();
    QVariantList res;
    for (int i=0; i<vl.size(); i++)
        res << CurF->eval( vl.at(i).toDouble() );

    return res;
#else
    abort("CurveFitter is supported only if ANTS2 is compliled with Eigen library enabled");
    return 0;
#endif
}

bool ACore_SI::createFile(QString fileName, bool AbortIfExists)
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

bool ACore_SI::isFileExists(QString fileName)
{
    return QFileInfo(fileName).exists();
}

bool ACore_SI::deleteFile(QString fileName)
{
    return QFile(fileName).remove();
}

bool ACore_SI::createDir(QString path)
{
    //QDir dir(path);
    //return dir.mkdir(".");
    QDir dir("");
    return dir.mkpath(path);
}

QString ACore_SI::getCurrentDir()
{
    return QDir::currentPath();
}

bool ACore_SI::setCirrentDir(QString path)
{
    return QDir::setCurrent(path);
}

const QString ACore_SI::StartExternalProcess(QString command, QVariant argumentArray, bool waitToFinish, int milliseconds)
{
#ifndef _ALLOW_LAUNCH_EXTERNAL_PROCESS_
    abort("Launch of external process is not allowed.\nEnable \"_ALLOW_LAUNCH_EXTERNAL_PROCESS_\" in ants2.pro");
    return "";
#else
    QStringList arg;
    QString type = argumentArray.typeName();
    if (type == "QString") arg << argumentArray.toString();
    else if (type == "int") arg << QString::number(argumentArray.toInt());
    else if (type == "double") arg << QString::number(argumentArray.toDouble());
    else if (type == "QVariantList")
    {
        QVariantList vl = argumentArray.toList();
        QJsonArray ar = QJsonArray::fromVariantList(vl);
        for (int i=0; i<ar.size(); i++) arg << ar.at(i).toString();
    }
    else
    {
        qDebug() << "Format error in argument list";
        return "bad arguments";
    }

    QString str = command + " ";
    for (QString &s : arg) str += s + " ";
    qDebug() << "Executing external command:" << str;

    QProcess *process = new QProcess(this);
    QString errorString;

    if (waitToFinish)
    {
        process->start(command, arg);
        process->waitForFinished(milliseconds);
        errorString = process->errorString();
        delete process;
    }
    else
    {
        QObject::connect(process, SIGNAL(finished(int)), process, SLOT(deleteLater()));
        process->start(command, arg);
    }

    return errorString;
#endif // ANTS2_ALLOW_EXTERNAL_PROCESS
}
