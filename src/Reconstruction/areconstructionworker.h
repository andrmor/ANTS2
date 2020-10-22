#ifndef PROCESSORCLASS_H
#define PROCESSORCLASS_H

#include <QObject>
#include "afunctorbase.h"

class ReconstructionSettings;
class APmHub;
class APmGroupsManager;
class SensorLRFs;
class DynamicPassivesHandler;
class EventsDataClass;
struct AReconRecord;
class AEventFilteringSettings;
namespace ROOT { namespace Minuit2 { class Minuit2Minimizer; } }
namespace ROOT { namespace Math { class Functor; } }

class AReconstructionWorker : public QObject
{
    Q_OBJECT
public:
    AReconstructionWorker(APmHub* PMs,
                          APmGroupsManager* PMgroups,
                          SensorLRFs *LRFs,
                          EventsDataClass *EventsDataHub,
                          ReconstructionSettings *RecSet,
                          int ThisPmGroup,
                          int EventsFrom, int EventsTo);
    virtual ~AReconstructionWorker();

    // thread control - consider change to atomic
    char Progress = 0;
    int eventsProcessed = 0;
    bool fFinished = false;
    int Id;
    bool fStopRequested = false;

    // local objects used by minimizer
    DynamicPassivesHandler * DynamicPassives = nullptr;

    // external objects used by minimizer
    APmHub                 * PMs = nullptr;
    APmGroupsManager       * PMgroups = nullptr;
    SensorLRFs             * LRFs;
    EventsDataClass        * EventsDataHub = nullptr;
    ReconstructionSettings * RecSet = nullptr;

    //this sensor group
    int ThisPmGroup;

public slots:
    virtual void execute() = 0;

signals:
    void finished();

protected:
    int EventsFrom;
    int EventsTo;

protected:
    double calculateChi2NoDegFree(int iev, AReconRecord *rec);
    double calculateMLfactor(int iev, AReconRecord *rec);
};

// ------ Center of Gravity ------
class CoGReconstructorClass : public AReconstructionWorker
{
  Q_OBJECT
public:
  CoGReconstructorClass(APmHub* PMs,
                        APmGroupsManager* PMgroups,
                        SensorLRFs * LRFs,
                        EventsDataClass *EventsDataHub,
                        ReconstructionSettings *RecSet,
                        int CurrentGroup,
                        int EventsFrom, int EventsTo)
    : AReconstructionWorker(PMs, PMgroups, LRFs, EventsDataHub, RecSet, CurrentGroup, EventsFrom, EventsTo) {}
  ~CoGReconstructorClass(){}
public slots:
  virtual void execute();
};

// ------ Contracting grids on CPU ------
class CGonCPUreconstructorClass : public AReconstructionWorker
{
  Q_OBJECT
public:
  CGonCPUreconstructorClass(APmHub* PMs,
                            APmGroupsManager* PMgroups,
                            SensorLRFs * LRFs,
                            EventsDataClass *EventsDataHub,
                            ReconstructionSettings *RecSet,
                            int CurrentGroup,
                            int EventsFrom, int EventsTo)
    : AReconstructionWorker(PMs, PMgroups, LRFs, EventsDataHub, RecSet, CurrentGroup, EventsFrom, EventsTo) {}
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
class RootMinReconstructorClass : public AReconstructionWorker
{
    Q_OBJECT
public:
    RootMinReconstructorClass(APmHub* PMs,
                              APmGroupsManager* PMgroups,
                              SensorLRFs *LRFs,
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
    ROOT::Math::Functor *FunctorLSML = nullptr;
    ROOT::Minuit2::Minuit2Minimizer* RootMinimizer = nullptr;
    AFunctorBase * Func = nullptr;
};

// ------ Root minimizer with double events ------
// RecSet->MultipleEventOption: 0-cannot be here, 1-only doubles+chi2, 2-does doubles+chi2 then copmares with already provided rec results for singles
class RootMinDoubleReconstructorClass : public AReconstructionWorker
{
    Q_OBJECT
public:
    RootMinDoubleReconstructorClass(APmHub* PMs,
                                    APmGroupsManager* PMgroups,
                                    SensorLRFs *LRFs,
                                    EventsDataClass *EventsDataHub,
                                    ReconstructionSettings *RecSet,
                                    int ThisPmGroup,
                                    int EventsFrom, int EventsTo);
    ~RootMinDoubleReconstructorClass();

    double LastMiniValue;
    const QVector< float > * PMsignals;

public slots:
    virtual void execute();

private:
    ROOT::Math::Functor * FunctorLSML = nullptr;
    ROOT::Minuit2::Minuit2Minimizer * RootMinimizer = nullptr;
    AFunctorBase * Func = nullptr;

private:
    double calculateChi2DoubleEvent(const double *result);
};

// ------ Claculates chi2 for all events ------
class Chi2calculatorClass : public AReconstructionWorker
{
  Q_OBJECT
public:
  Chi2calculatorClass(APmHub* PMs,
                      APmGroupsManager* PMgroups,
                      SensorLRFs* LRFs,
                      EventsDataClass *EventsDataHub,
                      ReconstructionSettings *RecSet,
                      int CurrentGroup,
                      int EventsFrom, int EventsTo)
    : AReconstructionWorker(PMs, PMgroups, LRFs, EventsDataHub, RecSet, CurrentGroup, EventsFrom, EventsTo){}
  ~Chi2calculatorClass(){}
public slots:
  virtual void execute();
};

// ------ Checks all filters which can be multithreaded ------
class EventFilterClass : public AReconstructionWorker
{
  Q_OBJECT
public:
    EventFilterClass(APmHub* PMs,
                     APmGroupsManager* PMgroups,
                     SensorLRFs* LRFs,
                     EventsDataClass *EventsDataHub,
                     ReconstructionSettings *RecSet,
                     AEventFilteringSettings *FiltSet,
                     int CurrentGroup,
                     int EventsFrom, int EventsTo)
        : AReconstructionWorker(PMs, PMgroups, LRFs, EventsDataHub, RecSet, CurrentGroup, EventsFrom, EventsTo), FiltSet(FiltSet) {}
    ~EventFilterClass(){}
public slots:
  virtual void execute();

private:
   AEventFilteringSettings* FiltSet;
};
    //double Chi2static(const double *p);
class AFunc_Chi2 : public AFunctorBase
{
public:
    AFunc_Chi2(RootMinReconstructorClass * Reconstructor) : Reconstructor(Reconstructor) {}
    double operator()(const double *p);
private:
    RootMinReconstructorClass * Reconstructor = nullptr;
};
    //double Chi2staticDouble(const double *p);
class AFunc_Chi2double : public AFunctorBase
{
public:
    AFunc_Chi2double(RootMinDoubleReconstructorClass * Reconstructor) : Reconstructor(Reconstructor) {}
    double operator()(const double *p);
private:
    RootMinDoubleReconstructorClass * Reconstructor = nullptr;
};
    //double MLstatic(const double *p);
class AFunc_ML : public AFunctorBase
{
public:
    AFunc_ML(RootMinReconstructorClass * Reconstructor) : Reconstructor(Reconstructor) {}
    double operator()(const double *p);
private:
    RootMinReconstructorClass * Reconstructor = nullptr;
};
    //double MLstaticDouble(const double *p);
class AFunc_MLdouble : public AFunctorBase
{
public:
    AFunc_MLdouble(RootMinDoubleReconstructorClass * Reconstructor) : Reconstructor(Reconstructor) {}
    double operator()(const double *p);
private:
    RootMinDoubleReconstructorClass * Reconstructor = nullptr;
};

class TFormula;
class AFunc_TFormula : public AFunctorBase
{
public:
    AFunc_TFormula(RootMinReconstructorClass * Reconstructor) : Reconstructor(Reconstructor) {}
    ~AFunc_TFormula();
    double operator()(const double *p);
    bool parse(const QString & formula);
private:
    RootMinReconstructorClass * Reconstructor = nullptr;
    TFormula * tform = nullptr;
};

#endif // PROCESSORCLASS_H
