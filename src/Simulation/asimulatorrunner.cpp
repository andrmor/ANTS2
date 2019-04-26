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

    //simulator->getDetector()->GeoManager->AddNavigator();
    if (!simulator->getDetector()->GeoManager->GetCurrentNavigator())
    {
        //qDebug() << "No current navigator for this thread, adding one";
        simulator->getDetector()->GeoManager->AddNavigator();
    }

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

ASimulatorRunner::ASimulatorRunner(DetectorClass *detector, EventsDataClass *dataHub, ASimulationManager *simMan, QObject *parent) :
    QObject(parent), detector(detector), dataHub(dataHub), simMan(simMan)
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

bool ASimulatorRunner::setup(QJsonObject &json, int threadCount, bool bFromGui)
{
    bRunFromGui = bFromGui;
    threadCount = std::max(threadCount, 1);
    //qDebug() << "\n\n is multi?"<< detector->GeoManager->IsMultiThread();

    if (!json.contains("SimulationConfig"))
    {
        ErrorString = "Json does not contain simulation config!";
        return false;
    }
    QJsonObject jsSimSet = json["SimulationConfig"].toObject();
    totalEventCount = 0;
    lastProgress = 0;
    lastEventsDone = 0;
    lastRefreshTime = 0;
    usPerEvent = 0;
    fStopRequested = false;

    bool fRunThreads = threadCount > 0;
    bool bOK = simSettings.readFromJson(jsSimSet);
    if (!bOK)
    {
        ErrorString = simSettings.ErrorString;
        return false;
    }

    bool bGeant4ParticleSim = simSettings.G4SimSet.bTrackParticles;
    if (bNextSimExternal)
    {
        bGeant4ParticleSim = true; // override for script use
        bNextSimExternal= false; //single session trigger
    }

    modeSetup = jsSimSet["Mode"].toString();

    if (bGeant4ParticleSim)
    {
        bool bBuildTracks = simSettings.TrackBuildOptions.bBuildParticleTracks;
        bool bLogHistory = simSettings.fLogsStat;
        int  maxTracks = simSettings.TrackBuildOptions.MaxParticleTracks / threadCount + 1;

        bool bOK = detector->generateG4interfaceFiles(simSettings.G4SimSet, threadCount, bBuildTracks, bLogHistory, maxTracks);
        if (!bOK)
        {
            ErrorString = "Failed to create Ants2 <-> Geant4 interface files";
            return false;
        }
    }
    else
    {
#ifndef __USE_ANTS_NCRYSTAL__
        if ( modeSetup != "PointSim" && detector->MpCollection->isNCrystalInUse())
        {
            ErrorString = "NCrystal library is in use by material collection, but ANTS2 was compiled without this library!";
            return false;
        }
#endif
    }

    // Custom nodes - if load from file
    if (modeSetup == "PointSim")
    {
        const QJsonObject psc = jsSimSet["PointSourcesConfig"].toObject();

        int simMode = psc["ControlOptions"].toObject()["Single_Scan_Flood"].toInt();

        if (simMode != 4) simMan->clearNodes(); // script will have the nodes already defined

        if (simMode == 3)
        {
            QString fileName = psc["CustomNodesOptions"].toObject()["FileWithNodes"].toString();
            const QString err = simMan->loadNodesFromFile(fileName);
            if (!err.isEmpty())
            {
                ErrorString = err;
                return false;
            }
            //qDebug() << "Custom nodes loaded from files, top nodes:"<<simMan->Nodes.size();
        }
    }

    dataHub->clear();
    dataHub->initializeSimStat(detector->Sandwich->MonitorsRecords, simSettings.DetStatNumBins, (simSettings.fWaveResolved ? simSettings.WaveNodes : 0) );

    //qDebug() << "-------------";
    //qDebug() << "Setup to run with "<<(fRunTThreads ? "TThreads" : "QThread");
    //qDebug() << "Simulation mode:" << modeSetup;
    //qDebug() << "Monitors:"<<dataHub->SimStat->Monitors.size();

    //qDebug() << "Updating PMs module according to sim settings";
    detector->PMs->configure(&simSettings); //Setup pms module and QEaccelerator if needed
    //qDebug() << "Updating MaterialColecftion module according to sim settings";
    detector->MpCollection->UpdateRuntimePropertiesAndWavelengthBinning(&simSettings, detector->RandGen, threadCount); //update wave-resolved properties of materials and runtime properties for neutrons

    ErrorString = detector->MpCollection->CheckOverrides();
    if (!ErrorString.isEmpty()) return false;

    clearWorkers();

    for (int i = 0; i < threadCount; i++)
    {
        ASimulator *worker;
        if (modeSetup == "PointSim") //Photon simulator
            worker = new APointSourceSimulator(detector, simMan, i);
        else //Particle simulator
        {
            AParticleSourceSimulator* pss = new AParticleSourceSimulator(detector, simMan, i);
            if (bGeant4ParticleSim && bOnlyFileExport)
            {
                qDebug() << "--- only file export, external/internal sim will not be started! ---";
                pss->setOnlySavePrimaries();
            }
            worker = pss;
        }
        bOnlyFileExport = false; //single session trigger from script

        worker->setSimSettings(&simSettings);
        int seed = detector->RandGen->Rndm() * 100000;
        worker->setRngSeed(seed);
        bool bOK = worker->setup(jsSimSet);
        if (!bOK)
        {
            ErrorString = worker->getErrorString();
            delete worker;
            clearWorkers();
            return false;
        }
        worker->initSimStat();

        worker->divideThreadWork(i, threadCount);
        int workerEventCount = worker->getEventCount();
        //Let's not create more than the necessary number of workers, but require at least 1
        if (workerEventCount == 0 && i != 0)
        {
            //qDebug()<<"Worker ("<<(i+1)<<") discarded, no job assigned";
            delete worker;
            break;
        }
        totalEventCount += worker->getEventCount();
        workers.append(worker);
    }

    if(fRunThreads)
    {
        backgroundWorker = 0;
        //Assumes that detector always adds default navigator. Otherwise we need to add it ourselves!
        detector->GeoManager->SetMaxThreads(workers.count());
#if ROOT_VERSION_CODE < ROOT_VERSION(6,11,1)
        //Create any missing TThreads, they are all the same, they only differ in run() call
        while(workers.count() > threads.count())
            threads.append(new TThread(&SimulationManagerRunWorker));
#else
        for (int i=0; i<workers.size(); i++)
            threads.append(new std::thread(&SimulationManagerRunWorker, workers[i]));
#endif
    }
    else  backgroundWorker = workers.last();
    simState = SSetup;
    //qDebug() << "\n and now multi?"<< detector->GeoManager->IsMultiThread();
    //qDebug() << "Num of workers:"<< workers.size();

    return true;
}

void ASimulatorRunner::updateGeoManager()  // *** obsolete!
{
    for(int i = 0; i < workers.count(); i++)
        workers[i]->updateGeoManager();
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
        //qDebug()<<"progress["<<i+1<<"]"<<progressThread << " events done:"<<events;
        eventsDone += events;
        progress += progressThread;
    }
    progress /= workers.count();
    //int deltaProgress = progress - lastProgress;
    int deltaEventsDone = eventsDone - lastEventsDone;
    double deltaRefreshTime = refreshTimeElapsed - lastRefreshTime;
    //if (deltaProgress>0 && deltaRefreshTime>0)

    //qDebug() << "---Time passed:"<<refreshTimeElapsed<<"Events done:"<<eventsDone<< " progress:"<<progress << "deltaEventsDone:"<<deltaEventsDone << "Delta time:"<<deltaRefreshTime;

    if (deltaEventsDone>0 && deltaRefreshTime>0)
    {
        //usPerEvent = deltaRefreshTime / (deltaProgress/100.0*totalEventCount);
        usPerEvent = deltaRefreshTime / (double)deltaEventsDone;
        //qDebug()<<"-------\nTotal progress:"<<progress<<"  Current us/ev:"<<usPerEvent;
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

QString ASimulatorRunner::getErrorMessages() const
{
    QString msg;
    if(simState != SFinished)
        return "Simulation not finished yet";
    if(fStopRequested)
        return "Simulation stopped by user";
    return ErrorString;
}

void ASimulatorRunner::clearWorkers()
{
    for(int i = 0; i < workers.count(); i++)
        delete workers[i];
    workers.clear();
}

void ASimulatorRunner::simulate()
{
    switch(simState)
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
    //dataHub->clear();
    progress = 0;
    usPerEvent = 0;
    simState = SRunning;
    //qDebug()<<"Emitting update ready to reset progress";
    if (bRunFromGui) emit updateReady(0, 0);
    startTime.restart();
    if(backgroundWorker)
    {
        //qDebug()<<"Launching background worker";
        //SimulationManagerRunWorker(backgroundWorker); // *** Andr
        //qDebug() << "List of navigators:" << detector->GeoManager->GetListOfNavigators();
        //if (detector->GeoManager->GetListOfNavigators()) qDebug() << detector->GeoManager->GetListOfNavigators()->GetEntries();
        backgroundWorker->simulate();                   // *** Andr
    }
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
    progress = 100;
    usPerEvent = startTime.elapsed() / (double)totalEventCount;
    if (bRunFromGui) emit updateReady(100.0, usPerEvent);

    //Merging data from the workers
    dataHub->fSimulatedData = true;
    dataHub->LastSimSet = simSettings;
    ErrorString.clear();
    simMan->SeenNonRegisteredParticles.clear();
    simMan->DepoByNotRegistered = 0;
    simMan->DepoByRegistered = 0;
    for(int i = 0; i < workers.count(); i++)
    {
        workers[i]->appendToDataHub(dataHub);
        QString err = workers.at(i)->getErrorString();
        if (!err.isEmpty())
            ErrorString += QString("Thread %1 reported error: %2\n").arg(i).arg(err);

        simMan->SeenNonRegisteredParticles += workers[i]->SeenNonRegisteredParticles;
        simMan->DepoByNotRegistered += workers[i]->DepoByNotRegistered;
        simMan->DepoByRegistered += workers[i]->DepoByRegistered;
    }

    //qDebug()<<"Sim runner reports simulation finished!";

    //    qDebug() << "\n Pointer of the List of navigators: "<< detector->GeoManager->GetListOfNavigators();
    //    if (detector->GeoManager->IsMultiThread())
    //      {
    //        qDebug() << "We are in multithread mode";
    //        //qDebug() << "Set multithread off";
    //        //detector->GeoManager->SetMultiThread(kFALSE);
    //        qDebug() << "List of navigators:"<< detector->GeoManager->GetListOfNavigators();
    //        detector->GeoManager->ClearNavigators();
    //      }
    //    if (!detector->GeoManager->GetListOfNavigators())
    //      {
    //        qDebug() << "Adding navigator";
    //        detector->GeoManager->AddNavigator();
    //      }
    //    qDebug() << "\n navigators before leaving sim manager: "<< detector->GeoManager->GetListOfNavigators()->GetEntries();

    emit simulationFinished();
}

void ASimulatorRunner::requestStop()
{
    this->fStopRequested = true;
    for(int i = 0; i < workers.count(); i++)
        workers[i]->requestStop();
}

void ASimulatorRunner::updateGui()
{
    switch(simState)
    {
    case SClean:
    case SSetup:
        return;
    case SRunning:
        updateStats();
        emit updateReady(progress, usPerEvent);
        break;
    case SFinished:
        qDebug()<<"Simulation has emitted finish, but updateGui() is still being called";
    }
}
