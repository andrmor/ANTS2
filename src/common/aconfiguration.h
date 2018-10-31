#ifndef ACONFIGURATION_H
#define ACONFIGURATION_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>

class DetectorClass;
class ASourceParticleGenerator;

class AConfiguration : public QObject
{
  Q_OBJECT
public:
  explicit AConfiguration(QObject *parent = 0);
  ~AConfiguration() {}

  QJsonObject JSON;         // Json file with all configuration (detector, sim, reconstructions)
  QString ErrorString;      // Last detected error (load config)

  void SetDetector(DetectorClass* detector) {Detector = detector;}
  void SetParticleSources(ASourceParticleGenerator* particleSources) {ParticleSources = particleSources;}
  DetectorClass* GetDetector() {return Detector;}

  // save/load to json
  bool LoadConfig(QJsonObject &json, bool DetConstructor = true, bool SimSettings = true, bool ReconstrSettings = true);
  void SaveConfig(QJsonObject &json, bool DetConstructor, bool SimSettings, bool ReconstrSettings);

  // save/load to file
  bool LoadConfig(QString fileName, bool DetConstructor = true, bool SimSettings = true, bool ReconstrSettings = true, QJsonObject *jsout = 0);
  bool SaveConfig(QString fileName, bool DetConstructor = true, bool SimSettings = true, bool ReconstrSettings = true);

  //Simulation module specific
  void ClearCustomNodes();
  QJsonArray GetCustomNodes();
  void AddCustomNode(double x, double y, double z);
  bool SetCustomNodes(QJsonArray arr);

  //remove particle
  const QString RemoveParticle(int particleId);  //returns "" on sucess, otherwise gives error string

  //update of sim-related settings in PMs and MPcollection
  void UpdateSimSettingsOfDetector(); //has to be called after simulation (rebuilds detector) and in gui after detector rebuild

  void AskForAllGuiUpdate();
  void AskForDetectorGuiUpdate();
  void AskForSimulationGuiUpdate();
  void AskForReconstructionGuiUpdate();
  void AskForLRFGuiUpdate();
  void AskChangeGuiBusy(bool flag);

public slots:
  //module-specific
  void UpdateLRFmakeJson();
  void UpdateLRFv3makeJson();
  void UpdateReconstructionSettings(QJsonObject& jsonRec, int iGroup);
  void UpdateFilterSettings(QJsonObject& jsonFilt, int iGroup);
  void UpdateParticlesJson();  //does not trigger gui update anymore!
  void UpdateSourcesJson(QJsonObject& sourcesJson);  

signals:
  void requestDetectorGuiUpdate();          //connect in MainWindowInits.cpp
  void requestSimulationGuiUpdate();        //connect in MainWindowInits.cpp
  void requestSelectFirstActiveParticleSource(); //same
  void requestReconstructionGuiUpdate();    //connect in MainWindowInits.cpp
  void requestLRFGuiUpdate();               //connect in MainWindowInits.cpp
  void NewConfigLoaded();                   //connect in MainWindowInits.cpp // update GUI after new detector was loaded - only those things which should not update on each GUI update
  void requestGuiBusyStatusChange(bool);

  //for remove particle
  void IsParticleInUseByMaterials(int particleId, bool& bInUse, QString& s);
  void IsParticleInUseBySources(int particleId, bool& bInUse, QString& s);
  void IsParticleInUseByMonitors(int particleId, bool& bInUse, QString& s);
  void RequestRemoveParticle(int particleId);
  void RequestClearParticleStack();

public slots:

private:
  DetectorClass* Detector;                  // Link to the Detector object
  ASourceParticleGenerator* ParticleSources;    // Link to the ParticleSources object of SimulationManager

};

#endif // ACONFIGURATION_H
