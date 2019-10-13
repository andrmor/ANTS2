#include "acore_si.h"
#include "ascriptmanager.h"
#include "afiletools.h"

#ifdef USE_EIGEN
#include "curvefit.h"
#endif

#include "TRandom2.h"

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

#include <QVector>

// WATER'S INCLUDES

#include "readDiffusionVolume.h"
#include "diffusion.h"
#include "hits2idealWaveform.h"
#include "getImpulseResponse.h"
#include "getWaveform.h"
#include <iostream>
#include <algorithm>

//~ #include <vector> // MAY CAUSE PROBLEMS, REMOVE IF NECESSARY
//~ #include <Eigen/Dense>

// ------------------- CORE ----------------------

ACore_SI::ACore_SI(AScriptManager* ScriptManager) :
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

// =====================================================================
// WATER'S FUNCTIONS ===================================================
// =====================================================================

// -- getDiffusionVolume.h ---------------------------------------------

//~ a_sentence AInterfaceToCore::makeSentenceWithWord(QString word, int times)
//~ {
	//~ a_sentence out;
	//~ for(int i = 0 ; i < times ; i++){
		//~ out.push_back(word);
	//~ }
	//~ return out;
//~ }

//~ QString AInterfaceToCore::repeatAndJoin(QString word, int times)
//~ {
	//~ a_sentence in = makeSentenceWithWord(word, times);
	//~ QString out;
	//~ for(int i = 0 ; i < in.size() ; i++){
	
		//~ out += in[i];
	//~ }
	
	//~ return out;
//~ }

QVariant ACore_SI::giveMap()
{
	QVariantMap out;
	
	out["one"] = 1;
	out["two"] = 2;
	out["three"] = 3;
	
	return out;
}

QVariant ACore_SI::a_VolumeInfo2QVariant(a_VolumeInfo in)
{
	QVariantMap out;
	
	out["ID"] = in.ID;
	out["entry_line"] = in.entry_line;
	
	QVariantList dV;
	dV.push_back(in.dV[0]);
	dV.push_back(in.dV[1]);
	dV.push_back(in.dV[2]);
	
	QVariantList sz;
	sz.push_back(in.sz[0]);
	sz.push_back(in.sz[1]);
	sz.push_back(in.sz[2]);
	
	out["dV"] = dV;
	out["sz"] = sz;
	
	return out;
}

QVariant ACore_SI::a_Volume2QVariant(a_Volume in)
{
	QVariantMap out;
	
	out["V_info"] = a_VolumeInfo2QVariant(in.V_info);
	
	a_tensor V_tensor = in.V;
	
	QVariantList V_list;
	
	for (int ez ; ez < V_tensor.size() ; ez++){
	
		QVariantList plane;
		for(int i = 0 ; i < V_tensor[ez].size() ; i++){
	
			QVariantList row;
			for(int j = 0; j < V_tensor[ez][i].size() ; j++){
				
				row.push_back(V_tensor[ez][i][j]);
			}
			plane.push_back(row);
			row.clear();
		}
		V_list.push_back(plane);
		plane.clear();
	}
	
	out["V"] = V_list;
	
	return out;	
}

QVariant ACore_SI::a_Slice2QVariant(a_Slice in)
{
	QVariantMap out;
	
	out["V_info"] = a_VolumeInfo2QVariant(in.V_info);
	out["ID"] = in.ID;
	
	a_matrix S_matrix = in.S;
	
	QVariantList S_list;
	
	for(int i = 0 ; i < S_matrix.size() ; i++){
	
		QVariantList row;
		for(int j = 0; j < S_matrix[i].size() ; j++){
			
			row.push_back(S_matrix[i][j]);
		}
		S_list.push_back(row);
		row.clear();
	}
	
	out["S"] = S_list;
	
	return out;
}

QVariant ACore_SI::antsGetVolumeInfo(int V_ID, QString q_filename)
{
	//QVariantMap out;
	
	std::string filename = q_filename.toUtf8().constData();
	a_VolumeInfo vol_info_struct = getVolumeInfo(V_ID, filename);
	//~ out = a_VolumeInfo2QVariant(vol_info_struct);
	
	//~ return out;
	return a_VolumeInfo2QVariant(vol_info_struct);
}

QVariant ACore_SI::antsGetSlice(int V_ID, int S_ID, QString q_filename)
{
	QVariantMap out;
	
	std::string filename = q_filename.toUtf8().constData();
	a_Slice slice_struct = getSlice(V_ID, S_ID, filename);
	//stuff
	
	return a_Slice2QVariant(slice_struct);
}

QVariant ACore_SI::antsGetVolume(int V_ID, QString q_filename)
{
	QVariantMap out;
	
	std::string filename = q_filename.toUtf8().constData();
	a_Volume volume_struct = getVolume(V_ID, filename);
	//stuff
	
	return a_Volume2QVariant(volume_struct);
}

// -- diffusion.h ------------------------------------------------------

QVariant ACore_SI::e_list2QVariantList(e_list in){
	
	QVariantList electronPos_list;
	QVariantList electronPos_vec; // "vector" representing pos of e-
	
	for(int i = 0 ; i < in.rows() ; i++){
		
		electronPos_vec.push_back(in(i,0));
		electronPos_vec.push_back(in(i,1));
		electronPos_vec.push_back(in(i,2));
		
		electronPos_list.push_back(electronPos_vec);
		
		electronPos_vec.clear();
		
	}
	
	return electronPos_list;
}

QVariant ACore_SI::antsGetVolume(QString q_filename,
                                          double depthFrac,
                                          double h,
                                          double v_d,
                                          double sigma_Tcath,
                                          double sigma_Lcath){

	//QVariantList<QVariantList<double>> out;
	
	std::string filename = q_filename.toUtf8().constData();
	
	e_list diffusion_e_list = diffusion(filename,
                                        depthFrac,
                                        h,
                                        v_d,
                                        sigma_Tcath,
                                        sigma_Lcath);
	
	return e_list2QVariantList(diffusion_e_list);

}

// -- hits2idealWaveform.h ---------------------------------------------

QVariant ACore_SI::antsHits2idealWaveform(QVariant PMT_hits_lst,
                                                    double ADC_reso){

	QVariant out;
	
	std::cout << "antsH2IW: PMT_hits_lst type: " << getVarQtype(PMT_hits_lst).toUtf8().constData() << std::endl;
	
	std::cout << "antsH2IW: QVariant -> QVariantList" << std::endl;
	
	QVariantList PMT_hits_qvl;
	QSequentialIterable it = PMT_hits_lst.value<QSequentialIterable>();
	for (const QVariant &v : it){
		PMT_hits_qvl << v;
	}
	
	//~ QVariantList PMT_hits_qvl = PMT_hits_lst.toList();
	
	std::cout << "antsH2IW: PMT_hits_qvl.size() = " << PMT_hits_qvl.size() << std::endl;
	
	std::cout << "antsH2IW: QVariantList -> QList<double>" << std::endl;
	
	QList<double> PMT_hits_qvld;
	foreach(QVariant v, PMT_hits_lst.value<QVariantList>()) {
        PMT_hits_qvld << v.value<double>();
    }
	
	std::cout << "antsH2IW: PMT_hits_qvld.size() = " << PMT_hits_qvld.size() << std::endl;
	
	std::cout << "antsH2IW: QList<double> -> QVector<double>" << std::endl;
	
	QVector<double> PMT_hits_qvec = QVector<double>::fromList(PMT_hits_qvld);
	
	std::cout << "antsH2IW: PMT_hits_qvec.size() = " << PMT_hits_qvec.size() << std::endl;
	
	std::cout << "antsH2IW: QVector<double> -> std::vector<double>" << std::endl;
	
	std::vector<double> PMT_hits_stdvec = PMT_hits_qvec.toStdVector();
	
	std::cout << "antsH2IW: PMT_hits_stdvec.size() = " << PMT_hits_stdvec.size() << std::endl;
	
	std::cout << "antsH2IW: doing the thing" << std::endl;
	
	std::vector<int> out_std = hits2IdealWaveform(PMT_hits_stdvec,
	                                                 ADC_reso);
	
	std::cout << "antsH2IW: std::vector<int> -> QVector<int>" << std::endl;
	
	QVector<int> out_qvec = QVector<int>::fromStdVector(out_std);
	
	std::cout << "antsH2IW: QVector<int> -> QList<int>" << std::endl;
	
	QList<int> out_qlist_int = out_qvec.toList();
	
	
	std::cout << "antsH2IW: QList<int> -> QVariant" << std::endl;
	
	out = QVariant::fromValue<QList<int>>(out_qlist_int);

    std::cout << "antsH2IW: type of out: " << getVarQtype(out).toUtf8().constData() << std::endl;
	
	return out;

}

// -- getImpulseResponse.h ---------------------------------------------
                                
QVariant ACore_SI::antsGetImpulseResponse(QString PMTfilename,
                                                   double ADC_reso){

	//~ QVariant out;
	
	std::cout << "antsGIR: doing the thing" << std::endl;
	std::string stdPMTfilename = PMTfilename.toUtf8().constData();
	QVariantList out = getImpulseResponse(stdPMTfilename, ADC_reso);
	//~ std::vector<double> out_std = getImpulseResponse(stdPMTfilename,
	                                                 //~ ADC_reso);
	//~ std::cout << "antsGIR: out_std.size() = " << out_std.size() << std::endl;
	
	//~ std::cout << "antsGIR: std::vector<double> -> QVector<double>" << std::endl;
	
	//~ QVector<double> out_qvec = QVector<double>::fromStdVector(out_std);
	//~ std::cout << "antsGIR: out_qvec.size()" << out_qvec.size() << std::endl;
	
	//~ std::cout << "antsGIR: QVector<double> -> QList<double>" << std::endl;
	
	
	//~ QList<double> out_qlist_double = out_qvec.toList();
	
	//~ std::cout << "antsGIR: out_qlist_double.size() = " << out_qlist_double.size() << std::endl;
	
	//~ std::cout << "antsGIR: QList<double> -> QVariant" << std::endl;
	//~ std::cout << "antsGIR: out_qlist_double[10] - 0.5 = " <<  out_qlist_double[10] - 0.5 << std::endl;
	
	//~ out = QVariant::fromValue<QList<double>>(out_qlist_double);
	
	std::cout << "antsGIR: type of out: " << getVarQtype(out).toUtf8().constData() << std::endl;
	
	return out;

}
  
// -- getWaveform.h ----------------------------------------------------  

QVariant ACore_SI::antsGetWaveform(QVariant idealWaveform,
                                           QVariant impulseResponse){

	//~ QVariant out;
	
	//~ std::cout << "aGW: QVariant -> QVariantList" << std::endl;
	
	//~ QVariantList IWqvl;
	//~ QSequentialIterable it1 = idealWaveform.value<QSequentialIterable>();
	//~ for (const QVariant &v : it1){
		//~ IWqvl << v;
	//~ }
	
	//~ std::cout << "aGW: QVariantList -> QList<double>" << std::endl;
	
	//~ QList<double> IWqvld;
	//~ foreach(QVariant v, idealWaveform.value<QVariantList>()) {
        //~ IWqvld << v.value<double>();
    //~ }
	
	//~ std::cout << "aGW: QList<double> -> QVector<double>" << std::endl;
	
	//~ QVector<double> IWqvec = QVector<double>::fromList(IWqvld);
	//~ std::cout << "aGW: QVector<double> -> std::vector<double>" << std::endl;
	//~ std::vector<double> IWstdvec = IWqvec.toStdVector();
	
	//~ std::cout << "aGW: QVariant -> QVariantList" << std::endl;
	//~ QVariantList IRqvl;
	//~ QSequentialIterable it2 = impulseResponse.value<QSequentialIterable>();
	//~ for (const QVariant &v : it2){
		//~ IRqvl << v;
	//~ }
	//~ std::cout << "aGW: QVariantList -> QList<double>" << std::endl;
	//~ QList<double> IRqvld;
	//~ foreach(QVariant v, impulseResponse.value<QVariantList>()) {
        //~ IRqvld << v.value<double>();
    //~ }
    //~ std::cout << "aGW: QList<double> -> QVector<double>" << std::endl;
	//~ QVector<double> IRqvec = QVector<double>::fromList(IRqvld);
	//~ std::cout << "aGW: QVector<double> -> std::vector<double>" << std::endl;
	//~ std::vector<double> IRstdvec = IRqvec.toStdVector();
	
	//~ std::cout << "aGW: doing the thing" << std::endl;
	//~ std::vector<double> out_std = getWaveform(IWstdvec, IRstdvec);
	//~ std::cout << "aGW: std::vector<double> -> QVector<double>" << std::endl;
	//~ QVector<double> out_qvec = QVector<double>::fromStdVector(out_std);
	//~ std::cout << "aGW: QVector<double> ->  QList<double>" << std::endl;
	//~ QList<double> out_qlist_double = out_qvec.toList();
	//~ std::cout << "aGW: QList<double> -> QVariant" << std::endl;
	//~ out = QVariant::fromValue<QList<double>>(out_qlist_double);
	
	std::cout << "aGW: inputs to QVarianList" << std::endl;
	
	QVariantList IWqvl = idealWaveform.toList();
	QVariantList IRqvl = impulseResponse.toList();
	
	std::cout << "aGW: doing the thing" << std::endl;
	
	QVariantList out = getWaveform(IWqvl, IRqvl);
	
	std::cout << "aGW: returning" << std::endl;
	
	return out;

}

double ACore_SI::minimumValueInList(QVariant in){

	QVariantList in_qvl;
	QSequentialIterable it = in.value<QSequentialIterable>();
	for (const QVariant &v : it){
		in_qvl << v;
	}
	
	QList<double> in_qvld;
	foreach(QVariant v, in.value<QVariantList>()) {
        in_qvld << v.value<double>();
    }
	
	QVector<double> in_qvec = QVector<double>::fromList(in_qvld);
	
	std::vector<double> in_stdvec = in_qvec.toStdVector();
	
	std::vector<double>::iterator result = std::min_element(std::begin(in_stdvec), std::end(in_stdvec));
	
	return in_stdvec[std::distance(std::begin(in_stdvec), result)];
}

//~ bool AInterfaceToCore::saveObj2Table(QString FileName, 
                                    //~ QVariant Object, 
                                        //~ bool CanOverride){

    //~ QString type = Object.typeName();
    //~ if (type != "QVariantMap")
    //~ {
        //~ qDebug() << "Not an object - cannot use saveObject function";
        //~ return false;
    //~ }

    //~ if (QFileInfo(FileName).exists() && !CanOverride)
    //~ {
        //~ //abort("File already exists: " + fileName);
        //~ qDebug() << "File already exists: " << FileName << " Skipping!";
        //~ return false;
    //~ }

	//~ QList<QVariantList> out;

    //~ QVariantMap mp = Object.toMap();
    
    //~ QList<QString> mp_keys = mp.keys();
	
	//~ out.push_back(mp_keys);
	
	//~ for(int i = 0; i < mp_keys.size(); i++){
	
		//~ out[]
		
	//~ }

    //~ return saveArray(fileName, out);
											
//~ }

void ACore_SI::shellMarker(QString in){
	
	std::cout << in.toUtf8().constData() << std::endl;
	
}

// =====================================================================
// END OF WATER'S FUNCTIONS ============================================
// =====================================================================

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
