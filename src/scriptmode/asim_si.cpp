#include "asim_si.h"
#include "simulationmanager.h"
#include "eventsdataclass.h"
#include "aglobalsettings.h"
#include "aconfiguration.h"
#include "detectorclass.h"
#include "amonitor.h"

#include <QJsonObject>
#include <QApplication>
#include <QDebug>

#include "TRandom2.h"
#include "TH1.h"
#include "TH1D.h"
#include "TH2.h"
#include "TH2D.h"
#include "TAxis.h"

ASim_SI::ASim_SI(ASimulationManager* SimulationManager, EventsDataClass *EventsDataHub, AConfiguration* Config, bool fGuiPresent)
  : SimulationManager(SimulationManager), EventsDataHub(EventsDataHub), Config(Config), fGuiPresent(fGuiPresent)
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

void ASim_SI::ForceStop()
{
  emit requestStopSimulation();
}

bool ASim_SI::RunPhotonSources(int NumThreads, bool AllowGuiUpdate)
{
    bool bGuiUpdate = fGuiPresent && AllowGuiUpdate;

    if (NumThreads == -1) NumThreads = AGlobalSettings::getInstance().RecNumTreads;
    QJsonObject sim = Config->JSON["SimulationConfig"].toObject();
    sim["Mode"] = "PointSim";
    Config->JSON["SimulationConfig"] = sim;
    if (bGuiUpdate) Config->AskForSimulationGuiUpdate();

    SimulationManager->StartSimulation(Config->JSON, NumThreads, bGuiUpdate);
    do
    {
        QThread::usleep(100);
        qApp->processEvents();
    }
    while (!SimulationManager->fFinished);
    return SimulationManager->fSuccess;
}

bool ASim_SI::RunParticleSources(int NumThreads, bool AllowGuiUpdate)
{
    bool bGuiUpdate = fGuiPresent && AllowGuiUpdate;

    if (NumThreads == -1) NumThreads = AGlobalSettings::getInstance().RecNumTreads;
    QJsonObject sim = Config->JSON["SimulationConfig"].toObject();
    sim["Mode"] = "SourceSim";
    Config->JSON["SimulationConfig"] = sim;
    if (bGuiUpdate) Config->AskForSimulationGuiUpdate();

    SimulationManager->StartSimulation(Config->JSON, NumThreads, bGuiUpdate);
    do
    {
        QThread::usleep(100);
        qApp->processEvents();
    }
    while (!SimulationManager->fFinished);
    return SimulationManager->fSuccess;
}

void ASim_SI::SetSeed(long seed)
{
  Config->GetDetector()->RandGen->SetSeed(seed);
}

long ASim_SI::GetSeed()
{
    return Config->GetDetector()->RandGen->GetSeed();
}

void ASim_SI::ClearCustomNodes()
{
    Config->ClearCustomNodes();
}

void ASim_SI::AddCustomNode(double x, double y, double z)
{
    Config->AddCustomNode(x, y, z);
}

QVariant ASim_SI::GetCustomNodes()
{
    QJsonArray arr = Config->GetCustomNodes();
    QVariant vr = arr.toVariantList();
    return vr;
}

bool ASim_SI::SetCustomNodes(QVariant ArrayOfArray3)
{
    QString type = ArrayOfArray3.typeName();
    if (type != "QVariantList") return false;

    QVariantList vl = ArrayOfArray3.toList();
    QJsonArray ar = QJsonArray::fromVariantList(vl);
    return Config->SetCustomNodes(ar);
}

bool ASim_SI::SaveAsTree(QString fileName)
{
  return EventsDataHub->saveSimulationAsTree(fileName);
}

bool ASim_SI::SaveAsText(QString fileName, bool IncludeTruePositionAndNumPhotons)
{
  return EventsDataHub->saveSimulationAsText(fileName, IncludeTruePositionAndNumPhotons, IncludeTruePositionAndNumPhotons);
}

int ASim_SI::countMonitors()
{
    if (!EventsDataHub->SimStat) return 0;
    return EventsDataHub->SimStat->Monitors.size();
}

int ASim_SI::getMonitorHits(QString monitor)
{
    if (!EventsDataHub->SimStat)
    {
        abort("Collect simulation statistics is OFF");
        return 0;
    }
    for (int i=0; i<EventsDataHub->SimStat->Monitors.size(); i++)
    {
        const AMonitor* mon = EventsDataHub->SimStat->Monitors.at(i);
        if (mon->getName() == monitor)
            return mon->getHits();
    }
    abort(QString("Monitor %1 not found").arg(monitor));
    return 0;
}

QVariant ASim_SI::getMonitorData1D(QString monitor, QString whichOne)
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

          if (!h) return vl;

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

QVariant ASim_SI::getMonitorTime(QString monitor)
{
    return getMonitorData1D(monitor, "time");
}

QVariant ASim_SI::getMonitorAngular(QString monitor)
{
  return getMonitorData1D(monitor, "angle");
}

QVariant ASim_SI::getMonitorWave(QString monitor)
{
  return getMonitorData1D(monitor, "wave");
}

QVariant ASim_SI::getMonitorEnergy(QString monitor)
{
  return getMonitorData1D(monitor, "energy");
}

QVariant ASim_SI::getMonitorXY(QString monitor)
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

void ASim_SI::SetGeant4Executable(QString FileName)
{
    AGlobalSettings::getInstance().G4antsExec = FileName;
}
