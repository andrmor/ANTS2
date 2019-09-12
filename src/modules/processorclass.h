#ifndef PROCESSORCLASS_H
#define PROCESSORCLASS_H

#include <QObject>
#include "alrfmoduleselector.h"

class ReconstructionSettings;
class APmHub;
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
  ProcessorClass(APmHub* PMs,
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
  APmHub* PMs;
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
  int EventsFrom, EventsTo;
  double calculateChi2NoDegFree(int iev, AReconRecord *rec);
  double calculateMLfactor(int iev, AReconRecord *rec);
};

// ------ Center of Gravity ------
class CoGReconstructorClass : public ProcessorClass
{
  Q_OBJECT
public:
  CoGReconstructorClass(APmHub* PMs,
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

// ------ Contracting grids on CPU ------
class CGonCPUreconstructorClass : public ProcessorClass
{
  Q_OBJECT
public:
  CGonCPUreconstructorClass(APmHub* PMs,
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

// ------ Root minimizer - single point events ------
class RootMinReconstructorClass : public ProcessorClass
{
    Q_OBJECT
public:
    RootMinReconstructorClass(APmHub* PMs,
                              APmGroupsManager* PMgroups,
                              ALrfModuleSelector* LRFs,
                              EventsDataClass *EventsDataHub,
                              ReconstructionSettings *RecSet,
                              int ThisPmGroup,
                              int EventsFrom, int EventsTo);
    ~RootMinReconstructorClass();

    double LastMiniValue;
    const QVector< float >* PMsignals;

public slots:
    virtual void execute();

protected:
    ROOT::Math::Functor *FunctorLSML;
    ROOT::Minuit2::Minuit2Minimizer* RootMinimizer;
};

// ------ Root minimizer for single point events with possibility that some events have known range in X or Y ------
class RootMinRangedReconstructorClass : public ProcessorClass
{
  Q_OBJECT
public:
    RootMinRangedReconstructorClass(APmHub* PMs,
                                    APmGroupsManager* PMgroups,
                                    ALrfModuleSelector* LRFs,
                                    EventsDataClass *EventsDataHub,
                                    ReconstructionSettings *RecSet,
                                    int ThisPmGroup,
                                    int EventsFrom, int EventsTo,
                                    double Range, bool UseGauss);
    ~RootMinRangedReconstructorClass();

    double LastMiniValue;
    const QVector< float >* PMsignals;

    double Range; // minimization will be within +-range around the true/scan value

    //internal - used by the minimizer in Gaussian mode
    bool bUseGauss;
    bool RangedX, RangedY;
    double CenterX, CenterY;

public slots:
    virtual void execute();

protected:
    ROOT::Math::Functor *FunctorLSML;
    ROOT::Minuit2::Minuit2Minimizer* RootMinimizer;
};

// ------ Root minimizer with double events ------
// RecSet->MultipleEventOption: 0-cannot be here, 1-only doubles+chi2, 2-does doubles+chi2 then copmares with already provided rec results for singles
class RootMinDoubleReconstructorClass : public ProcessorClass
{
  Q_OBJECT
public:
  RootMinDoubleReconstructorClass(APmHub* PMs,
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

// ------ Claculates chi2 for all events ------
class Chi2calculatorClass : public ProcessorClass
{
  Q_OBJECT
public:
  Chi2calculatorClass(APmHub* PMs,
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

// ------ Checks all filters which can be multithreaded ------
class EventFilterClass : public ProcessorClass
{
  Q_OBJECT
public:
    EventFilterClass(APmHub* PMs,
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

    //double Chi2static(const double *p);
class AFunc_Chi2
{
public:
    AFunc_Chi2(RootMinReconstructorClass * Reconstructor) : Reconstructor(Reconstructor) {}
    double operator()(const double *p);
private:
    RootMinReconstructorClass * Reconstructor = nullptr;
};
    //double Chi2staticGauss(const double *p);
class AFunc_Chi2range
{
public:
    AFunc_Chi2range(RootMinRangedReconstructorClass * Reconstructor) : Reconstructor(Reconstructor) {}
    double operator()(const double *p);
private:
    RootMinRangedReconstructorClass * Reconstructor = nullptr;
};
    //double Chi2staticDouble(const double *p);
class AFunc_Chi2double
{
public:
    AFunc_Chi2double(RootMinDoubleReconstructorClass * Reconstructor) : Reconstructor(Reconstructor) {}
    double operator()(const double *p);
private:
    RootMinDoubleReconstructorClass * Reconstructor = nullptr;
};
    //double MLstatic(const double *p);
class AFunc_ML
{
public:
    AFunc_ML(RootMinReconstructorClass * Reconstructor) : Reconstructor(Reconstructor) {}
    double operator()(const double *p);
private:
    RootMinReconstructorClass * Reconstructor = nullptr;
};
    //double MLstaticGauss(const double *p);
class AFunc_MLrange
{
public:
    AFunc_MLrange(RootMinRangedReconstructorClass * Reconstructor) : Reconstructor(Reconstructor) {}
    double operator()(const double *p);
private:
    RootMinRangedReconstructorClass * Reconstructor = nullptr;
};
    //double MLstaticDouble(const double *p);
class AFunc_MLdouble
{
public:
    AFunc_MLdouble(RootMinDoubleReconstructorClass * Reconstructor) : Reconstructor(Reconstructor) {}
    double operator()(const double *p);
private:
    RootMinDoubleReconstructorClass * Reconstructor = nullptr;
};

#endif // PROCESSORCLASS_H
