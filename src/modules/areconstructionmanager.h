#ifndef ARECONSTRUCTIONMANAGER_H
#define ARECONSTRUCTIONMANAGER_H

// Performs reconstruction: CoG, MG and RootMini - multithread; CUDA and ANN - single thread;
// CoG - preparation phase for most of algorithms (multithread)
// Then recon with selected algorithm
// Chi2 calculation (multithread)
// Basic filtering  (multithread)
// Advanced filters: correlation, PrimScint, kNN filters - single thread!

#include "reconstructionsettings.h"
#include "aeventfilteringsettings.h"

#include <QObject>
#include <QVector>
#include <atomic>

class EventsDataClass;
class DetectorClass;
class APmHub;
class APmGroupsManager;
class ALrfModuleSelector;
class TmpObjHubClass;
class DynamicPassivesHandler;
class ProcessorClass;
class NNmoduleClass;
class ACalibratorSignalPerPhEl_Stat;
class ACalibratorSignalPerPhEl_Peaks;

#ifdef ANTS_FANN
class NeuralNetworksModule;
#endif

class AReconstructionManager : public QObject
{
  Q_OBJECT
  friend class ProcessorClass;
  friend class CoGReconstructorClass;
  friend class CGonCPUreconstructorClass;
  friend class RootMinReconstructorClass;
  friend class RootMinDoubleReconstructorClass;
  friend class Chi2calculatorClass;
  friend class EventFilterClass;

public:
  AReconstructionManager(EventsDataClass *eventsDataHub, DetectorClass* Detector, TmpObjHubClass* TmpObjHub);
  ~AReconstructionManager();

  bool reconstructAll(QJsonObject &json, int numThreads, bool fShow = true); //fShow=true -> send signal to show reconstructed positions if Geom window is visible
  void filterEvents(QJsonObject &json, int NumThreads);

  bool isBusy() const {return bBusy;}

  QString getErrorString() {return ErrorString;}
  double getUsPerEvent() {return usPerEvent;}

  void setMaxThread(int maxThreads) {MaxThreads = maxThreads;}

  //kNN module, can be directly accessed from outside
#ifdef ANTS_FLANN
  NNmoduleClass* KNNmodule;
#endif
  //Artificail neural network module, can be accessed directly
#ifdef ANTS_FANN
  NeuralNetworksModule *ANNmodule;
#endif

  EventsDataClass *EventsDataHub;
  DetectorClass* Detector;
  APmHub* PMs;
  APmGroupsManager* PMgroups;
  ALrfModuleSelector* LRFs;

  //Calibrators for signal per photo electron
  ACalibratorSignalPerPhEl_Stat* Calibrator_Stat; //based on statistics
  ACalibratorSignalPerPhEl_Peaks* Calibrator_Peaks; //based on distinguishable individual photoelectron peaks

private:
  TmpObjHubClass* TmpObjHub;
  QVector<ReconstructionSettings> RecSet;
  int CurrentGroup;
  QVector<AEventFilteringSettings> FiltSet;
  int NumThreads;
  int MaxThreads = -1;

  bool bBusy;

  std::atomic<bool> fDoingCopyLRFs;
 
  bool run(QList<ProcessorClass*> reconstructorList);

  //misc
  QString ErrorString;
  bool fStopRequested;
  int numEvents;
  double usPerEvent;

  bool fillSettingsAndVerify(QJsonObject &json, bool fCheckLRFs);
  bool configureFilters(QJsonObject &json);
  void distributeWork(int Algorithm, QList<ProcessorClass*> &todo);
  void doFilters();
  void singleThreadEventFilters(); //used to process Volume-specific spatial, correlation and kNN filters which HAS to be one thread  
  void assureReconstructionDataContainersExist();

public slots:
  void requestStop() {fStopRequested = true;}
  void onRequestClearKNNfilter();
  void onLRFsCopied();
  void onRequestFilterAndAskToUpdateGui(); //for gui remote, triggered by EventsDataHub

signals:
  void UpdateReady(int Progress, double MsPerEv);
  void ReconstructionFinished(bool fSuccess, bool fShow);
  void RequestShowStatistics(); // emitted after, e.g., refiltering events (show good events)
};

#endif // ARECONSTRUCTIONMANAGER_H
