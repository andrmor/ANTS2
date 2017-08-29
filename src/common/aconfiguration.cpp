#include "aconfiguration.h"
#include "detectorclass.h"
#include "ajsontools.h"
#include "alrfmoduleselector.h"
#include "apmgroupsmanager.h"
#include "amaterialparticlecolection.h"
#include "particlesourcesclass.h"
#include "asandwich.h"

#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QApplication>

AConfiguration::AConfiguration(QObject *parent) :
  QObject(parent), Detector(0), ParticleSources(0) {}

//to do: //add messenger for warnings
bool AConfiguration::LoadConfig(QJsonObject &json, bool DetConstructor, bool SimSettings, bool ReconstrSettings)
{
  ErrorString.clear();
  if (json.isEmpty())
    {
      ErrorString = "Json is empty!";
      qWarning() << ErrorString;
      return false;
    }

  // qDebug() << " Loading detector config";
  if (DetConstructor)
    {
      if (json.contains("DetectorConfig"))
        {          
          //qDebug() << ">>> Loading detector configuration...";
          emit Detector->requestClearEventsData();
          QJsonObject DetJson = json["DetectorConfig"].toObject();
          JSON["DetectorConfig"] = DetJson;
          Detector->BuildDetector(true); //if GUI present, update will trigger automatically //suppress sim gui update, json is stuill old!
        }
      else
        {
          ErrorString = "Json does not contain detector settings!";
          qWarning() << ErrorString;
        }
    }

  //qDebug() << "Loading simulation config";
  if (SimSettings)
    {
      if (json.contains("SimulationConfig"))
        {
          //qDebug() << ">>> Loading simulation settings...";
          QJsonObject SimJson = json["SimulationConfig"].toObject();
          JSON["SimulationConfig"] = SimJson;
          emit requestSimulationGuiUpdate();
          emit requestSelectFirstActiveParticleSource();
        }
      else
        {
          ErrorString = "Json does not contain simulation settings!";
          qWarning() << ErrorString;
        }
    }
  else
  {
      emit requestSimulationGuiUpdate(); //in case new detector was loaded
  }

  //qDebug() << "Loading reconstruction configuration";
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
              Detector->LRFs->loadAll_v2(lj);
            }
          if (ReconJson.contains("ActiveLRFv3"))
            {
              QJsonObject lj = ReconJson["ActiveLRFv3"].toObject();
              Detector->LRFs->loadAll_v3(lj);
            }

          emit requestReconstructionGuiUpdate();
          AskForLRFGuiUpdate();
        }
      else
      {
          ErrorString = "Json does not contain reconstruction settings!";
          qWarning() << ErrorString;
      }
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

  emit NewConfigLoaded();
  //qDebug() << ">>> Load done";
  return true;
}

void AConfiguration::SaveConfig(QJsonObject &json, bool DetConstructor, bool SimSettings, bool ReconstrSettings)
{
  json = JSON;
  if (!DetConstructor)   json["DetectorConfig"] = QJsonObject();
  if (!SimSettings)      json["SimulationConfig"] = QJsonObject();
  if (!ReconstrSettings) json["ReconstructionConfig"] = QJsonObject();
}

bool AConfiguration::LoadConfig(QString fileName, bool DetConstructor, bool SimSettings, bool ReconstrSettings, QJsonObject *jsout)
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
        ErrorString = "File does not contain json object:\nWrong file or unsupported old format";
        qWarning() << "ErrorString";
        return false;
      }
    if (jsout) *jsout = json; //for global script mode
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

void AConfiguration::UpdateLRFmakeJson()
{
    QJsonObject obj = JSON["ReconstructionConfig"].toObject();
    obj["LRFmakeJson"] = Detector->LRFs->getLRFmakeJson();
    //qDebug() << Detector->SensLRF->LRFmakeJson;
    JSON["ReconstructionConfig"] = obj;
}

void AConfiguration::UpdateLRFv3makeJson()
{
    QJsonObject obj = JSON["ReconstructionConfig"].toObject();
    obj["LRFv3makeJson"] = Detector->LRFs->getLRFv3makeJson();
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
  emit requestDetectorGuiUpdate();
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

void AConfiguration::ClearCustomNodes()
{
    QJsonObject sim = JSON["SimulationConfig"].toObject();
    QJsonObject ps = sim["PointSourcesConfig"].toObject();
    QJsonObject cn = ps["CustomNodesOptions"].toObject();

    cn["Nodes"] = QJsonArray();

    ps["CustomNodesOptions"] = cn;
    sim["PointSourcesConfig"] = ps;
    JSON["SimulationConfig"] = sim;

    emit requestSimulationGuiUpdate();
}

QJsonArray AConfiguration::GetCustomNodes()
{
    QJsonObject sim = JSON["SimulationConfig"].toObject();
    QJsonObject ps = sim["PointSourcesConfig"].toObject();
    QJsonObject cn = ps["CustomNodesOptions"].toObject();

    QJsonArray arr = cn["Nodes"].toArray();
    return arr;
}

void AConfiguration::AddCustomNode(double x, double y, double z)
{
    QJsonObject sim = JSON["SimulationConfig"].toObject();
    QJsonObject ps = sim["PointSourcesConfig"].toObject();
    QJsonObject cn = ps["CustomNodesOptions"].toObject();
    QJsonArray arr = cn["Nodes"].toArray();

    QJsonArray el;
    el << x << y << z;
    arr.append(el);

    cn["Nodes"] = arr;
    ps["CustomNodesOptions"] = cn;
    sim["PointSourcesConfig"] = ps;
    JSON["SimulationConfig"] = sim;

    emit requestSimulationGuiUpdate();
}

bool AConfiguration::SetCustomNodes(QJsonArray arr)
{
    for (int i=0; i<arr.size(); i++)
    {
        if (!arr.at(i).isArray()) return false;
        QJsonArray a = arr.at(i).toArray();
        if (a.size()!=3) return false;
        if (!a.at(0).isDouble()) return false;
        if (!a.at(1).isDouble()) return false;
        if (!a.at(2).isDouble()) return false;
    }

    QJsonObject sim = JSON["SimulationConfig"].toObject();
    QJsonObject ps = sim["PointSourcesConfig"].toObject();
    QJsonObject cn = ps["CustomNodesOptions"].toObject();

    cn["Nodes"] = arr;

    ps["CustomNodesOptions"] = cn;
    sim["PointSourcesConfig"] = ps;
    JSON["SimulationConfig"] = sim;

    emit requestSimulationGuiUpdate();
    return true;
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

#include "arepository.h"
void AConfiguration::AskForLRFGuiUpdate()
{
    //qDebug() << "Emitting signal to update lrf gui of the old module";
    emit requestLRFGuiUpdate();

    QJsonObject jrec = JSON["ReconstructionConfig"].toObject();
    QJsonObject js = jrec["LRFv3makeJson"].toObject();
    //qDebug() << js;
    if (!js.isEmpty()) Detector->LRFs->getNewModule()->setNextUpdateConfig(js);
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

  ParticleSources->onIsParticleInUse1(particleId, bInUse, s);
  if (bInUse) return "This particle is in use by the particle source(s):\n" + s;

  Detector->Sandwich->onIsParticleInUse1(particleId, bInUse, s);
  if (bInUse) return "This particle is currently in use by the monitor(s):\n" + s;

  //this particle is not in use, so can be removed
  Detector->MpCollection->RemoveParticle(particleId);
  ParticleSources->RemoveParticle(particleId);
  Detector->Sandwich->onRequestRemoveParticle(particleId);

  //updating JSON config
    //detector - MpCollection and Monitors
  Detector->writeToJson(JSON);
    //particle sources
  QJsonObject sj = JSON["SimulationConfig"].toObject();
    QJsonObject pj = sj["ParticleSourcesConfig"].toObject();
       ParticleSources->writeToJson(pj);
    sj["ParticleSourcesConfig"] = pj;
  JSON["SimulationConfig"] = sj;

  //updating gui if present
  emit requestDetectorGuiUpdate();
  emit requestSimulationGuiUpdate();
  emit RequestClearParticleStack(); //clear defined ParticleStack in GUI

  return "";
}
