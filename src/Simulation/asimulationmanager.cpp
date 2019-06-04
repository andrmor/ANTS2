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
#include "amaterialparticlecolection.h"
#include "aeventtrackingrecord.h"
#include "asandwich.h"
#include "apmhub.h"

#include <QDebug>
#include <QJsonObject>
#include <QStringList>
#include <QFile>
#include <QTextStream>

#include "TRandom2.h"

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
    clearTrackingHistory();

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

    threads = std::max(threads, 1); // TODO check we still can run in "0" threads

    if (MaxThreads > 0 && threads > MaxThreads)
    {
        qDebug() << "Simulation manager: Enforcing max threads to " << MaxThreads;
        threads = MaxThreads;
    }

    // reading simulation settings
    bool     bOK = setup(json, threads);
    if (bOK) bOK = Runner->setup(threads, bPhotonSourceSim);

    if (!bOK)
        QTimer::singleShot(100, this, &ASimulationManager::onSimFailedToStart); // timer is to allow the start cycle to finish in the main thread
    else
    {
        simRunnerThread.start();
        if (fStartedFromGui) simTimerGuiUpdate.start();
    }
}

bool ASimulationManager::setup(const QJsonObject & json, int threads)
{
    if ( !json.contains("SimulationConfig") )
    {
        ErrorString = "Json does not contain simulation config!";
        return false;
    }

    jsSimSet = json["SimulationConfig"].toObject();
    if ( !simSettings.readFromJson(jsSimSet) )
    {
        ErrorString = simSettings.ErrorString;
        return false;
    }

    ErrorString = Detector.MpCollection->CheckOverrides();
    if (!ErrorString.isEmpty()) return false;

    QString mode = jsSimSet["Mode"].toString();
    bPhotonSourceSim = ( mode == "PointSim" );

    // handling of custom nodes
    if (bPhotonSourceSim)
    {
        const QJsonObject psc = jsSimSet["PointSourcesConfig"].toObject();
        int simMode = psc["ControlOptions"].toObject()["Single_Scan_Flood"].toInt();
        if (simMode != 4) clearNodes(); // script will have the nodes already defined
        if (simMode == 3)
        {
            QString fileName = psc["CustomNodesOptions"].toObject()["FileWithNodes"].toString();
            const QString err = loadNodesFromFile(fileName);
            if (!err.isEmpty())
            {
                ErrorString = err;
                return false;
            }
            //qDebug() << "Custom nodes loaded from files, top nodes:"<<simMan->Nodes.size();
        }
    }
    else // particle source sim
    {
        if (simSettings.G4SimSet.bTrackParticles)
        {
            const QString gdmlName = simSettings.G4SimSet.getGdmlFileName();
            QString err = Detector.exportToGDML(gdmlName);
            if ( !err.isEmpty() )
            {
                ErrorString = err;
                return false;
            }
        }

#ifndef __USE_ANTS_NCRYSTAL__
        if (Detector.MpCollection->isNCrystalInUse())
        {
            ErrorString = "NCrystal library is in use by material collection, but ANTS2 was compiled without this library!";
            return false;
        }
#endif
    }

    EventsDataHub.clear();
    EventsDataHub.initializeSimStat(Detector.Sandwich->MonitorsRecords, simSettings.DetStatNumBins, (simSettings.fWaveResolved ? simSettings.WaveNodes : 0) );

    Detector.PMs->configure(&simSettings); //Setup pms module and QEaccelerator if needed
    Detector.MpCollection->UpdateRuntimePropertiesAndWavelengthBinning(&simSettings, Detector.RandGen, threads); //update wave-resolved properties of materials and runtime properties for neutrons

    return true;
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

    EventsDataHub.fSimulatedData = true;
    EventsDataHub.LastSimSet = simSettings;

    copyDataFromWorkers();

    Runner->clearWorkers();

    if (fHardAborted)
        EventsDataHub.clear(); //data are not valid!
    else
    {
        // scan and custom nodes might have nodes outside of the limiting object -
        // they are still present but marked with 1e10 X and Y true coordinates
        EventsDataHub.purge1e10events(); //purging events with "true" positions x==1e10 && y==1e10
    }

    bGuardTrackingHistory = true;
    Detector.BuildDetector(true, true);  // <- still needed on Windows
    bGuardTrackingHistory = false;

    //Detector.GeoManager->CleanGarbage();

    if (fStartedFromGui) emit SimulationFinished();

    SiPMpixels.clear();  // main window copied if needed
    //qDebug() << "SimManager: Sim finished";
}

void ASimulationManager::clearG4data()
{
    SeenNonRegisteredParticles.clear();
    DepoByNotRegistered = 0;
    DepoByRegistered = 0;
    clearTrackingHistory();
}

void ASimulationManager::copyDataFromWorkers()
{
    //Merging data from the workers
    QVector<ASimulator *> workers = Runner->getWorkers();

    clearG4data();
    clearTracks();
    for (int i = 0; i < workers.count(); i++)
    {
        workers[i]->appendToDataHub(&EventsDataHub); //EventsDataHub should be already cleared in setup
        workers[i]->mergeData();

        QString err = workers.at(i)->getErrorString();
        if (!err.isEmpty()) ErrorString += QString("Thread %1 reported error: %2\n").arg(i).arg(err);

        Tracks.insert( Tracks.end(),
                       std::make_move_iterator(workers[i]->tracks.begin()),
                       std::make_move_iterator(workers[i]->tracks.end()) );
        workers[i]->tracks.clear();  //to avoid delete objects on simulator delete
    }

    SiPMpixels.clear();
    clearEnergyVector();
    if (!workers.isEmpty() && !fHardAborted)
    {
        if (bPhotonSourceSim)
        {
            APointSourceSimulator *lastPointSrcSimulator = static_cast< APointSourceSimulator *>(workers.last());
            EventsDataHub.ScanNumberOfRuns = lastPointSrcSimulator->getNumRuns();
            SiPMpixels = lastPointSrcSimulator->getLastEvent()->SiPMpixels; //only the last event!
        }
        else
        {
            AParticleSourceSimulator *lastPartSrcSimulator = static_cast< AParticleSourceSimulator *>(workers.last());
            EnergyVector = lastPartSrcSimulator->getEnergyVector();
            lastPartSrcSimulator->ClearEnergyVectorButKeepObjects(); // to avoid clearing the energy vector cells
        }
    }
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

void ASimulationManager::clearTrackingHistory()
{
    if (bGuardTrackingHistory) return;
    for (AEventTrackingRecord * r : TrackingHistory) delete r;
    TrackingHistory.clear();
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

void ASimulationManager::onNewGeoManager(TObject *)
{
    clearTrackingHistory();
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

void ASimulationManager::removeOldFile(const QString & fileName, const QString & txt)
{
    QFile f(fileName);
    if (f.exists())
    {
        //qDebug() << "Removing old file with" << txt << ":" << fileName;
        bool bOK = f.remove();
        if (!bOK) qWarning() << "Was unable to remove old file with" << txt << ":" << fileName;
    }
}

void ASimulationManager::generateG4antsConfigCommon(QJsonObject & json, int ThreadId)
{
    const AG4SimulationSettings & G4SimSet = simSettings.G4SimSet;
    const AMaterialParticleCollection & MpCollection = *Detector.MpCollection;

    json["PhysicsList"] = G4SimSet.PhysicsList;

    json["LogHistory"] = simSettings.fLogsStat;

    QJsonArray Parr;
    const int numPart = MpCollection.countParticles();
    for (int iP=0; iP<numPart; iP++)
    {
        const AParticle * part = MpCollection.getParticle(iP);
        if (part->isIon())
        {
            QJsonArray ar;
            ar << part->ParticleName << part->ionZ << part->ionA;
            Parr << ar;
        }
        else Parr << part->ParticleName;
    }
    json["Particles"] = Parr;

    const QStringList Materials = MpCollection.getListOfMaterialNames();
    QJsonArray Marr;
    for (auto & mname : Materials ) Marr << mname;
    json["Materials"] = Marr;

    QJsonArray SVarr;
    for (auto & v : G4SimSet.SensitiveVolumes ) SVarr << v;
    json["SensitiveVolumes"] = SVarr;

    json["GDML"] = G4SimSet.getGdmlFileName();

    QJsonArray arSL;
    for (auto & key : G4SimSet.StepLimits.keys())
    {
        QJsonArray el;
        el << key << G4SimSet.StepLimits.value(key);
        arSL.push_back(el);
    }
    json["StepLimits"] = arSL;

    QJsonArray Carr;
    for (auto & c : G4SimSet.Commands ) Carr << c;
    json["Commands"] = Carr;

    json["GuiMode"] = false;

    json["Seed"] = static_cast<int>(Detector.RandGen->Rndm()*10000000);

    QString primFN = G4SimSet.getPrimariesFileName(ThreadId);
    json["File_Primaries"] = primFN;
    removeOldFile(primFN, "primaries");

    QString depoFN = G4SimSet.getDepositionFileName(ThreadId);
    json["File_Deposition"] = depoFN;
    removeOldFile(depoFN, "deposition");

    QString recFN = G4SimSet.getReceitFileName(ThreadId);
    json["File_Receipt"] = recFN;
    removeOldFile(recFN, "receipt");

    QString tracFN = G4SimSet.getTracksFileName(ThreadId);
    json["File_Tracks"] = tracFN;
    removeOldFile(tracFN, "tracking");

    json["Precision"]    = G4SimSet.Precision;
}
