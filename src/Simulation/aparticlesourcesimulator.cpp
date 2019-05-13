#include "aparticlesourcesimulator.h"
#include "detectorclass.h"
#include "asimulationmanager.h"
#include "amaterialparticlecolection.h"
#include "eventsdataclass.h"
//#include "primaryparticletracker.h"
#include "aparticletracker.h"
#include "aphotontracer.h"
#include "aoneevent.h"
#include "s1_generator.h"
#include "s2_generator.h"
#include "afileparticlegenerator.h"
#include "asourceparticlegenerator.h"
#include "ascriptparticlegenerator.h"
#include "apositionenergyrecords.h"
#include "aglobalsettings.h"
#include "atrackingdataimporter.h"
#include "aenergydepositioncell.h"
#include "ajsontools.h"

#include <memory>
#include <algorithm>

#include <QDebug>
#include <QJsonObject>
#include <QFile>
#include <QTextStream>
#include <QProcess>

#include "TRandom2.h"
#include "TGeoManager.h"

AParticleSourceSimulator::AParticleSourceSimulator(const DetectorClass *detector, ASimulationManager *simMan, int ID) :
    ASimulator(detector, simMan, ID)
{
    if (!simMan) qDebug() << "simMan is nullptr" ;  // ***obsolete?
    detector->MpCollection->updateRandomGenForThread(ID, RandGen);

    //ParticleTracker = new PrimaryParticleTracker(detector->GeoManager, RandGen, detector->MpCollection, &ParticleStack, &EnergyVector, &dataHub->EventHistory, dataHub->SimStat, ID);
    ParticleTracker = new AParticleTracker(*RandGen, *detector->MpCollection, ParticleStack, EnergyVector, TrackingHistory, *dataHub->SimStat, ID);
    S1generator = new S1_Generator(photonGenerator, photonTracker, detector->MpCollection, &EnergyVector, &dataHub->GeneratedPhotonsHistory, RandGen);
    S2generator = new S2_Generator(photonGenerator, photonTracker, &EnergyVector, RandGen, detector->GeoManager, detector->MpCollection, &dataHub->GeneratedPhotonsHistory);

    //ParticleSources = new ParticleSourcesClass(detector, RandGen); /too early - do not know yet the particle generator mode
}

AParticleSourceSimulator::~AParticleSourceSimulator()
{
    delete S2generator;
    delete S1generator;
    delete ParticleTracker;
    delete ParticleGun;
    clearParticleStack();
    clearGeneratedParticles(); //if something was not transferred
    clearEnergyVector();
}

bool AParticleSourceSimulator::setup(QJsonObject &json)
{
    if ( !ASimulator::setup(json) ) return false;

    //prepare this module
    timeFrom = simSettings->TimeFrom;
    timeRange = simSettings->fTimeResolved ? (simSettings->TimeTo - simSettings->TimeFrom) : 0;

    //extracting generation configuration
    if (!json.contains("ParticleSourcesConfig"))
    {
        ErrorString = "Json sent to simulator does not contain particle sim config data!";
        return false;
    }

    QJsonObject js = json["ParticleSourcesConfig"].toObject();
        //control options
        QJsonObject cjs = js["SourceControlOptions"].toObject();
        if (cjs.isEmpty())
        {
            ErrorString = "Json sent to simulator does not contain proper sim config data!";
            return false;
        }
        totalEventCount = cjs["EventsToDo"].toInt();
        fAllowMultiple = false; //only applies to 'Sources' mode
        fDoS1 = cjs["DoS1"].toBool();
        fDoS2 = cjs["DoS2"].toBool();
        fBuildParticleTracks = simSettings->TrackBuildOptions.bBuildParticleTracks;
        fIgnoreNoHitsEvents = false; //compatibility
        parseJson(cjs, "IgnoreNoHitsEvents", fIgnoreNoHitsEvents);
        fIgnoreNoDepoEvents = true; //compatibility
        parseJson(cjs, "IgnoreNoDepoEvents", fIgnoreNoDepoEvents);
        ClusterMergeRadius2 = 1.0;
        parseJson(cjs, "ClusterMergeRadius", ClusterMergeRadius2);
        ClusterMergeRadius2 *= ClusterMergeRadius2;

        // particle generation mode
        QString PartGenMode = "Sources"; //compatibility
        parseJson(cjs, "ParticleGenerationMode", PartGenMode);

        if (PartGenMode == "Sources")
        {
            // particle sources
            if (js.contains("ParticleSources"))
            {
                ParticleGun = new ASourceParticleGenerator(detector, RandGen);
                ParticleGun->readFromJson(js);

                fAllowMultiple = cjs["AllowMultipleParticles"].toBool();
                AverageNumParticlesPerEvent = cjs["AverageParticlesPerEvent"].toDouble();
                TypeParticlesPerEvent = cjs["TypeParticlesPerEvent"].toInt();
            }
            else
            {
                ErrorString = "Simulation settings do not contain particle source configuration";
                return false;
            }
        }
        else if (PartGenMode == "File")
        {
            //  generation from file
            QJsonObject fjs;
            parseJson(js, "GenerationFromFile", fjs);
            if (fjs.isEmpty())
            {
                ErrorString = "Simulation settings do not contain 'from file' generator configuration";
                return false;
            }
            else
            {
                ParticleGun = new AFileParticleGenerator(*detector->MpCollection);
                ParticleGun->readFromJson(fjs);
            }
        }
        else if (PartGenMode == "Script")
        {
            // script based generator
            //ErrorString = "Script particle generator is not yet implemented";
            //return false;
            QJsonObject sjs;
            parseJson(js, "GenerationFromScript", sjs);
            if (sjs.isEmpty())
            {
                ErrorString = "Simulation settings do not contain 'from script' generator configuration";
                return false;
            }
            else
            {
                ParticleGun = new AScriptParticleGenerator(*detector->MpCollection, RandGen);
                ParticleGun->readFromJson(sjs);
            }
        }
        else
        {
            ErrorString = "Load sim settings: Unknown particle generation mode!";
            return false;
        }

        // trying to initialize the gun
        if ( !ParticleGun->Init() )
        {
            ErrorString = ParticleGun->GetErrorString();
            return false;
        }

    //update config according to the selected generation mode
    if (PartGenMode == "File") totalEventCount = static_cast<AFileParticleGenerator*>(ParticleGun)->NumEventsInFile;
    //inits
    ParticleTracker->configure(simSettings, fBuildParticleTracks, &tracks, fIgnoreNoDepoEvents);
    ParticleTracker->resetCounter();
    S1generator->setDoTextLog(simSettings->fLogsStat);
    S2generator->setDoTextLog(simSettings->fLogsStat);
    S2generator->setOnlySecondary(!fDoS1);

    return true;
}

void AParticleSourceSimulator::updateGeoManager()
{
    //ParticleTracker->UpdateGeoManager(detector->GeoManager);
    S2generator->UpdateGeoManager(detector->GeoManager);
}

void AParticleSourceSimulator::simulate()
{
    if ( !ParticleStack.isEmpty() ) clearParticleStack();

    ReserveSpace(getEventCount());
    if (ParticleGun) ParticleGun->SetStartEvent(eventBegin);
    fStopRequested = false;
    fHardAbortWasTriggered = false;

    std::unique_ptr<QFile> pFile;
    std::unique_ptr<QTextStream> pStream;
    if (simSettings->G4SimSet.bTrackParticles)
    {
        const QString name = simSettings->G4SimSet.getPrimariesFileName(ID);//   FilePath + QString("primaries-%1.txt").arg(ID);
        pFile.reset(new QFile(name));
        if(!pFile->open(QIODevice::WriteOnly | QFile::Text))
        {
            ErrorString = QString("Cannot open file: %1").arg(name);
            fSuccess = false;
            return;
        }
        pStream.reset(new QTextStream(&(*pFile)));
    }

    // -- Main simulation cycle --
    updateFactor = 100.0 / ( eventEnd - eventBegin );
    for (eventCurrent = eventBegin; eventCurrent < eventEnd; eventCurrent++)
    {
        if (fStopRequested) break;
        if (!EnergyVector.isEmpty()) clearEnergyVector();

        int numPrimaries = chooseNumberOfParticlesThisEvent();
            //qDebug() << "---- primary particles in this event: " << numPrimaries;
        bool bGenerationSuccessful = choosePrimariesForThisEvent(numPrimaries);
        if (!bGenerationSuccessful) break; //e.g. in file gen -> end of file reached

        //event prepared
            //qDebug() << "Event composition ready!  Particle stack length:" << ParticleStack.size();

        if (!simSettings->G4SimSet.bTrackParticles)
            ParticleTracker->TrackParticlesOnStack(eventCurrent);
        else
        {
            //prepare file with primaries for export to Geant4 particle tracker
            *pStream << QString("#%1\n").arg(eventCurrent);
            for (const AParticleRecord* p : ParticleStack)
            {
                QString t = QString::number(p->Id);
                t += QString(" %1").arg(p->energy);
                t += QString(" %1 %2 %3").arg(p->r[0]).arg(p->r[1]).arg(p->r[2]);
                t += QString(" %1 %2 %3").arg(p->v[0]).arg(p->v[1]).arg(p->v[2]);
                t += QString(" %1").arg(p->time);
                t += "\n";
                *pStream << t;
                delete p;
            }
            ParticleStack.clear();

            if (bOnlySavePrimariesToFile)
                progress = (eventCurrent - eventBegin + 1) * updateFactor;

            continue; //skip tracking and go to the next event
        }

        //-- only local tracking ends here --
        //energy vector is ready

        if ( fIgnoreNoDepoEvents && EnergyVector.isEmpty() ) //if there is no deposition can ignore this event
        {
            eventCurrent--;
            continue;
        }

        bool bOK = generateAndTrackPhotons();
        if (!bOK)
        {
            fSuccess = false;
            return;
        }

        if ( fIgnoreNoHitsEvents && OneEvent->isHitsEmpty() ) // if there were no PM hits can ignore this event
        {
            eventCurrent--;
            continue;
        }

        if (!simSettings->fLRFsim) OneEvent->HitsToSignal();

        dataHub->Events.append(OneEvent->PMsignals);
        if (timeRange != 0) dataHub->TimedEvents.append(OneEvent->TimedPMsignals);

        EnergyVectorToScan(); //prepare true position data using deposition data

        progress = (eventCurrent - eventBegin + 1) * updateFactor;
    }

    if (ParticleGun) ParticleGun->ReleaseResources();
    if (simSettings->G4SimSet.bTrackParticles)
    {
        pStream->flush();
        pFile->close();
    }

    if (simSettings->G4SimSet.bTrackParticles && !bOnlySavePrimariesToFile)
    {
        // track primaries in Geant4, read deposition, generate photons, trace photons, process hits
        bool bOK = geant4TrackAndProcess();
        if (!bOK)
        {
            fSuccess = false;
            return;
        }
    }

    fSuccess = !fHardAbortWasTriggered;
    //  qDebug() << "done!"<<ID << "events collected:"<<dataHub->countEvents();
}

void AParticleSourceSimulator::appendToDataHub(EventsDataClass *dataHub)
{
    //qDebug() << "Thread #"<<ID << " PartSim ---> appending data";
    ASimulator::appendToDataHub(dataHub);
    int oldSize = dataHub->EventHistory.count();
    dataHub->EventHistory.reserve(oldSize + this->dataHub->EventHistory.count());
    for(int i = 0; i < this->dataHub->EventHistory.count(); i++)
    {
        EventHistoryStructure *hist = new EventHistoryStructure;
        *hist = *this->dataHub->EventHistory[i];
        hist->index += oldSize;
        if (hist->SecondaryOf > -1) hist->SecondaryOf += oldSize;
        dataHub->EventHistory.append(hist);
    }
    dataHub->GeneratedPhotonsHistory << this->dataHub->GeneratedPhotonsHistory;
    dataHub->ScanNumberOfRuns = 1;
}

void AParticleSourceSimulator::mergeData()
{
    simMan->SeenNonRegisteredParticles += SeenNonRegisteredParticles;
    SeenNonRegisteredParticles.clear();

    simMan->DepoByNotRegistered += DepoByNotRegistered;
    DepoByNotRegistered = 0;

    simMan->DepoByRegistered += DepoByRegistered;
    DepoByRegistered = 0;

    simMan->TrackingHistory.insert(
          simMan->TrackingHistory.end(),
          std::make_move_iterator(TrackingHistory.begin()),
          std::make_move_iterator(TrackingHistory.end())
        );
    TrackingHistory.clear();
}

void AParticleSourceSimulator::hardAbort()
{
    //qDebug() << "HARD abort"<<ID;
    ASimulator::hardAbort();

    ParticleGun->abort();
    if (bG4isRunning && G4antsProcess) G4antsProcess->kill();
}

void AParticleSourceSimulator::updateMaxTracks(int maxPhotonTracks, int maxParticleTracks)
{
    ParticleTracker->setMaxTracks(maxParticleTracks);
    photonTracker->setMaxTracks(maxPhotonTracks);
}

void AParticleSourceSimulator::EnergyVectorToScan()
{
    if (EnergyVector.isEmpty())
    {
        AScanRecord* scs = new AScanRecord();
        scs->Points.Reinitialize(0);
        scs->ScintType = 0;
        dataHub->Scan.append(scs);
        return;
    }

    //== new system ==
    AScanRecord* scs = new AScanRecord();
    scs->ScintType = 0;

    if (EnergyVector.size() == 1) //want to have the case of 1 fast - in many modes this will be the only outcome
    {
        for (int i=0; i<3; i++) scs->Points[0].r[i] = EnergyVector[0]->r[i];
        scs->Points[0].energy = EnergyVector[0]->dE;
    }
    else
    {
        QVector<APositionEnergyRecord> Depo(EnergyVector.size()); // TODO : make property of PSS
        for (int i=0; i<3; i++) Depo[0].r[i] = EnergyVector[0]->r[i];
        Depo[0].energy = EnergyVector[0]->dE;

        //first pass is copy from EnergyVector (since cannot modify EnergyVector itself),
        //if next point is within cluster range, merge with previous point
        //if outside, start with a new cluster
        int iPoint = 0;  //merging to this point
        int numMerges = 0;
        for (int iCell = 1; iCell < EnergyVector.size(); iCell++)
        {
            const AEnergyDepositionCell * EVcell = EnergyVector.at(iCell);
            APositionEnergyRecord & Point = Depo[iPoint];
            if (EVcell->isCloser(ClusterMergeRadius2, Point.r))
            {
                Point.MergeWith(EVcell->r, EVcell->dE);
                numMerges++;
            }
            else
            {
                //start the next cluster
                iPoint++;
                for (int i=0; i<3; i++) Depo[iPoint].r[i] = EVcell->r[i];
                Depo[iPoint].energy = EVcell->dE;
            }
        }

        //direct pass finished, if there were no merges, work is done
            //qDebug() << "-----Merges in the first pass:"<<numMerges;
        if (numMerges > 0)
        {
            Depo.resize(EnergyVector.size() - numMerges); //only shrinking
            // pass for all clusters -> not needed for charged, but it is likely needed for Geant4 import depo data
            int iThisCluster = 0;
            while (iThisCluster < Depo.size()-1)
            {
                int iOtherCluster = iThisCluster + 1;
                while (iOtherCluster < Depo.size())
                {
                    if (Depo[iThisCluster].isCloser(ClusterMergeRadius2, Depo[iOtherCluster]))
                    {
                        Depo[iThisCluster].MergeWith(Depo[iOtherCluster]);
                        Depo.removeAt(iOtherCluster);
                    }
                    else iOtherCluster++;
                }
                iThisCluster++;
            }

            // in principle, after merging some clusters can become within merging range
            // to make the procedure complete, the top while cycle have to be restarted until number of merges is 0
            // but it will slow down without much effect - skip for now
        }

        //sort (make it optional?)
        std::stable_sort(Depo.rbegin(), Depo.rend()); //reverse order

        scs->Points.Reinitialize(Depo.size());
        for (int iDe=0; iDe<Depo.size(); iDe++)
            scs->Points[iDe] = Depo[iDe];
    }

    dataHub->Scan.append(scs);

    /*
    //-- old system --
    int PreviousIndex = EnergyVector.first()->index; //particle number - not to be confused with ParticleId!!!
    bool fTrackStarted = false;
    double EnergyAccumulator = 0;

    AScanRecord* scs = new AScanRecord();
    for (int i=0; i<3; i++) scs->Points[0].r[i] = EnergyVector[0]->r[i];
    scs->ScintType = 0; //unknown - could be that both primary and secondary were activated!
    scs->Points[0].energy = EnergyVector[0]->dE;

    if (EnergyVector.size() == 1)
    {
        dataHub->Scan.append(scs);
        return;
    }

    for (int ip=1; ip<EnergyVector.size(); ip++) //protected against EnegyVector with size 1
    {
        int ThisIndex = EnergyVector[ip]->index;
        if (ThisIndex == PreviousIndex)
        {
            //continuing the track
            fTrackStarted = true; //nothing to do until the track ends
            EnergyAccumulator += EnergyVector[ip-1]->dE; //last cell energy added to accumulator
        }
        else
        {
            //another particle starts here
            if (ip == 0)
            {
                //already filled scs data for the very first cell - does not matter it was track or single point
                PreviousIndex = ThisIndex;
                continue;
            }
            //track teminated last cell?
            if (fTrackStarted)
            {
                //finishing the track of the previous particle
                EnergyAccumulator += EnergyVector[ip-1]->dE;
                scs->Points.AddPoint( EnergyVector[ip-1]->r, EnergyAccumulator );
                fTrackStarted = false;
            }

            //adding this point - same for both the track (start point) or point deposition
            scs->Points.AddPoint( EnergyVector[ip]->r, EnergyVector[ip]->dE); //protected against double filling of ip=0
        }
        PreviousIndex = ThisIndex;
    }

    if (fTrackStarted)
    {
        //if EnergyVector has finished with continuous deposition, the track is not finished yet
        EnergyAccumulator += EnergyVector.last()->dE;
        scs->Points.AddPoint(EnergyVector.last()->r, EnergyAccumulator);
    }

    dataHub->Scan.append(scs);
    */
}

void AParticleSourceSimulator::clearParticleStack()
{
    for (int i=0; i<ParticleStack.size(); i++) delete ParticleStack[i];
    ParticleStack.clear();
}

void AParticleSourceSimulator::clearEnergyVector()
{
    for (int i = 0; i < EnergyVector.size(); i++) delete EnergyVector[i];
    EnergyVector.clear();
}

void AParticleSourceSimulator::clearGeneratedParticles()
{
    for (AParticleRecord * p : GeneratedParticles) delete p;
    GeneratedParticles.clear();
}

int AParticleSourceSimulator::chooseNumberOfParticlesThisEvent() const
{
    if (!fAllowMultiple) return 1;

    if (TypeParticlesPerEvent == 0)
        return std::round(AverageNumParticlesPerEvent);
    else
    {
        int num = RandGen->Poisson(AverageNumParticlesPerEvent);
        return std::max(1, num);
    }
}

bool AParticleSourceSimulator::choosePrimariesForThisEvent(int numPrimaries)
{
    for (int iPart = 0; iPart < numPrimaries; iPart++)
    {
        if (!ParticleGun) break; //paranoic
        //generating one event
        bool bGenerationSuccessful = ParticleGun->GenerateEvent(GeneratedParticles);
        //  qDebug() << "thr--->"<<ID << "ev:" << eventBegin <<"P:"<<iRun << "  =====> " << bGenerationSuccessful << GeneratedParticles.size();
        if (!bGenerationSuccessful) return false;

        //adding particles to the stack
        for (AParticleRecord * p : GeneratedParticles)
            ParticleStack << p;

        GeneratedParticles.clear(); //do not delete particles - they were transferred to the ParticleStack!
    }
    return true;
}

bool AParticleSourceSimulator::generateAndTrackPhotons()
{
    OneEvent->clearHits();

    if (fDoS1 && !S1generator->Generate())
    {
        ErrorString = "Error executing S1 generation!";
        return false;
    }

    if (fDoS2 && !S2generator->Generate())
    {
        ErrorString = "Error executing S2 generation!";
        return false;
    }

    return true;
}

bool AParticleSourceSimulator::geant4TrackAndProcess()
{
    const QString exe = AGlobalSettings::getInstance().G4antsExec;
    const QString confFile = simSettings->G4SimSet.getConfigFileName(ID); // FilePath + QString("aga-%1.json").arg(ID);

    QStringList ar;
    ar << confFile;

    G4antsProcess = new QProcess();
    //QObject::connect(G4antsProcess, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), [&isRunning](){isRunning = false; qDebug() << "----FINISHED!-----";});//this, &MainWindow::on_cameraControlExit);
    bG4isRunning = true;
        G4antsProcess->start(exe, ar);
        bool bStartedOK = G4antsProcess->waitForStarted(1000);
        if (bStartedOK) G4antsProcess->waitForFinished(-1);
    bG4isRunning = false;

    QString err = G4antsProcess->errorString();

    if (!bStartedOK)
    {
        ErrorString = "Failed to start G4ants executable\n" + err;
        return false;
    }

    QProcess::ExitStatus exitStat = G4antsProcess->exitStatus();

    //qDebug() << "g4---->"<<err<<(int)exitStat;

    delete G4antsProcess; G4antsProcess = 0;
    if (fHardAbortWasTriggered) return false;

    if (err.contains("No such file or directory"))
    {
        ErrorString = "Cannot find G4ants executable";
        return false;
    }
    if (exitStat == QProcess::CrashExit)
    {
        ErrorString = "G4ants executable crashed:\nCheck that it was compiled with correct environment variables.\nDo you use the correct version of Geant4?";
        return false;
    }

    // read receipt file, stop if not "OK"
    QString receipeFileName = simSettings->G4SimSet.getReceitFileName(ID); // FilePath + QString("receipt-%1.txt").arg(ID);
    QJsonObject jrec;
    bool bOK = LoadJsonFromFile(jrec, receipeFileName);
    //qDebug() << jrec;
    if (!bOK)
    {
        ErrorString = "Could not read the receipt file";
        return false;
    }
    if (!jrec.contains("Success"))
    {
        ErrorString = "Unknown format of receipt file";
        return false;
    }
    bool bStatus = jrec["Success"].toBool();
    if (!bStatus)
    {
        parseJson(jrec, "Error", ErrorString);
        return false;
    }

    // for warning generation
    DepoByRegistered = 0;
    parseJson(jrec, "DepoByRegistered", DepoByRegistered);
    DepoByNotRegistered = 0;
    parseJson(jrec, "DepoByNotRegistered", DepoByNotRegistered);
    QJsonArray arNP;
    parseJson(jrec, "SeenNotRegisteredParticles", arNP);
    SeenNonRegisteredParticles.clear();
    for (int i=0; i<arNP.size(); i++)
        SeenNonRegisteredParticles.insert(arNP.at(i).toString());

    //read and process depo data
    QString DepoFileName = simSettings->G4SimSet.getDepositionFileName(ID); // FilePath + QString("deposition-%1.txt").arg(ID);
    QFile inFile(DepoFileName);
    if(!inFile.open(QIODevice::ReadOnly | QFile::Text))
    {
        ErrorString = "Cannot open file with energy deposition data:\n" + DepoFileName;
        return false;
    }

    QTextStream in(&inFile);

    QString line = in.readLine();
    for (eventCurrent = eventBegin; eventCurrent < eventEnd; eventCurrent++)
    {
        if (fStopRequested) break;
        if (EnergyVector.size() > 0) clearEnergyVector();

            //qDebug() << ID << " CurEv:"<<eventCurrent <<" -> " << line;
        if (!line.startsWith('#') || line.size() < 2)
        {
            ErrorString = "Format error for #event field in file:\n" + DepoFileName;
            return false;
        }
        line.remove(0, 1);
        if (line.toInt() != eventCurrent)
        {
            ErrorString = "Missmatch of event number in file:\n" + DepoFileName;
            return false;
        }

        do
        {
            line = in.readLine();
            if (line.startsWith('#'))
                break; //next event

            //qDebug() << ID << "->"<<line;
            //pId mId dE x y z t
            // 0   1   2 3 4 5 6     // pId can be = -1 !
            //populating energy vector data
            QStringList fields = line.split(' ', QString::SkipEmptyParts);
            if (fields.isEmpty()) break; //last event had no depo - end of file reached
            if (fields.size() < 7)
            {
                ErrorString = "Format error in file:\n" + DepoFileName;
                return false;
            }
            AEnergyDepositionCell* cell = new AEnergyDepositionCell(fields[3].toDouble(), fields[4].toDouble(), fields[5].toDouble(), //x y z
                                                                    0, fields[6].toDouble(), fields[2].toDouble(),  // length time dE
                                                                    fields[0].toInt(), fields[1].toInt(), 0, eventCurrent); //part mat sernum event
            EnergyVector << cell;
        }
        while (!in.atEnd());

        //Energy vector is filled for this event
            //qDebug() << "Energy vector contains" << EnergyVector.size() << "cells";

        bool bOK = generateAndTrackPhotons();
        if (!bOK) return false;

        if (!simSettings->fLRFsim) OneEvent->HitsToSignal();

        dataHub->Events.append(OneEvent->PMsignals);
        if (timeRange != 0) dataHub->TimedEvents.append(OneEvent->TimedPMsignals);

        EnergyVectorToScan(); // TODO another version of conversion have to be written ***!!!

        progress = (eventCurrent - eventBegin + 1) * updateFactor;
    }

    inFile.close();


    //read tracks
    if (simSettings->TrackBuildOptions.bBuildParticleTracks || simSettings->fLogsStat)
    {
        ATrackingDataImporter ti(simSettings->TrackBuildOptions,
                                 detector->MpCollection->getListOfParticleNames(),
                                 (simSettings->fLogsStat ? &TrackingHistory : nullptr),
                                 (simSettings->TrackBuildOptions.bBuildParticleTracks ? &tracks : nullptr),
                                 maxParticleTracks);
        QString TrackingFileName = simSettings->G4SimSet.getTracksFileName(ID);
        ErrorString = ti.processFile(TrackingFileName, eventBegin);
        if (!ErrorString.isEmpty()) return false;
    }

    return true;
}
