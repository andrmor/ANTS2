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
  ASim_SI(ASimulationManager* SimulationManager, EventsDataClass* EventsDataHub, AConfiguration* Config, bool fGuiPresent = true);
  ~ASim_SI(){}

  virtual bool InitOnRun() override;
  virtual void ForceStop() override;

public slots:
  bool RunPhotonSources(int NumThreads = -1, bool AllowGuiUpdate = false);
  bool RunParticleSources(int NumThreads = -1, bool AllowGuiUpdate = false);

  void SetSeed(long seed);
  long GetSeed() const;

  void ClearNodes();
  int  CountNodes(bool onlyTop);
  void AddNode(double X, double Y, double Z, double Time = 0, int numPhotons = -1);
  void AddNodes(QVariantList nodes);
  void AddSubNode(double X, double Y, double Z, double Time = 0, int numPhotons = -1);

  bool SaveAsTree(QString fileName);
  bool SaveAsText(QString fileName, bool IncludeTruePositionAndNumPhotons = true);

  int countMonitors() const;
  int getMonitorHits(QString monitor); //this and next: cannot be const beacuse of abort()
  int getMonitorHits(int index);

  QVariantList getMonitorHitsAll();
  QVariant getMonitorTime(QString monitor);
  QVariant getMonitorTime(int index);
  QVariant getMonitorAngular(QString monitor);
  QVariant getMonitorAngular(int index);
  QVariant getMonitorWave(QString monitor);
  QVariant getMonitorWave(int index);
  QVariant getMonitorEnergy(QString monitor);
  QVariant getMonitorEnergy(int index);
  QVariant getMonitorXY(QString monitor);
  QVariant getMonitorXY(int index);

  QVariant getMonitorEnergyStats(int index);

  void SetGeant4Executable(QString FileName) const;

signals:
  void requestStopSimulation();

private:
  ASimulationManager* SimulationManager;
  EventsDataClass* EventsDataHub;
  AConfiguration* Config;

  bool fGuiPresent;

  enum dataType {dat_time, dat_angle, dat_wave, dat_energy};
  QVariant getMonitorData1D(const QString & monitor, dataType type) const;
  QVariant getMonitorData1D(int index, dataType type) const;

  QVariant getMonitorStats1D(int index, dataType type) const;
};

#endif // ASIM_SI_H
