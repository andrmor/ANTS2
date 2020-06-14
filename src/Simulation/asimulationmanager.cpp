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
#include "ageoobject.h"

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
    ParticleSources = new ASourceParticleGenerator(Settings.partSimSet.SourceGenSettings, Detector, *Detector.RandGen);
    FileParticleGenerator = new AFileParticleGenerator(Settings.partSimSet.FileGenSettings, *Detector.MpCollection);
    ScriptParticleGenerator = new AScriptParticleGenerator(Settings.partSimSet.ScriptGenSettings, *Detector.MpCollection, *Detector.RandGen); //, 0, &NumberOfWorkers);
    ScriptParticleGenerator->SetProcessInterval(200);

    Runner = new ASimulatorRunner(Detector, EventsDataHub, *this);

    QObject::connect(Runner, &ASimulatorRunner::simulationFinished, this, &ASimulationManager::onSimulationFinished);
    QObject::connect(this, &ASimulationManager::RequestStopSimulation, Runner, &ASimulatorRunner::requestStop, Qt::DirectConnection);

    Runner->moveToThread(&simRunnerThread);
    QObject::connect(&simRunnerThread, &QThread::started, Runner, &ASimulatorRunner::simulate);

    simTimerGuiUpdate.setInterval(1000);  //GUI and websocket server update
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
    bDoGuiUpdate = fFromGui;
    threads = std::max(threads, 1);
    if (MaxThreads > 0 && threads > MaxThreads)
    {
        qDebug() << "Simulation manager: Limiting max threads to " << MaxThreads;
        threads = MaxThreads;
    }

    if (fFromGui) Detector.clearTracks();
    else
    {
        Settings.genSimSet.TrackBuildOptions.bBuildPhotonTracks   = false;
        Settings.genSimSet.TrackBuildOptions.bBuildParticleTracks = false;
    }

    Detector.assureNavigatorPresent(); // this is only for the gui thread

    // reading simulation settings
    bool    ok = setup(json, threads);
    if (ok) ok = Runner->setup(threads, Settings.bOnlyPhotons);

    if (ok)
    {
        simRunnerThread.start();
        if (bDoGuiUpdate) simTimerGuiUpdate.start();
    }
    else
    {
        //cannot call directly: the timer is to allow the start cycle to finish in the main thread
        QTimer::singleShot(100, this, &ASimulationManager::onSimFailedToStart);
    }
}

bool ASimulationManager::setup(const QJsonObject & json, int threads)
{
    bool ok = Settings.readFromJson(json);
    if (!ok)
    {
        ErrorString = "Failed to read sim settings";
        return false;
    }

    ErrorString = Detector.MpCollection->CheckOverrides();
    if (!ErrorString.isEmpty()) return false;

    if (Settings.bOnlyPhotons)
    {
        bool ok = preparePhotonMode();
        if (!ok) return false;
    }
    else
    {
        bool ok = prepareParticleMode();
        if (!ok) return false;
    }

    EventsDataHub.clear();
    AGeneralSimSettings & GenSimSet = Settings.genSimSet;
    EventsDataHub.initializeSimStat(Detector.Sandwich->MonitorsRecords, GenSimSet.DetStatNumBins, (GenSimSet.fWaveResolved ? GenSimSet.WaveNodes : 0) );

    Detector.PMs->configure(&GenSimSet); //Setup pms module and QEaccelerator if needed
    Detector.MpCollection->UpdateRuntimePropertiesAndWavelengthBinning(&GenSimSet, Detector.RandGen, threads); //update wave-resolved properties of materials and runtime properties for neutrons

    return true;
}

bool ASimulationManager::preparePhotonMode()
{
    if (Settings.photSimSet.GenMode != APhotonSimSettings::Script) clearNodes(); // script will have the nodes already defined
    if (Settings.photSimSet.GenMode == APhotonSimSettings::File)
    {
        ErrorString = checkPnotonNodeFile(Settings.photSimSet.CustomNodeSettings.FileName);
        if (!ErrorString.isEmpty()) return false;
    }
    return true;
}

bool ASimulationManager::prepareParticleMode()
{
    const AG4SimulationSettings & G4SimSet = Settings.genSimSet.G4SimSet;

    if (Settings.partSimSet.GenerationMode == AParticleSimSettings::File)
    {
        Settings.partSimSet.FileGenSettings.ValidationMode = (G4SimSet.bTrackParticles ? AFileGenSettings::Relaxed : AFileGenSettings::Strict);
        bool bOK = FileParticleGenerator->Init();
        FileParticleGenerator->ReleaseResources();
        if (!bOK)
        {
            ErrorString = FileParticleGenerator->GetErrorString();
            return false;
        }
    }

    const ASaveParticlesToFileSettings & ExitSimSet = Settings.genSimSet.ExitParticleSettings;
    if (ExitSimSet.SaveParticles)
    {
        QFile file(ExitSimSet.FileName);
        if (!file.exists())
        {
            bool bOK = file.open(QIODevice::WriteOnly);
            file.close();
            if (!bOK)
            {
                ErrorString = "Cannot create file to save exiting particles:\n" + ExitSimSet.FileName;
                return false;
            }
        }

        if (!G4SimSet.bTrackParticles)
            Detector.assignSaveOnExitFlag(ExitSimSet.VolumeName);
    }

    if (G4SimSet.bTrackParticles)
    {
        if (!G4SimSet.checkPathValid())
        {
            ErrorString = "Exchange path does not exist.\nCheck 'Geant4' options tab in ANTS2 global settings!";
            return false;
        }

        if (!G4SimSet.checkExecutableExists())
        {
            ErrorString = "File with the name configured for G4ants executable does not exist.\nCheck 'Geant4' options tab in ANTS2 global settings!";
            return false;
        }

        if (!G4SimSet.checkExecutablePermission())
        {
            ErrorString = "File with the name configured for G4ants executable has no execution permission.\nThe file name is specified in 'Geant4' options tab in ANTS2 global settings.";
            return false;
        }

        const QStringList matNames = Detector.MpCollection->getListOfMaterialNames();
        QString matsWithSpaces;
        for (const QString & mn : matNames)
            if (mn.contains(' ')) matsWithSpaces += " " + mn;
        if (!matsWithSpaces.isEmpty())
        {
            ErrorString = "Material names cannot contain spaces!\n" + matsWithSpaces;
            return false;
        }

        QString err = G4SimSet.checkSensitiveVolumes();
        if ( !err.isEmpty() )
        {
            ErrorString = err;
            return false;
        }

        const QString gdmlName = G4SimSet.getGdmlFileName();
        err = Detector.exportToGDML(gdmlName);
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

    return true;
}

QString ASimulationManager::checkPnotonNodeFile(const QString & fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QFile::Text))
        return "Cannot open file " + fileName;

    QTextStream in(&file);
    QString line = in.readLine().simplified();
    if (!line.startsWith('#'))
    {
        Settings.photSimSet.CustomNodeSettings.Mode = APhotonSim_CustomNodeSettings::CustomNodes;
        QString err = loadNodesFromFile(fileName);
        if (!err.isEmpty())
        {
            file.close();
            return err;
        }
    }
    else
    {
        Settings.photSimSet.CustomNodeSettings.Mode = APhotonSim_CustomNodeSettings::PhotonsDirectly;
        int numEvents = 1;
        while (!in.atEnd())
        {
            QString line = in.readLine().simplified();
            if (line.startsWith('#')) numEvents++;
            else
            {
                // !*! todo check as soon as settings will be updated on change of wave binning in gui!
            }
        }
        Settings.photSimSet.CustomNodeSettings.NumEventsInFile = numEvents;
    }

    return "";
}

int ASimulationManager::getNumThreads()
{
    if (!fFinished) return NumberOfWorkers;
    else return 1;
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
    EventsDataHub.LastSimSet = Settings.genSimSet;

    copyDataFromWorkers();

    if (fHardAborted)
        EventsDataHub.clear(); //data are not valid!
    else
    {
        // scan and custom nodes might have nodes outside of the limiting object -
        // they are still present but marked with 1e10 X and Y true coordinates
        EventsDataHub.purge1e10events(); //purging events with "true" positions x==1e10 && y==1e10
    }

    //saving logs
    if ( (LogsStatOptions.bParticleTransportLog && LogsStatOptions.bSaveParticleLog) || LogsStatOptions.bSaveDepositionLog)
    {
        const QString dir = makeLogDir();
        Detector.saveCurrentConfig(QString("%1/Config.json").arg(dir));
        if (LogsStatOptions.bParticleTransportLog && LogsStatOptions.bSaveParticleLog)
            saveParticleLog(dir);
        if (LogsStatOptions.bSaveDepositionLog)
            saveDepositionLog(dir);
    }
    if (Settings.genSimSet.ExitParticleSettings.SaveParticles) saveExitLog();

    Runner->clearWorkers();


    bGuardTrackingHistory = true;
    Detector.BuildDetector(true, true);  // <- still needed on Windows
    bGuardTrackingHistory = false;

    if (bDoGuiUpdate) emit SimulationFinished();

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
    QVector<ASimulator *> & workers = Runner->getWorkers();

    clearG4data();
    clearTracks();
    for (int i = 0; i < workers.count(); i++)
    {
        workers[i]->appendToDataHub(&EventsDataHub); //EventsDataHub should be already cleared in setup

        AParticleSourceSimulator * pss = dynamic_cast<AParticleSourceSimulator *>(workers[i]);
        if (pss) pss->mergeData(SeenNonRegisteredParticles, DepoByNotRegistered, DepoByRegistered, TrackingHistory);

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
        if (Settings.bOnlyPhotons)
        {
            EventsDataHub.ScanNumberOfRuns = Settings.photSimSet.getActiveRuns();
            APointSourceSimulator *lastPointSrcSimulator = static_cast< APointSourceSimulator *>(workers.last());
            SiPMpixels = lastPointSrcSimulator->getLastEvent()->SiPMpixels; //only the last event!
        }
        else
        {
            EventsDataHub.ScanNumberOfRuns = 1;
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

QString ASimulationManager::loadNodesFromFile(const QString &fileName)
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
        QStringList sl = line.split(' ', QString::SkipEmptyParts);
        if (sl.isEmpty()) continue; //allow empty lines
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

void ASimulationManager::onNewGeoManager()
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
        emit updateReady(std::round(Runner->progress), Runner->usPerEvent, std::round(Runner->progressG4));
        emitProgressSignal();
        break;
    case ASimulatorRunner::SFinished:
        qDebug()<<"Simulation has emitted finish, but updateGui() is still being called";
    }
}

void ASimulationManager::emitProgressSignal()
{
    bool bG4sim = false;
    bool bHavePhotonSim = true;
    if ( !Settings.bOnlyPhotons )
    {
        bG4sim = Settings.genSimSet.G4SimSet.bTrackParticles;
        bHavePhotonSim = Settings.partSimSet.bDoS1 || Settings.partSimSet.bDoS2;
    }

    double PrVal;
    if (bG4sim)
    {
        if (bHavePhotonSim)
            PrVal = 0.5 * (Runner->progress + Runner->progressG4);
        else
            PrVal = Runner->progressG4;
    }
    else    PrVal = Runner->progress;

    emit ProgressReport(PrVal);
}



#include "aglobalsettings.h"
#include <QDir>
#include <QDateTime>
QString ASimulationManager::makeLogDir() const
{
    const QString subdir = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss");
    AGlobalSettings & GlobSet = AGlobalSettings::getInstance();
    const QString dirBase = QString("%1/%2").arg(GlobSet.LibLogs).arg(subdir);

    QString dir = dirBase;
    int iCounter = 1;
    while (QDir(dir).exists())
    {
        dir = QString("%1-%2").arg(dirBase).arg(iCounter);
        iCounter++;
    }
    QDir(dir).mkpath(dir);

    return dir;
}

void ASimulationManager::saveParticleLog(const QString & dir) const
{
    if (Settings.genSimSet.G4SimSet.bTrackParticles)
        saveG4ParticleLog(dir);
    else
        saveA2ParticleLog(dir);
}

void ASimulationManager::saveG4ParticleLog(const QString & dir) const
{
    QString outName = QString("%1/ParticleLog.txt").arg(dir);
    QFile fOut(outName);

    if (!fOut.open(QFile::Text | QFile::WriteOnly | QFile::Truncate))
    {
        qWarning() << "Cannot open" << outName << " file to save particle transport log";
        return;
    }

    QTextStream out(&fOut);

    int iThread = 0;
    do
    {
        const QString fileName = Settings.genSimSet.G4SimSet.getTracksFileName(iThread);
        //qDebug() << "Looking for file:"<<fileName;
        if (!QFile(fileName).exists())
        {
            //qDebug() << "No more files!";
            break;
        }

        QFile fIn(fileName);
        if (!fIn.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            qWarning() << "Cannot open" << fileName << "to import particle transport log";
            return;
        }
        QTextStream stream(&fIn);
        QString line;
        do
        {
            line = stream.readLine();
            //qDebug() << line;
            out << line << '\n';
        }
        while (!stream.atEnd());

        fIn.close();
        iThread++;
    }
    while (true);

    fOut.close();
}

void ASimulationManager::saveA2ParticleLog(const QString &) const
{
    qDebug() << "Not implemented yet!";
}

void ASimulationManager::saveDepositionLog(const QString & dir) const
{
    if (Settings.genSimSet.G4SimSet.bTrackParticles)
        saveG4depositionLog(dir);
    else
        saveA2depositionLog(dir);
}

void ASimulationManager::saveG4depositionLog(const QString &dir) const
{
    QString outName = QString("%1/DepositionLog.txt").arg(dir);
    QFile fOut(outName);

    if (!fOut.open(QFile::Text | QFile::WriteOnly | QFile::Truncate))
    {
        qWarning() << "Cannot open" << outName << " file to save deposition log";
        return;
    }

    QTextStream out(&fOut);

    int iThread = 0;
    do
    {
        const QString fileName = Settings.genSimSet.G4SimSet.getDepositionFileName(iThread);
        //qDebug() << "Looking for file:"<<fileName;
        if (!QFile(fileName).exists())
        {
            //qDebug() << "No more files!";
            break;
        }

        QFile fIn(fileName);
        if (!fIn.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            qWarning() << "Cannot open" << fileName << "to import deposition log";
            return;
        }
        QTextStream stream(&fIn);
        QString line;
        do
        {
            line = stream.readLine();
            //qDebug() << line;
            out << line << '\n';
        }
        while (!stream.atEnd());

        fIn.close();
        iThread++;
    }
    while (true);

    fOut.close();
}

void ASimulationManager::saveA2depositionLog(const QString & ) const
{
    qDebug() << "Not implemented yet!";
}

#include <iostream>
#include <fstream>
void ASimulationManager::saveExitLog()
{
    const AG4SimulationSettings & G4SimSet = Settings.genSimSet.G4SimSet;
    const ASaveParticlesToFileSettings & ExitSimSet = Settings.genSimSet.ExitParticleSettings;

    bool bFromGeant = G4SimSet.bTrackParticles;

    const int numTreads = Runner->getWorkers().size();
    if (ExitSimSet.UseBinary)
    {
        std::ofstream outStream;
        outStream.open(ExitSimSet.FileName.toLatin1().data(), std::ios::out | std::ios::binary);
        if (!outStream.is_open())
        {
            qWarning() << "Cannot open" << ExitSimSet.FileName << "to export exit particles";
            return;
        }

        for (int iThread = 0; iThread < numTreads; iThread++)
        {
            const QString fileName = ( bFromGeant ? G4SimSet.getExitParticleFileName(iThread)
                                                  : QString("%1/out-%2.dat").arg(AGlobalSettings::getInstance().TmpDir).arg(iThread) );

            std::ifstream inStream(fileName.toLatin1().data());
            if (!inStream.is_open())
            {
                qWarning() << "Cannot open" << fileName << "with exit particles for thread #" << iThread;
                return;
            }

            char ch;
            while (inStream.get(ch)) //cannot use operator>> as it swallows all new line characters (e.g. 0x20)
                outStream << ch;

            inStream.close();
        }

        outStream.close();
    }
    else
    {
        QFile fOut(ExitSimSet.FileName);

        if (!fOut.open(QFile::Text | QFile::WriteOnly | QFile::Truncate))
        {
            qWarning() << "Cannot open" << ExitSimSet.FileName << " file to save exit particles";
            return;
        }

        QTextStream out(&fOut);

        for (int iThread = 0; iThread < numTreads; iThread++)
        {
            const QString fileName = ( bFromGeant ? G4SimSet.getExitParticleFileName(iThread)
                                                  : QString("%1/out-%2.dat").arg(AGlobalSettings::getInstance().TmpDir).arg(iThread) );

            QFile fIn(fileName);
            if (!fIn.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                qWarning() << "Cannot open" << fileName << "to import deposition log";
                return;
            }
            QTextStream stream(&fIn);
            QString line;
            do
            {
                line = stream.readLine();
                out << line << '\n';
            }
            while (!stream.atEnd());

            fIn.close();
        }

        fOut.close();
    }
}
