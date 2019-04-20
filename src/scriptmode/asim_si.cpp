#include "asim_si.h"
#include "simulationmanager.h"
#include "eventsdataclass.h"
#include "aglobalsettings.h"
#include "aconfiguration.h"
#include "detectorclass.h"
#include "amonitor.h"
#include "anoderecord.h"

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

bool ASim_SI::InitOnRun()
{
    SimulationManager->clearNodes();
    return true;
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

    if (!SimulationManager->fSuccess)
        abort(SimulationManager->Runner->getErrorMessages());
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

long ASim_SI::GetSeed() const
{
    return Config->GetDetector()->RandGen->GetSeed();
}

void ASim_SI::ClearNodes()
{
    SimulationManager->clearNodes();
}

int ASim_SI::CountNodes(bool onlyTop)
{
    if (onlyTop) return SimulationManager->Nodes.size();

    int counter = 0;
    for (ANodeRecord * rec : SimulationManager->Nodes)
        counter += 1 + rec->getNumberOfLinkedNodes();
    return counter;
}

void ASim_SI::AddNode(double X, double Y, double Z, double Time, int numPhotons)
{
    ANodeRecord * n = ANodeRecord::createS(X, Y, Z, Time, numPhotons);
    SimulationManager->Nodes.push_back(n);
}

void ASim_SI::AddSubNode(double X, double Y, double Z, double Time, int numPhotons)
{
    int nodes = SimulationManager->Nodes.size();
    if (nodes == 0)
        abort("Cannot add a subnode: Container with nodes is empty");
    else
    {
        ANodeRecord * n = ANodeRecord::createS(X, Y, Z, Time, numPhotons);
        SimulationManager->Nodes[nodes-1]->addLinkedNode(n);
    }
}

bool ASim_SI::SaveAsTree(QString fileName)
{
  return EventsDataHub->saveSimulationAsTree(fileName);
}

bool ASim_SI::SaveAsText(QString fileName, bool IncludeTruePositionAndNumPhotons)
{
  return EventsDataHub->saveSimulationAsText(fileName, IncludeTruePositionAndNumPhotons, IncludeTruePositionAndNumPhotons);
}

int ASim_SI::countMonitors() const
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

int ASim_SI::getMonitorHits(int index)
{
    if (!EventsDataHub->SimStat)
    {
        abort("Collect simulation statistics is OFF");
        return 0;
    }
    if (index < 0 || index >= EventsDataHub->SimStat->Monitors.size())
    {
        abort("Bad monitor index");
        return 0;
    }

    const AMonitor* mon = EventsDataHub->SimStat->Monitors.at(index);
    return mon->getHits();
}

QVariantList ASim_SI::getMonitorHitsAll()
{
    QVariantList vl;
    if (!EventsDataHub->SimStat)
    {
        abort("Collect simulation statistics is OFF");
        return vl;
    }

    const int num = EventsDataHub->SimStat->Monitors.size();
    for (int i=0; i<num; i++)
    {
        const AMonitor* mon = EventsDataHub->SimStat->Monitors.at(i);
        vl.append(mon->getHits());
    }
    return vl;
}

QVariant ASim_SI::getMonitorData1D(const QString &monitor, dataType type) const
{
  QVariantList vl;
  if (!EventsDataHub->SimStat) return vl;
  for (int i=0; i<EventsDataHub->SimStat->Monitors.size(); i++)
  {
      const AMonitor * mon = EventsDataHub->SimStat->Monitors.at(i);
      if (mon->getName() == monitor)
      {
          TH1D* h = nullptr;
          switch (type)
          {
              case dat_time:   h = mon->getTime(); break;
              case dat_angle:  h = mon->getAngle(); break;
              case dat_wave:   h = mon->getWave(); break;
              case dat_energy: h = mon->getEnergy(); break;
          }
          if (!h) return vl;

          TAxis* axis = h->GetXaxis();
          for (int i=1; i<axis->GetNbins()+1; i++)
          {
              QVariantList el;
              el << axis->GetBinCenter(i);
              el << h->GetBinContent(i);
              vl.push_back(el);
          }
          break;
      }
  }
  return vl;
}

QVariant ASim_SI::getMonitorData1D(int index, dataType type) const
{
    QVariantList vl;
    if (!EventsDataHub->SimStat) return vl;
    if (index < 0 || index >= EventsDataHub->SimStat->Monitors.size()) return vl;

    const AMonitor* mon = EventsDataHub->SimStat->Monitors.at(index);
    TH1D* h = nullptr;
    switch (type)
    {
        case dat_time:   h = mon->getTime(); break;
        case dat_angle:  h = mon->getAngle(); break;
        case dat_wave:   h = mon->getWave(); break;
        case dat_energy: h = mon->getEnergy(); break;
    }
    if (!h) return vl;

    TAxis* axis = h->GetXaxis();
    for (int i=1; i<axis->GetNbins()+1; i++)
    {
        QVariantList el;
        el << axis->GetBinCenter(i);
        el << h->GetBinContent(i);
        vl.push_back(el);
    }
    return vl;
}

QVariant ASim_SI::getMonitorTime(QString monitor)
{
    return getMonitorData1D(monitor, dat_time);
}

QVariant ASim_SI::getMonitorTime(int index)
{
    return getMonitorData1D(index, dat_time);
}

QVariant ASim_SI::getMonitorAngular(QString monitor)
{
    return getMonitorData1D(monitor, dat_angle);
}

QVariant ASim_SI::getMonitorAngular(int index)
{
    return getMonitorData1D(index, dat_angle);
}

QVariant ASim_SI::getMonitorWave(QString monitor)
{
    return getMonitorData1D(monitor, dat_wave);
}

QVariant ASim_SI::getMonitorWave(int index)
{
    return getMonitorData1D(index, dat_wave);
}

QVariant ASim_SI::getMonitorEnergy(QString monitor)
{
    return getMonitorData1D(monitor, dat_energy);
}

QVariant ASim_SI::getMonitorEnergy(int index)
{
    return getMonitorData1D(index, dat_energy);
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
          break;
      }
  }
  return vl;
}

QVariant ASim_SI::getMonitorXY(int index)
{
    QVariantList vl;
    if (!EventsDataHub->SimStat) return vl;
    if (index < 0 || index >= EventsDataHub->SimStat->Monitors.size()) return vl;

    const AMonitor * mon = EventsDataHub->SimStat->Monitors.at(index);
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
    return vl;
}

void ASim_SI::SetGeant4Executable(QString FileName) const
{
    AGlobalSettings::getInstance().G4antsExec = FileName;
}
