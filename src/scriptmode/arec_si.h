#ifndef AREC_SI_H
#define AREC_SI_H

#include "ascriptinterface.h"

#include <QString>
#include <QVariant>

class AReconstructionManager;
class AConfiguration;
class EventsDataClass;
class TmpObjHubClass;
class APmGroupsManager;

class ARec_SI : public AScriptInterface
{
  Q_OBJECT

public:
  ARec_SI(AReconstructionManager* RManager, AConfiguration* Config, EventsDataClass* EventsDataHub, TmpObjHubClass* TmpHub);
  ~ARec_SI(){}

  virtual void ForceStop() override;

public slots:
  void ReconstructEvents(int NumThreads = -1, bool fShow = true);
  void UpdateFilters(int NumThreads = -1);

  double GetChi2valueToCutTop(double cutUpper_fraction, int sensorGroup = 0);

  void DoBlurUniform(double range, bool fUpdateFilters = true);
  void DoBlurGauss(double sigma, bool fUpdateFilters = true);

  //Sensor groups
  int countSensorGroups();
  void clearSensorGroups();
  void addSensorGroup(QString name);
  void setPMsOfGroup(int igroup, QVariant PMlist);
  QVariant getPMsOfGroup(int igroup);

  //passive PMs
  bool isStaticPassive(int ipm);
  void setStaticPassive(int ipm);
  void clearStaticPassive(int ipm);
  void selectSensorGroup(int igroup);
  void clearSensorGroupSelection();

  //reconstruction data save
  void SaveAsTree(QString fileName, bool IncludePMsignals=true, bool IncludeRho=true, bool IncludeTrue=true, int SensorGroup=0);
  void SaveAsText(QString fileName);

  // manifest item handling
  void ClearManifestItems();
  int  CountManifestItems();
  void AddRoundManisfetItem(double x, double y, double Diameter);
  void AddRectangularManisfetItem(double x, double y, double dX, double dY, double Angle);
  void SetManifestItemLineProperties(int i, int color, int width, int style);

  //signal per ph el extraction
  const QVariant Peaks_GetSignalPerPhE() const;
  void Peaks_PrepareData();
  void Peaks_Configure(int bins, double from, double to, double sigmaPeakfinder, double thresholdPeakfinder, int maxPeaks = 30);
  double Peaks_Extract_NoAbortOnFail(int ipm);
  void Peaks_ExtractAll();
  QVariant Peaks_GetPeakPositions(int ipm);

  const QVariant GetSignalPerPhE_stat() const;

private:
  AReconstructionManager* RManager;
  AConfiguration* Config;
  EventsDataClass* EventsDataHub;
  APmGroupsManager* PMgroups;
  TmpObjHubClass* TmpHub;

signals:
  void RequestUpdateGuiForManifest();
  void RequestStopReconstruction();
};

#endif // AREC_SI_H
