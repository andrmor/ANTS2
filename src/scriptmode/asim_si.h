#ifndef ASIM_SI_H
#define ASIM_SI_H

#include "ascriptinterface.h"

#include <QObject>
#include <QVariant>

class ASimulationManager;
class EventsDataClass;
class AConfiguration;

class ASim_SI : public AScriptInterface
{
  Q_OBJECT

public:
  ASim_SI(ASimulationManager* SimulationManager, EventsDataClass* EventsDataHub, AConfiguration* Config, int RecNumThreads, bool fGuiPresent = true);
  ~ASim_SI(){}

  virtual void ForceStop();

public slots:
  bool RunPhotonSources(int NumThreads = -1);
  bool RunParticleSources(int NumThreads = -1);

  void SetSeed(long seed);
  long GetSeed();

  void ClearCustomNodes();
  void AddCustomNode(double x, double y, double z);
  QVariant GetCustomNodes();
  bool SetCustomNodes(QVariant ArrayOfArray3);

  bool SaveAsTree(QString fileName);
  bool SaveAsText(QString fileName, bool IncludeTruePositionAndNumPhotons = true);

  int countMonitors();
  int getMonitorHits(QString monitor);

  QVariant getMonitorTime(QString monitor);
  QVariant getMonitorAngular(QString monitor);
  QVariant getMonitorWave(QString monitor);
  QVariant getMonitorEnergy(QString monitor);
  QVariant getMonitorXY(QString monitor);

  void SetGeant4Executable(QString FileName);
  void RunSim_Geant4(int numThreads, bool OnlyGenerateFiles = false);

signals:
  void requestStopSimulation();

private:
  ASimulationManager* SimulationManager;
  EventsDataClass* EventsDataHub;
  AConfiguration* Config;

  int RecNumThreads;
  bool fGuiPresent;

  QVariant getMonitorData1D(QString monitor, QString whichOne);
};

#endif // ASIM_SI_H
