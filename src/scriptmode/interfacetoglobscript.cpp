#include "interfacetoglobscript.h"
#include "detectorclass.h"
#include "eventsdataclass.h"
#include "tmpobjhubclass.h"
#include "pms.h"
#include "apositionenergyrecords.h"
#include "atrackrecords.h"
#include "sensorlrfs.h"
#include "alrfmoduleselector.h"
#include "globalsettingsclass.h"
#include "ajsontools.h"
#include "pmtypeclass.h"
#include "aconfiguration.h"
#include "apreprocessingsettings.h"
#include "reconstructionmanagerclass.h"
#include "apmgroupsmanager.h"
#include "modules/lrf_v3/arepository.h"
#include "modules/lrf_v3/asensor.h"
#include "modules/lrf_v3/ainstructioninput.h"
#include "amonitor.h"

#ifdef SIM
#include "simulationmanager.h"
#endif

#ifdef GUI
#include "mainwindow.h"
#include "exampleswindow.h"
#include "reconstructionwindow.h"
#include "reconstructionwindow.h"
#include "lrfwindow.h"
#include "outputwindow.h"
#include "geometrywindowclass.h"
#include "graphwindowclass.h"
#endif

#include <QVariant>
#include <QThread>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QDateTime>
#include <QDialog>
#include <QApplication>
#include <QVector3D>
#include <QJsonDocument>
#include <QDebug>

#include "TRandom2.h"
#include "TGeoManager.h"
#include "TGeoTrack.h"
#include "TColor.h"
#include "TH1D.h"
#include "TH1.h"
#include "TH2D.h"
#include "TH2.h"
#include "TF2.h"
#include "TGraph.h"
#include "TF1.h"
#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"
#include "TLeaf.h"

/*
bool InterfaceToConfig::keyToNameAndIndex(QString Key, QString& Name, int& Index)
{
  //qDebug() << "Array candidate:"<<Key;
  if (Key.contains("["))
    {
      if (!Key.endsWith("]"))
        {
          LastError = "Format error: "+Key;
          qDebug() << LastError;
          return false;
        }

      Name = "";
      QString indexStr = Key;
      for (int i=0; i<Key.size(); i++)
        {
          QChar c = Key[i];
          indexStr = indexStr.mid(1);
          if (c == '[') break;
          else Name += c;
        }
      indexStr.chop(1);
      //qDebug() << "  Index candidate:"<<indexStr;
      bool ok;
      Index = indexStr.toInt(&ok);
      if (!ok)
        {
          LastError = "Index extraction error for "+Key;
          qDebug() << LastError;
          return false;
        }
    }
  else
    { // It is NOT an array
      Name = Key;
      Index = -1;
    }

  //qDebug() << "Extracted Name/Index:"<<Name<<Index;
  return true;
}
*/

bool InterfaceToConfig::keyToNameAndIndex(QString Key, QString &Name, QVector<int> &Indexes)
{
    Indexes.clear();
    if (Key.contains('['))
      { // it is an array
        if (Key.startsWith('[') || !Key.endsWith(']'))
          {
            LastError = "Format error: "+Key;
            qDebug() << LastError;
            return false;
          }
        QStringList ArFields = Key.split('[');
        //qDebug() << ArFields;
        Name = ArFields.first();
        ArFields.removeFirst();
        for (int i=0; i<ArFields.size(); i++)
        {
            QString a = ArFields[i];
            if (!a.endsWith(']'))
            {
                LastError = "Format error: "+Key;
                qDebug() << LastError;
                return false;
            }
            a.chop(1);
            bool ok;
            int index = a.toInt(&ok);
            if (!ok)
            {
                LastError = "Format error: "+Key;
                qDebug() << LastError;
                return false;
            }
            Indexes << index;
        }
      }
    else // it is not an array
        Name = Key;

    //qDebug() << "Extracted Name/Indexes:"<<Name<<Indexes;
    return true;
}

/*
bool InterfaceToConfig::modifyJsonValue(QJsonObject& obj, const QString& path, const QJsonValue& newValue)
{
  int indexOfDot = path.indexOf('.');
  QString propertyName = path.left(indexOfDot);
  QString subPath = indexOfDot>0 ? path.mid(indexOfDot+1) : QString();
  //qDebug() << "subPath:"<<subPath;

  QString name;
  int index;
  bool ok = keyToNameAndIndex(propertyName, name, index);
  if (!ok) return false;
  propertyName = name;

  //qDebug() << "Attempting to extract:"<<propertyName<<index;
  QJsonValue subValue = obj[propertyName];
  //qDebug() << "QJsonvalue extraction success?" << (subValue != QJsonValue());
  if (subValue == QJsonValue())
    {
      LastError = "Property not found:"+propertyName;
      qDebug() << LastError;
      return false;
    }

  //updating QJsonObject
  if(subPath.isEmpty() && index==-1)
    {
      subValue = newValue;
    }
  else
    {
      QJsonObject obj1;
      if (index == -1)
        {
          obj1 = subValue.toObject();
          bool ok = modifyJsonValue(obj1, subPath, newValue);
          if (!ok) return false;
          subValue = obj1;
        }
      else
        {
          QJsonArray arr = subValue.toArray();
          if (index<0 || index>arr.size()-1)
            {
              LastError = "Wrong array index ("+QString::number(index)+") of "+propertyName+": array size is " + QString::number(arr.size());
              qDebug() << LastError;
              return false;
            }
          //qDebug() << arr[index].isArray() << subPath.isEmpty();
          if (arr[index].isArray() && subPath.isEmpty())
          {
              arr[index] = newValue;
              obj[propertyName] = arr;
              return true;
          }
          obj1 = arr[index].toObject();
          if (obj1.isEmpty() && !subPath.isEmpty())
            {
              LastError = "Array element of "+propertyName+"["+QString::number(index)+"] is not QJsonObject!";
              qDebug() << LastError;
              return false;
            }
          if (subPath.isEmpty())
          {
              //qDebug() << index << newValue;
              arr[index] = newValue;
              subValue = arr;
          }
          else
          {
              bool ok = modifyJsonValue(obj1, subPath, newValue);
              if (!ok) return false;
              arr[index] = obj1;
              subValue = arr;
          }
        }
    }

  obj[propertyName] = subValue;
  return true;
}
*/

bool InterfaceToConfig::modifyJsonValue(QJsonObject &obj, const QString &path, const QJsonValue &newValue)
{
    int indexOfDot = path.indexOf('.');
    QString propertyName = path.left(indexOfDot);
    QString subPath = indexOfDot>0 ? path.mid(indexOfDot+1) : QString();
    //qDebug() << "subPath:"<<subPath;

    QString name;
    QVector<int> indexes;
    bool ok = keyToNameAndIndex(propertyName, name, indexes);
    if (!ok) return false;
    propertyName = name;

    //qDebug() << "Attempting to extract:"<<propertyName<<indexes;
    QJsonValue subValue = obj[propertyName];
    //qDebug() << "QJsonvalue extraction success?" << (subValue != QJsonValue());
    if (subValue == QJsonValue())
      {
        LastError = "Property not found:"+propertyName;
        qDebug() << LastError;
        return false;
      }

    //updating QJsonObject
    if(indexes.isEmpty())
      { // it is an object
        if (subPath.isEmpty()) subValue = newValue;
        else
        {
            QJsonObject obj1;
            obj1 = subValue.toObject();
            bool ok = modifyJsonValue(obj1, subPath, newValue);
            if (!ok) return false;
            subValue = obj1;
        }
      }
    else
      { // it is an array
        QVector<QJsonArray> arrays;
        arrays << subValue.toArray();
        for (int i=0; i<indexes.size()-1; i++)  //except the last one - it has special treatment
          {
            int index = indexes.at(i);
            if (index<0 || index>arrays.last().size()-1)
              {
                LastError = "Array index is out of bounds for property name "+propertyName;
                qDebug() << LastError;
                return false;
              }
            arrays << arrays.last()[index].toArray();
          }

        if (subPath.isEmpty())
          { //it is the last subkey
             arrays.last()[indexes.last()] = newValue;
             //propagating to parent arrays
             for (int i=arrays.size()-2; i>-1; i--) arrays[i][indexes[i]] = arrays[i+1];
             subValue = arrays.first();
          }
        else
          { //not the last subkey, so it should be an object
             QJsonObject obj1 = arrays.last()[indexes.last()].toObject();
             if (obj1.isEmpty())
               {
                  LastError = "Array element of "+propertyName+" is not QJsonObject!";
                  qDebug() << LastError;
                  return false;
               }
             bool ok = modifyJsonValue(obj1, subPath, newValue);
             if (!ok) return false;
             arrays.last()[indexes.last()] = obj1;
             //propagating to parent arrays
             for (int i=arrays.size()-2; i>-1; i--) arrays[i][indexes[i]] = arrays[i+1];
             subValue = arrays.first();
          }
      }

    obj[propertyName] = subValue;
    return true;
}

/*
void InterfaceToConfig::find(const QJsonObject &obj, QStringList Keys, QStringList &Found, QString Path)
{
  // script interface replaces ".." or the leading "." with an empty string
  bool fLast = (Keys.size() == 1); //it is the last key in the Keys list
  if (fLast && Keys.last()=="")
    { //bad format - Keys have to end with a concrete key
      LastError = "Bad format - Last object has to be concrete";
      qDebug() << LastError;
      return;
    }

  QString Key = Keys.first();
  QString Name;
  int Index;
  if (Key != "")
    { //looking for a concrete key
      if (!keyToNameAndIndex(Key, Name, Index)) return; //format error

      if (obj.contains(Name))
        { //object does contain the key name! (we do not check here if the index is adequate)
          Path += "." + Key;
          if (fLast)
            { //mission accomplished
              Found.append(Path.mid(1)); //remove the starting "." in the found path
              return;
            }
          //pass to the next key in the chain
          Keys.removeFirst();
          if (Index == -1)
            find(obj[Name].toObject(), Keys, Found, Path);
          else
            find(obj[Name].toArray().at(Index).toObject(), Keys, Found, Path);
        }
      else return; //does not contains the given key
    }
  else
    { // "" Key
      QString Key = Keys.at(1); //this is next key to find
      if (Key == "") return;  //format problem - cannot be "" followed by ""
      //does the object contain the next key?
      if (!keyToNameAndIndex(Key, Name, Index)) return; //format error
      if  (obj.contains(Name))
        { //object does contain the key!
          //we can reuse the function:
          Keys.removeFirst(); //remove ""
          find(obj, Keys, Found, Path);
        }
      else
        { //have to check every sub-object
          foreach(QString oneKey, obj.keys())
            {
              QJsonValue Val = obj[oneKey];
              if (Val.isObject())
                find(obj[oneKey].toObject(), Keys, Found, Path+"."+oneKey);
              else if (Val.isArray())
                {
                  QJsonArray arr = Val.toArray();
                  for (int i=0; i<arr.size(); i++)
                    find(arr[i].toObject(), Keys, Found, Path+"."+oneKey+"["+QString::number(i)+"]");
                }
              //else do nothing for other types
            }
        }
  }
}
*/

void InterfaceToConfig::find(const QJsonObject &obj, QStringList Keys, QStringList &Found, QString Path)
{
    // script interface replaces ".." or the leading "." with an empty string
    bool fLast = (Keys.size() == 1); //it is the last key in the Keys list
    if (fLast && Keys.last()=="")
      { //bad format - Keys have to end with a concrete key
        LastError = "Bad format - Last object has to be concrete";
        qDebug() << LastError;
        return;
      }

    //qDebug() << "find1 triggered for:"<<Keys;
    QString Key = Keys.first();
    QString Name;
    QVector<int> indexes;
    if (Key != "")
      { //looking for a concrete key
        if (!keyToNameAndIndex(Key, Name, indexes)) return; //format error

        if (obj.contains(Name))
          { //object does contain the key name! (we do not check here if the index is adequate)
            Path += "." + Key;
            if (fLast)
              { //mission accomplished
                Found.append(Path.mid(1)); //remove the starting "." in the found path
                return;
              }
            //pass to the next key in the chain
            Keys.removeFirst();
            if (indexes.isEmpty())
              find(obj[Name].toObject(), Keys, Found, Path);
            else
            {
              QJsonArray ar = obj[Name].toArray();
              for (int i=0; i<indexes.size()-1; i++) ar = ar[indexes[i]].toArray();
              QJsonObject arob = ar[indexes.last()].toObject();
              find(arob, Keys, Found, Path);
            }
          }
        else return; //does not contains the given key
      }
    else
      { // "" Key
        QString Key = Keys.at(1); //this is next key to find
        if (Key == "") return;  //format problem - cannot be "" followed by ""
        //does the object contain the next key?
        if (!keyToNameAndIndex(Key, Name, indexes)) return; //format error
        if  (obj.contains(Name))
          { //object does contain the key!
            //we can reuse the function:
            Keys.removeFirst(); //remove ""
            find(obj, Keys, Found, Path);
          }
        else
          { //have to check every sub-object
            foreach(QString oneKey, obj.keys())
              {
                QJsonValue Val = obj[oneKey];
                if (Val.isObject())
                  find(obj[oneKey].toObject(), Keys, Found, Path+"."+oneKey);
                else if (Val.isArray())
                  {
                    QJsonArray arr = Val.toArray();
                    for (int i=0; i<arr.size(); i++)
                      find(arr[i].toObject(), Keys, Found, Path+"."+oneKey+"["+QString::number(i)+"]");
                  }
                //else do nothing for other types
              }
          }
    }
}

bool InterfaceToConfig::Replace(QString Key, QVariant val)
{
  LastError = "";
  //qDebug() << Key << val << val.typeName();
  QString type = val.typeName();

  QJsonValue jv;
  QString rep;
  if (type == "int")
    {
      jv = QJsonValue(val.toInt());
      rep = QString::number(val.toInt());
    }
  else if (type == "double")
    {
      jv = QJsonValue(val.toDouble());
      rep = QString::number(val.toDouble());
    }
  else if (type == "bool")
    {
      jv = QJsonValue(val.toBool());
      rep = val.toBool() ? "true" : "false";
    }
  else if (type == "QString")
    {
      jv = QJsonValue(val.toString());
      rep = val.toString();
    }
  else if (type == "QVariantList")
    {
      QVariantList vl = val.toList();
      QJsonArray ar = QJsonArray::fromVariantList(vl);
      jv = ar;
      rep = "-Array-";
    }
    else if (type == "QVariantMap")
      {
        QVariantMap mp = val.toMap();
        QJsonObject ob = QJsonObject::fromVariantMap(mp);
        jv = ob;
        rep = "-Object-";
      }
  else
    {
      abort("Wrong argument type ("+type+") in Replace with the key line:\n"+Key);
      return false;
    }

  if (!expandKey(Key)) return false;

  bool ok = modifyJsonValue(Config->JSON, Key, jv);
  if (ok)
    {
      //qDebug() << "-------Key:"<<Key;
      if (Key.startsWith("DetectorConfig")) //rebuild detector if detector settings were changed
          Config->GetDetector()->BuildDetector();
      else if (Key.startsWith("ReconstructionConfig.LRFmakeJson"))
          Config->AskForLRFGuiUpdate();
      else if (Key.startsWith("ReconstructionConfig"))
          Config->AskForReconstructionGuiUpdate();
      else if (Key.startsWith("SimulationConfig"))
          Config->AskForSimulationGuiUpdate();
      return true;
    }

  abort("Replace("+Key+", "+rep+") format error:<br>"+LastError);
  return false;
}

/*
QVariant InterfaceToConfig::GetKeyValue(QString Key)
{
    LastError = "";

    if (!expandKey(Key)) return 0; //aborted anyway
    //qDebug() << "Key after expansion:"<<Key;

    QJsonObject obj = Config->JSON;
    int indexOfDot;
    QString path = Key;
    do
    {
        indexOfDot = path.indexOf('.');
        QString propertyName = path.left(indexOfDot);
        QString path1 = (indexOfDot>0 ? path.mid(indexOfDot+1) : QString());
        path = path1;
        //qDebug() << "property, path"<<propertyName<<path;

        QString name;
        int index;
        bool ok = keyToNameAndIndex(propertyName, name, index);
        if (!ok)
        {
            abort("Get key value for "+Key+" error");
            return false;
        }        
        propertyName = name;
        //qDebug() << "Attempting to extract:"<<propertyName<<index;
        QJsonValue subValue = obj[propertyName];
        //qDebug() << "QJsonValue extraction success?" << (subValue != QJsonValue());
        if (subValue == QJsonValue())
          {
            LastError = "Property not found:"+propertyName;
            qDebug() << LastError;
            return false;
          }

        //updating QJsonObject
        if(path.isEmpty() && index==-1)
          {
            //here attempt to get value
            //qDebug() << "QJsonValue to attempt to report back:"<<subValue;
            QVariant res = subValue.toVariant();
            //qDebug() << "QVariant:"<<res;
            return res;
          }
        else
          {
            if (index == -1) obj = subValue.toObject();
            else
              {
                QJsonArray arr = subValue.toArray();                
                if (index<0 || index>arr.size()-1)
                  {
                    LastError = "Wrong array index ("+QString::number(index)+") of "+propertyName+": array size is " + QString::number(arr.size());
                    qDebug() << LastError;
                    return false;
                  }
                obj = arr[index].toObject();
                if (obj.isEmpty())
                  {
                    if (arr[index].isArray() && path.isEmpty())
                    {
                        QJsonArray ar = arr[index].toArray();
                        QVariant res = ar.toVariantList();
                        return res;
                    }

                    if (path.isEmpty())
                    {
                        QVariant res = arr[index].toVariant();
                        return res;
                    }

                    LastError = "Array element of "+propertyName+"["+QString::number(index)+"] is not QJsonObject!";
                    qDebug() << LastError;
                    return false;
                  }
                if (path.isEmpty())
                {
                    QVariant res = obj.toVariantMap();
                    return res;
                }
              }
          }
    }
    while (indexOfDot>0);

    abort("Get key value for "+Key+" error");
    return 0;
}
*/

QVariant InterfaceToConfig::GetKeyValue(QString Key)
{
    LastError = "";

    if (!expandKey(Key)) return 0; //aborted anyway
    //qDebug() << "Key after expansion:"<<Key;

    QJsonObject obj = Config->JSON;
    int indexOfDot;
    QString path = Key;
    do
    {
        indexOfDot = path.indexOf('.');
        QString propertyName = path.left(indexOfDot);
        QString path1 = (indexOfDot>0 ? path.mid(indexOfDot+1) : QString());
        path = path1;
        //qDebug() << "property, path"<<propertyName<<path;

        QString name;
        QVector<int> indexes;
        bool ok = keyToNameAndIndex(propertyName, name, indexes);
        if (!ok)
        {
            abort("Get key value for "+Key+" format error");
            return false;
        }
        propertyName = name;
        //qDebug() << "Attempting to extract:"<<propertyName<<indexes;
        QJsonValue subValue = obj[propertyName];
        //qDebug() << "QJsonValue extraction success?" << (subValue != QJsonValue());
        if (subValue == QJsonValue())
          {
            LastError = "Property not found:"+propertyName;
            qDebug() << LastError;
            return false;
          }

        //updating QJsonObject
        if(indexes.isEmpty())
          { //not an array
            if (path.isEmpty())
            {
                //qDebug() << "QJsonValue to attempt to report back:"<<subValue;
                QVariant res = subValue.toVariant();
                //qDebug() << "QVariant:"<<res;
                return res;
            }
            else obj = subValue.toObject();
          }
        else
          { //it is an array
            //if path is not empty, subValue has to be either object or array
                QJsonArray arr = subValue.toArray();
                for (int i=0; i<indexes.size(); i++)
                {
                    int index = indexes.at(i);
                    if (index<0 || index>arr.size()-1)
                      {
                        LastError = "Array index is out of bounds ("+QString::number(index)+") for property anme "+propertyName;
                        qDebug() << LastError;
                        return false;
                      }
                    if (i == indexes.size()-1)
                    {
                        //it is the last index
                        if (path.isEmpty())
                          {
                            QJsonValue jv = arr[index];
                            QVariant res = jv.toVariant();
                            return res;
                          }
                        else obj = arr[index].toObject();
                    }
                    else arr = arr[index].toArray();
                }

                if (obj.isEmpty())
                  {
                    LastError = "Array element of "+propertyName+" is not QJsonObject!";
                    qDebug() << LastError;
                    return false;
                  }
          }
    }
    while (indexOfDot>0);

    abort("Get key value for "+Key+" error");
    return 0;
}


QString InterfaceToConfig::GetLastError()
{
    return LastError;
}

void InterfaceToConfig::RebuildDetector()
{
    Config->GetDetector()->BuildDetector();
}

bool InterfaceToConfig::expandKey(QString &Key)
{
    if (Key.startsWith(".") || Key.contains(".."))
      {
        QStringList keys = Key.split(".");
        //qDebug() << keys;
        QStringList res;
        find(Config->JSON, keys, res);

        if (!LastError.isEmpty())
          {
            abort("Search error for config key "+Key+":<br>" + LastError);
            return false;
          }
        if (res.isEmpty())
          {
            abort("Search error for config key "+Key+":<br>" +
                  "Not found config object according to this key");
            return false;
          }
        if (res.size()>1)
          {
            abort("Search error for config key "+Key+":<br>" +
                  "Match is not unique!");
            return false;
          }

        Key = res.first();
        //qDebug() << "*-----------"<<res;
      }

    return true;
}

//void InterfaceToConfig::PedestalSet(int ipm, double offset)
//{
//  if (!checkValidPM(ipm)) return;

//  APreprocessingSettings set;
//  set.readFromJson(Config->JSON, Config->GetDetector()->PMs, "");

//  Config->GetDetector()->PMs->at(ipm).PreprocessingAdd = offset;
//  set.fActive = true;

//  set.writeToJson(Config->GetDetector()->PreprocessingJson, Config->GetDetector()->PMs);
//  Config->GetDetector()->writeToJson(Config->JSON);
//}

//void InterfaceToConfig::PedestalAdd(int ipm, double offset)
//{
//  if (!checkValidPM(ipm)) return;

//  APreprocessingSettings set;
//  set.readFromJson(Config->JSON, Config->GetDetector()->PMs, "");

//  Config->GetDetector()->PMs->at(ipm).PreprocessingAdd += offset;
//  set.fActive = true;

//  set.writeToJson(Config->GetDetector()->PreprocessingJson, Config->GetDetector()->PMs);
//  Config->GetDetector()->writeToJson(Config->JSON);
//}

//void InterfaceToConfig::ScaleSet(int ipm, double factor)
//{
//  if (!checkValidPM(ipm)) return;

//  APreprocessingSettings set;
//  set.readFromJson(Config->JSON, Config->GetDetector()->PMs, "");

//  Config->GetDetector()->PMs->at(ipm).PreprocessingMultiply = factor;
//  set.fActive = true;

//  set.writeToJson(Config->GetDetector()->PreprocessingJson, Config->GetDetector()->PMs);
//  Config->GetDetector()->writeToJson(Config->JSON);
//}

//void InterfaceToConfig::ScaleMultiply(int ipm, double factor)
//{
//  if (!checkValidPM(ipm)) return;

//  APreprocessingSettings set;
//  set.readFromJson(Config->JSON, Config->GetDetector()->PMs, "");

//  Config->GetDetector()->PMs->at(ipm).PreprocessingMultiply *= factor;
//  set.fActive = true;

//  set.writeToJson(Config->GetDetector()->PreprocessingJson, Config->GetDetector()->PMs);
//  Config->GetDetector()->writeToJson(Config->JSON);
//}


void InterfaceToConfig::UpdateGui()
{
    Config->AskForAllGuiUpdate();
}

InterfaceToConfig::InterfaceToConfig(AConfiguration *config)
{
  Config = config;
  emit requestReadRasterGeometry();

  H["RebuildDetector"]  = "Force to rebuild detector using the current configuration file.\nNot anymore required after calling config.Replace - it is called automatically if Detector settings were modified.";
  H["Load"]  = "Load ANTS2 configuration file (.json).";
  H["Replace"] = "Replace the value of the key in the configuration object with the new one. Key value can be basic types (bool, double, string) as well as arrays and objects. Changing detector-related settings (DetectorConfig top node) will automatically run RebuildDetector!";
  H["UpdateGui"] = "Update GUI during script execution according to current settings in Config.";
  H["GetKeyValue"] = "Return the value of the Key: it can be basic types (bool, double, string) as well as arrays and objects.";

}

//bool InterfaceToConfig::InitOnRun()
//{
//  //qDebug() << "InitOnRun triggered for config unit";
//  emit requestReadRasterGeometry(); //only GUI
//  return true;
//}

bool InterfaceToConfig::Load(QString FileName)
{
  //bool ok = MW->ELwindow->LoadAllConfig(FileName, true, true, true, &Config->JSON);
  bool ok = Config->LoadConfig(FileName, true, true, true);
  if (ok) return true;

  abort("Failed to load config from file "+FileName);
  return false;
}

bool InterfaceToConfig::Save(QString FileName)
{
    return SaveJsonToFile(Config->JSON, FileName);
}

//void InterfaceToGlobScript::ShowOutputWindow(bool flag, int tab)
//{
//  if (flag)
//    {
//      MW->Owindow->showNormal();
//      MW->Owindow->raise();
//    }
//  else MW->Owindow->hide();

//  if (tab>-1 && tab<4) MW->Owindow->SetTab(tab);
//  qApp->processEvents();
//}

//-----------------------------------
static int msgH = 500, msgW = 300, msgX=50, msgY=50;
InterfaceToTexter::InterfaceToTexter(QMainWindow* parent) : D(0), Parent(parent)
{
  bEnabled = true;
  bActivated = false;
  init(false);
}

void InterfaceToTexter::init(bool fTransparent)
{
  D = new QDialog(Parent);
  QObject::connect(D, &QDialog::finished, this, &InterfaceToTexter::Hide);

  QVBoxLayout* l = new QVBoxLayout;
  e = new QPlainTextEdit();
  e->setReadOnly(true);
  l->addWidget(e);
  D->setLayout(l);

  X = msgX;
  Y = msgY;
  W = msgW;
  H = msgH;

  D->setGeometry(X, Y, W, H);
  D->setWindowTitle("Script msg");

  if (fTransparent)
    {
      D->setWindowFlags(Qt::FramelessWindowHint);
      D->setAttribute(Qt::WA_TranslucentBackground);

      e->setStyleSheet("background: rgba(0,0,255,0%)");
      e->setFrameStyle(QFrame::NoFrame);
    }
}


InterfaceToTexter::~InterfaceToTexter()
{
  //qDebug() << "Msg destructor";
  deleteDialog();
}

void InterfaceToTexter::SetTransparent(bool flag)
{
  QString text = e->document()->toPlainText();
  delete D;
  D = 0;
  init(flag);
  e->setPlainText(text);
}

void InterfaceToTexter::Append(QString txt)
{
  e->appendHtml(txt);
}

void InterfaceToTexter::Clear()
{
  e->clear();
}

void InterfaceToTexter::Show(QString txt, int ms)
{
  if (!bEnabled) return;
  e->clear();
  e->appendHtml(txt);

  if (ms == -1)
    {
      D->show();
      D->raise();
      bActivated = true;
      return;
    }

  D->show();
  D->raise();
  bActivated = true;
  QTime t;
  t.restart();
  do qApp->processEvents();
  while (t.elapsed()<ms);
  D->hide();
  bActivated = false;
}

void InterfaceToTexter::Move(double x, double y)
{
  X = msgX = x; Y = msgY = y;
  D->move(X, Y);
}

void InterfaceToTexter::Resize(double w, double h)
{
  W = msgW = w; H = msgH = h;
  D->resize(W, H);
}

void InterfaceToTexter::Show()
{
  if (!bEnabled) return;
  D->show();
  D->raise();
  bActivated = true;
}

void InterfaceToTexter::Hide()
{
  D->hide();
  bActivated = false;
}

void InterfaceToTexter::SetFontSize(int size)
{
  QFont f = e->font();
  f.setPointSize(size);
  e->setFont(f);
}

void InterfaceToTexter::deleteDialog()
{
   delete D;
   D = 0;
}

void InterfaceToTexter::hide()
{
    if (D) D->hide();
}

void InterfaceToTexter::restore()
{
    if (D) D->show();
}

#ifdef SIM
//----------------------------------
InterfaceToSim::InterfaceToSim(ASimulationManager* SimulationManager, EventsDataClass *EventsDataHub, AConfiguration* Config, int RecNumThreads, bool fGuiPresent)
  : SimulationManager(SimulationManager), EventsDataHub(EventsDataHub), Config(Config), RecNumThreads(RecNumThreads), fGuiPresent(fGuiPresent)
{
  H["RunPhotonSources"] = "Perform simulation with photon sorces.\nPhoton tracks are not shown!";
  H["RunParticleSources"] = "Perform simulation with particle sorces.\nPhoton tracks are not shown!";
  H["SetSeed"] = "Set random generator seed";
  H["GetSeed"] = "Get random generator seed";
  H["SaveAsTree"] = "Save simulation results as a ROOT tree file";
  H["SaveAsText"] = "Save simulation results as an ASCII file";

  H["getMonitorTime"] = "returns array of arrays: [time, value]";
  H["getMonitorWave"] = "returns array of arrays: [wave index, value]";
  H["getMonitorEnergy"] = "returns array of arrays: [energy, value]";
  H["getMonitorAngular"] = "returns array of arrays: [angle, value]";
  H["getMonitorXY"] = "returns array of arrays: [x, y, value]";
  H["getMonitorHits"] = "returns the number of detected hits by the monitor with the given name";
}

void InterfaceToSim::ForceStop()
{
  emit requestStopSimulation();
}

bool InterfaceToSim::RunPhotonSources(int NumThreads)
{
    if (NumThreads == -1) NumThreads = RecNumThreads;
    QJsonObject sim = Config->JSON["SimulationConfig"].toObject();
    sim["Mode"] = "PointSim";
    Config->JSON["SimulationConfig"] = sim;
    Config->AskForSimulationGuiUpdate();
    SimulationManager->StartSimulation(Config->JSON, NumThreads, fGuiPresent);

    do
      {
        qApp->processEvents();        
      }
    while (!SimulationManager->fFinished);
    return SimulationManager->fSuccess;
}

bool InterfaceToSim::RunParticleSources(int NumThreads)
{
    if (NumThreads == -1) NumThreads = RecNumThreads;
    QJsonObject sim = Config->JSON["SimulationConfig"].toObject();
    sim["Mode"] = "SourceSim";
    Config->JSON["SimulationConfig"] = sim;
    Config->AskForSimulationGuiUpdate();
    SimulationManager->StartSimulation(Config->JSON, NumThreads, fGuiPresent);

    do
      {
        qApp->processEvents();        
      }
    while (!SimulationManager->fFinished);
    return SimulationManager->fSuccess;


  /* 
  //watchdog on particle sources, can be transferred later to check-upwindow
  if (MW->ParticleSources.size() == 0)
    {
      qDebug() << "No particle sources defined!";
      return false;
    }
  for (int i = 0; i<MW->ParticleSources.size(); i++)
    {
      int error = MW->ParticleSources.CheckSource(i);
      if (error == 0) continue;

      qDebug() << "Error in source " << MW->ParticleSources[i].name << MW->ParticleSources.getErrorString(error);
      return false;
    }
  */
}

void InterfaceToSim::SetSeed(long seed)
{
  Config->GetDetector()->RandGen->SetSeed(seed);
}

long InterfaceToSim::GetSeed()
{
    return Config->GetDetector()->RandGen->GetSeed();
}

void InterfaceToSim::ClearCustomNodes()
{
    Config->ClearCustomNodes();
}

void InterfaceToSim::AddCustomNode(double x, double y, double z)
{
    Config->AddCustomNode(x, y, z);
}

QVariant InterfaceToSim::GetCustomNodes()
{
    QJsonArray arr = Config->GetCustomNodes();
    QVariant vr = arr.toVariantList();
    return vr;
}

bool InterfaceToSim::SetCustomNodes(QVariant ArrayOfArray3)
{
    QString type = ArrayOfArray3.typeName();
    if (type != "QVariantList") return false;

    QVariantList vl = ArrayOfArray3.toList();
    QJsonArray ar = QJsonArray::fromVariantList(vl);
    return Config->SetCustomNodes(ar);
}

bool InterfaceToSim::SaveAsTree(QString fileName)
{
  return EventsDataHub->saveSimulationAsTree(fileName);
}

bool InterfaceToSim::SaveAsText(QString fileName)
{
  return EventsDataHub->saveSimulationAsText(fileName);
}

int InterfaceToSim::countMonitors()
{
    if (!EventsDataHub->SimStat) return 0;
    return EventsDataHub->SimStat->Monitors.size();
}

//int InterfaceToSim::getMonitorHits(int imonitor)
//{
//    if (!EventsDataHub->SimStat || imonitor<0 || imonitor>EventsDataHub->SimStat->Monitors.size())
//        return std::numeric_limits<int>::quiet_NaN();
//    return EventsDataHub->SimStat->Monitors.at(imonitor)->getHits();
//}

int InterfaceToSim::getMonitorHits(QString monitor)
{
    if (!EventsDataHub->SimStat) return std::numeric_limits<int>::quiet_NaN();
    for (int i=0; i<EventsDataHub->SimStat->Monitors.size(); i++)
    {
        const AMonitor* mon = EventsDataHub->SimStat->Monitors.at(i);
        if (mon->getName() == monitor)
            return mon->getHits();
    }
    return std::numeric_limits<int>::quiet_NaN();
}

QVariant InterfaceToSim::getMonitorData1D(QString monitor, QString whichOne)
{
  QVariantList vl;
  if (!EventsDataHub->SimStat) return vl;
  for (int i=0; i<EventsDataHub->SimStat->Monitors.size(); i++)
  {
      const AMonitor* mon = EventsDataHub->SimStat->Monitors.at(i);
      if (mon->getName() == monitor)
      {
          TH1D* h;
          if      (whichOne == "time")   h = mon->getTime();
          else if (whichOne == "angle")  h = mon->getAngle();
          else if (whichOne == "wave")   h = mon->getWave();
          else if (whichOne == "energy") h = mon->getEnergy();

          TAxis* axis = h->GetXaxis();
          for (int i=1; i<axis->GetNbins()+1; i++)
          {
              QVariantList el;
              el << axis->GetBinCenter(i);
              el << h->GetBinContent(i);
              vl.push_back(el);
          }
      }
  }
  return vl;
}

QVariant InterfaceToSim::getMonitorTime(QString monitor)
{
    return getMonitorData1D(monitor, "time");
}

QVariant InterfaceToSim::getMonitorAngular(QString monitor)
{
  return getMonitorData1D(monitor, "angle");
}

QVariant InterfaceToSim::getMonitorWave(QString monitor)
{
  return getMonitorData1D(monitor, "wave");
}

QVariant InterfaceToSim::getMonitorEnergy(QString monitor)
{
  return getMonitorData1D(monitor, "energy");
}

QVariant InterfaceToSim::getMonitorXY(QString monitor)
{
  QVariantList vl;
  if (!EventsDataHub->SimStat) return vl;
  for (int i=0; i<EventsDataHub->SimStat->Monitors.size(); i++)
  {
      const AMonitor* mon = EventsDataHub->SimStat->Monitors.at(i);
      if (mon->getName() == monitor)
      {
          TH2D* h = mon->getXY();

          TAxis* axisX = h->GetXaxis();
          TAxis* axisY = h->GetYaxis();
          for (int ix=1; ix<axisX->GetNbins()+1; ix++)
            for (int iy=1; iy<axisY->GetNbins()+1; iy++)
          {
              QVariantList el;
              el << axisX->GetBinCenter(ix);
              el << axisY->GetBinCenter(iy);
              el << h->GetBinContent( h->GetBin(ix,iy) );
              vl.push_back(el);
          }
      }
  }
  return vl;
}
#endif

//----------------------------------
InterfaceToData::InterfaceToData(AConfiguration *Config, ReconstructionManagerClass *RManager, EventsDataClass* EventsDataHub)
  : Config(Config), RManager(RManager), EventsDataHub(EventsDataHub)
{
  H["GetNumPMs"] = "Number of sensors in the available events dataset. If the dataset is empty, 0 is returned.";
  H["GetNumEvents"] = "Number of available events.";
  H["GetPMsignal"] = "Get signal for the event number ievent and PM number ipm.";
  H["SetPMsignal"] = "Set signal value for the event number ievent and PM number ipm.";

  H["GetRho2"] = "Square of the distance between the reconstructed position of the ievent event and the center of the iPM photosensor";
  H["GetRho"] = "Distance between the reconstructed position of the ievent event and the center of the iPM photosensor";

  H["SetReconstructed"] = "For event number ievent set reconstructed x,y,z and energy.\n"
                          "The function automatically sets status ReconstructionOK and GoodEvent to true for this event.\n"
                          "After all events are reconstructed, SetReconstructionReady() has to be called!";

  H["GetStatistics"] = "Returns (if available) an array with GoodEvents, Average_Chi2, Average_XY_deviation";
}

double InterfaceToData::GetPMsignal(int ievent, int ipm)
{
  int numEvents = GetNumEvents();
  int numPMs = GetNumPMs();
  if (ievent<0 || ievent>numEvents-1)
    {
      abort("Wrong event number "+QString::number(ievent)+" Events available: "+QString::number(numEvents));
      return 0;
    }
  if (ipm<0 || ipm>numPMs-1)
    {
      abort("Wrong PM number "+QString::number(ipm)+"; PMs in the events data file: "+QString::number(numPMs));
      return 0;
    }

  return EventsDataHub->Events.at(ievent).at(ipm);
}

void InterfaceToData::SetPMsignal(int ievent, int ipm, double value)
{
    int numEvents = GetNumEvents();
    int numPMs = GetNumPMs();
    if (ievent<0 || ievent>numEvents-1)
      {
        abort("Wrong event number "+QString::number(ievent)+" Events available: "+QString::number(numEvents));
        return;
      }
    if (ipm<0 || ipm>numPMs-1)
      {
        abort("Wrong PM number "+QString::number(ipm)+"; PMs in the events data file: "+QString::number(numPMs));
        return;
      }

    EventsDataHub->Events[ievent][ipm] = value;
}

int InterfaceToData::GetNumPMs()
{
  if (EventsDataHub->Events.isEmpty()) return 0;
  return EventsDataHub->Events.first().size();
}

int InterfaceToData::countPMs()
{
    if (EventsDataHub->Events.isEmpty()) return 0;
    return EventsDataHub->Events.first().size();
}

int InterfaceToData::GetNumEvents()
{
    return EventsDataHub->Events.size();
}

int InterfaceToData::countEvents()
{
    return EventsDataHub->Events.size();
}

bool InterfaceToData::checkEventNumber(int ievent)
{ 
  int numEvents = EventsDataHub->Events.size();
  if (ievent<0 || ievent>numEvents-1)
    {
      abort("Wrong event number "+QString::number(ievent)+" Events available: "+QString::number(numEvents));
      return false;
    }
  return true;
}

bool InterfaceToData::checkEventNumber(int igroup, int ievent, int ipoint)
{
    int numGroups = EventsDataHub->ReconstructionData.size();
    if (igroup<0 || igroup>numGroups-1)
      {
        abort("Wrong group number "+QString::number(igroup)+" Groups available: "+QString::number(numGroups));
        return false;
      }
    //if (!EventsDataHub->isReconstructionReady())
    //  {
    //    abort("Reconstruction was not yet performed");
    //    return false;
    //  }
    int numEvents = EventsDataHub->ReconstructionData.at(igroup).size();
    if (ievent<0 || ievent>numEvents-1)
      {
        abort("Wrong event number "+QString::number(ievent)+" Reconstructed events available: "+QString::number(numEvents));
        return false;
      }
    int numPoints = EventsDataHub->ReconstructionData.at(igroup).at(ievent)->Points.size();
    if (ipoint<0 || ipoint>numPoints-1)
      {
        abort("Wrong point "+QString::number(ipoint)+" Points available: "+QString::number(numPoints));
        return false;
      }
    return true;
}

bool InterfaceToData::checkPM(int ipm)
{
  if (ipm<0 || ipm>Config->GetDetector()->PMs->count()-1)
      {
        abort("Invalid PM index: "+QString::number(ipm));
        return false;
      }
  return true;
}

bool InterfaceToData::checkSetReconstructionDataRequest(int ievent)
{
  int numEvents = EventsDataHub->Events.size();
  if (ievent<0 || ievent>numEvents-1)
    {
      abort("Wrong event number "+QString::number(ievent)+" Events available: "+QString::number(numEvents));
      return false;
    }
  return true;
}

bool InterfaceToData::checkTrueDataRequest(int ievent)
{
  if (EventsDataHub->isScanEmpty())
    {
      abort("There are no scan/true data!");
      return false;
    }
  int numEvents = EventsDataHub->Scan.size();
  if (ievent<0 || ievent>numEvents-1)
    {
      abort("Wrong event number "+QString::number(ievent)+" Events available: "+QString::number(numEvents));
      return false;
    }
  return true;
}

double InterfaceToData::GetReconstructedX(int ievent)
{
  if (!checkEventNumber(ievent)) return 0; //anyway aborted
  return EventsDataHub->ReconstructionData.at(0).at(ievent)->Points[0].r[0];
}

double InterfaceToData::GetReconstructedX(int igroup, int ievent, int ipoint)
{
  if (!checkEventNumber(igroup, ievent, ipoint)) return 0; //anyway aborted
  return EventsDataHub->ReconstructionData.at(igroup).at(ievent)->Points[ipoint].r[0];
}

double InterfaceToData::GetReconstructedY(int ievent)
{
  if (!checkEventNumber(ievent)) return 0; //anyway aborted
  return EventsDataHub->ReconstructionData.at(0).at(ievent)->Points[0].r[1];
}

double InterfaceToData::GetReconstructedY(int igroup, int ievent, int ipoint)
{
  if (!checkEventNumber(igroup, ievent, ipoint)) return 0; //anyway aborted
  return EventsDataHub->ReconstructionData.at(igroup).at(ievent)->Points[ipoint].r[1];
}

double InterfaceToData::GetReconstructedZ(int ievent)
{
  if (!checkEventNumber(ievent)) return 0; //anyway aborted
  return EventsDataHub->ReconstructionData.at(0).at(ievent)->Points[0].r[2];
}

double InterfaceToData::GetReconstructedZ(int igroup, int ievent, int ipoint)
{
  if (!checkEventNumber(igroup, ievent, ipoint)) return 0; //anyway aborted
  return EventsDataHub->ReconstructionData.at(igroup).at(ievent)->Points[ipoint].r[2];
}

double InterfaceToData::GetRho(int ievent, int iPM)
{
    if (!checkPM(iPM)) return 0;
    if (!checkEventNumber(ievent)) return 0; //anyway aborted
    return sqrt( GetRho2(ievent, iPM) );
}

double InterfaceToData::GetRho(int igroup, int ievent, int ipoint, int iPM)
{
  if (!checkPM(iPM)) return 0;
  if (!checkEventNumber(igroup, ievent, ipoint)) return 0; //anyway aborted
  return EventsDataHub->ReconstructionData.at(igroup).at(ievent)->Points[ipoint].r[2];
}

double InterfaceToData::GetRho2(int ievent, int iPM)
{
  if (!checkPM(iPM)) return 0;
  if (!checkEventNumber(ievent)) return 0; //anyway aborted
  double dx2 = EventsDataHub->ReconstructionData.at(0).at(ievent)->Points[0].r[0] - Config->GetDetector()->PMs->X(iPM);
  dx2 *= dx2;
  double dy2 = EventsDataHub->ReconstructionData.at(0).at(ievent)->Points[0].r[1] - Config->GetDetector()->PMs->Y(iPM);
  dy2 *= dy2;
  return dx2+dy2;
}

double InterfaceToData::GetRho2(int igroup, int ievent, int ipoint, int iPM)
{
    if (!checkPM(iPM)) return 0;
    if (!checkEventNumber(igroup, ievent, ipoint)) return 0; //anyway aborted
    double dx2 = EventsDataHub->ReconstructionData.at(0).at(ievent)->Points[0].r[0] - Config->GetDetector()->PMs->X(iPM);
    dx2 *= dx2;
    double dy2 = EventsDataHub->ReconstructionData.at(0).at(ievent)->Points[0].r[1] - Config->GetDetector()->PMs->Y(iPM);
    dy2 *= dy2;
    return dx2+dy2;
}

double InterfaceToData::GetReconstructedEnergy(int ievent)
{
  if (!checkEventNumber(ievent)) return 0; //anyway aborted
  return EventsDataHub->ReconstructionData.at(0).at(ievent)->Points[0].energy;
}

double InterfaceToData::GetReconstructedEnergy(int igroup, int ievent, int ipoint)
{
    if (!checkEventNumber(igroup, ievent, ipoint)) return 0;
    return EventsDataHub->ReconstructionData.at(igroup).at(ievent)->Points[ipoint].energy;
}

bool InterfaceToData::IsReconstructedGoodEvent(int ievent)
{
  if (!checkEventNumber(ievent)) return 0; //anyway aborted
  return EventsDataHub->ReconstructionData.at(0).at(ievent)->GoodEvent;
}

bool InterfaceToData::IsReconstructedGoodEvent(int igroup, int ievent)
{
    if (!checkEventNumber(igroup, ievent, 0)) return 0;
    return EventsDataHub->ReconstructionData.at(igroup).at(ievent)->GoodEvent;
}

int InterfaceToData::countReconstructedGroups()
{
    return EventsDataHub->ReconstructionData.size();
}

int InterfaceToData::countReconstructedEvents(int igroup)
{
    if (igroup<0 || igroup>EventsDataHub->ReconstructionData.size()-1) return -1;
    return EventsDataHub->ReconstructionData.at(igroup).size();
}

int InterfaceToData::countReconstructedPoints(int igroup, int ievent)
{
    if (igroup<0 || igroup>EventsDataHub->ReconstructionData.size()-1) return -1;
    if (ievent<0 || ievent>EventsDataHub->ReconstructionData.at(igroup).size()-1) return -1;
    return EventsDataHub->ReconstructionData.at(igroup).at(ievent)->Points.size();
}

double InterfaceToData::GetTrueX(int ievent)
{
  if (!checkTrueDataRequest(ievent)) return 0; //anyway aborted
  return EventsDataHub->Scan.at(ievent)->Points[0].r[0];
}

double InterfaceToData::GetTrueY(int ievent)
{
  if (!checkTrueDataRequest(ievent)) return 0; //anyway aborted
  return EventsDataHub->Scan.at(ievent)->Points[0].r[1];
}

double InterfaceToData::GetTrueZ(int ievent)
{
  if (!checkTrueDataRequest(ievent)) return 0; //anyway aborted
  return EventsDataHub->Scan.at(ievent)->Points[0].r[2];
}

double InterfaceToData::GetTrueEnergy(int ievent)
{
  if (!checkTrueDataRequest(ievent)) return 0; //anyway aborted
  return EventsDataHub->Scan.at(ievent)->Points[0].energy;
}

int InterfaceToData::GetTruePoints(int ievent)
{
  if (!checkTrueDataRequest(ievent)) return 0; //anyway aborted
  return EventsDataHub->Scan.at(ievent)->Points.size();
}

bool InterfaceToData::IsTrueGoodEvent(int ievent)
{
  if (!checkTrueDataRequest(ievent)) return 0; //anyway aborted
  return EventsDataHub->Scan.at(ievent)->GoodEvent;
}

bool InterfaceToData::GetTrueNumberPoints(int ievent)
{
  if (!checkTrueDataRequest(ievent)) return 0; //anyway aborted
  return EventsDataHub->Scan.at(ievent)->Points.size();
}

void InterfaceToData::SetScanX(int ievent, double value)
{
    if (!checkTrueDataRequest(ievent)) return; //anyway aborted
    EventsDataHub->Scan.at(ievent)->Points[0].r[0] = value;
}

void InterfaceToData::SetScanY(int ievent, double value)
{
    if (!checkTrueDataRequest(ievent)) return; //anyway aborted
    EventsDataHub->Scan.at(ievent)->Points[0].r[1] = value;
}

void InterfaceToData::SetScanZ(int ievent, double value)
{
    if (!checkTrueDataRequest(ievent)) return; //anyway aborted
    EventsDataHub->Scan.at(ievent)->Points[0].r[2] = value;
}

void InterfaceToData::SetScanEnergy(int ievent, double value)
{
    if (!checkTrueDataRequest(ievent)) return; //anyway aborted
    EventsDataHub->Scan.at(ievent)->Points[0].energy = value;
}

bool pairMore (const std::pair<int, float>& p1, const std::pair<int, float>& p2)
{
    return (p1.second > p2.second);
}

QVariant InterfaceToData::GetPMsSortedBySignal(int ievent)
{
    if (!checkEventNumber(ievent)) return 0; //aborted anyway

    const int numPMs = EventsDataHub->Events.at(ievent).size();
    std::vector< std::pair<int, float> > ar;
    ar.reserve(numPMs);

    for (int i=0; i<numPMs; i++)
        ar.push_back( std::pair<int, float>(i, EventsDataHub->Events.at(ievent).at(i)) );

    std::sort(ar.begin(), ar.end(), pairMore);

    QVariantList aa;
    for (const std::pair<int, float>& p : ar )
        aa << p.first;
    return aa;
}

int InterfaceToData::GetPMwithMaxSignal(int ievent)
{
    if (!checkEventNumber(ievent)) return 0; //aborted anyway

    float MaxSig = EventsDataHub->Events.at(ievent).at(0);
    int iMaxSig = 0;
    for (int i=0; i<EventsDataHub->Events.at(ievent).size(); i++)
    {
        float sig = EventsDataHub->Events.at(ievent).at(i);
        if (sig > MaxSig)
        {
            MaxSig = sig;
            iMaxSig = i;
        }
    }
    return iMaxSig;
}

void InterfaceToData::SetReconstructed(int ievent, double x, double y, double z, double e)
{
  if (!checkSetReconstructionDataRequest(ievent)) return;
  EventsDataHub->ReconstructionData[0][ievent]->Points[0].r[0] = x;
  EventsDataHub->ReconstructionData[0][ievent]->Points[0].r[1] = y;
  EventsDataHub->ReconstructionData[0][ievent]->Points[0].r[2] = z;
  EventsDataHub->ReconstructionData[0][ievent]->Points[0].energy = e;
  EventsDataHub->ReconstructionData[0][ievent]->ReconstructionOK = true;
  EventsDataHub->ReconstructionData[0][ievent]->GoodEvent = true;
}

void InterfaceToData::SetReconstructedX(int ievent, double x)
{
  if (!checkSetReconstructionDataRequest(ievent)) return;
  EventsDataHub->ReconstructionData[0][ievent]->Points[0].r[0] = x;
}

void InterfaceToData::SetReconstructedY(int ievent, double y)
{
  if (!checkSetReconstructionDataRequest(ievent)) return;
  EventsDataHub->ReconstructionData[0][ievent]->Points[0].r[1] = y;
}

void InterfaceToData::SetReconstructedZ(int ievent, double z)
{
  if (!checkSetReconstructionDataRequest(ievent)) return;
  EventsDataHub->ReconstructionData[0][ievent]->Points[0].r[2] = z;
}

void InterfaceToData::SetReconstructedEnergy(int ievent, double e)
{
  if (!checkSetReconstructionDataRequest(ievent)) return;
  EventsDataHub->ReconstructionData[0][ievent]->Points[0].energy = e;
}

void InterfaceToData::SetReconstructedGoodEvent(int ievent, bool good)
{
  if (!checkSetReconstructionDataRequest(ievent)) return;
  EventsDataHub->ReconstructionData[0][ievent]->ReconstructionOK = true;
  EventsDataHub->ReconstructionData[0][ievent]->GoodEvent = good;
}

void InterfaceToData::SetReconstructedAllEventsGood(bool flag)
{
    for (int i=0; i<EventsDataHub->ReconstructionData.at(0).size(); i++)
        EventsDataHub->ReconstructionData[0][i]->GoodEvent = flag;
}

void InterfaceToData::SetReconstructionOK(int ievent, bool OK)
{
  if (!checkSetReconstructionDataRequest(ievent)) return;
  EventsDataHub->ReconstructionData[0][ievent]->ReconstructionOK = OK;
}

void InterfaceToData::SetReconstructed(int igroup, int ievent, int ipoint, double x, double y, double z, double e)
{
  if (!checkEventNumber(igroup, ievent, ipoint)) return;
  EventsDataHub->ReconstructionData[igroup][ievent]->Points[ipoint].r[0] = x;
  EventsDataHub->ReconstructionData[igroup][ievent]->Points[ipoint].r[1] = y;
  EventsDataHub->ReconstructionData[igroup][ievent]->Points[ipoint].r[2] = z;
  EventsDataHub->ReconstructionData[igroup][ievent]->Points[ipoint].energy = e;
  EventsDataHub->ReconstructionData[igroup][ievent]->ReconstructionOK = true;
  EventsDataHub->ReconstructionData[igroup][ievent]->GoodEvent = true;
}

void InterfaceToData::SetReconstructedFast(int igroup, int ievent, int ipoint, double x, double y, double z, double e)
{
    EventsDataHub->ReconstructionData[igroup][ievent]->Points[ipoint].r[0] = x;
    EventsDataHub->ReconstructionData[igroup][ievent]->Points[ipoint].r[1] = y;
    EventsDataHub->ReconstructionData[igroup][ievent]->Points[ipoint].r[2] = z;
    EventsDataHub->ReconstructionData[igroup][ievent]->Points[ipoint].energy = e;
    EventsDataHub->ReconstructionData[igroup][ievent]->ReconstructionOK = true;
    EventsDataHub->ReconstructionData[igroup][ievent]->GoodEvent = true;
}

void InterfaceToData::AddReconstructedPoint(int igroup, int ievent, double x, double y, double z, double e)
{
  if (!checkEventNumber(igroup, ievent, 0)) return;
  EventsDataHub->ReconstructionData[igroup][ievent]->Points.AddPoint(x, y, z, e);
}

void InterfaceToData::SetReconstructedX(int igroup, int ievent, int ipoint, double x)
{
  if (!checkEventNumber(igroup, ievent, ipoint)) return;
  EventsDataHub->ReconstructionData[igroup][ievent]->Points[ipoint].r[0] = x;
}

void InterfaceToData::SetReconstructedY(int igroup, int ievent, int ipoint, double y)
{
    if (!checkEventNumber(igroup, ievent, ipoint)) return;
    EventsDataHub->ReconstructionData[igroup][ievent]->Points[ipoint].r[1] = y;
}

void InterfaceToData::SetReconstructedZ(int igroup, int ievent, int ipoint, double z)
{
    if (!checkEventNumber(igroup, ievent, ipoint)) return;
    EventsDataHub->ReconstructionData[igroup][ievent]->Points[ipoint].r[2] = z;
}

void InterfaceToData::SetReconstructedEnergy(int igroup, int ievent, int ipoint, double e)
{
  if (!checkEventNumber(igroup, ievent, ipoint)) return;
  EventsDataHub->ReconstructionData[igroup][ievent]->Points[ipoint].energy = e;
}

void InterfaceToData::SetReconstructedGoodEvent(int igroup, int ievent, int ipoint, bool good)
{
    if (!checkEventNumber(igroup, ievent, ipoint)) return;
    EventsDataHub->ReconstructionData[igroup][ievent]->GoodEvent = good;
}

void InterfaceToData::SetReconstructionOK(int igroup, int ievent, int ipoint, bool OK)
{
  if (!checkEventNumber(igroup, ievent, ipoint)) return;
  EventsDataHub->ReconstructionData[igroup][ievent]->ReconstructionOK = OK;
}

void InterfaceToData::SetReconstructionReady()
{
  EventsDataHub->fReconstructionDataReady = true;
  emit RequestEventsGuiUpdate();
}

void InterfaceToData::ResetReconstructionData(int numGroups)
{
    for (int ig=0; ig<numGroups; ig++)
      EventsDataHub->resetReconstructionData(numGroups);
}

void InterfaceToData::LoadEventsTree(QString fileName, bool Append, int MaxNumEvents)
{
  if (!Append) EventsDataHub->clear();
  EventsDataHub->loadSimulatedEventsFromTree(fileName, Config->GetDetector()->PMs, MaxNumEvents);
  //EventsDataHub->clearReconstruction();
  EventsDataHub->createDefaultReconstructionData();
  //RManager->filterEvents(Config->JSON);
  EventsDataHub->squeeze();
  emit RequestEventsGuiUpdate();
}

void InterfaceToData::LoadEventsAscii(QString fileName, bool Append)
{
  if (!Append) EventsDataHub->clear();
  EventsDataHub->loadEventsFromTxtFile(fileName, Config->JSON, Config->GetDetector()->PMs);
  //EventsDataHub->clearReconstruction();
  EventsDataHub->createDefaultReconstructionData();
  //RManager->filterEvents(Config->JSON);
  EventsDataHub->squeeze();
  emit RequestEventsGuiUpdate();
}

void InterfaceToData::ClearEvents()
{
  EventsDataHub->clear(); //gui update is triggered inside
}

void InterfaceToData::PurgeBad()
{
  EventsDataHub->PurgeFilteredEvents();
}

void InterfaceToData::Purge(int LeaveOnePer)
{
  EventsDataHub->Purge(LeaveOnePer);
}

QVariant InterfaceToData::GetStatistics(int igroup)
{
  int GoodEvents;
  double AvChi2, AvDeviation;
  EventsDataHub->prepareStatisticsForEvents(Config->GetDetector()->LRFs->isAllLRFsDefined(), GoodEvents, AvChi2, AvDeviation, igroup);

  QVariantList l;
  l << GoodEvents << AvChi2 << AvDeviation;
  return l;
}

//----------------------------------
InterfaceToLRF::InterfaceToLRF(AConfiguration *Config, EventsDataClass *EventsDataHub)
  : Config(Config), EventsDataHub(EventsDataHub) //,f2d(0)
{
  SensLRF = Config->GetDetector()->LRFs->getOldModule();

  H["Make"] = "Calculates new LRFs";

  H["CountIterations"] = "Returns the number of LRF iterations in history.";
  H["GetCurrent"] = "Returns the index of the current LRF iteration.";
  //H["ShowVsXY"] = "Plots a 2D histogram of the LRF. Does not work for 3D LRFs!";
}

QString InterfaceToLRF::Make()
{
  QJsonObject jsR = Config->JSON["ReconstructionConfig"].toObject();
  SensLRF->LRFmakeJson = jsR["LRFmakeJson"].toObject();
  bool ok = SensLRF->makeLRFs(SensLRF->LRFmakeJson, EventsDataHub, Config->GetDetector()->PMs);
  Config->AskForLRFGuiUpdate();
  if (!ok) return SensLRF->getLastError();
  else return "";
}

double InterfaceToLRF::GetLRF(int ipm, double x, double y, double z)
{
    //qDebug() << ipm<<x<<y<<z;
    //qDebug() << SensLRF->getIteration()->countPMs();
    if (!SensLRF->isAllLRFsDefined()) return 0;
    if (ipm<0 || ipm >= SensLRF->getIteration()->countPMs()) return 0;
    return SensLRF->getLRF(ipm, x, y, z);
}

double InterfaceToLRF::GetLRFerror(int ipm, double x, double y, double z)
{
    if (!SensLRF->isAllLRFsDefined()) return 0;
    if (ipm<0 || ipm >= SensLRF->getIteration()->countPMs()) return 0;
    return SensLRF->getLRFErr(ipm, x, y, z);
}

//void InterfaceToLRF::ShowVsXY(int ipm, int PointsX, int PointsY)
//{
//  int iterIndex = -1;
//  if (!getValidIteration(iterIndex)) return;
//  if (ipm < 0 || ipm >= Config->GetDetector()->PMs->count())
//    return abort("Invalid sensor number "+QString::number(ipm)+"\n");

//  double minmax[4];
//  const PMsensor *sensor = SensLRF->getIteration(iterIndex)->sensor(ipm);
//  sensor->getGlobalMinMax(minmax);

//  if (f2d) delete f2d;
//  f2d = new TF2("f2d", SensLRF, &SensorLRFs::getFunction2D,
//                minmax[0], minmax[1], minmax[2], minmax[3], 2,
//                "SensLRF", "getFunction2D");

//  f2d->SetParameter(0, ipm);
//  f2d->SetParameter(1, 0);//z0);

//  f2d->SetNpx(PointsX);
//  f2d->SetNpy(PointsY);
//  MW->GraphWindow->DrawWithoutFocus(f2d, "surf");
//}

int InterfaceToLRF::CountIterations()
{
  return SensLRF->countIterations();
}

int InterfaceToLRF::GetCurrent()
{
  return SensLRF->getCurrentIterIndex();
}

void InterfaceToLRF::SetCurrent(int iterIndex)
{
  if(!getValidIteration(iterIndex)) return;
  SensLRF->setCurrentIter(iterIndex);
  Config->AskForLRFGuiUpdate();
}

void InterfaceToLRF::SetCurrentName(QString name)
{
    SensLRF->setCurrentIterName(name);
    Config->AskForLRFGuiUpdate();
}

void InterfaceToLRF::DeleteCurrent()
{
  int iterIndex = -1;
  if(!getValidIteration(iterIndex)) return;
  SensLRF->deleteIteration(iterIndex);
  Config->AskForLRFGuiUpdate();
}

QString InterfaceToLRF::Save(QString fileName)
{
  int iterIndex = -1;
  if (!getValidIteration(iterIndex)) return "No data to save";

  QFile saveFile(fileName);
  if (!saveFile.open(QIODevice::WriteOnly))
      return "Cannot open file "+fileName+" to save LRFs!";

  QJsonObject json;
  if (!SensLRF->saveIteration(iterIndex, json))
    {
      saveFile.close();
      return SensLRF->getLastError();
    }

  QJsonDocument saveDoc(json);
  saveFile.write(saveDoc.toJson());
  saveFile.close();
  return "";
}

int InterfaceToLRF::Load(QString fileName)
{
  QFile loadFile(fileName);
  if (!loadFile.open(QIODevice::ReadOnly)) {
    abort("Cannot open save file\n");
    return -1;
  }

  QByteArray saveData = loadFile.readAll();
  loadFile.close();

  QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));
  QJsonObject json = loadDoc.object();
  if(!SensLRF->loadAll(json)) {
    abort("Failed to load LRFs: "+SensLRF->getLastError()+"\n");
    return -1;
  }

  //update GUI
  Config->AskForLRFGuiUpdate();

  return SensLRF->getCurrentIterIndex();
}

bool InterfaceToLRF::getValidIteration(int &iterIndex)
{
  if (iterIndex < -1) {
      abort(QString::number(iterIndex)+" &lt; -1, therefore invalid iteration index.\n");
      return false;
  }
  if (iterIndex == -1)
    iterIndex = SensLRF->getCurrentIterIndex();
  if (iterIndex == -1) {
    abort("There is no LRF data!\n");
    return false;
  }

  int countIter = SensLRF->countIterations();
  if (iterIndex >= countIter) {
    abort("Invalid iteration index "+QString::number(iterIndex)+
          ". There are only "+QString::number(countIter)+" iteration(s).\n");
    return false;
  }

  return true;
}

#ifdef GUI
//----------------------------------
InterfaceToGeoWin::InterfaceToGeoWin(MainWindow *MW, TmpObjHubClass* TmpHub)
 : MW(MW), TmpHub(TmpHub)
{
  Detector = MW->Detector;
  H["SaveImage"] = "Save image currently shown on the geometry window to an image file.\nTip: use .png extension";
}

InterfaceToGeoWin::~InterfaceToGeoWin()
{
   TmpHub->ClearTracks();
}

double InterfaceToGeoWin::GetPhi()
{
    return MW->GeometryWindow->Phi;
}

double InterfaceToGeoWin::GetTheta()
{
  return MW->GeometryWindow->Theta;
}

void InterfaceToGeoWin::SetPhi(double phi)
{
  MW->GeometryWindow->Phi = phi;
}

void InterfaceToGeoWin::SetTheta(double theta)
{
  MW->GeometryWindow->Theta = theta;
}

void InterfaceToGeoWin::Rotate(double Theta, double Phi, int Steps, int msPause)
{
  if (Steps <= 0) return;
  double stepT = Theta/Steps;
  double stepP = Phi/Steps;
  double T0 = GetTheta();
  double P0 = GetPhi();

  QTime time;
  MW->GeometryWindow->fNeedZoom = false;
  MW->GeometryWindow->fRecallWindow = true;
  for (int i=0; i<Steps; i++)
    {
      qApp->processEvents();
      time.restart();
      MW->GeometryWindow->Theta = T0 + stepT*(i+1);
      MW->GeometryWindow->Phi   = P0 + stepP*(i+1);
      MW->GeometryWindow->PostDraw();

      int msPassed = time.elapsed();
      if (msPassed<msPause) QThread::msleep(msPause-msPassed);
    }
}

void InterfaceToGeoWin::SetZoom(int level)
{
  MW->GeometryWindow->ZoomLevel = level;
  MW->GeometryWindow->Zoom(true);
  MW->GeometryWindow->on_pbShowGeometry_clicked();
  MW->GeometryWindow->readRasterWindowProperties();
}

void InterfaceToGeoWin::UpdateView()
{
  MW->GeometryWindow->fRecallWindow = true;
  MW->GeometryWindow->PostDraw();
  MW->GeometryWindow->UpdateRootCanvas();
}

void InterfaceToGeoWin::SetParallel(bool on)
{
  MW->GeometryWindow->ModePerspective = !on;
}

void InterfaceToGeoWin::Show()
{
  MW->GeometryWindow->showNormal();
  MW->GeometryWindow->raise();
}

void InterfaceToGeoWin::Hide()
{
  MW->GeometryWindow->hide();
}

void InterfaceToGeoWin::BlockUpdates(bool on)
{
  MW->DoNotUpdateGeometry = on;
  MW->GeometryDrawDisabled = on;
}

int InterfaceToGeoWin::GetX()
{
  return MW->GeometryWindow->x();
}

int InterfaceToGeoWin::GetY()
{
  return MW->GeometryWindow->y();
}

int InterfaceToGeoWin::GetW()
{
  return MW->GeometryWindow->width();
}

int InterfaceToGeoWin::GetH()
{
  return MW->GeometryWindow->height();
}

void InterfaceToGeoWin::ShowGeometry()
{
  MW->GeometryWindow->readRasterWindowProperties();
  MW->ShowGeometry(false);
}

void InterfaceToGeoWin::ShowPMnumbers()
{
  MW->GeometryWindow->on_pbShowPMnumbers_clicked();
}

void InterfaceToGeoWin::ShowReconstructedPositions()
{
  //MW->Rwindow->ShowReconstructionPositionsIfWindowVisible();
  MW->Rwindow->ShowPositions(0, true);
}

void InterfaceToGeoWin::SetShowOnlyFirstEvents(bool fOn, int number)
{
  MW->Rwindow->SetShowFirst(fOn, number);
}

void InterfaceToGeoWin::ShowTruePositions()
{
  MW->Rwindow->DotActualPositions();
  MW->ShowGeometry(false, false);
}

void InterfaceToGeoWin::ShowTracks(int num, int OnlyColor)
{
  Detector->GeoManager->ClearTracks();
  if (TmpHub->TrackInfo.isEmpty()) return;

  for (int iTr=0; iTr<TmpHub->TrackInfo.size() && iTr<num; iTr++)
    {
      TrackHolderClass* th = TmpHub->TrackInfo.at(iTr);
      TGeoTrack* track = new TGeoTrack(1, th->UserIndex);
      track->SetLineColor(th->Color);
      track->SetLineWidth(th->Width);
      for (int iNode=0; iNode<th->Nodes.size(); iNode++)
        track->AddPoint(th->Nodes[iNode].R[0], th->Nodes[iNode].R[1], th->Nodes[iNode].R[2], th->Nodes[iNode].Time);

      if (track->GetNpoints()>1)
        {
          if (OnlyColor == -1  || OnlyColor == th->Color)
            Detector->GeoManager->AddTrack(track);
        }
      else delete track;
    }
  MW->ShowTracks();
}

void InterfaceToGeoWin::ShowSPS_position()
{
  MW->on_pbSingleSourceShow_clicked();
}

void InterfaceToGeoWin::ClearTracks()
{
  MW->GeometryWindow->on_pbClearTracks_clicked();
}

void InterfaceToGeoWin::ClearMarkers()
{
  MW->GeometryWindow->on_pbClearDots_clicked();
}

void InterfaceToGeoWin::SaveImage(QString fileName)
{
  MW->GeometryWindow->SaveAs(fileName);
}

void InterfaceToGeoWin::ShowTracksMovie(int num, int steps, int msDelay, double dTheta, double dPhi, double rotSteps, int color)
{
  int tracks = TmpHub->TrackInfo.size();
  if (tracks == 0) return;
  if (num > tracks) num = tracks;

  int toDo;
  int thisNodes = 2;
  do //finished when all nodes of the longest track are shown
    {
      //this iteration will show track nodes < thisNode
      toDo = num; //every track when finished results in toDo--

      // cycle by indication step with interpolation between the nodes
      for (int iStep=0; iStep<steps; iStep++)
        {
          MW->Detector->GeoManager->ClearTracks();

          //cycle by tracks
          for (int iTr=0; iTr<num; iTr++)
            {
              TrackHolderClass* th = TmpHub->TrackInfo.at(iTr);
              int ThisTrackNodes = th->Nodes.size();
              if (ThisTrackNodes <= thisNodes && iStep == steps-1) toDo--; //last node of this track

              TGeoTrack* track = new TGeoTrack(1, th->UserIndex);
              track->SetLineWidth(th->Width+1);
              if (color == -1) track->SetLineColor(15); // during tracking
              else track->SetLineColor(color); // during tracking
              int lim = std::min(thisNodes, th->Nodes.size());
              for (int iNode=0; iNode<lim; iNode++)
                {
                  double x, y, z;
                  if (iNode == thisNodes-1)
                    {
                      //here track is in interpolation mode
                      x = th->Nodes[iNode-1].R[0] + (th->Nodes[iNode].R[0]-th->Nodes[iNode-1].R[0])*iStep/(steps-1);
                      y = th->Nodes[iNode-1].R[1] + (th->Nodes[iNode].R[1]-th->Nodes[iNode-1].R[1])*iStep/(steps-1);
                      z = th->Nodes[iNode-1].R[2] + (th->Nodes[iNode].R[2]-th->Nodes[iNode-1].R[2])*iStep/(steps-1);
                      if (color == -1) track->SetLineColor(15); //black during tracking
                    }
                  else
                    {
                      x = th->Nodes[iNode].R[0];
                      y = th->Nodes[iNode].R[1];
                      z = th->Nodes[iNode].R[2];
                      if (color == -1) track->SetLineColor( th->Color );
                    }
                  track->AddPoint(x, y, z, th->Nodes[iNode].Time);
                }
              if (color == -1)
                if (ThisTrackNodes <= thisNodes && iStep == steps-1) track->SetLineColor( th->Color );

              //adding track to GeoManager
              if (track->GetNpoints()>1)
                Detector->GeoManager->AddTrack(track);
              else delete track;
            }
          MW->ShowTracks();
          Rotate(dTheta, dPhi, rotSteps);
          QThread::msleep(msDelay);
        }
      thisNodes++;
    }
  while (toDo>0);
}

void InterfaceToGeoWin::ShowEnergyVector()
{
  MW->Rwindow->UpdateSimVizData(0);
}
#endif
//----------------------------------
InterfaceToGraphs::InterfaceToGraphs(TmpObjHubClass *TmpHub)
  : TmpHub(TmpHub)
{
  H["NewGraph"] = "Creates a new graph (Root TGraph object)";
  H["SetMarkerProperties"] = "Default marker properties are 1, 20, 1";
  H["SetLineProperties"] = "Default line properties are 1, 1, 2";
  H["Draw"] = "Draws the graph (use \"APL\" options if in doubt)";
}

bool InterfaceToGraphs::InitOnRun()
{
  TmpHub->ScriptDrawObjects.clear();
  return true;
}

void InterfaceToGraphs::NewGraph(QString GraphName)
{
  int index = TmpHub->ScriptDrawObjects.findIndexOf(GraphName);
  if (index != -1)
    {
      abort("Bad new graph name! Object "+GraphName+" already exists");
      return;
    }

  TGraph* gr = new TGraph();
  TmpHub->ScriptDrawObjects.append(gr, GraphName, "TGraph");
  gr->SetFillColor(0);
  gr->SetFillStyle(0);
}

void InterfaceToGraphs::SetMarkerProperties(QString GraphName, int MarkerColor, int MarkerStyle, int MarkerSize)
{
  int index = TmpHub->ScriptDrawObjects.findIndexOf(GraphName);
  if (index == -1)
    {
      abort("Graph "+GraphName+" not found!");
      return;
    }

  RootDrawObj& r = TmpHub->ScriptDrawObjects.List[index];
  r.MarkerColor = MarkerColor;
  r.MarkerStyle = MarkerStyle;
  r.MarkerSize = MarkerSize;
}

void InterfaceToGraphs::SetLineProperties(QString GraphName, int LineColor, int LineStyle, int LineWidth)
{
  int index = TmpHub->ScriptDrawObjects.findIndexOf(GraphName);
  if (index == -1)
    {
      abort("Graph "+GraphName+" not found!");
      return;
    }

  RootDrawObj& r = TmpHub->ScriptDrawObjects.List[index];
  r.LineColor = LineColor;
  r.LineStyle = LineStyle;
  r.LineWidth = LineWidth;
}

void InterfaceToGraphs::SetTitles(QString GraphName, QString X_Title, QString Y_Title)
{
  int index = TmpHub->ScriptDrawObjects.findIndexOf(GraphName);
  if (index == -1)
    {
      abort("Graph "+GraphName+" not found!");
      return;
    }

  RootDrawObj& r = TmpHub->ScriptDrawObjects.List[index];
  r.Xtitle = X_Title;
  r.Ytitle = Y_Title;
}

void InterfaceToGraphs::AddPoint(QString GraphName, double x, double y)
{
  int index = TmpHub->ScriptDrawObjects.findIndexOf(GraphName);
  if (index == -1)
    {
      abort("Graph "+GraphName+" not found!");
      return;
    }

  RootDrawObj& r = TmpHub->ScriptDrawObjects.List[index];
  if (r.type == "TGraph")
    {
      TGraph* gr = static_cast<TGraph*>(r.Obj);
      gr->SetPoint(gr->GetN(), x, y);
    }
  else
    {
      abort("Graph "+GraphName+" not found!");
      return;
  }
}

void InterfaceToGraphs::AddPoints(QString GraphName, QVariant xArray, QVariant yArray)
{
    int index = TmpHub->ScriptDrawObjects.findIndexOf(GraphName);
    if (index == -1)
      {
        abort("Graph "+GraphName+" not found!");
        return;
      }

    QString typeX = xArray.typeName();
    QString typeY = yArray.typeName();
    if (typeX != "QVariantList" || typeY != "QVariantList")
    {
        qWarning() << "arrays are expected in graph.AddPoints()";
        return;
    }

    QVariantList vx = xArray.toList();
    QVariantList vy = yArray.toList();
    QJsonArray X = QJsonArray::fromVariantList(vx);
    QJsonArray Y = QJsonArray::fromVariantList(vy);
    if (X.isEmpty() || Y.isEmpty() || X.size()!=Y.size())
    {
        qWarning() << "Empty or mismatch in add array to graph";
        return;
    }

    RootDrawObj& r = TmpHub->ScriptDrawObjects.List[index];
    if (r.type == "TGraph")
      {
        TGraph* gr = static_cast<TGraph*>(r.Obj);

        for (int i=0; i<X.size(); i++)
            if (X[i].isDouble() && Y[i].isDouble())
            {
                double x = X[i].toDouble();
                double y = Y[i].toDouble();
                gr->SetPoint(gr->GetN(), x, y);
            }
      }
    else
      {
        abort("Graph "+GraphName+" not found!");
        return;
    }
}

void InterfaceToGraphs::AddPoints(QString GraphName, QVariant xyArray)
{
    int index = TmpHub->ScriptDrawObjects.findIndexOf(GraphName);
    if (index == -1)
      {
        abort("Graph "+GraphName+" not found!");
        return;
      }

    QString typeArr = xyArray.typeName();
    if (typeArr != "QVariantList")
    {
        qWarning() << "arrays are expected in graph.AddPoints()";
        return;
    }

    QVariantList xy = xyArray.toList();
    QJsonArray XYarr = QJsonArray::fromVariantList(xy);

    RootDrawObj& r = TmpHub->ScriptDrawObjects.List[index];
    if (r.type == "TGraph")
      {
        TGraph* gr = static_cast<TGraph*>(r.Obj);

        for (int i=0; i<XYarr.size(); i++)
        {
           QJsonArray el = XYarr[i].toArray();
           if (el.size() == 2)
           {
               if (el[0].isDouble() && el[1].isDouble())
               {
                   double x = el[0].toDouble();
                   double y = el[1].toDouble();
                   gr->SetPoint(gr->GetN(), x, y);
               }
           }
        }
      }
    else
      {
        abort("Graph "+GraphName+" not found!");
        return;
    }
}

void InterfaceToGraphs::Draw(QString GraphName, QString options)
{
  int index = TmpHub->ScriptDrawObjects.findIndexOf(GraphName);
  if (index == -1)
    {
      abort("Graph "+GraphName+" not found!");
      return;
    }

  RootDrawObj& r = TmpHub->ScriptDrawObjects.List[index];
  if (r.type == "TGraph")
    {
      TGraph* gr = static_cast<TGraph*>(r.Obj);
      if (gr->GetN() == 0) return;

      gr->SetLineColor(r.LineColor);
      gr->SetLineWidth(r.LineWidth);
      gr->SetLineStyle(r.LineStyle);
      gr->SetMarkerColor(r.MarkerColor);
      gr->SetMarkerSize(r.MarkerSize);
      gr->SetMarkerStyle(r.MarkerStyle);
      gr->SetEditable(false);
      gr->GetYaxis()->SetTitleOffset((Float_t)1.30);
      emit RequestDraw(gr, options, true);

      gr->GetXaxis()->SetTitle(r.Xtitle.toLatin1().data());
      gr->GetYaxis()->SetTitle(r.Ytitle.toLatin1().data());
      emit RequestDraw(0, "", true); //to update canvas so axes titles are visible
    }
  else
    {
      abort("Graph "+GraphName+" not found!");
      return;
  }
}

bool InterfaceToGraphs::Delete(QString GraphName)
{
    return TmpHub->ScriptDrawObjects.remove(GraphName);
}

void InterfaceToGraphs::DeleteAllGraph()
{
    TmpHub->ScriptDrawObjects.removeAllGraphs();
}

//----------------------------------
InterfaceToHistD::InterfaceToHistD(TmpObjHubClass* TmpHub)
  : TmpHub(TmpHub)
{
    H["FitGauss"] = "Fit histogram with a Gaussian. The returned result (is successful) contains an array [Constant,Mean,Sigma,ErrConstant,ErrMean,ErrSigma]"
            "\nOptional 'options' parameter is directly forwarded to TH1::Fit()";
    H["FitGaussWithInit"] = "Fit histogram with a Gaussian. The returned result (is successful) contains an array [Constant,Mean,Sigma,ErrConstant,ErrMean,ErrSigma]"
                            "\nInitialParValues is an array of initial parameters of the values [Constant,Mean,Sigma]"
                            "\nOptional 'options' parameter is directly forwarded to TH1::Fit()";
}

bool InterfaceToHistD::InitOnRun()
{
  TmpHub->ScriptDrawObjects.clear();
  return true;
}

void InterfaceToHistD::NewHist(QString HistName, int bins, double start, double stop)
{
  int index = TmpHub->ScriptDrawObjects.findIndexOf(HistName);
  if (index != -1)
    {
      abort("Bad new hist name! Object "+HistName+" already exists");
      return;
    }

  TH1D* hist = new TH1D("", HistName.toLatin1().data(), bins, start, stop);
  hist->GetYaxis()->SetTitleOffset((Float_t)1.30);
  TmpHub->ScriptDrawObjects.append(hist, HistName, "TH1D");
}

void InterfaceToHistD::NewHist2D(QString HistName, int binsX, double startX, double stopX, int binsY, double startY, double stopY)
{
  int index = TmpHub->ScriptDrawObjects.findIndexOf(HistName);
  if (index != -1)
    {
      abort("Bad new hist name! Object "+HistName+" already exists");
      return;
    }

  TH2D* hist = new TH2D("", HistName.toLatin1().data(), binsX, startX, stopX, binsY, startY, stopY);
  hist->GetYaxis()->SetTitleOffset((Float_t)1.30);
  TmpHub->ScriptDrawObjects.append(hist, HistName, "TH2D");
}

void InterfaceToHistD::SetTitles(QString HistName, QString X_Title, QString Y_Title, QString Z_Title)
{
  int index = TmpHub->ScriptDrawObjects.findIndexOf(HistName);
  if (index == -1)
    {
      abort("Histogram "+HistName+" not found!");
      return;
    }

  RootDrawObj& r = TmpHub->ScriptDrawObjects.List[index];
  r.Xtitle = X_Title;
  r.Ytitle = Y_Title;
  r.Ztitle = Z_Title;
}

void InterfaceToHistD::SetLineProperties(QString HistName, int LineColor, int LineStyle, int LineWidth)
{
  int index = TmpHub->ScriptDrawObjects.findIndexOf(HistName);
  if (index == -1)
    {
      abort("Histogram "+HistName+" not found!");
      return;
    }

  RootDrawObj& r = TmpHub->ScriptDrawObjects.List[index];
  r.LineColor = LineColor;
  r.LineStyle = LineStyle;
  r.LineWidth = LineWidth;
}

void InterfaceToHistD::Fill(QString HistName, double val, double weight)
{
  int index = TmpHub->ScriptDrawObjects.findIndexOf(HistName);
  if (index == -1)
    {
      abort("Histogram "+HistName+" not found!");
      return;
    }

  RootDrawObj& r = TmpHub->ScriptDrawObjects.List[index];
  if (r.type == "TH1D")
    {
      TH1D* h = static_cast<TH1D*>(r.Obj);
      h->Fill(val, weight);
    }
  else
    {
      abort("TH1D histogram "+HistName+" not found!");
      return;
    }
}

void InterfaceToHistD::Fill2D(QString HistName, double x, double y, double weight)
{
  int index = TmpHub->ScriptDrawObjects.findIndexOf(HistName);
  if (index == -1)
    {
      abort("Histogram "+HistName+" not found!");
      return;
    }

  RootDrawObj& r = TmpHub->ScriptDrawObjects.List[index];
  if (r.type == "TH2D")
    {
      TH2D* h = static_cast<TH2D*>(r.Obj);
      h->Fill(x, y, weight);
    }
  else
    {
      abort("TH2D histogram "+HistName+" not found!");
      return;
    }
}

void InterfaceToHistD::Smooth(QString HistName, int times)
{
  int index = TmpHub->ScriptDrawObjects.findIndexOf(HistName);
  if (index == -1)
    {
      abort("Histogram "+HistName+" not found!");
      return;
    }

  RootDrawObj& r = TmpHub->ScriptDrawObjects.List[index];
  if (r.type == "TH1D")
    {
      TH1D* h = static_cast<TH1D*>(r.Obj);
      h->Smooth(times);
      emit RequestDraw(0, "", true); //to update
    }
  else if (r.type == "TH2D")
    {
      TH2D* h = static_cast<TH2D*>(r.Obj);
      h->Smooth(times);
      emit RequestDraw(0, "", true); //to update
    }
  else
    {
      abort("Object "+HistName+": unknown histogram type!");
      return;
  }
}

QVariant ReturnNanArray(int num)
{
    QJsonArray ar;
    for (int i=0; i<num; i++) ar << std::numeric_limits<double>::quiet_NaN();
    QJsonValue jv = ar;
    QVariant res = jv.toVariant();
    return res;
}

QVariant InterfaceToHistD::FitGauss(QString HistName, QString options)
{
    int index = TmpHub->ScriptDrawObjects.findIndexOf(HistName);
    if (index == -1)
      {
        abort("Histogram "+HistName+" not found!");
        return ReturnNanArray(6);
      }

    RootDrawObj& r = TmpHub->ScriptDrawObjects.List[index];
    if (r.type.startsWith("TH1"))
      {
        TH1* h = static_cast<TH1*>(r.Obj);
        TF1 *f1 = new TF1("f1", "gaus");
        int status = h->Fit(f1, options.toLatin1());
        if (status != 0) return ReturnNanArray(6);

        emit RequestDraw(0, "", true); //to update

        QJsonArray ar;
        for (int i=0; i<3; i++) ar << f1->GetParameter(i);
        for (int i=0; i<3; i++) ar << f1->GetParError(i);

        QJsonValue jv = ar;
        QVariant res = jv.toVariant();
        return res;
      }
    else
      {
        abort("Object "+HistName+": unsupported histogram type!");
        return ReturnNanArray(6);
    }
}

QVariant InterfaceToHistD::FitGaussWithInit(QString HistName, QVariant InitialParValues, QString options)
{
    int index = TmpHub->ScriptDrawObjects.findIndexOf(HistName);
    if (index == -1)
      {
        abort("Histogram "+HistName+" not found!");
        return ReturnNanArray(6);
      }

    QString type = InitialParValues.typeName();
    if (type != "QVariantList")
    {
        abort("InitialParValues has to be an array of three numeric values");
        return ReturnNanArray(6);
    }

    QVariantList vl = InitialParValues.toList();
    QJsonArray ar = QJsonArray::fromVariantList(vl);
    if (ar.size() < 3)
    {
        abort("InitialParValues has to be an array of three numeric values");
        return ReturnNanArray(6);
    }
    if (!ar[0].isDouble() || !ar[1].isDouble() || !ar[2].isDouble() )
    {
        abort("InitialParValues has to be an array of three numeric values");
        return ReturnNanArray(6);
    }

    RootDrawObj& r = TmpHub->ScriptDrawObjects.List[index];
    if (r.type.startsWith("TH1"))
      {
        TH1* h = static_cast<TH1*>(r.Obj);

        TF1 *f1 = new TF1("f1","[0]*exp(-0.5*((x-[1])/[2])^2)");
        f1->SetParameters(ar[0].toDouble(), ar[1].toDouble(), ar[2].toDouble());

        int status = h->Fit(f1, options.toLatin1());
        if (status != 0) return ReturnNanArray(6);

        emit RequestDraw(0, "", true); //to update

        QJsonArray ar;
        for (int i=0; i<3; i++) ar << f1->GetParameter(i);
        for (int i=0; i<3; i++) ar << f1->GetParError(i);

        QJsonValue jv = ar;
        QVariant res = jv.toVariant();
        return res;
      }
    else
      {
        abort("Object "+HistName+": unsupported histogram type!");
        return ReturnNanArray(6);
    }
}

bool InterfaceToHistD::Delete(QString HistName)
{
    return TmpHub->ScriptDrawObjects.remove(HistName);
}

void InterfaceToHistD::DeleteAllHist()
{
    TmpHub->ScriptDrawObjects.removeAllHists();
}

void InterfaceToHistD::Draw(QString HistName, QString options)
{
  int index = TmpHub->ScriptDrawObjects.findIndexOf(HistName);
  if (index == -1)
    {
      abort("Histogram "+HistName+" not found!");
      return;
    }

  RootDrawObj& r = TmpHub->ScriptDrawObjects.List[index];
  if (r.type == "TH1D")
    {
      TH1D* h = static_cast<TH1D*>(r.Obj);
      h->SetXTitle(r.Xtitle.toLatin1().data());
      h->SetYTitle(r.Ytitle.toLatin1().data());
      h->SetLineColor(r.LineColor);
      h->SetLineWidth(r.LineWidth);
      h->SetLineStyle(r.LineStyle);
      emit RequestDraw(h, options, true);
    }
  else if (r.type == "TH2D")
    {
      TH2D* h = static_cast<TH2D*>(r.Obj);
      h->SetXTitle(r.Xtitle.toLatin1().data());
      h->SetYTitle(r.Ytitle.toLatin1().data());
      h->SetZTitle(r.Ztitle.toLatin1().data());
      h->SetLineColor(r.LineColor);
      h->SetLineWidth(r.LineWidth);
      h->SetLineStyle(r.LineStyle);
      emit RequestDraw(h, options, true);
    }
  else
    {
      abort("Object "+HistName+": unknown histogram type!");
      return;
    }
}

//----------------------------------
InterfaceToReconstructor::InterfaceToReconstructor(ReconstructionManagerClass *RManager, AConfiguration *Config, EventsDataClass *EventsDataHub, int RecNumThreads)
 : RManager(RManager), Config(Config), EventsDataHub(EventsDataHub), PMgroups(RManager->PMgroups), RecNumThreads(RecNumThreads) { }

void InterfaceToReconstructor::ForceStop()
{
  emit RequestStopReconstruction();
}

void InterfaceToReconstructor::ReconstructEvents(int NumThreads, bool fShow)
{
  if (NumThreads == -1) NumThreads = RecNumThreads;
  RManager->reconstructAll(Config->JSON, NumThreads, fShow);
}

void InterfaceToReconstructor::UpdateFilters(int NumThreads)
{
  if (NumThreads == -1) NumThreads = RecNumThreads;
  RManager->filterEvents(Config->JSON, NumThreads);
}

void InterfaceToReconstructor::DoBlurUniform(double range, bool fUpdateFilters)
{
  EventsDataHub->BlurReconstructionData(0, range, Config->GetDetector()->RandGen);
  if (fUpdateFilters) UpdateFilters();
}

void InterfaceToReconstructor::DoBlurGauss(double sigma, bool fUpdateFilters)
{
  EventsDataHub->BlurReconstructionData(1, sigma, Config->GetDetector()->RandGen);
  if (fUpdateFilters) UpdateFilters();
}

int InterfaceToReconstructor::countSensorGroups()
{
    return PMgroups->countPMgroups();
}

void InterfaceToReconstructor::clearSensorGroups()
{
    PMgroups->clearPMgroups();
}

void InterfaceToReconstructor::addSensorGroup(QString name)
{
    PMgroups->definePMgroup(name);
}

void InterfaceToReconstructor::setPMsOfGroup(int igroup, QVariant PMlist)
{
    if (igroup<0 || igroup>PMgroups->countPMgroups()-1)
    {
        abort("Script: attempt to add pms to a non-existent group");
        return;
    }

    PMgroups->removeAllPmRecords(igroup);
    QString type = PMlist.typeName();
    if (type == "QVariantList")
        {
          QVariantList vl = PMlist.toList();
          //qDebug() << vl;
          QJsonArray ar = QJsonArray::fromVariantList(vl);
          for (int i=0; i<ar.size(); i++)
          {
             int ipm = ar.at(i).toInt();
             PMgroups->addPMtoGroup(ipm, igroup);
          }
          PMgroups->updateGroupsInGlobalConfig();
    }
    else abort("Script: list of PMs should be an array of ints");
}

QVariant InterfaceToReconstructor::getPMsOfGroup(int igroup)
{

    if (igroup<0 || igroup>PMgroups->countPMgroups()-1)
    {
        abort("Bad sensor group number!");
        QJsonValue jv = QJsonArray();
        QVariant res = jv.toVariant();
        return res;
    }

    QJsonArray ar;
    for (int i=0; i<PMgroups->Groups.at(igroup)->PMS.size(); i++)
        if (PMgroups->Groups.at(igroup)->PMS.at(i).member) ar << i;

    QJsonValue jv = ar;
    QVariant res = jv.toVariant();
    return res;
}

bool InterfaceToReconstructor::isStaticPassive(int ipm)
{
    return PMgroups->isStaticPassive(ipm);
}

void InterfaceToReconstructor::setStaticPassive(int ipm)
{
    PMgroups->setStaticPassive(ipm);
}

void InterfaceToReconstructor::clearStaticPassive(int ipm)
{
    PMgroups->clearStaticPassive(ipm);
}

void InterfaceToReconstructor::selectSensorGroup(int igroup)
{
    PMgroups->selectActiveSensorGroup(igroup);
}

void InterfaceToReconstructor::clearSensorGroupSelection()
{
    PMgroups->clearActiveSensorGroups();
}

void InterfaceToReconstructor::SaveAsTree(QString fileName, bool IncludePMsignals, bool IncludeRho, bool IncludeTrue, int SensorGroup)
{
  if (SensorGroup<0 || SensorGroup>PMgroups->countPMgroups()-1)
  {
      abort("Wrong sensor group!");
      return;
  }
  EventsDataHub->saveReconstructionAsTree(fileName, Config->GetDetector()->PMs, IncludePMsignals, IncludeRho, IncludeTrue, SensorGroup);
}

void InterfaceToReconstructor::SaveAsText(QString fileName)
{
  EventsDataHub->saveReconstructionAsText(fileName);
}

void InterfaceToReconstructor::ClearManifestItems()
{
    EventsDataHub->clearManifest();
    emit RequestUpdateGuiForManifest();
}

void InterfaceToReconstructor::AddRoundManisfetItem(double x, double y, double Diameter)
{
    ManifestItemHoleClass* m = new ManifestItemHoleClass(Diameter);
    m->X = x;
    m->Y = y;
    EventsDataHub->Manifest.append(m);
    emit RequestUpdateGuiForManifest();
}

void InterfaceToReconstructor::AddRectangularManisfetItem(double x, double y, double dX, double dY, double Angle)
{
    ManifestItemSlitClass* m = new ManifestItemSlitClass(Angle, dX, dY);
    m->X = x;
    m->Y = y;
    EventsDataHub->Manifest.append(m);
    emit RequestUpdateGuiForManifest();
}

int InterfaceToReconstructor::CountManifestItems()
{
    return EventsDataHub->Manifest.size();
}

void InterfaceToReconstructor::SetManifestItemLineProperties(int i, int color, int width, int style)
{
    if (i<0 || i>EventsDataHub->Manifest.size()-1) return;
    EventsDataHub->Manifest[i]->LineColor = color;
    EventsDataHub->Manifest[i]->LineStyle = style;
    EventsDataHub->Manifest[i]->LineWidth = width;
}

#ifdef GUI
//----------------------------------
InterfaceToGraphWin::InterfaceToGraphWin(MainWindow *MW)
  : MW(MW)
{
  H["SaveImage"] = "Save image currently shown on the graph window to an image file.\nTip: use .png extension";
  H["GetAxis"] = "Returns an object with the values presented to user in 'Range' boxes.\n"
                 "They can be accessed with min/max X/Y/Z (e.g. grwin.GetAxis().maxY).\n"
                 "The values can be 'undefined'";
  H["AddLegend"] = "Adds a temporary (not savable yet!) legend to the graph.\n"
      "x1,y1 and x2,y2 are the bottom-left and top-right corner coordinates (0..1)";
}

void InterfaceToGraphWin::Show()
{
  MW->GraphWindow->showNormal();
}

void InterfaceToGraphWin::Hide()
{
  MW->GraphWindow->hide();
}

void InterfaceToGraphWin::PlotDensityXY()
{
  MW->Rwindow->DoPlotXY(0);
}

void InterfaceToGraphWin::PlotEnergyXY()
{
  MW->Rwindow->DoPlotXY(2);
}

void InterfaceToGraphWin::PlotChi2XY()
{
    MW->Rwindow->DoPlotXY(3);
}

void InterfaceToGraphWin::ConfigureXYplot(int binsX, double X0, double X1, int binsY, double Y0, double Y1)
{
   MW->Rwindow->ConfigurePlotXY(binsX, X0, X1, binsY, Y0, Y1);
   MW->Rwindow->updateGUIsettingsInConfig();
}

void InterfaceToGraphWin::SetLog(bool Xaxis, bool Yaxis)
{
  MW->GraphWindow->SetLog(Xaxis, Yaxis);
}

void InterfaceToGraphWin::AddLegend(double x1, double y1, double x2, double y2, QString title)
{
  MW->GraphWindow->AddLegend(x1, y1, x2, y2, title);
}

void InterfaceToGraphWin::AddToBasket(QString Title)
{
  MW->GraphWindow->AddCurrentToBasket(Title);
}

void InterfaceToGraphWin::ClearBasket()
{
  MW->GraphWindow->ClearBasket();

}

void InterfaceToGraphWin::SaveImage(QString fileName)
{
    MW->GraphWindow->SaveGraph(fileName);
}

void InterfaceToGraphWin::ExportTH2AsText(QString fileName)
{
    MW->GraphWindow->ExportTH2AsText(fileName);
}

QVariant InterfaceToGraphWin::GetProjection()
{
    QVector<double> vec = MW->GraphWindow->Get2DArray();
    QJsonArray arr;
    for (auto v : vec) arr << v;
    QJsonValue jv = arr;
    QVariant res = jv.toVariant();
    return res;
}

QVariant InterfaceToGraphWin::GetAxis()
{
  bool ok;
  QJsonObject result;

  result["minX"] = MW->GraphWindow->getMinX(&ok);
  if (!ok) result["minX"] = QJsonValue();
  result["maxX"] = MW->GraphWindow->getMaxX(&ok);
  if (!ok) result["maxX"] = QJsonValue();

  result["minY"] = MW->GraphWindow->getMinY(&ok);
  if (!ok) result["minY"] = QJsonValue();
  result["maxY"] = MW->GraphWindow->getMaxY(&ok);
  if (!ok) result["maxY"] = QJsonValue();

  result["minZ"] = MW->GraphWindow->getMinZ(&ok);
  if (!ok) result["minZ"] = QJsonValue();
  result["maxZ"] = MW->GraphWindow->getMaxZ(&ok);
  if (!ok) result["maxZ"] = QJsonValue();

  return QJsonValue(result).toVariant();
}
#endif

AInterfaceToPMs::AInterfaceToPMs(AConfiguration *Config) : Config(Config)
{
    PMs = Config->GetDetector()->PMs;

    H["SetPMQE"] = "Sets the QE of PM. Forces a call to UpdateAllConfig().";
    H["SetElGain"]  = "Set the relative strength for the electronic channel of ipm PMs.\nForces UpdateAllConfig()";

    H["GetElGain"]  = "Get the relative strength for the electronic channel of ipm PMs";

    H["GetRelativeGains"]  = "Get array with relative gains (max scaled to 1.0) of PMs. The factors take into account both QE and electronic channel gains (if activated)";

    H["GetPMx"] = "Return x position of PM.";
    H["GetPMy"] = "Return y position of PM.";
    H["GetPMz"] = "Return z position of PM.";

    H["SetPassivePM"] = "Set this PM status as passive.";
    H["SetActivePM"] = "Set this PM status as active.";

    H["CountPMs"] = "Return number of PMs";
}


bool AInterfaceToPMs::checkValidPM(int ipm)
{
  if (ipm<0 || ipm>PMs->count()-1)
    {
      abort("Wrong PM number!");
      return false;
    }
  return true;
}

bool AInterfaceToPMs::checkAddPmCommon(int UpperLower, int type)
{
    if (UpperLower<0 || UpperLower>1)
      {
        qWarning() << "Wrong UpperLower parameter: 0 - upper PM array, 1 - lower";
        return false;
      }
    if (type<0 || type>PMs->countPMtypes()-1)
      {
        qWarning() << "Attempting to add PM of non-existing type.";
        return false;
      }
    return true;
}

int AInterfaceToPMs::CountPM()
{
    return PMs->count();
}

double AInterfaceToPMs::GetPMx(int ipm)
{
  if (!checkValidPM(ipm)) return false;
  return PMs->X(ipm);
}

double AInterfaceToPMs::GetPMy(int ipm)
{
  if (!checkValidPM(ipm)) return false;
  return PMs->Y(ipm);
}

double AInterfaceToPMs::GetPMz(int ipm)
{
  if (!checkValidPM(ipm)) return false;
  return PMs->Z(ipm);
}

QVariant AInterfaceToPMs::GetPMtypes()
{
   QJsonObject obj;
   PMs->writePMtypesToJson(obj);
   QJsonArray ar = obj["PMtypes"].toArray();

   QVariant res = ar.toVariantList();
   return res;
}

void AInterfaceToPMs::RemoveAllPMs()
{
   Config->GetDetector()->PMarrays[0] = APmArrayData();
   Config->GetDetector()->PMarrays[1] = APmArrayData();
   Config->GetDetector()->writeToJson(Config->JSON);
   Config->GetDetector()->BuildDetector();
}

bool AInterfaceToPMs::AddPMToPlane(int UpperLower, int type, double X, double Y, double angle)
{
  if (!checkAddPmCommon(UpperLower, type)) return false;

  APmArrayData& ArrData = Config->GetDetector()->PMarrays[UpperLower];
  //qDebug() << "Size:"<<ArrData.PositionsAnglesTypes.size()<<"Reg:"<<ArrData.Regularity;

  if (ArrData.PositionsAnglesTypes.isEmpty())
    {
      //this is the first PM, can define array regularity
      ArrData.Regularity = 1;
      ArrData.PMtype = type;
    }
  else
    {
      if (ArrData.Regularity != 1)
      {
          qWarning() << "Attempt to add auto-z PM to a regular or full-custom PM array";
          return false;
      }
    }

  ArrData.PositionsAnglesTypes.append(APmPosAngTypeRecord(X, Y, 0, 0,0,angle, type));
  ArrData.PositioningScript.clear();
  ArrData.fActive = true;

  Config->GetDetector()->writeToJson(Config->JSON);
  Config->GetDetector()->BuildDetector();
  return true;
}

bool AInterfaceToPMs::AddPM(int UpperLower, int type, double X, double Y, double Z, double phi, double theta, double psi)
{
    if (!checkAddPmCommon(UpperLower, type)) return false;

    APmArrayData& ArrData = Config->GetDetector()->PMarrays[UpperLower];
    //qDebug() << "Size:"<<ArrData.PositionsAnglesTypes.size()<<"Reg:"<<ArrData.Regularity;

    if (ArrData.PositionsAnglesTypes.isEmpty())
      {
        //this is the first PM, can define array regularity
        ArrData.Regularity = 2;
        ArrData.PMtype = type;
      }
    else
      {
        if (ArrData.Regularity != 2)
        {
            qWarning() << "Attempt to add full-custom position PM to a auto-z PM array";
            return false;
        }
      }

    ArrData.PositionsAnglesTypes.append(APmPosAngTypeRecord(X, Y, Z, phi,theta,psi, type));
    ArrData.PositioningScript.clear();
    ArrData.fActive = true;

    Config->GetDetector()->writeToJson(Config->JSON);
    Config->GetDetector()->BuildDetector();
    return true;
}

void AInterfaceToPMs::SetAllArraysFullyCustom()
{
    for (int i=0; i<Config->GetDetector()->PMarrays.size(); i++)
        Config->GetDetector()->PMarrays[i].Regularity = 2;
    Config->GetDetector()->writeToJson(Config->JSON);
}

#ifdef GUI
AInterfaceToOutputWin::AInterfaceToOutputWin(MainWindow *MW)
{
    this->MW = MW;
}

void AInterfaceToOutputWin::ShowOutputWindow(bool flag, int tab)
{
    if (flag)
      {
        MW->Owindow->showNormal();
        MW->Owindow->raise();
      }
    else MW->Owindow->hide();

    if (tab>-1 && tab<4) MW->Owindow->SetTab(tab);
    qApp->processEvents();
}

void AInterfaceToOutputWin::Show()
{
    MW->Owindow->show();
}

void AInterfaceToOutputWin::Hide()
{
    MW->Owindow->hide();
}
#endif

// ------------- MATH ------------

MathInterfaceClass::MathInterfaceClass(TRandom2* RandGen)
{
  //srand (time(NULL));
    this->RandGen = RandGen;

  H["random"] = "Returns a random number between 0 and 1.\nGenerator respects the seed set by SetSeed method of the sim module!";
  H["gauss"] = "Returns a random value sampled from Gaussian distribution with mean and sigma given by the user";
  H["poisson"] = "Returns a random value sampled from Poisson distribution with mean given by the user";
  H["maxwell"] = "Returns a random value sampled from maxwell distribution with Sqrt(kT/M) given by the user";
}

void MathInterfaceClass::setRandomGen(TRandom2 *RandGen)
{
  this->RandGen = RandGen;
}

double MathInterfaceClass::abs(double val)
{
  return std::abs(val);
}

double MathInterfaceClass::acos(double val)
{
  return std::acos(val);
}

double MathInterfaceClass::asin(double val)
{
  return std::asin(val);
}

double MathInterfaceClass::atan(double val)
{
  return std::atan(val);
}

double MathInterfaceClass::atan2(double y, double x)
{
  return std::atan2(y, x);
}

double MathInterfaceClass::ceil(double val)
{
  return std::ceil(val);
}

double MathInterfaceClass::cos(double val)
{
  return std::cos(val);
}

double MathInterfaceClass::exp(double val)
{
  return std::exp(val);
}

double MathInterfaceClass::floor(double val)
{
  return std::floor(val);
}

double MathInterfaceClass::log(double val)
{
  return std::log(val);
}

double MathInterfaceClass::max(double val1, double val2)
{
  return std::max(val1, val2);
}

double MathInterfaceClass::min(double val1, double val2)
{
  return std::min(val1, val2);
}

double MathInterfaceClass::pow(double val, double power)
{
  return std::pow(val, power);
}

double MathInterfaceClass::sin(double val)
{
  return std::sin(val);
}

double MathInterfaceClass::sqrt(double val)
{
  return std::sqrt(val);
}

double MathInterfaceClass::tan(double val)
{
    return std::tan(val);
}

double MathInterfaceClass::round(double val)
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

double MathInterfaceClass::random()
{
  //return rand()/(double)RAND_MAX;

  if (!RandGen) return 0;
  return RandGen->Rndm();
}

double MathInterfaceClass::gauss(double mean, double sigma)
{
  if (!RandGen) return 0;
  return RandGen->Gaus(mean, sigma);
}

double MathInterfaceClass::poisson(double mean)
{
  if (!RandGen) return 0;
  return RandGen->Poisson(mean);
}

double MathInterfaceClass::maxwell(double a)
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

// ------------- End of MATH -------------

// ------------- New LRF module interface ------------

ALrfScriptInterface::ALrfScriptInterface(DetectorClass *Detector, EventsDataClass *EventsDataHub) :
  Detector(Detector), EventsDataHub(EventsDataHub), repo(Detector->LRFs->getNewModule()) {}

QString ALrfScriptInterface::Make(QString name, QVariantList instructions, bool use_scan_data, bool fit_error, bool scale_by_energy)
{
  if(instructions.length() < 1)
    return "No instructions were given";

  if(EventsDataHub->Events.isEmpty())
    return "There are no events loaded";

  if(use_scan_data) {
    if(EventsDataHub->Scan.isEmpty())
      return "Scan data is not setup!";
  } else if(!EventsDataHub->isReconstructionReady())
    return "Reconstruction data is not setup!";

  std::vector<LRF::AInstructionID> current_instructions;
  for(const QVariant &variant : instructions) {
    QJsonObject json = QJsonObject::fromVariantMap(variant.toMap());
    LRF::AInstruction *instruction = LRF::AInstruction::fromJson(json);
    if(instruction == nullptr)
      return "Failed to process instruction: "+QJsonDocument(json).toJson();
    current_instructions.push_back(repo->addInstruction(std::unique_ptr<LRF::AInstruction>(instruction)));
  }

  LRF::ARecipeID recipe;
  bool is_new_recipe = !repo->findRecipe(current_instructions, &recipe);
  if(is_new_recipe)
    recipe = repo->addRecipe(name.toLocal8Bit().data(), current_instructions);

  std::vector<APoint> sensorPos;
  pms *PMs = Detector->PMs;
  for(int i = 0; i < PMs->count(); i++) {
    pm &PM = PMs->at(i);
    sensorPos.push_back(APoint(PM.x, PM.y, PM.z));
  }

  Detector->Config->AskChangeGuiBusy(true);

  LRF::AInstructionInput data(repo, sensorPos, Detector->PMgroups,
                         EventsDataHub, use_scan_data, fit_error, scale_by_energy);

  if(!repo->updateRecipe(data, recipe)) {
    if(is_new_recipe) {
      repo->remove(recipe);
      repo->removeUnusedInstructions();
    }
    return "Failed to update recipe!";
  }
  else repo->setCurrentRecipe(recipe);

  Detector->Config->AskChangeGuiBusy(false);

  return "";  //empty = no error message
}

QString ALrfScriptInterface::Make(int recipe_id, bool use_scan_data, bool fit_error, bool scale_by_energy)
{
  LRF::ARecipeID rid(recipe_id);
  if(!repo->hasRecipe(rid))
    return "Non-existing recipe id";

  if(EventsDataHub->Events.isEmpty())
    return "There are no events loaded";

  if(use_scan_data) {
    if(EventsDataHub->Scan.isEmpty())
      return "Scan data is not setup";
  } else if(!EventsDataHub->isReconstructionReady())
    return "Reconstruction data is not setup";

  std::vector<APoint> sensorPos;
  pms *PMs = Detector->PMs;
  for(int i = 0; i < PMs->count(); i++) {
    pm &PM = PMs->at(i);
    sensorPos.push_back(APoint(PM.x, PM.y, PM.z));
  }

  Detector->Config->AskChangeGuiBusy(true);

  LRF::AInstructionInput data(repo, sensorPos, Detector->PMgroups,
                         EventsDataHub, use_scan_data, fit_error, scale_by_energy);

  if(!repo->updateRecipe(data, rid))
    return "Failed to update recipe!";
  else repo->setCurrentRecipe(rid);

  Detector->Config->AskChangeGuiBusy(false);

  return "";  //empty = no error message
}

int ALrfScriptInterface::GetCurrentRecipeId()
{
  return repo->getCurrentRecipeID().val();
}

int ALrfScriptInterface::GetCurrentVersionId()
{
  return repo->getCurrentRecipeVersionID().val();
}

bool ALrfScriptInterface::SetCurrentRecipeId(int rid)
{
  return repo->setCurrentRecipe(LRF::ARecipeID(rid));
}

bool ALrfScriptInterface::SetCurrentVersionId(int vid, int rid)
{
  if(rid == -1)
    return repo->setCurrentRecipe(repo->getCurrentRecipeID(), LRF::ARecipeVersionID(vid));
  else
    return repo->setCurrentRecipe(LRF::ARecipeID(rid), LRF::ARecipeVersionID(vid));
}

void ALrfScriptInterface::DeleteCurrentRecipe()
{
  repo->remove(repo->getCurrentRecipeID());
}

void ALrfScriptInterface::DeleteCurrentRecipeVersion()
{
  repo->removeVersion(repo->getCurrentRecipeID(), repo->getCurrentRecipeVersionID());
}

double ALrfScriptInterface::GetLRF(int ipm, double x, double y, double z)
{
  const LRF::ASensor *sensor = repo->getCurrentLrfs().getSensor(ipm);
  return sensor != nullptr ? sensor->eval(APoint(x, y, z)) : 0;
}

QList<int> ALrfScriptInterface::GetListOfRecipes()
{
  QList<int> recipes;
  for(auto rid : repo->getRecipeList())
    recipes.append(rid.val());
  return recipes;
}

bool ALrfScriptInterface::SaveRepository(QString fileName)
{
  QFile saveFile(fileName);
  if (!saveFile.open(QIODevice::WriteOnly))
    return false;

  QJsonDocument saveDoc(repo->toJson());
  saveFile.write(saveDoc.toJson());
  saveFile.close();
  return true;
}

bool ALrfScriptInterface::SaveCurrentRecipe(QString fileName)
{
  QFile saveFile(fileName);
  if (!saveFile.open(QIODevice::WriteOnly))
    return false;

  QJsonDocument saveDoc(repo->toJson(repo->getCurrentRecipeID()));
  saveFile.write(saveDoc.toJson());
  saveFile.close();
  return true;
}

bool ALrfScriptInterface::SaveCurrentVersion(QString fileName)
{
  QFile saveFile(fileName);
  if (!saveFile.open(QIODevice::WriteOnly))
    return false;

  QJsonDocument saveDoc(repo->toJson(repo->getCurrentRecipeID(), repo->getCurrentRecipeVersionID()));
  saveFile.write(saveDoc.toJson());
  saveFile.close();
  return true;
}

QList<int> ALrfScriptInterface::Load(QString fileName)
{
  QFile loadFile(fileName);
  if (!loadFile.open(QIODevice::ReadOnly)) {
    abort("Cannot open save file\n");
    return QList<int>();
  }
  QJsonObject json = QJsonDocument::fromJson(loadFile.readAll()).object();
  loadFile.close();

  LRF::ARepository new_repo(json);
  QList<int> new_recipes;
  for(auto rid : new_repo.getRecipeList())
    new_recipes.append(rid.val());
  if(repo->mergeRepository(new_repo))
    return new_recipes;
  else return QList<int>();
}


// ------------- End of New LRF module interface ------------

AInterfaceToTree::AInterfaceToTree(TmpObjHubClass *TmpHub) : TmpHub(TmpHub)
{}

void AInterfaceToTree::OpenTree(QString TreeName, QString FileName, QString TreeNameInFile)
{
    if (TmpHub->Trees.findIndexOf(TreeName) !=-1)
    {
        abort("Tree with name " + TreeName + " already exists!");
        return;
    }

    TFile *f = TFile::Open(FileName.toLocal8Bit().data(), "READ");
    if (!f)
    {
        abort("Cannot open file " + FileName);
        return;
    }
    TTree *t = 0;
    f->GetObject(TreeNameInFile.toLocal8Bit().data(), t);
    if (!t)
    {
        abort("Tree " + TreeNameInFile + " not found in file " + FileName);
        return;
    }
    t->Print();

    TmpHub->Trees.addTree(TreeName, t);
    return;
}

QString AInterfaceToTree::PrintBranches(QString TreeName)
{
    int index = TmpHub->Trees.findIndexOf(TreeName);
    if (index == -1)
    {
        abort("Tree with name " + TreeName + " does not exist!");
        return "";
    }
    TTree *t = TmpHub->Trees.getTree(TreeName);
    if (!t)
    {
        abort("Tree " + TreeName + " not found!");
        return "";
    }

    QString s = "Thee ";
    s += TreeName;
    s += " has the following branches (-> data_type):<br>";
    for (int i=0; i<t->GetNbranches(); i++)
    {
        TObjArray* lb = t->GetListOfBranches();
        const TBranch* b = (const TBranch*)(lb->At(i));
        QString name = b->GetName();
        s += name;
        s += " -> ";
        QString type = b->GetClassName();
        if (type.isEmpty())
        {
            QString title = b->GetTitle();
            title.remove(name);
            title.remove("/");
            s += title;
        }
        else
        {
            type.replace("<", "(");
            type.replace(">", ")");
            s += type;
        }
        s += "<br>";
    }
    return s;
}

QVariant AInterfaceToTree::GetBranch(QString TreeName, QString BranchName)
{
    int index = TmpHub->Trees.findIndexOf(TreeName);
    if (index == -1)
    {
        abort("Tree with name " + TreeName + " does not exist!");
        return QVariant();
    }

    TTree *t = TmpHub->Trees.getTree(TreeName);
    if (!t)
    {
        abort("Tree " + TreeName + " not found!");
        return QVariant();
    }

    TBranch* branch = t->GetBranch(BranchName.toLocal8Bit().data());
    if (!branch)
    {
        abort("Tree " + TreeName + " does not have branch " + BranchName);
        return QVariant();
    }

    int numEntries = branch->GetEntries();
    qDebug() << "The branch contains:" << numEntries << "elements";

    QList< QVariant > varList;
    QString type = branch->GetClassName();
    qDebug() << "Element type:" << type;
    if (type == "vector<double>")
    {
        std::vector<double> *v = 0;
        t->SetBranchAddress(BranchName.toLocal8Bit().data(), &v, &branch);

        for (Int_t i = 0; i < numEntries; i++)
        {
            Long64_t tentry = t->LoadTree(i);
            branch->GetEntry(tentry);
            QList<QVariant> ll;
            for (UInt_t j = 0; j < v->size(); ++j)
                ll.append( (*v)[j] );
            QVariant r = ll;
            varList << r;
        }
    }
    else if (type == "vector<float>")
    {
        std::vector<float> *v = 0;
        t->SetBranchAddress(BranchName.toLocal8Bit().data(), &v, &branch);

        for (Int_t i = 0; i < numEntries; i++)
        {
            Long64_t tentry = t->LoadTree(i);
            branch->GetEntry(tentry);
            QList<QVariant> ll;
            for (UInt_t j = 0; j < v->size(); ++j)
                ll.append( (*v)[j] );
            QVariant r = ll;
            varList << r;
        }
    }
    else if (type == "vector<int>")
    {
        std::vector<int> *v = 0;
        t->SetBranchAddress(BranchName.toLocal8Bit().data(), &v, &branch);

        for (Int_t i = 0; i < numEntries; i++)
        {
            Long64_t tentry = t->LoadTree(i);
            branch->GetEntry(tentry);
            QList<QVariant> ll;
            for (UInt_t j = 0; j < v->size(); ++j)
                ll.append( (*v)[j] );
            QVariant r = ll;
            varList << r;
        }
    }
    else if (type == "")
    {
        //have to use another system
        QString title = branch->GetTitle();  //  can be, e.g., "blabla/D" or "signal[19]/F"

        if (title.contains("["))
        {
            qDebug() << "Array of data"<<title;
            QRegExp selector("\\[(.*)\\]");
            selector.indexIn(title);
            QStringList List = selector.capturedTexts();
            if (List.size()!=2)
            {
               abort("Cannot extract the length of the array");
               return QVariant();
            }
            else
            {
               QString s = List.at(1);
               bool fOK = false;
               int numInArray = s.toInt(&fOK);
               if (!fOK)
               {
                  abort("Cannot extract the length of the array");
                  return QVariant();
               }
               qDebug() << "in the array there are"<<numInArray<<"elements";

               //type dependent too!
               if (title.contains("/I"))
               {
                   qDebug() << "It is an array with ints";
                   int *array = new int[numInArray];
                   t->SetBranchAddress(BranchName.toLocal8Bit().data(), array, &branch);

                   for (Int_t i = 0; i < numEntries; i++)
                   {
                       Long64_t tentry = t->LoadTree(i);
                       branch->GetEntry(tentry);
                       QList<QVariant> ll;
                       for (int j = 0; j < numInArray; j++)
                           ll.append( array[j] );
                       QVariant r = ll;
                       varList << r;
                   }
                   delete [] array;
               }
               else if (title.contains("/D"))
               {
                   qDebug() << "It is an array with doubles";
                   double *array = new double[numInArray];
                   t->SetBranchAddress(BranchName.toLocal8Bit().data(), array, &branch);

                   for (Int_t i = 0; i < numEntries; i++)
                   {
                       Long64_t tentry = t->LoadTree(i);
                       branch->GetEntry(tentry);
                       QList<QVariant> ll;
                       for (int j = 0; j < numInArray; j++)
                           ll.append( array[j] );
                       QVariant r = ll;
                       varList << r;
                   }
                   delete [] array;
               }
               else if (title.contains("/F"))
               {
                   qDebug() << "It is an array with floats";
                   float *array = new float[numInArray];
                   t->SetBranchAddress(BranchName.toLocal8Bit().data(), array, &branch);

                   for (Int_t i = 0; i < numEntries; i++)
                   {
                       Long64_t tentry = t->LoadTree(i);
                       branch->GetEntry(tentry);
                       QList<QVariant> ll;
                       for (int j = 0; j < numInArray; j++)
                           ll.append( array[j] );
                       QVariant r = ll;
                       varList << r;
                   }
                   delete [] array;
               }
               else
               {
                   abort("Cannot extract the type of the array");
                   return QVariant();
               }
            }
        }
        else if (title.contains("/I"))
        {
            qDebug() << "Int data - scalar";
            int v = 0;
            t->SetBranchAddress(BranchName.toLocal8Bit().data(), &v, &branch);
            for (Int_t i = 0; i < numEntries; i++)
            {
                Long64_t tentry = t->LoadTree(i);
                branch->GetEntry(tentry);
                varList.append(v);
            }
        }
        else if (title.contains("/D"))
        {
            qDebug() << "Double data - scalar";
            double v = 0;
            t->SetBranchAddress(BranchName.toLocal8Bit().data(), &v, &branch);
            for (Int_t i = 0; i < numEntries; i++)
            {
                Long64_t tentry = t->LoadTree(i);
                branch->GetEntry(tentry);
                varList.append(v);
            }
        }
        else if (title.contains("/F"))
        {
            qDebug() << "Float data - scalar";
            float v = 0;
            t->SetBranchAddress(BranchName.toLocal8Bit().data(), &v, &branch);
            for (Int_t i = 0; i < numEntries; i++)
            {
                Long64_t tentry = t->LoadTree(i);
                branch->GetEntry(tentry);
                varList.append(v);
            }
        }
        else
        {
            abort("Unsupported data type of the branch - title is: "+title);
            return QVariant();
        }

    }
    else
    {
        abort("Tree branch type " + type + " is not supported");
        return QVariant();
    }

    t->ResetBranchAddresses();
    return varList;
}

void AInterfaceToTree::CloseTree(QString TreeName)
{
   int index = TmpHub->Trees.findIndexOf(TreeName);
   if (index == -1) return;

   TmpHub->Trees.remove(TreeName);
}
