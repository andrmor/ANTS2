#ifndef AEVENTS_SI_H
#define AEVENTS_SI_H

#include "ascriptinterface.h"

#include <QVariant>
#include <QString>

class AConfiguration;
class EventsDataClass;

class AEvents_SI : public AScriptInterface
{
  Q_OBJECT

public:
  AEvents_SI(AConfiguration* Config, EventsDataClass* EventsDataHub);
  ~AEvents_SI(){}

  bool IsMultithreadCapable() const override {return true;}

public slots:
  int GetNumPMs();
  int countPMs();
  int GetNumEvents();
  int countEvents();
  int countTimedEvents();
  int countTimeBins();
  double GetPMsignal(int ievent, int ipm);
  QVariant GetPMsignals(int ievent);
  const QVariantList GetPMsignals() const;
  void SetPMsignal(int ievent, int ipm, double value);
  double GetPMsignalTimed(int ievent, int ipm, int iTimeBin);
  QVariant GetPMsignalVsTime(int ievent, int ipm);

  // Reconstructed values
    //assuming there is only one group, and single point reconstruction
  double GetReconstructedX(int ievent);
  double GetReconstructedY(int ievent);
  double GetReconstructedZ(int ievent);
  QVariantList GetReconstructedXYZ(int ievent);
  QVariantList GetReconstructedXYZE(int ievent);
  double GetRho(int ievent, int iPM);
  double GetRho2(int ievent, int iPM);
  double GetReconstructedEnergy(int ievent);
  bool IsReconstructedGoodEvent(int ievent);
    //general
  double GetReconstructedX(int igroup, int ievent, int ipoint);
  double GetReconstructedY(int igroup, int ievent, int ipoint);
  double GetReconstructedZ(int igroup, int ievent, int ipoint);
  QVariantList GetReconstructedPoints(int ievent);
  QVariantList GetReconstructedPoints(int igroup, int ievent);
  double GetRho(int igroup, int ievent, int ipoint, int iPM);
  double GetRho2(int igroup, int ievent, int ipoint, int iPM);
  double GetReconstructedEnergy(int igroup, int ievent, int ipoint);
  bool IsReconstructedGoodEvent(int igroup, int ievent);
  bool IsReconstructed_ScriptFilterPassed(int igroup, int ievent);
    //counters - return -1 when invalid parameters
  int countReconstructedGroups();
  int countReconstructedEvents(int igroup);
  int countReconstructedPoints(int igroup, int ievent);

  // True values known in sim or from a calibration dataset
  double GetTrueX(int ievent, int iPoint = 0);
  double GetTrueY(int ievent, int iPoint = 0);
  double GetTrueZ(int ievent, int iPoint = 0);
  double GetTrueEnergy(int ievent, int iPoint = 0);
  const QVariant GetTruePoints(int ievent);
  const QVariant GetTruePointsXYZE(int ievent);
  const QVariant GetTruePointsXYZEiMat(int ievent);
  bool IsTrueGoodEvent(int ievent);
  int  GetTrueNumberPoints(int ievent);

  void SetScanX(int ievent, double value);
  void SetScanY(int ievent, double value);
  void SetScanZ(int ievent, double value);
  void SetScanEnergy(int ievent, double value);

  //raw signal values
  QVariant GetPMsSortedBySignal(int ievent);
  int GetPMwithMaxSignal(int ievent);

  //for custom reconstrtuctions
    //assuming there is only one group, and single point reconstruction
  void SetReconstructed(int ievent, double x, double y, double z, double e);
  void SetReconstructed(int ievent, double x, double y, double z, double e, double chi2);
  void SetReconstructedX(int ievent, double x);
  void SetReconstructedY(int ievent, double y);
  void SetReconstructedZ(int ievent, double z);
  void SetReconstructedEnergy(int ievent, double e);
  void SetReconstructed_ScriptFilterPass(int ievent, bool flag);
  void SetReconstructedGoodEvent(int ievent, bool good);
  void SetReconstructedAllEventsGood(bool flag);
  void SetReconstructionOK(int ievent, bool OK);

    //general
  void SetReconstructed(int igroup, int ievent, int ipoint, double x, double y, double z, double e);
  void SetReconstructedFast(int igroup, int ievent, int ipoint, double x, double y, double z, double e); // no checks!!! unsafe
  void AddReconstructedPoint(int igroup, int ievent, double x, double y, double z, double e);
  void SetReconstructedX(int igroup, int ievent, int ipoint, double x);
  void SetReconstructedY(int igroup, int ievent, int ipoint, double y);
  void SetReconstructedZ(int igroup, int ievent, int ipoint, double z);
  void SetReconstructedEnergy(int igroup, int ievent, int ipoint, double e);
  void SetReconstructedGoodEvent(int igroup, int ievent, int ipoint, bool good);
  void SetReconstructed_ScriptFilterPass(int igroup, int ievent, bool flag);
  void SetReconstructionOK(int igroup, int ievent, int ipoint, bool OK);
    //set when reconstruction is ready for all events! - otherwise GUI will complain
  void SetReconstructionReady();
    //clear reconstruction and prepare containers
  void ResetReconstructionData(int numGroups);

  //load data
  void LoadEventsTree(QString fileName, bool Append = false, int MaxNumEvents = -1);
  void LoadEventsAscii(QString fileName, bool Append = false);

  //clear data
  void ClearEvents();

  //Purges
  void PurgeBad();
  void Purge(int LeaveOnePer);

  //Statistics
  QVariant GetStatistics(int igroup);

private:
  AConfiguration* Config;
  EventsDataClass* EventsDataHub;

  bool checkEventNumber(int ievent);
  bool checkEventNumber(int igroup, int ievent, int ipoint);
  bool checkPM(int ipm);
  bool checkTrueDataRequest(int ievent, int iPoint = 0);
  bool checkSetReconstructionDataRequest(int ievent);

signals:
  void RequestEventsGuiUpdate();
};

#endif // AEVENTS_SI_H
