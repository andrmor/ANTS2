#ifndef RECONSTRUCTIONMANAGERCLASS_H
#define RECONSTRUCTIONMANAGERCLASS_H

/////////////////////////////////////
// Performs reconstruction: CoG, MG and RootMini - multithread; CUDA and ANN - single thread;
// CoG - preparation phase for all algorithms (multithread)
// Then recons with selected algorithm
// Chi2 calculation (multithread)
// Basic filtering  (multithread)
// Advanced filters: correlation, PrimScint, kNN filters - single thread!
/////////////////////////////////////

#include "reconstructionsettings.h"
#include "aeventfilteringsettings.h"

#include <QObject>
#include <QVector>
#include <atomic>

class EventsDataClass;
class pms;
class APmGroupsManager;
class ALrfModuleSelector;
class TGeoManager;
class DynamicPassivesHandler;
class ProcessorClass;
class NNmoduleClass;
#ifdef ANTS_FANN
class NeuralNetworksModule;
#endif

class ReconstructionManagerClass : public QObject
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
  //ReconstructionManagerClass(EventsDataClass *eventsDataHub, DetectorClass *detector);
  ReconstructionManagerClass(EventsDataClass *eventsDataHub, pms* PMs, APmGroupsManager* PMgroups, ALrfModuleSelector* LRFs, TGeoManager* *PGeoManager = 0);
  ~ReconstructionManagerClass();

  bool reconstructAll(QJsonObject &json, int numThreads, bool fShow = true); //fShow=true -> send signal to show reconstructed positions if Geom window is visible
  void filterEvents(QJsonObject &json, int NumThreads);

  bool isBusy() const {return bBusy;}

  QString getErrorString() {return ErrorString;}
  double getUsPerEvent() {return usPerEvent;}

  //kNN module, can be directly accessed from outside
#ifdef ANTS_FLANN
  NNmoduleClass* KNNmodule;
#endif
  //Artificail neural network module, can be accessed directly
#ifdef ANTS_FANN
  NeuralNetworksModule *ANNmodule;
#endif

  EventsDataClass *EventsDataHub;
  pms* PMs;
  APmGroupsManager* PMgroups;
  ALrfModuleSelector* LRFs;

private:
  TGeoManager** PGeoManager;  //pointer to pinter, since GeoManager recreated on each detector rebuild
  QVector<ReconstructionSettings> RecSet;
  int CurrentGroup;
  QVector<AEventFilteringSettings> FiltSet;
  int NumThreads;

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

signals:
  void UpdateReady(int Progress, double MsPerEv);
  void ReconstructionFinished(bool fSuccess, bool fShow);
  void RequestShowStatistics(); // emitted after, e.g., refiltering events (show good events)
};

#endif // RECONSTRUCTIONMANAGERCLASS_H
