#ifndef ASIMULATORRUNNER_H
#define ASIMULATORRUNNER_H

#include "generalsimsettings.h" //to be moved

#include <QObject>
#include <QTime>
#include <QString>

class DetectorClass;
class EventsDataClass;
class ASimulationManager;
class ASimulator;

class QJsonObject; // move?

#include "RVersion.h"

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
    QString modeSetup;
    GeneralSimSettings simSettings; //to move to simManager

    ASimulatorRunner(DetectorClass & detector, EventsDataClass & dataHub, ASimulationManager & simMan);
    virtual ~ASimulatorRunner();

    bool setup(QJsonObject & json, int threadCount); // main processing of the configuration is here

    void updateGeoManager();
    bool isStoppedByUser() const { return fStopRequested; }
    void updateStats();
    double getProgress() const { return progress; }
    bool wasSuccessful() const;
    bool wasHardAborted() const;
    bool isFinished() const {return simState == SFinished;}
    void setFinished() {simState = SFinished;}
    //Use as read-only. Anything else is undefined behaviour! If your toast gets burnt, it's not my fault!
    //Also remember that simulators will be deleted on setup()!
    QVector<ASimulator *> getWorkers() { return workers; }
    State getSimState() const {return simState;}

    //Use this with caution too, e.g. after simulation finished! We need to expose this to clear memory after simulation,
    //since we provide extra information of simulators to the outside (and it's actually used). In a way this is a hack
    //that should be improved, maybe through external container (EventDataHub is not enough or suitable for this)
    void clearWorkers();

    void setG4Sim() {bNextSimExternal = true;}
    void setG4Sim_OnlyGenerateFiles(bool flag) {bOnlyFileExport = flag;}

private:
    DetectorClass      & detector;
    EventsDataClass    & dataHub;
    ASimulationManager & simMan;

    //Manager's state
    enum State simState;
    bool fStopRequested;
    //bool bRunFromGui;

    //Threads
    QVector<ASimulator *> workers;
#if ROOT_VERSION_CODE < ROOT_VERSION(6,11,1)
    QVector<TThread *> threads;
#else
    QVector<std::thread *> threads;
#endif
    ASimulator * backgroundWorker;



    //Time
    QTime startTime;
    int lastRefreshTime;
    int totalEventCount;
    int lastProgress;
    int lastEventsDone;


    bool bNextSimExternal = false;
    bool bOnlyFileExport = false;

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
