#include "aconfiguration.h"
#include "detectorclass.h"
#include "asimsettings.h"
#include "aparticlesimsettings.h"
#include "ajsontools.h"
#include "sensorlrfs.h"
#include "apmgroupsmanager.h"
#include "amaterialparticlecolection.h"
#include "asandwich.h"
#include "ageneralsimsettings.h"
#include "apmhub.h"
#include "ageoconsts.h"

#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QtWidgets/QApplication>

AConfiguration::AConfiguration(QObject *parent) :
  QObject(parent) {}

//to do: //add messenger for warnings
bool AConfiguration::LoadConfig(QJsonObject &json, bool DetConstructor, bool SimSettings, bool ReconstrSettings)
{
  AGeoConsts::getInstance().clearConstants();
  ErrorString.clear();
  if (json.isEmpty())
    {
      ErrorString = "Json is empty!";
      qWarning() << ErrorString;
      return false;
    }

  //    qDebug() << " Loading detector config";
  if (DetConstructor)
    {
      if (json.contains("DetectorConfig"))
        {          
          //    qDebug() << ">>> Loading detector configuration...";
          emit Detector->requestClearEventsData();
          //    qDebug() << ">>> DataHub cleared";
          QJsonObject DetJson = json["DetectorConfig"].toObject();
          JSON["DetectorConfig"] = DetJson;
          Detector->BuildDetector(true); //if GUI present, update will trigger automatically //suppress sim gui update, json is stuill old!
        }
      else ErrorString = "Json does not contain detector settings!";
    }

  //    qDebug() << "Loading simulation config";
  if (SimSettings)
    {
      if (json.contains("SimulationConfig"))
        {
          //qDebug() << ">>> Loading simulation settings...";
          QJsonObject SimJson = json["SimulationConfig"].toObject();
          JSON["SimulationConfig"] = SimJson;

          UpdateSimSettingsOfDetector();

          emit requestSimulationGuiUpdate();
          emit requestSelectFirstActiveParticleSource();
        }
      else ErrorString = "Json does not contain simulation settings!";
    }
  else
  {
      emit requestSimulationGuiUpdate(); //in case new detector was loaded
  }

  //    qDebug() << "Loading reconstruction configuration";
  if (ReconstrSettings)
    {
      if (json.contains("ReconstructionConfig"))
        {
          //qDebug() << ">>> Loading reconstruction settings...";
          QJsonObject ReconJson = json["ReconstructionConfig"].toObject();
          Detector->PMgroups->fixSensorGroupsCompatibility(ReconJson);
          Detector->PMgroups->readGroupsFromJson(ReconJson);
          JSON["ReconstructionConfig"] = ReconJson;

          if (ReconJson.contains("ActiveLRF"))
          {
              QJsonObject lj = ReconJson["ActiveLRF"].toObject();
              Detector->LRFs->clear(Detector->PMs->count());
              Detector->LRFs->loadAll(lj);
          }

          emit requestReconstructionGuiUpdate();
          AskForLRFGuiUpdate();
        }
      else ErrorString = "Json does not contain reconstruction settings!";
    }

  if (json.contains("GUI"))
    {
      QJsonObject j = json["GUI"].toObject();
      JSON["GUI"] = j;
    }

  //compatibility: conversions
  if (json.contains("GeometryWindow"))
    {
      QJsonObject J = JSON["GUI"].toObject();
      QJsonObject j = json["GeometryWindow"].toObject();
      J["GeometryWindow"] = j;
      JSON["GUI"] = J;
    }
  if (json.contains("PartcleStackChecker"))
    {
      QJsonObject J = JSON["GUI"].toObject();
      QJsonObject j = json["PartcleStackChecker"].toObject();
      J["PartcleStackChecker"] = j;
      JSON["GUI"] = J;
    }
  if (JSON.contains("ReconstructionConfig"))
  {
      QJsonObject js = JSON["ReconstructionConfig"].toObject();
      if (js.contains("GuiMisc"))
      {
          QJsonObject jg = JSON["GUI"].toObject();
          QJsonObject jj = js["GuiMisc"].toObject();
          jg["ReconstructionWindow"] = jj;
          JSON["GUI"] = jg;

          js.remove("GuiMisc");
          JSON["ReconstructionConfig"] = js;
      }
  }

  bool bRes = true;
#ifndef __USE_ANTS_NCRYSTAL__
  if (Detector->MpCollection->isNCrystalInUse())
  {
      ErrorString = "Loaded config has material(s) configured for NCrystal library,\nwhich was disabled during ANTS2 compilation";
      bRes = false;
  }
#endif

  if (!ErrorString.isEmpty()) qWarning() << ErrorString;

  emit NewConfigLoaded();
  //qDebug() << ">>> Load done";
  return bRes;
}

void AConfiguration::SaveConfig(QJsonObject &json, bool DetConstructor, bool SimSettings, bool ReconstrSettings)
{
  json = JSON;
  if (!DetConstructor)   json["DetectorConfig"] = QJsonObject();
  if (!SimSettings)      json["SimulationConfig"] = QJsonObject();
  if (!ReconstrSettings) json["ReconstructionConfig"] = QJsonObject();
}

bool AConfiguration::LoadConfig(QString fileName, bool DetConstructor, bool SimSettings, bool ReconstrSettings)//, QJsonObject *jsout)
{
    ErrorString.clear();

    QFile loadFile(fileName);
    if (!loadFile.open(QIODevice::ReadOnly))
    {
        ErrorString = "Cannot open file " + fileName;
        qWarning() << ErrorString;
        return false;
    }

    QByteArray saveData = loadFile.readAll();
    QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));
    QJsonObject json = loadDoc.object();

    if (json.isEmpty())
    {
        ErrorString = fileName + " is not valid configuration file";
        qWarning() << ErrorString;
        return false;
    }

    //if (jsout) *jsout = json; //for global script mode

    invalidateUndo();
    invalidateRedo();

    return LoadConfig(json, DetConstructor, SimSettings, ReconstrSettings);
}

bool AConfiguration::SaveConfig(QString fileName, bool DetConstructor, bool SimSettings, bool ReconstrSettings)
{
    ErrorString.clear();

    QJsonObject json;
    SaveConfig(json, DetConstructor, SimSettings, ReconstrSettings);

    bool fOk = SaveJsonToFile(json, fileName);
    if (!fOk)
    {
        ErrorString = "Failed to save config to file: " + fileName;
        qWarning() <<ErrorString;
    }
    return fOk;
}

#include "aglobalsettings.h"
void AConfiguration::createUndo()
{
    AGlobalSettings & GS = AGlobalSettings::getInstance();
    SaveConfig(GS.QuicksaveDir + "/undo.json");
}

bool AConfiguration::isUndoAvailable() const
{
    AGlobalSettings & GS = AGlobalSettings::getInstance();
    QFile qf(GS.QuicksaveDir + "/undo.json");
    return qf.exists();
}

bool AConfiguration::isRedoAvailable() const
{
    AGlobalSettings & GS = AGlobalSettings::getInstance();
    QFile qf(GS.QuicksaveDir + "/redo.json");
    return qf.exists();
}

void AConfiguration::invalidateUndo()
{
    AGlobalSettings & GS = AGlobalSettings::getInstance();
    QFile qf(GS.QuicksaveDir + "/undo.json");
    qf.remove();
}

void AConfiguration::invalidateRedo()
{
    AGlobalSettings & GS = AGlobalSettings::getInstance();
    QFile qf(GS.QuicksaveDir + "/redo.json");
    qf.remove();
}

QString AConfiguration::doUndo()
{
    AGlobalSettings & GS = AGlobalSettings::getInstance();
    SaveConfig(GS.QuicksaveDir + "/redoTmp.json");
    LoadConfig(GS.QuicksaveDir + "/undo.json");
    //invalidateUndo();  //automatic now
    QFile qf(GS.QuicksaveDir + "/redoTmp.json");
    qf.rename(GS.QuicksaveDir + "/redo.json");
    return ErrorString;
}

QString AConfiguration::doRedo()
{
    AGlobalSettings & GS = AGlobalSettings::getInstance();
    LoadConfig(GS.QuicksaveDir + "/redo.json");
    //invalidateRedo();  //automatic now
    //invalidateUndo();  //automatic now
    return ErrorString;
}

void AConfiguration::UpdateLRFmakeJson()
{
    QJsonObject obj = JSON["ReconstructionConfig"].toObject();
    obj["LRFmakeJson"] = Detector->LRFs->LRFmakeJson;
    //qDebug() << Detector->SensLRF->LRFmakeJson;
    JSON["ReconstructionConfig"] = obj;
}

void AConfiguration::UpdateReconstructionSettings(QJsonObject &jsonRec, int iGroup)
{
    QJsonObject jRC = JSON["ReconstructionConfig"].toObject();
    QJsonArray jRO = jRC["ReconstructionOptions"].toArray();
    jRO[iGroup] = jsonRec;
    jRC["ReconstructionOptions"] = jRO;
    JSON["ReconstructionConfig"] = jRC;
}

void AConfiguration::UpdateFilterSettings(QJsonObject &jsonFilt, int iGroup)
{
    QJsonObject jRC = JSON["ReconstructionConfig"].toObject();
    QJsonArray jFO = jRC["FilterOptions"].toArray();
    jFO[iGroup] = jsonFilt;
    jRC["FilterOptions"] = jFO;
    JSON["ReconstructionConfig"] = jRC;
}

void AConfiguration::UpdateParticlesJson()
{
  QJsonObject djson = JSON["DetectorConfig"].toObject();
  Detector->MpCollection->writeParticleCollectionToJson(djson);
  JSON["DetectorConfig"] = djson;
  //emit requestDetectorGuiUpdate();
}

void AConfiguration::UpdateSourcesJson(QJsonObject &sourcesJson)
{
    QJsonObject sj = JSON["SimulationConfig"].toObject();
    QJsonObject pj = sj["ParticleSourcesConfig"].toObject();

    QJsonArray sArray = sourcesJson["ParticleSources"].toArray();
    pj["ParticleSources"] = sArray;

    sj["ParticleSourcesConfig"] = pj;
    JSON["SimulationConfig"] = sj;

    emit requestSimulationGuiUpdate();
}

void AConfiguration::AskForAllGuiUpdate()
{
   AskForDetectorGuiUpdate();
   AskForSimulationGuiUpdate();
   AskForReconstructionGuiUpdate();
}

void AConfiguration::AskForDetectorGuiUpdate()
{
    //qDebug() << "Emitting signal to update detector gui";
    emit requestDetectorGuiUpdate();
}

void AConfiguration::AskForSimulationGuiUpdate()
{
    //qDebug() << "Emitting signal to update simulation gui";
    emit requestSimulationGuiUpdate();
}

void AConfiguration::AskForReconstructionGuiUpdate()
{
    //qDebug() << "Emitting signal to update reconstruction gui";
  emit requestReconstructionGuiUpdate();
}

//#include "arepository.h"
void AConfiguration::AskForLRFGuiUpdate()
{
    //qDebug() << "Emitting signal to update lrf gui of the old module";
    emit requestLRFGuiUpdate();

    QJsonObject jrec = JSON["ReconstructionConfig"].toObject();
//    QJsonObject js = jrec["LRFv3makeJson"].toObject();
    //qDebug() << js;
//    if (!js.isEmpty()) Detector->LRFs->getNewModule()->setNextUpdateConfig(js);
}

void AConfiguration::AskChangeGuiBusy(bool flag)
{
    emit requestGuiBusyStatusChange(flag);
    qApp->processEvents();
}

const QString AConfiguration::RemoveParticle(int particleId)
{
  QString s;
  bool bInUse = true;

  Detector->MpCollection->IsParticleInUse(particleId, bInUse, s);
  if (bInUse) return "This particle is a secondary particle defined in neutron capture.\nIt appears in the following material(s):\n"+s;

  s = SimSettings->partSimSet.isParticleInUse(particleId);
  if (!s.isEmpty()) return s;

  Detector->Sandwich->IsParticleInUse(particleId, bInUse, s);
  if (bInUse) return "This particle is currently in use by the monitor(s):\n" + s;

  //this particle is not in use, so can be removed
  Detector->MpCollection->RemoveParticle(particleId);
  SimSettings->partSimSet.removeParticle(particleId);
  Detector->Sandwich->RemoveParticle(particleId);

  //updating JSON config
    //detector - MpCollection and Monitors
  Detector->writeToJson(JSON);
    //particle sources
  QJsonObject sj = JSON["SimulationConfig"].toObject();
    QJsonObject pj = sj["ParticleSourcesConfig"].toObject();
       SimSettings->partSimSet.writeToJson(pj);   // !*! to be changed when SimSettings will be the main line
    sj["ParticleSourcesConfig"] = pj;
  JSON["SimulationConfig"] = sj;

  //updating gui if present
  emit requestDetectorGuiUpdate();
  emit requestSimulationGuiUpdate();

  return "";
}

void AConfiguration::UpdateSimSettingsOfDetector()
{
    if (JSON.contains("SimulationConfig"))
    {
        QJsonObject SimJson = JSON["SimulationConfig"].toObject();

        AGeneralSimSettings simSettings;
        simSettings.readFromJson(SimJson);
        Detector->PMs->configure(&simSettings); //wave, angle properties + rebin, prepare crosstalk
        Detector->MpCollection->UpdateRuntimePropertiesAndWavelengthBinning(&simSettings, Detector->RandGen);
    }
}
