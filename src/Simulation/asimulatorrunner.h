#ifndef ASIMULATORRUNNER_H
#define ASIMULATORRUNNER_H

#include <QObject>
#include <QVector>
#include <QTime>
#include <QString>

#include "RVersion.h"

class DetectorClass;
class EventsDataClass;
class ASimulationManager;
class ASimulator;

#if ROOT_VERSION_CODE < ROOT_VERSION(6,11,1)
    class TThread;
#else
    namespace std { class thread;}
#endif

class ASimulatorRunner : public QObject
{
    Q_OBJECT
public:
    enum State { SClean, SSetup, SRunning, SFinished };

    ASimulatorRunner(DetectorClass & detector, EventsDataClass & dataHub, ASimulationManager & simMan);
    virtual ~ASimulatorRunner();

    bool setup(int threadCount, bool bPhotonSourceSim); // if bPhotonSourceSim==false -> it is particle source sim

    bool isStoppedByUser() const { return fStopRequested; }
    void updateStats();
    //double getProgress() const { return progress; }
    bool wasSuccessful() const;
    bool wasHardAborted() const;
    //bool isFinished() const {return simState == SFinished;}
    void setFinished() {simState = SFinished;}    
    QVector<ASimulator *> getWorkers() { return workers; }
    State getSimState() const {return simState;}
    void clearWorkers();

private:
    DetectorClass      & detector;
    EventsDataClass    & dataHub;
    ASimulationManager & simMan;

    enum State simState;
    bool fStopRequested;

    //Threads
    QVector<ASimulator *> workers;
#if ROOT_VERSION_CODE < ROOT_VERSION(6,11,1)
    QVector<TThread *> threads;
#else
    QVector<std::thread *> threads;
#endif
    ASimulator * backgroundWorker; // obsolete?

    //Time
    QTime startTime;
    int lastRefreshTime;
    int totalEventCount;
    int lastProgress;
    int lastEventsDone;

public:
    //Stats for gui report
    double progress;
    double usPerEvent;

public slots:
    void simulate();
    void requestStop();

signals:
    void simulationFinished();
};

#endif // ASIMULATORRUNNER_H
