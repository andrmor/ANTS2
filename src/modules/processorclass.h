#ifndef PROCESSORCLASS_H
#define PROCESSORCLASS_H

#include <QObject>
#include "alrfmoduleselector.h"

class ReconstructionSettings;
class pms;
class APmGroupsManager;
class ALrfModuleSelector;
class DynamicPassivesHandler;
class EventsDataClass;
struct AReconRecord;
class AEventFilteringSettings;
namespace ROOT { namespace Minuit2 { class Minuit2Minimizer; } }
namespace ROOT { namespace Math { class Functor; } }

class ProcessorClass : public QObject
{
  Q_OBJECT
public:
  ProcessorClass(pms* PMs,
                 APmGroupsManager* PMgroups,
                 ALrfModuleSelector* LRFs,
                 EventsDataClass *EventsDataHub,
                 ReconstructionSettings *RecSet,
                 int ThisPmGroup,
                 int EventsFrom, int EventsTo);
  virtual ~ProcessorClass();

    // thread control
  char Progress; //char - it is completely safe (?) to retrieve from thread
  int eventsProcessed;
  bool fFinished;
  int Id;
  bool fStopRequested;

    // local object - for static use should be public
  DynamicPassivesHandler *DynamicPassives;

    // pointers: for static use with minimizer, should be public:  
  pms* PMs;
  APmGroupsManager* PMgroups;
  ALrfModuleSelector LRFs;
  EventsDataClass *EventsDataHub;
  ReconstructionSettings *RecSet;

  //this sensor group
  int ThisPmGroup;

public slots:
  virtual void execute() = 0;
  virtual void copyLrfsAndExecute();
signals:
  void finished();
  void lrfsCopied();

protected:
  //ReconstructionManagerClass *RecManager;
  int EventsFrom, EventsTo;
  double calculateChi2NoDegFree(int iev, AReconRecord *rec);
  double calculateMLfactor(int iev, AReconRecord *rec);
};

/// Center of Gravity
class CoGReconstructorClass : public ProcessorClass
{
  Q_OBJECT
public:
  CoGReconstructorClass(pms* PMs,
                        APmGroupsManager* PMgroups,
                        ALrfModuleSelector* LRFs,
                        EventsDataClass *EventsDataHub,
                        ReconstructionSettings *RecSet,
                        int CurrentGroup,
                        int EventsFrom, int EventsTo)
    : ProcessorClass(PMs, PMgroups, LRFs, EventsDataHub, RecSet, CurrentGroup, EventsFrom, EventsTo) {}
  ~CoGReconstructorClass(){}
public slots:
  virtual void execute();
};

struct ASliceRecRecord
{

};

/// Contracting grids on CPU
class CGonCPUreconstructorClass : public ProcessorClass
{
  Q_OBJECT
public:
  CGonCPUreconstructorClass(pms* PMs,
                            APmGroupsManager* PMgroups,
                            ALrfModuleSelector* LRFs,
                            EventsDataClass *EventsDataHub,
                            ReconstructionSettings *RecSet,
                            int CurrentGroup,
                            int EventsFrom, int EventsTo)
    : ProcessorClass(PMs, PMgroups, LRFs, EventsDataHub, RecSet, CurrentGroup, EventsFrom, EventsTo) {}
  ~CGonCPUreconstructorClass(){}
public slots:
  virtual void execute();

private:
  void executeSliced3Dold();
  void oneSlice(int iev, int iSlice);

  double BestSlResult;
  double BestSlX, BestSlY, BestSlZ;
  double BestSlEnergy;
};

/// Root minimizer (Migrad2 or Simplex) for point events
class RootMinReconstructorClass : public ProcessorClass
{
  Q_OBJECT
public:
  RootMinReconstructorClass(pms* PMs,
                            APmGroupsManager* PMgroups,
                            ALrfModuleSelector* LRFs,
                            EventsDataClass *EventsDataHub,
                            ReconstructionSettings *RecSet,
                            int ThisPmGroup,
                            int EventsFrom, int EventsTo);
  ~RootMinReconstructorClass();
 //public for static use
 double LastMiniValue;
 const QVector< float >* PMsignals;
public slots:
  virtual void execute();
private:
  ROOT::Math::Functor *FunctorLSML;
  ROOT::Minuit2::Minuit2Minimizer* RootMinimizer;
};

/// Root minimizer (Migrad2 or Simplex) with double events
/// RecSet->MultipleEventOption: 0-cannot be here, 1-only doubles+chi2, 2-does doubles+chi2 then copmares with already provided rec results for singles
class RootMinDoubleReconstructorClass : public ProcessorClass
{
  Q_OBJECT
public:
  RootMinDoubleReconstructorClass(pms* PMs,
                                  APmGroupsManager* PMgroups,
                                  ALrfModuleSelector* LRFs,
                                  EventsDataClass *EventsDataHub,
                                  ReconstructionSettings *RecSet,
                                  int ThisPmGroup,
                                  int EventsFrom, int EventsTo);
  ~RootMinDoubleReconstructorClass();
 //public for static use
 double LastMiniValue;
 const QVector< float >* PMsignals;
public slots:
  virtual void execute();
private:
  ROOT::Math::Functor *FunctorLSML;
  ROOT::Minuit2::Minuit2Minimizer* RootMinimizer;
  double calculateChi2DoubleEvent(const double *result);
};

/// Claculates chi2 for all events
class Chi2calculatorClass : public ProcessorClass
{
  Q_OBJECT
public:
  Chi2calculatorClass(pms* PMs,
                      APmGroupsManager* PMgroups,
                      ALrfModuleSelector* LRFs,
                      EventsDataClass *EventsDataHub,
                      ReconstructionSettings *RecSet,
                      int CurrentGroup,
                      int EventsFrom, int EventsTo)
    : ProcessorClass(PMs, PMgroups, LRFs, EventsDataHub, RecSet, CurrentGroup, EventsFrom, EventsTo){}
  ~Chi2calculatorClass(){}
public slots:
  virtual void execute();
};

/// Checks all filters which can be multithreaded
class EventFilterClass : public ProcessorClass
{
  Q_OBJECT
public:
    EventFilterClass(pms* PMs,
                     APmGroupsManager* PMgroups,
                     ALrfModuleSelector* LRFs,
                     EventsDataClass *EventsDataHub,
                     ReconstructionSettings *RecSet,
                     AEventFilteringSettings *FiltSet,
                     int CurrentGroup,
                     int EventsFrom, int EventsTo)
        : ProcessorClass(PMs, PMgroups, LRFs, EventsDataHub, RecSet, CurrentGroup, EventsFrom, EventsTo), FiltSet(FiltSet) {}
    ~EventFilterClass(){}
public slots:
  virtual void execute();

private:
   AEventFilteringSettings* FiltSet;
};

//static functions to use with Root minimizer
double Chi2static(const double *p);
double Chi2staticDouble(const double *p);
double MLstatic(const double *p);
double MLstaticDouble(const double *p);
#endif // PROCESSORCLASS_H
