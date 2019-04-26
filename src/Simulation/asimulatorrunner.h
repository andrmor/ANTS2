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
    enum State { SClean, SSetup, SRunning, SFinished/*, SStopRequest*/ };
    QString modeSetup;
    GeneralSimSettings simSettings; //to move to simManager

    explicit ASimulatorRunner(DetectorClass * detector, EventsDataClass * dataHub, ASimulationManager * simMan, QObject * parent = 0);
    virtual ~ASimulatorRunner();

    bool setup(QJsonObject & json, int threadCount, bool bFromGui); // main processing of the configuration is here

    void updateGeoManager();
    bool getStoppedByUser() const { return fStopRequested; /*simState == SStopRequest;*/ }
    void updateStats();
    double getProgress() const { return progress; }
    bool wasSuccessful() const;
    bool wasHardAborted() const;
    bool isFinished() const {return simState == SFinished;}
    void setFinished() {simState = SFinished;}
    QString getErrorMessages() const;
    //Use as read-only. Anything else is undefined behaviour! If your toast gets burnt, it's not my fault!
    //Also remember that simulators will be deleted on setup()!
    QVector<ASimulator *> getWorkers() { return workers; }

    //Use this with caution too, e.g. after simulation finished! We need to expose this to clear memory after simulation,
    //since we provide extra information of simulators to the outside (and it's actually used). In a way this is a hack
    //that should be improved, maybe through external container (EventDataHub is not enough or suitable for this)
    void clearWorkers();

    void setG4Sim() {bNextSimExternal = true;}
    void setG4Sim_OnlyGenerateFiles(bool flag) {bOnlyFileExport = flag;}

private:
    //Manager's state
    enum State simState;
    bool fStopRequested;
    bool bRunFromGui;

    //Threads
    QVector<ASimulator *> workers;
#if ROOT_VERSION_CODE < ROOT_VERSION(6,11,1)
    QVector<TThread *> threads;
#else
    QVector<std::thread *> threads;
#endif
    ASimulator * backgroundWorker;

    //External settings
    DetectorClass * detector = 0;
    EventsDataClass * dataHub = 0;
    ASimulationManager * simMan = 0;

    //Time
    QTime startTime;
    int lastRefreshTime;
    int totalEventCount;
    int lastProgress;
    int lastEventsDone;

    //Stats
    double progress;
    double usPerEvent;

    QString ErrorString;

    bool bNextSimExternal = false;
    bool bOnlyFileExport = false;

public slots:
    void simulate();
    void requestStop();
    void updateGui();

signals:
    void updateReady(int Progress, double msPerEvent);
    void simulationFinished();
};

#endif // ASIMULATORRUNNER_H
