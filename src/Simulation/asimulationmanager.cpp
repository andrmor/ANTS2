#include "asimulationmanager.h"
#include "eventsdataclass.h"
#include "detectorclass.h"
#include "asourceparticlegenerator.h"
#include "afileparticlegenerator.h"
#include "ascriptparticlegenerator.h"
#include "asimulatorrunner.h"
#include "aenergydepositioncell.h"
#include "atrackrecords.h"
#include "anoderecord.h"
#include "apointsourcesimulator.h"
#include "aparticlesourcesimulator.h"
#include "aoneevent.h"

#include <QDebug>
#include <QJsonObject>
#include <QStringList>
#include <QFile>
#include <QTextStream>

ASimulationManager::ASimulationManager(EventsDataClass & EventsDataHub, DetectorClass & Detector) :
    QObject(nullptr),
    EventsDataHub(EventsDataHub), Detector(Detector)
{
    ParticleSources = new ASourceParticleGenerator(&Detector, Detector.RandGen);
    FileParticleGenerator = new AFileParticleGenerator(*Detector.MpCollection);
    ScriptParticleGenerator = new AScriptParticleGenerator(*Detector.MpCollection, Detector.RandGen);
    ScriptParticleGenerator->SetProcessInterval(200);

    Runner = new ASimulatorRunner(Detector, EventsDataHub, *this);

    QObject::connect(Runner, &ASimulatorRunner::simulationFinished, this, &ASimulationManager::onSimulationFinished);
    QObject::connect(this, &ASimulationManager::RequestStopSimulation, Runner, &ASimulatorRunner::requestStop, Qt::DirectConnection);

    Runner->moveToThread(&simRunnerThread); //Move to background thread, as it always runs synchronously even in MT
    QObject::connect(&simRunnerThread, &QThread::started, Runner, &ASimulatorRunner::simulate); //Connect thread to simulation start

    //GUI and websocket update
    simTimerGuiUpdate.setInterval(1000);
    QObject::connect(&simTimerGuiUpdate, &QTimer::timeout, this, &ASimulationManager::updateGui, Qt::DirectConnection);
}

ASimulationManager::~ASimulationManager()
{
    delete Runner;

    clearEnergyVector();
    clearTracks();
    clearNodes();

    delete ScriptParticleGenerator;
    delete FileParticleGenerator;
    delete ParticleSources;
}

bool ASimulationManager::isSimulationAborted() const
{
    return Runner->isStoppedByUser();
}

void ASimulationManager::StartSimulation(QJsonObject& json, int threads, bool fFromGui)
{
    fFinished = false;
    fSuccess = false;
    fStartedFromGui = fFromGui;

    if (MaxThreads > 0 && threads > MaxThreads)
    {
        qDebug() << "Simulation manager: Enforcing max threads to " << MaxThreads;
        threads = MaxThreads;
    }

    bool bOK = Runner->setup(json, threads);
    if (!bOK)
    {
        QTimer::singleShot(100, this, &ASimulationManager::onSimFailedToStart); // timer is to allow the start cycle to finish in the main thread
    }
    else
    {
        simRunnerThread.start();
        if (fStartedFromGui) simTimerGuiUpdate.start();
    }
}



void ASimulationManager::onSimFailedToStart()
{
    fFinished = true;
    fSuccess = false;
    Runner->setFinished();

    emit SimulationFinished();
}

void ASimulationManager::onSimulationFinished()
{
    //  qDebug() << "Simulation manager: Simulation finished";

    //qDebug() << "after finish, manager has monitors:"<< EventsDataHub->SimStat->Monitors.size();
    simTimerGuiUpdate.stop();
    QThread::usleep(5000); // to make sure update cycle is finished
    simRunnerThread.quit();

    fFinished = true;
    fSuccess = Runner->wasSuccessful();
    fHardAborted = Runner->wasHardAborted();

    //clear data containers -> they will be loaded with new data from workers
    clearEnergyVector();
    SiPMpixels.clear();
    clearTracks();

    if (Runner->modeSetup == "PointSim") LastSimType = 0; //was Point sources sim
    else if (Runner->modeSetup == "SourceSim") LastSimType = 1; //was Particle sources sim
    else LastSimType = -1;

    QVector<ASimulator *> simulators = Runner->getWorkers();
    if (!simulators.isEmpty() && !fHardAborted)
    {
        if (LastSimType == 0)
        {
            APointSourceSimulator *lastPointSrcSimulator = static_cast< APointSourceSimulator *>(simulators.last());
            EventsDataHub.ScanNumberOfRuns = lastPointSrcSimulator->getNumRuns();

            SiPMpixels = lastPointSrcSimulator->getLastEvent()->SiPMpixels; //only makes sense if there was only 1 event
        }
        else if (LastSimType == 1)
        {
            AParticleSourceSimulator *lastPartSrcSimulator = static_cast< AParticleSourceSimulator *>(simulators.last());
            EnergyVector = lastPartSrcSimulator->getEnergyVector();
            lastPartSrcSimulator->ClearEnergyVectorButKeepObjects(); // to avoid clearing the energy vector cells
        }

        for (ASimulator * sim : simulators)
        {
            //Tracks += sim->tracks;
            Tracks.insert( Tracks.end(),
                           std::make_move_iterator(sim->tracks.begin()),
                           std::make_move_iterator(sim->tracks.end()) );
            sim->tracks.clear();  //to avoid delete objects on simulator delete
        }
        while (Tracks.size() > Runner->simSettings.TrackBuildOptions.MaxParticleTracks)
        {
            if (Tracks.empty()) break;
            delete Tracks.at(Tracks.size()-1);
            Tracks.resize(Tracks.size()-1); // max number of resizes will be small (~ num treads)
        }
    }

    //Raimundo's comment: This is part of a hack. Check this function declaration in .h for more details.
    Runner->clearWorkers();

    if (fHardAborted)
        EventsDataHub.clear(); //data are not valid!
    else
    {
        //before was limited to the mode whenlocations are limited to a certain object in the detector geometry
        //scan and custom nodes might have nodes outside of the limiting object - they are still present but marked with 1e10 X and Y true coordinates
        EventsDataHub.purge1e10events(); //purging events with "true" positions x==1e10 && y==1e10
    }

    Detector.BuildDetector(true, true);  // <- still needed on Windows
    //Detector->GeoManager->CleanGarbage();

    if (fStartedFromGui) emit SimulationFinished();

    clearEnergyVector(); // main window copied if needed
    SiPMpixels.clear();  // main window copied if needed
    //qDebug() << "SimManager: Sim finished";
}


void ASimulationManager::clearTracks()
{
    for (auto * tr : Tracks) delete tr;
    Tracks.clear();
}

void ASimulationManager::clearNodes()
{
    for (auto * n : Nodes) delete n;
    Nodes.clear();
}

void ASimulationManager::clearEnergyVector()
{
    for (int i=0; i<EnergyVector.size(); i++) delete EnergyVector[i];
    EnergyVector.clear();
}

const QString ASimulationManager::loadNodesFromFile(const QString &fileName)
{
    clearNodes();

    QFile file(fileName);
    if(!file.open(QIODevice::ReadOnly | QFile::Text))
        return "Failed to open file "+fileName;

    QTextStream in(&file);
    ANodeRecord * prevNode = nullptr;
    bool bAppendThis = false;

    while (!in.atEnd())
    {
        QString line = in.readLine();
        if (line.startsWith('#')) continue; //comment
        QStringList sl = line.split(' ', QString::SkipEmptyParts);
        if (sl.isEmpty()) continue; //alow empty lines
        // x y z time num *
        // 0 1 2   3   4  last
        int size = sl.size();
        if (size < 3) return "Unexpected format of line: cannot find x y z t information:\n" + line;

        bool bAppendNext = false; // bAppendNext will be in effect for the next node, not this!
        if (sl.last() == '*')
        {
            bAppendNext = true;
            sl.removeLast();
        }

        bool bOK;
        double x = sl.at(0).toDouble(&bOK); if (!bOK) return "Bad format of line: conversion to number failed:\n" + line;
        double y = sl.at(1).toDouble(&bOK); if (!bOK) return "Bad format of line: conversion to number failed:\n" + line;
        double z = sl.at(2).toDouble(&bOK); if (!bOK) return "Bad format of line: conversion to number failed:\n" + line;

        double t = 0;
        if (sl.size() > 3)
        {
            t = sl.at(3).toDouble(&bOK);
            if (!bOK) return "Bad format of line: conversion to number failed:\n" + line;
        }

        int n = -1;
        if (sl.size() > 4)
        {
            n = sl.at(4).toInt(&bOK);
            if (!bOK) return "Bad format of line: conversion to int failed:\n" + line;
        }

        ANodeRecord * node = ANodeRecord::createS(x, y, z, t, n);
        if (bAppendThis) prevNode->addLinkedNode(node);
        else Nodes.push_back(node);
        prevNode = node;
        bAppendThis = bAppendNext;
    }

    file.close();
    return "";
}

void ASimulationManager::StopSimulation()
{
    emit RequestStopSimulation();
}

void ASimulationManager::updateGui()
{
    switch (Runner->getSimState())
    {
    case ASimulatorRunner::SClean:
    case ASimulatorRunner::SSetup:
        return;
    case ASimulatorRunner::SRunning:
        Runner->updateStats();
        emit updateReady(Runner->progress, Runner->usPerEvent);
        emit ProgressReport(Runner->progress);
        break;
    case ASimulatorRunner::SFinished:
        qDebug()<<"Simulation has emitted finish, but updateGui() is still being called";
    }
}
