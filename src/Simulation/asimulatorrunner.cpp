#include "asimulatorrunner.h"
#include "detectorclass.h"
#include "eventsdataclass.h"
#include "asimulationmanager.h"
#include "asimulator.h"
#include "amaterialparticlecolection.h"
#include "asandwich.h"
#include "apmhub.h"
#include "apointsourcesimulator.h"
#include "aparticlesourcesimulator.h"

#include <QDebug>
#include <QJsonObject>  //move?

#include "TRandom2.h"
#include "TGeoManager.h"

#if ROOT_VERSION_CODE < ROOT_VERSION(6,11,1)
#include "TThread.h"
#else
#include <thread>
#endif

//static method to give to TThread or std::thread which will run simulation
//Needs to return something, otherwise root always starts detached thread
static void *SimulationManagerRunWorker(void *workerSimulator)
{
    if (!workerSimulator)
    {
        qDebug()<<"ThreadCall: workerSimulatorIsNull!!";
        return NULL;
    }
    ASimulator *simulator = (ASimulator *)workerSimulator;
    //QString nameID = simulator->getNameId();
    //qDebug() << nameID<<"Thread has a list of navigators?"<<simulator->getDetector()->GeoManager->GetListOfNavigators();  /// at this point the list pointer is NULL
    //if (simulator->getDetector()->GeoManager->GetListOfNavigators())
    //  qDebug() << nameID<<"|||||||||||| already defined navigators:"<<simulator->getDetector()->GeoManager->GetListOfNavigators()->GetEntries();

    //qDebug() << nameID<<"Navigator added";
    //qDebug() << nameID<<"List?"<< simulator->getDetector()->GeoManager->GetListOfNavigators();  /// List contains one navigator now
    //qDebug() << nameID<<"entries?"<<simulator->getDetector()->GeoManager->GetListOfNavigators()->GetEntries();
    simulator->simulate();
    //simulator->getDetector()->GeoManager->RemoveNavigator(navigator);
    //qDebug() << nameID<<"after sim list:"<<simulator->getDetector()->GeoManager->GetListOfNavigators();  /// at this point list is NULL again
    //if (simulator->getDetector()->GeoManager->GetListOfNavigators())
    //  qDebug() << nameID<<"after sim entries: "<< simulator->getDetector()->GeoManager->GetListOfNavigators()->GetEntries();
    return NULL;
}

ASimulatorRunner::ASimulatorRunner(DetectorClass & detector, EventsDataClass & dataHub, ASimulationManager & simMan) :
    QObject(nullptr),
    detector(detector), dataHub(dataHub), simMan(simMan)
{
    startTime.start(); //can restart be done without start() ?
    simState = SClean;
}

ASimulatorRunner::~ASimulatorRunner()
{
    clearWorkers();
    for(int i = 0; i < threads.count(); i++)
        delete threads[i];
    threads.clear();
}

bool ASimulatorRunner::setup(int threadCount, bool bPhotonSourceSim)
{
    totalEventCount = 0;
    lastProgress = 0;
    lastEventsDone = 0;
    lastRefreshTime = 0;
    usPerEvent = 0;
    fStopRequested = false;

    clearWorkers();

    const QString err = detector.PMs->checkBeforeSimulation();
    if (!err.isEmpty())
    {
        simMan.setErrorString(err);
        return false;
    }

    for (int iWorker = 0; iWorker < threadCount; iWorker++)
    {
        ASimulator *worker;
        if (bPhotonSourceSim) worker = new APointSourceSimulator(simMan, iWorker);
        else                  worker = new AParticleSourceSimulator(simMan, iWorker);

        bool bOK = worker->setup(simMan.jsSimSet);
        if (!bOK)
        {
            simMan.setErrorString( worker->getErrorString() );
            delete worker;
            clearWorkers();
            return false;
        }
        worker->initSimStat();

        worker->divideThreadWork(iWorker, threadCount);
        int workerEventCount = worker->getEventCount();
        if (workerEventCount == 0 && iWorker != 0) // require at least 1 worker, even if no work assigned
        {
            delete worker;
            break;
        }
        totalEventCount += worker->getEventCount();

        bOK = worker->finalizeConfig();
        if (!bOK)
        {
            simMan.setErrorString( worker->getErrorString() );
            delete worker;
            clearWorkers();
            return false;
        }

        workers.append(worker);
    }

    simMan.NumberOfWorkers = workers.size();  //used by particle sources - script gen

    if (threadCount > 1)
    {
        backgroundWorker = nullptr;
        detector.GeoManager->SetMaxThreads(workers.count());
#if ROOT_VERSION_CODE < ROOT_VERSION(6,11,1)
        while(workers.count() > threads.count())
            threads.append(new TThread(&SimulationManagerRunWorker));
#else
        for (int i=0; i<workers.size(); i++)
            threads.append(new std::thread(&SimulationManagerRunWorker, workers[i]));
#endif
    }
    else backgroundWorker = workers.last();

    simState = SSetup;
    return true;
}

void ASimulatorRunner::updateStats()
{
    int refreshTimeElapsed = startTime.elapsed();
    progress = 0;
    int eventsDone = 0;
    for(int i = 0; i < workers.count(); i++)
    {
        int progressThread = workers[i]->progress;
        int events = workers[i]->getEventsDone();
        eventsDone += events;
        progress += progressThread;
    }
    progress /= workers.count();

    int deltaEventsDone = eventsDone - lastEventsDone;
    double deltaRefreshTime = refreshTimeElapsed - lastRefreshTime;

    if (deltaEventsDone>0 && deltaRefreshTime>0)
    {
        usPerEvent = deltaRefreshTime / (double)deltaEventsDone;
        lastRefreshTime = refreshTimeElapsed;
        lastEventsDone = eventsDone;
        lastProgress = progress;
    }
}

bool ASimulatorRunner::wasSuccessful() const
{
    if(fStopRequested || simState != SFinished) return false;
    bool fOK = true;
    for(int i = 0; i < workers.count(); i++)
        fOK &= workers[i]->wasSuccessful();
    return fOK;
}

bool ASimulatorRunner::wasHardAborted() const
{
    for(const ASimulator * sim : workers)
        if (sim->wasHardAborted()) return true;
    return false;
}

void ASimulatorRunner::clearWorkers()
{
    for(int i = 0; i < workers.count(); i++)
        delete workers[i];
    workers.clear();
}

void ASimulatorRunner::simulate()
{
    switch (simState)
    {
    case SClean:
    case SFinished:
        qWarning()<<"SimulationManager::simulate() called without proper setup!";
        return;
    case SSetup:
        //qDebug()<<"Starting simulation!";
        break;
    case SRunning:
        qWarning()<<"Simulation already running!";
        return;
    }

    progress = 0;
    usPerEvent = 0;
    simState = SRunning;
    emit simMan.updateReady(0, 0); // set progress to 0 in GUI

    startTime.restart();

    if (backgroundWorker)
        backgroundWorker->simulate();
    else
    {
#if ROOT_VERSION_CODE < ROOT_VERSION(6,11,1)
        for(int i = 0; i < workers.count(); i++)
        {
            //qDebug()<<"Launching thread"<<(i+1);
            threads[i]->Run(workers[i]);
        }
        //QThread::usleep(100000);
        for(int i = 0; i < workers.count(); i++)
        {

            //  qDebug()<<"Asking thread"<<(i+1)<<" to join...";
            threads[i]->Join();
            //  qDebug() <<"Thread"<<(i+1)<<"joined";
        }
#else
        //std::thread -> start on construction
        for (int i=0; i < workers.count(); i++)
        {
            //qDebug() << "Asking thread"<<(i+1)<<" to join...";
            threads[i]->join();
            //qDebug() <<"   Thread"<<(i+1)<<"joined";
        }
#endif
        //qDebug() << "SimRunner: all threads joined";

        for (int i=0; i<threads.size(); i++) delete threads[i];
        threads.clear();
    }

    simState = SFinished;
    progress = 100.0;
    usPerEvent = startTime.elapsed() / (double)totalEventCount;
    emit simMan.updateReady(100.0, usPerEvent);

    emit simulationFinished();
}

void ASimulatorRunner::requestStop()
{
    this->fStopRequested = true;
    for(int i = 0; i < workers.count(); i++)
        workers[i]->requestStop();
}
