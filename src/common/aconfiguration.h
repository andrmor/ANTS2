#ifndef ACONFIGURATION_H
#define ACONFIGURATION_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>

class DetectorClass;

class AConfiguration : public QObject
{
  Q_OBJECT
public:
  explicit AConfiguration(QObject *parent = 0);
  ~AConfiguration() {}

  QJsonObject JSON;         // Json file with all configuration (detector, sim, reconstructions)
  QString ErrorString;      // Last detected error (load config)

  void SetDetector(DetectorClass* detector) {Detector = detector;}
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
  void UpdateParticlesJson();
  void UpdateSourcesJson(QJsonObject& sourcesJson);

signals:
  void requestDetectorGuiUpdate();          //connect in MainWindowInits.cpp
  void requestSimulationGuiUpdate();        //connect in MainWindowInits.cpp
  void requestReconstructionGuiUpdate();    //connect in MainWindowInits.cpp
  void requestLRFGuiUpdate();               //connect in MainWindowInits.cpp
  void NewConfigLoaded();                   //connect in MainWindowInits.cpp // update GUI after new detector was loaded - only those things which should not update on each GUI update
  void requestGuiBusyStatusChange(bool);

public slots:

private:
  DetectorClass* Detector;  // Link to the Detector object
};

#endif // ACONFIGURATION_H
