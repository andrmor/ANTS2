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
#include "arootobjbase.h"

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

#include <QFile>
#include <QVariant>
#include <QThread>
#include <QDateTime>
#include <QApplication>
#include <QVector3D>
#include <QJsonDocument>
#include <QDebug>

#include "TRandom2.h"
#include "TGeoManager.h"
#include "TGeoTrack.h"
#include "TColor.h"
#include "TAxis.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"
#include "TLeaf.h"

bool AInterfaceToConfig::keyToNameAndIndex(QString Key, QString &Name, QVector<int> &Indexes)
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

bool AInterfaceToConfig::modifyJsonValue(QJsonObject &obj, const QString &path, const QJsonValue &newValue)
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

void AInterfaceToConfig::find(const QJsonObject &obj, QStringList Keys, QStringList &Found, QString Path)
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

bool AInterfaceToConfig::Replace(QString Key, QVariant val)
{
  if (!bGuiThread)
    {
      abort("Only GUI thread can modify detector configuration!");
      return false;
    }

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

QVariant AInterfaceToConfig::GetKeyValue(QString Key)
{
    qDebug() << this << "get "<< Key << "triggered";

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


QString AInterfaceToConfig::GetLastError()
{
    return LastError;
}

void AInterfaceToConfig::RebuildDetector()
{
  if (!bGuiThread)
    {
      abort("Only GUI thread can modify detector configuration!");
      return;
    }

    Config->GetDetector()->BuildDetector();
}

bool AInterfaceToConfig::expandKey(QString &Key)
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

void AInterfaceToConfig::UpdateGui()
{
    if (!bGuiThread) return;

    Config->AskForAllGuiUpdate();
}

AInterfaceToConfig::AInterfaceToConfig(AConfiguration *config) :
    Config(config)
{
  emit requestReadRasterGeometry();

  Description = "Read/change/save/load configuration of the detector/simulator/reconstructor\nToggle Config button to access configuration tree";

  H["RebuildDetector"]  = "Force to rebuild detector using the current configuration file.\nNot anymore required after calling config.Replace - it is called automatically if Detector settings were modified.";
  H["Load"]  = "Load ANTS2 configuration file (.json).";
  H["Replace"] = "Replace the value of the key in the configuration object with the new one. Key value can be basic types (bool, double, string) as well as arrays and objects. Changing detector-related settings (DetectorConfig top node) will automatically run RebuildDetector!";
  H["UpdateGui"] = "Update GUI during script execution according to current settings in Config.";
  H["GetKeyValue"] = "Return the value of the Key: it can be basic types (bool, double, string) as well as arrays and objects.";

}

AInterfaceToConfig::AInterfaceToConfig(const AInterfaceToConfig& other)
  : AScriptInterface(other) {}

bool AInterfaceToConfig::Load(QString FileName)
{
  if (!bGuiThread)
    {
      abort("Script in threads: cannot modify detector configuration!");
      return false;
    }

  //bool ok = MW->ELwindow->LoadAllConfig(FileName, true, true, true, &Config->JSON);
  bool ok = Config->LoadConfig(FileName, true, true, true);
  if (ok) return true;

  abort("Failed to load config from file "+FileName);
  return false;
}

bool AInterfaceToConfig::Save(QString FileName)
{
    return SaveJsonToFile(Config->JSON, FileName);
}

#ifdef SIM
//----------------------------------
InterfaceToSim::InterfaceToSim(ASimulationManager* SimulationManager, EventsDataClass *EventsDataHub, AConfiguration* Config, int RecNumThreads, bool fGuiPresent)
  : SimulationManager(SimulationManager), EventsDataHub(EventsDataHub), Config(Config), RecNumThreads(RecNumThreads), fGuiPresent(fGuiPresent)
{
  Description = "Access to simulator";

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
          else return vl;

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
AInterfaceToData::AInterfaceToData(AConfiguration *Config, EventsDataClass* EventsDataHub)
  : Config(Config), EventsDataHub(EventsDataHub)
{
  Description = "Access to PM signals and reconstruction data";

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

double AInterfaceToData::GetPMsignal(int ievent, int ipm)
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

QVariant AInterfaceToData::GetPMsignals(int ievent)
{
  int numEvents = GetNumEvents();
  if (ievent<0 || ievent>=numEvents)
    {
      abort("Wrong event number "+QString::number(ievent)+" Events available: "+QString::number(numEvents));
      return 0;
    }

  const QVector< float >& sigs = EventsDataHub->Events.at(ievent);
  QVariantList l;
  for (float f : sigs) l << QVariant(f);
  return l;
}

void AInterfaceToData::SetPMsignal(int ievent, int ipm, double value)
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

double AInterfaceToData::GetPMsignalTimed(int ievent, int ipm, int iTimeBin)
{
    int numEvents = countTimedEvents();
    if (ievent<0 || ievent >= numEvents)
      {
        abort("Wrong event number "+QString::number(ievent)+" Events available: "+QString::number(numEvents));
        return 0;
      }

    int numTimeBins = countTimeBins();
    if (iTimeBin<0 || iTimeBin >= numTimeBins)
      {
        abort("Wrong time bin "+QString::number(ievent)+" Time bins available: "+QString::number(numTimeBins));
        return 0;
      }

    int numPMs = EventsDataHub->TimedEvents.at(ievent).at(iTimeBin).size();
    if (ipm<0 || ipm>=numPMs)
      {
        abort("Wrong PM number "+QString::number(ipm)+"; PMs in the events data file: "+QString::number(numPMs));
        return 0;
      }

    return EventsDataHub->TimedEvents.at(ievent).at(iTimeBin).at(ipm);
}

QVariant AInterfaceToData::GetPMsignalVsTime(int ievent, int ipm)
{
    int numEvents = countTimedEvents();
    if (ievent<0 || ievent >= numEvents)
      {
        abort("Wrong event number "+QString::number(ievent)+" Events available: "+QString::number(numEvents));
        return 0;
      }

    int numTimeBins = countTimeBins();
    if (numTimeBins == 0) return QVariantList();

    int numPMs = EventsDataHub->TimedEvents.at(ievent).first().size();
    if (ipm<0 || ipm>=numPMs)
      {
        abort("Wrong PM number "+QString::number(ipm)+"; PMs in the events data file: "+QString::number(numPMs));
        return 0;
      }

    QVariantList aa;
    for (int i=0; i<numTimeBins; i++)
        aa << EventsDataHub->TimedEvents.at(ievent).at(i).at(ipm);
    return aa;
}

int AInterfaceToData::GetNumPMs()
{
  if (EventsDataHub->Events.isEmpty()) return 0;
  return EventsDataHub->Events.first().size();
}

int AInterfaceToData::countPMs()
{
    if (EventsDataHub->Events.isEmpty()) return 0;
    return EventsDataHub->Events.first().size();
}

int AInterfaceToData::GetNumEvents()
{
    return EventsDataHub->Events.size();
}

int AInterfaceToData::countEvents()
{
    return EventsDataHub->Events.size();
}

int AInterfaceToData::countTimedEvents()
{
    return EventsDataHub->TimedEvents.size();
}

int AInterfaceToData::countTimeBins()
{
    if (EventsDataHub->TimedEvents.isEmpty()) return 0;
    return EventsDataHub->TimedEvents.first().size();
}

bool AInterfaceToData::checkEventNumber(int ievent)
{ 
  int numEvents = EventsDataHub->Events.size();
  if (ievent<0 || ievent>numEvents-1)
    {
      abort("Wrong event number "+QString::number(ievent)+" Events available: "+QString::number(numEvents));
      return false;
    }
  return true;
}

bool AInterfaceToData::checkEventNumber(int igroup, int ievent, int ipoint)
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

bool AInterfaceToData::checkPM(int ipm)
{
  if (ipm<0 || ipm>Config->GetDetector()->PMs->count()-1)
      {
        abort("Invalid PM index: "+QString::number(ipm));
        return false;
      }
  return true;
}

bool AInterfaceToData::checkSetReconstructionDataRequest(int ievent)
{
  int numEvents = EventsDataHub->Events.size();
  if (ievent<0 || ievent>numEvents-1)
    {
      abort("Wrong event number "+QString::number(ievent)+" Events available: "+QString::number(numEvents));
      return false;
    }
  return true;
}

bool AInterfaceToData::checkTrueDataRequest(int ievent)
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

double AInterfaceToData::GetReconstructedX(int ievent)
{
  if (!checkEventNumber(ievent)) return 0; //anyway aborted
  return EventsDataHub->ReconstructionData.at(0).at(ievent)->Points[0].r[0];
}

double AInterfaceToData::GetReconstructedX(int igroup, int ievent, int ipoint)
{
  if (!checkEventNumber(igroup, ievent, ipoint)) return 0; //anyway aborted
  return EventsDataHub->ReconstructionData.at(igroup).at(ievent)->Points[ipoint].r[0];
}

double AInterfaceToData::GetReconstructedY(int ievent)
{
  if (!checkEventNumber(ievent)) return 0; //anyway aborted
  return EventsDataHub->ReconstructionData.at(0).at(ievent)->Points[0].r[1];
}

double AInterfaceToData::GetReconstructedY(int igroup, int ievent, int ipoint)
{
  if (!checkEventNumber(igroup, ievent, ipoint)) return 0; //anyway aborted
  return EventsDataHub->ReconstructionData.at(igroup).at(ievent)->Points[ipoint].r[1];
}

double AInterfaceToData::GetReconstructedZ(int ievent)
{
  if (!checkEventNumber(ievent)) return 0; //anyway aborted
  return EventsDataHub->ReconstructionData.at(0).at(ievent)->Points[0].r[2];
}

double AInterfaceToData::GetReconstructedZ(int igroup, int ievent, int ipoint)
{
  if (!checkEventNumber(igroup, ievent, ipoint)) return 0; //anyway aborted
  return EventsDataHub->ReconstructionData.at(igroup).at(ievent)->Points[ipoint].r[2];
}

double AInterfaceToData::GetRho(int ievent, int iPM)
{
    if (!checkPM(iPM)) return 0;
    if (!checkEventNumber(ievent)) return 0; //anyway aborted
    return sqrt( GetRho2(ievent, iPM) );
}

double AInterfaceToData::GetRho(int igroup, int ievent, int ipoint, int iPM)
{
  if (!checkPM(iPM)) return 0;
  if (!checkEventNumber(igroup, ievent, ipoint)) return 0; //anyway aborted
  return EventsDataHub->ReconstructionData.at(igroup).at(ievent)->Points[ipoint].r[2];
}

double AInterfaceToData::GetRho2(int ievent, int iPM)
{
  if (!checkPM(iPM)) return 0;
  if (!checkEventNumber(ievent)) return 0; //anyway aborted
  double dx2 = EventsDataHub->ReconstructionData.at(0).at(ievent)->Points[0].r[0] - Config->GetDetector()->PMs->X(iPM);
  dx2 *= dx2;
  double dy2 = EventsDataHub->ReconstructionData.at(0).at(ievent)->Points[0].r[1] - Config->GetDetector()->PMs->Y(iPM);
  dy2 *= dy2;
  return dx2+dy2;
}

double AInterfaceToData::GetRho2(int igroup, int ievent, int ipoint, int iPM)
{
    if (!checkPM(iPM)) return 0;
    if (!checkEventNumber(igroup, ievent, ipoint)) return 0; //anyway aborted
    double dx2 = EventsDataHub->ReconstructionData.at(0).at(ievent)->Points[0].r[0] - Config->GetDetector()->PMs->X(iPM);
    dx2 *= dx2;
    double dy2 = EventsDataHub->ReconstructionData.at(0).at(ievent)->Points[0].r[1] - Config->GetDetector()->PMs->Y(iPM);
    dy2 *= dy2;
    return dx2+dy2;
}

double AInterfaceToData::GetReconstructedEnergy(int ievent)
{
  if (!checkEventNumber(ievent)) return 0; //anyway aborted
  return EventsDataHub->ReconstructionData.at(0).at(ievent)->Points[0].energy;
}

double AInterfaceToData::GetReconstructedEnergy(int igroup, int ievent, int ipoint)
{
    if (!checkEventNumber(igroup, ievent, ipoint)) return 0;
    return EventsDataHub->ReconstructionData.at(igroup).at(ievent)->Points[ipoint].energy;
}

bool AInterfaceToData::IsReconstructedGoodEvent(int ievent)
{
  if (!checkEventNumber(ievent)) return 0; //anyway aborted
  return EventsDataHub->ReconstructionData.at(0).at(ievent)->GoodEvent;
}

bool AInterfaceToData::IsReconstructedGoodEvent(int igroup, int ievent)
{
    if (!checkEventNumber(igroup, ievent, 0)) return 0;
    return EventsDataHub->ReconstructionData.at(igroup).at(ievent)->GoodEvent;
}

bool AInterfaceToData::IsReconstructed_ScriptFilterPassed(int igroup, int ievent)
{
  if (!checkEventNumber(igroup, ievent, 0)) return 0;
  return !EventsDataHub->ReconstructionData.at(igroup).at(ievent)->fScriptFiltered;
}

int AInterfaceToData::countReconstructedGroups()
{
    return EventsDataHub->ReconstructionData.size();
}

int AInterfaceToData::countReconstructedEvents(int igroup)
{
    if (igroup<0 || igroup>EventsDataHub->ReconstructionData.size()-1) return -1;
    return EventsDataHub->ReconstructionData.at(igroup).size();
}

int AInterfaceToData::countReconstructedPoints(int igroup, int ievent)
{
    if (igroup<0 || igroup>EventsDataHub->ReconstructionData.size()-1) return -1;
    if (ievent<0 || ievent>EventsDataHub->ReconstructionData.at(igroup).size()-1) return -1;
    return EventsDataHub->ReconstructionData.at(igroup).at(ievent)->Points.size();
}

double AInterfaceToData::GetTrueX(int ievent)
{
  if (!checkTrueDataRequest(ievent)) return 0; //anyway aborted
  return EventsDataHub->Scan.at(ievent)->Points[0].r[0];
}

double AInterfaceToData::GetTrueY(int ievent)
{
  if (!checkTrueDataRequest(ievent)) return 0; //anyway aborted
  return EventsDataHub->Scan.at(ievent)->Points[0].r[1];
}

double AInterfaceToData::GetTrueZ(int ievent)
{
  if (!checkTrueDataRequest(ievent)) return 0; //anyway aborted
  return EventsDataHub->Scan.at(ievent)->Points[0].r[2];
}

double AInterfaceToData::GetTrueEnergy(int ievent)
{
  if (!checkTrueDataRequest(ievent)) return 0; //anyway aborted
  return EventsDataHub->Scan.at(ievent)->Points[0].energy;
}

int AInterfaceToData::GetTruePoints(int ievent)
{
  if (!checkTrueDataRequest(ievent)) return 0; //anyway aborted
  return EventsDataHub->Scan.at(ievent)->Points.size();
}

bool AInterfaceToData::IsTrueGoodEvent(int ievent)
{
  if (!checkTrueDataRequest(ievent)) return 0; //anyway aborted
  return EventsDataHub->Scan.at(ievent)->GoodEvent;
}

bool AInterfaceToData::GetTrueNumberPoints(int ievent)
{
  if (!checkTrueDataRequest(ievent)) return 0; //anyway aborted
  return EventsDataHub->Scan.at(ievent)->Points.size();
}

void AInterfaceToData::SetScanX(int ievent, double value)
{
    if (!checkTrueDataRequest(ievent)) return; //anyway aborted
    EventsDataHub->Scan.at(ievent)->Points[0].r[0] = value;
}

void AInterfaceToData::SetScanY(int ievent, double value)
{
    if (!checkTrueDataRequest(ievent)) return; //anyway aborted
    EventsDataHub->Scan.at(ievent)->Points[0].r[1] = value;
}

void AInterfaceToData::SetScanZ(int ievent, double value)
{
    if (!checkTrueDataRequest(ievent)) return; //anyway aborted
    EventsDataHub->Scan.at(ievent)->Points[0].r[2] = value;
}

void AInterfaceToData::SetScanEnergy(int ievent, double value)
{
    if (!checkTrueDataRequest(ievent)) return; //anyway aborted
    EventsDataHub->Scan.at(ievent)->Points[0].energy = value;
}

bool pairMore (const std::pair<int, float>& p1, const std::pair<int, float>& p2)
{
    return (p1.second > p2.second);
}

QVariant AInterfaceToData::GetPMsSortedBySignal(int ievent)
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

int AInterfaceToData::GetPMwithMaxSignal(int ievent)
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

void AInterfaceToData::SetReconstructed(int ievent, double x, double y, double z, double e)
{
  if (!checkSetReconstructionDataRequest(ievent)) return;
  EventsDataHub->ReconstructionData[0][ievent]->Points[0].r[0] = x;
  EventsDataHub->ReconstructionData[0][ievent]->Points[0].r[1] = y;
  EventsDataHub->ReconstructionData[0][ievent]->Points[0].r[2] = z;
  EventsDataHub->ReconstructionData[0][ievent]->Points[0].energy = e;
  EventsDataHub->ReconstructionData[0][ievent]->ReconstructionOK = true;
  EventsDataHub->ReconstructionData[0][ievent]->GoodEvent = true;
}

void AInterfaceToData::SetReconstructed(int ievent, double x, double y, double z, double e, double chi2)
{
    if (!checkSetReconstructionDataRequest(ievent)) return;
    EventsDataHub->ReconstructionData[0][ievent]->Points[0].r[0] = x;
    EventsDataHub->ReconstructionData[0][ievent]->Points[0].r[1] = y;
    EventsDataHub->ReconstructionData[0][ievent]->Points[0].r[2] = z;
    EventsDataHub->ReconstructionData[0][ievent]->Points[0].energy = e;
    EventsDataHub->ReconstructionData[0][ievent]->chi2 = chi2;
    EventsDataHub->ReconstructionData[0][ievent]->ReconstructionOK = true;
    EventsDataHub->ReconstructionData[0][ievent]->GoodEvent = true;
}

void AInterfaceToData::SetReconstructedX(int ievent, double x)
{
  if (!checkSetReconstructionDataRequest(ievent)) return;
  EventsDataHub->ReconstructionData[0][ievent]->Points[0].r[0] = x;
}

void AInterfaceToData::SetReconstructedY(int ievent, double y)
{
  if (!checkSetReconstructionDataRequest(ievent)) return;
  EventsDataHub->ReconstructionData[0][ievent]->Points[0].r[1] = y;
}

void AInterfaceToData::SetReconstructedZ(int ievent, double z)
{
  if (!checkSetReconstructionDataRequest(ievent)) return;
  EventsDataHub->ReconstructionData[0][ievent]->Points[0].r[2] = z;
}

void AInterfaceToData::SetReconstructedEnergy(int ievent, double e)
{
  if (!checkSetReconstructionDataRequest(ievent)) return;
  EventsDataHub->ReconstructionData[0][ievent]->Points[0].energy = e;
}

void AInterfaceToData::SetReconstructed_ScriptFilterPass(int ievent, bool flag)
{
  if (!checkSetReconstructionDataRequest(ievent)) return;
  EventsDataHub->ReconstructionData[0][ievent]->fScriptFiltered = !flag;
}

void AInterfaceToData::SetReconstructedGoodEvent(int ievent, bool good)
{
  if (!checkSetReconstructionDataRequest(ievent)) return;
  EventsDataHub->ReconstructionData[0][ievent]->ReconstructionOK = true;
  EventsDataHub->ReconstructionData[0][ievent]->GoodEvent = good;
}

void AInterfaceToData::SetReconstructedAllEventsGood(bool flag)
{
    for (int i=0; i<EventsDataHub->ReconstructionData.at(0).size(); i++)
        EventsDataHub->ReconstructionData[0][i]->GoodEvent = flag;
}

void AInterfaceToData::SetReconstructionOK(int ievent, bool OK)
{
  if (!checkSetReconstructionDataRequest(ievent)) return;
  EventsDataHub->ReconstructionData[0][ievent]->ReconstructionOK = OK;
  if (!OK) EventsDataHub->ReconstructionData[0][ievent]->GoodEvent = false;
}

void AInterfaceToData::SetReconstructed(int igroup, int ievent, int ipoint, double x, double y, double z, double e)
{
  if (!checkEventNumber(igroup, ievent, ipoint)) return;
  EventsDataHub->ReconstructionData[igroup][ievent]->Points[ipoint].r[0] = x;
  EventsDataHub->ReconstructionData[igroup][ievent]->Points[ipoint].r[1] = y;
  EventsDataHub->ReconstructionData[igroup][ievent]->Points[ipoint].r[2] = z;
  EventsDataHub->ReconstructionData[igroup][ievent]->Points[ipoint].energy = e;
  EventsDataHub->ReconstructionData[igroup][ievent]->ReconstructionOK = true;
  EventsDataHub->ReconstructionData[igroup][ievent]->GoodEvent = true;
}

void AInterfaceToData::SetReconstructedFast(int igroup, int ievent, int ipoint, double x, double y, double z, double e)
{
    EventsDataHub->ReconstructionData[igroup][ievent]->Points[ipoint].r[0] = x;
    EventsDataHub->ReconstructionData[igroup][ievent]->Points[ipoint].r[1] = y;
    EventsDataHub->ReconstructionData[igroup][ievent]->Points[ipoint].r[2] = z;
    EventsDataHub->ReconstructionData[igroup][ievent]->Points[ipoint].energy = e;
    EventsDataHub->ReconstructionData[igroup][ievent]->ReconstructionOK = true;
    EventsDataHub->ReconstructionData[igroup][ievent]->GoodEvent = true;
}

void AInterfaceToData::AddReconstructedPoint(int igroup, int ievent, double x, double y, double z, double e)
{
  if (!checkEventNumber(igroup, ievent, 0)) return;
  EventsDataHub->ReconstructionData[igroup][ievent]->Points.AddPoint(x, y, z, e);
}

void AInterfaceToData::SetReconstructedX(int igroup, int ievent, int ipoint, double x)
{
  if (!checkEventNumber(igroup, ievent, ipoint)) return;
  EventsDataHub->ReconstructionData[igroup][ievent]->Points[ipoint].r[0] = x;
}

void AInterfaceToData::SetReconstructedY(int igroup, int ievent, int ipoint, double y)
{
    if (!checkEventNumber(igroup, ievent, ipoint)) return;
    EventsDataHub->ReconstructionData[igroup][ievent]->Points[ipoint].r[1] = y;
}

void AInterfaceToData::SetReconstructedZ(int igroup, int ievent, int ipoint, double z)
{
    if (!checkEventNumber(igroup, ievent, ipoint)) return;
    EventsDataHub->ReconstructionData[igroup][ievent]->Points[ipoint].r[2] = z;
}

void AInterfaceToData::SetReconstructedEnergy(int igroup, int ievent, int ipoint, double e)
{
  if (!checkEventNumber(igroup, ievent, ipoint)) return;
  EventsDataHub->ReconstructionData[igroup][ievent]->Points[ipoint].energy = e;
}

void AInterfaceToData::SetReconstructedGoodEvent(int igroup, int ievent, int ipoint, bool good)
{
    if (!checkEventNumber(igroup, ievent, ipoint)) return;
    EventsDataHub->ReconstructionData[igroup][ievent]->GoodEvent = good;
}

void AInterfaceToData::SetReconstructed_ScriptFilterPass(int igroup, int ievent, bool flag)
{
  if (!checkEventNumber(igroup, ievent, 0)) return;
  EventsDataHub->ReconstructionData[igroup][ievent]->fScriptFiltered = !flag;
}

void AInterfaceToData::SetReconstructionOK(int igroup, int ievent, int ipoint, bool OK)
{
  if (!checkEventNumber(igroup, ievent, ipoint)) return;
  EventsDataHub->ReconstructionData[igroup][ievent]->ReconstructionOK = OK;
}

void AInterfaceToData::SetReconstructionReady()
{
  if (!bGuiThread)
  {
      abort("Only GUI thread can do SetReconstructionReady()");
      return;
  }

  EventsDataHub->fReconstructionDataReady = true;
  emit RequestEventsGuiUpdate();
}

void AInterfaceToData::ResetReconstructionData(int numGroups)
{
    if (!bGuiThread)
    {
        abort("Only GUI thread can do ResetReconstructionData()");
        return;
    }

    for (int ig=0; ig<numGroups; ig++)
      EventsDataHub->resetReconstructionData(numGroups);
}

void AInterfaceToData::LoadEventsTree(QString fileName, bool Append, int MaxNumEvents)
{
    if (!bGuiThread)
    {
        abort("Only GUI thread can do LoadEventsTree()");
        return;
    }

  if (!Append) EventsDataHub->clear();
  EventsDataHub->loadSimulatedEventsFromTree(fileName, Config->GetDetector()->PMs, MaxNumEvents);
  //EventsDataHub->clearReconstruction();
  EventsDataHub->createDefaultReconstructionData();
  //RManager->filterEvents(Config->JSON);
  EventsDataHub->squeeze();
  emit RequestEventsGuiUpdate();
}

void AInterfaceToData::LoadEventsAscii(QString fileName, bool Append)
{
    if (!bGuiThread)
    {
        abort("Only GUI thread can do LoadEventsAscii()");
        return;
    }

  if (!Append) EventsDataHub->clear();
  EventsDataHub->loadEventsFromTxtFile(fileName, Config->JSON, Config->GetDetector()->PMs);
  //EventsDataHub->clearReconstruction();
  EventsDataHub->createDefaultReconstructionData();
  //RManager->filterEvents(Config->JSON);
  EventsDataHub->squeeze();
  emit RequestEventsGuiUpdate();
}

void AInterfaceToData::ClearEvents()
{
    if (!bGuiThread)
    {
        abort("Only GUI thread can clear events!");
        return;
    }

    EventsDataHub->clear(); //gui update is triggered inside
}

void AInterfaceToData::PurgeBad()
{
    if (!bGuiThread)
    {
        abort("Only GUI thread can purge events!");
        return;
    }
    EventsDataHub->PurgeFilteredEvents();
}

void AInterfaceToData::Purge(int LeaveOnePer)
{
    if (!bGuiThread)
    {
        abort("Only GUI thread can purge events!");
        return;
    }
    EventsDataHub->Purge(LeaveOnePer);
}

QVariant AInterfaceToData::GetStatistics(int igroup)
{
    if (!bGuiThread)
    {
        abort("Only GUI thread can do GetStatistics()");
        return 0;
    }

  int GoodEvents;
  double AvChi2, AvDeviation;
  EventsDataHub->prepareStatisticsForEvents(Config->GetDetector()->LRFs->isAllLRFsDefined(), GoodEvents, AvChi2, AvDeviation, igroup);

  QVariantList l;
  l << GoodEvents << AvChi2 << AvDeviation;
  return l;
}

//----------------------------------
AInterfaceToLRF::AInterfaceToLRF(AConfiguration *Config, EventsDataClass *EventsDataHub)
  : Config(Config), EventsDataHub(EventsDataHub) //,f2d(0)
{
  SensLRF = Config->GetDetector()->LRFs->getOldModule();

  Description = "Access to LRFs (B-spline module)";

  H["Make"] = "Calculates new LRFs";

  H["CountIterations"] = "Returns the number of LRF iterations in history.";
  H["GetCurrent"] = "Returns the index of the current LRF iteration.";
  //H["ShowVsXY"] = "Plots a 2D histogram of the LRF. Does not work for 3D LRFs!";
}

QString AInterfaceToLRF::Make()
{
  QJsonObject jsR = Config->JSON["ReconstructionConfig"].toObject();
  SensLRF->LRFmakeJson = jsR["LRFmakeJson"].toObject();
  bool ok = SensLRF->makeLRFs(SensLRF->LRFmakeJson, EventsDataHub, Config->GetDetector()->PMs);
  Config->AskForLRFGuiUpdate();
  if (!ok) return SensLRF->getLastError();
  else return "";
}

double AInterfaceToLRF::GetLRF(int ipm, double x, double y, double z)
{
    //qDebug() << ipm<<x<<y<<z;
    //qDebug() << SensLRF->getIteration()->countPMs();
    if (!SensLRF->isAllLRFsDefined()) return 0;
    if (ipm<0 || ipm >= SensLRF->getIteration()->countPMs()) return 0;
    return SensLRF->getLRF(ipm, x, y, z);
}

double AInterfaceToLRF::GetLRFerror(int ipm, double x, double y, double z)
{
    if (!SensLRF->isAllLRFsDefined()) return 0;
    if (ipm<0 || ipm >= SensLRF->getIteration()->countPMs()) return 0;
    return SensLRF->getLRFErr(ipm, x, y, z);
}

QVariant AInterfaceToLRF::GetAllLRFs(double x, double y, double z)
{
    if (!SensLRF->isAllLRFsDefined())
    {
        abort("Not all LRFs are defined!");
        return 0;
    }

    QVariantList arr;
    const int numPMs = SensLRF->getIteration()->countPMs(); //Config->Detector->PMs->count();
    for (int ipm=0; ipm<numPMs; ipm++)
        arr << QVariant( SensLRF->getLRF(ipm, x, y, z) );
    return arr;
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

int AInterfaceToLRF::CountIterations()
{
  return SensLRF->countIterations();
}

int AInterfaceToLRF::GetCurrent()
{
  return SensLRF->getCurrentIterIndex();
}

void AInterfaceToLRF::SetCurrent(int iterIndex)
{
  if(!getValidIteration(iterIndex)) return;
  SensLRF->setCurrentIter(iterIndex);
  Config->AskForLRFGuiUpdate();
}

void AInterfaceToLRF::SetCurrentName(QString name)
{
    SensLRF->setCurrentIterName(name);
    Config->AskForLRFGuiUpdate();
}

void AInterfaceToLRF::DeleteCurrent()
{
  int iterIndex = -1;
  if(!getValidIteration(iterIndex)) return;
  SensLRF->deleteIteration(iterIndex);
  Config->AskForLRFGuiUpdate();
}

QString AInterfaceToLRF::Save(QString fileName)
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

int AInterfaceToLRF::Load(QString fileName)
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

bool AInterfaceToLRF::getValidIteration(int &iterIndex)
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
  Description = "Access to the Geometry window of GUI";

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


InterfaceToReconstructor::InterfaceToReconstructor(ReconstructionManagerClass *RManager, AConfiguration *Config, EventsDataClass *EventsDataHub, int RecNumThreads)
 : RManager(RManager), Config(Config), EventsDataHub(EventsDataHub), PMgroups(RManager->PMgroups), RecNumThreads(RecNumThreads)
{
    Description = "Event reconstructor";
}

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
  Description = "Access to the Graph window of GUI";

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

void InterfaceToGraphWin::AddText(QString text, bool Showframe, int Alignment_0Left1Center2Right)
{
  MW->GraphWindow->AddText(text, Showframe, Alignment_0Left1Center2Right);
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
    Description = "Access to PM positions / add PMs or remove all PMs from the configuration";

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

int AInterfaceToPMs::CountPM() const
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

bool AInterfaceToPMs::IsPmCenterWithin(int ipm, double x, double y, double distance_in_square)
{
  if (!checkValidPM(ipm)) return false;
  double dx = x - PMs->at(ipm).x;
  double dy = y - PMs->at(ipm).y;
  return ( (dx*dx + dy*dy) < distance_in_square );
}

bool AInterfaceToPMs::IsPmCenterWithinFast(int ipm, double x, double y, double distance_in_square) const
{
  double dx = x - PMs->at(ipm).x;
  double dy = y - PMs->at(ipm).y;
  return ( (dx*dx + dy*dy) < distance_in_square );
}

QVariant AInterfaceToPMs::GetPMtypes()
{
   QJsonObject obj;
   PMs->writePMtypesToJson(obj);
   QJsonArray ar = obj["PMtypes"].toArray();

   QVariant res = ar.toVariantList();
   return res;
}

QVariant AInterfaceToPMs::GetPMpositions() const
{
    QVariantList arr;
    const int numPMs = PMs->count();
    for (int ipm=0; ipm<numPMs; ipm++)
    {
        QVariantList el;
        el << QVariant(PMs->at(ipm).x) << QVariant(PMs->at(ipm).y) << QVariant(PMs->at(ipm).z);
        arr << QVariant(el);
    }
    //  qDebug() << QVariant(arr);
    return arr;
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
AInterfaceToOutputWin::AInterfaceToOutputWin(MainWindow *MW) :
    MW(MW)
{
    Description = "Access to the Output window of GUI";
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

// ------------- New LRF module interface ------------

ALrfScriptInterface::ALrfScriptInterface(DetectorClass *Detector, EventsDataClass *EventsDataHub) :
  Detector(Detector), EventsDataHub(EventsDataHub), repo(Detector->LRFs->getNewModule())
{
    Description = "Access to new LRF module";
}

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

AInterfaceToTree::AInterfaceToTree(TmpObjHubClass *TmpHub) :
    TmpHub(TmpHub)
{
    Description = "Reader for CERN ROOT Trees";
}

void AInterfaceToTree::OpenTree(const QString& TreeName, const QString& FileName, const QString& TreeNameInFile)
{
    ARootObjBase* r = TmpHub->Trees.getRecord(TreeName);
    if (r)
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

    r = new ARootObjBase(t, TreeName, "TTree");
    TmpHub->Trees.append(TreeName, r);
}

QString AInterfaceToTree::PrintBranches(const QString& TreeName)
{
    ARootObjBase* r = TmpHub->Trees.getRecord(TreeName);
    if (!r)
    {
        abort("Tree " + TreeName + " not found!");
        return "";
    }
    TTree *t = static_cast<TTree*>(r->GetObject());

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

QVariant AInterfaceToTree::GetBranch(const QString& TreeName, const QString& BranchName)
{
    ARootObjBase* r = TmpHub->Trees.getRecord(TreeName);
    if (!r)
    {
        abort("Tree " + TreeName + " not found!");
        return "";
    }
    TTree *t = static_cast<TTree*>(r->GetObject());

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

bool AInterfaceToTree::CloseTree(const QString& TreeName)
{
    return TmpHub->Trees.remove(TreeName);
}

void AInterfaceToTree::CloseAllTrees()
{
    TmpHub->Trees.clear();
}
