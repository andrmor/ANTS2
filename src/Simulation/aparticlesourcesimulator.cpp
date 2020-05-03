#include "aparticlesourcesimulator.h"
#include "detectorclass.h"
#include "asimulationmanager.h"
#include "amaterialparticlecolection.h"
#include "eventsdataclass.h"
#include "aparticlerecord.h"
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
#include "aexternalprocesshandler.h"

#include <memory>
#include <algorithm>
#include <fstream>

#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QTextStream>

#include "TRandom2.h"
#include "TGeoManager.h"

AParticleSourceSimulator::AParticleSourceSimulator(ASimulationManager & simMan, int ID) :
    ASimulator(simMan, ID)
{
    detector.MpCollection->updateRandomGenForThread(ID, RandGen);

    ParticleTracker = new AParticleTracker(*RandGen, *detector.MpCollection, ParticleStack, EnergyVector, TrackingHistory, *dataHub->SimStat, ID);
    S1generator = new S1_Generator(photonGenerator, photonTracker, detector.MpCollection, &EnergyVector, &dataHub->GeneratedPhotonsHistory, RandGen);
    S2generator = new S2_Generator(photonGenerator, photonTracker, &EnergyVector, RandGen, detector.GeoManager, detector.MpCollection, &dataHub->GeneratedPhotonsHistory);
}

AParticleSourceSimulator::~AParticleSourceSimulator()
{
    delete G4handler;
    delete S2generator;
    delete S1generator;
    delete ParticleTracker;
    delete ParticleGun;
    clearParticleStack();
    clearGeneratedParticles(); //if something was not transferred
    clearEnergyVector();
}

int AParticleSourceSimulator::getEventsDone() const
{
    if (simSettings.G4SimSet.bTrackParticles)
        return 0;
    else
        return eventCurrent - eventBegin;
}

bool AParticleSourceSimulator::setup(QJsonObject &json)
{
    if ( !ASimulator::setup(json) ) return false;

    timeFrom = simSettings.TimeFrom;
    timeRange = simSettings.fTimeResolved ? (simSettings.TimeTo - simSettings.TimeFrom) : 0;

    if (!json.contains("ParticleSourcesConfig"))
    {
        ErrorString = "Simulation manager: config json does not contain particle sim data!";
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
        fBuildParticleTracks = simSettings.TrackBuildOptions.bBuildParticleTracks;
        fIgnoreNoHitsEvents = false; //compatibility
        parseJson(cjs, "IgnoreNoHitsEvents", fIgnoreNoHitsEvents);
        fIgnoreNoDepoEvents = true; //compatibility
        parseJson(cjs, "IgnoreNoDepoEvents", fIgnoreNoDepoEvents);

        bClusterMerge = true; //compatibility
        parseJson(cjs, "ClusterMerge", bClusterMerge);
        ClusterMergeRadius2 = 1.0;
        parseJson(cjs, "ClusterMergeRadius", ClusterMergeRadius2);
        ClusterMergeRadius2 *= ClusterMergeRadius2;
        ClusterMergeTimeDif = 1.0;
        parseJson(cjs, "ClusterMergeTime", ClusterMergeTimeDif);

        // particle generation mode
        QString PartGenMode = "Sources"; //compatibility
        parseJson(cjs, "ParticleGenerationMode", PartGenMode);

        if (PartGenMode == "Sources")
        {
            // particle sources
            if (js.contains("ParticleSources"))
            {
                ParticleGun = new ASourceParticleGenerator(&detector, RandGen);
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
            //  generation from ascii file
            QJsonObject fjs;
            parseJson(js, "GenerationFromFile", fjs);
            if (fjs.isEmpty())
            {
                ErrorString = "Simulation settings do not contain 'from file' generator configuration";
                return false;
            }
            else
            {
                AFileParticleGenerator * PG = new AFileParticleGenerator(*detector.MpCollection);
                PG->readFromJson(fjs);
                PG->SetValidationMode(simSettings.G4SimSet.bTrackParticles ? AFileParticleGenerator::Relaxed
                                                                           : AFileParticleGenerator::Strict);
                PG->Init();
                ParticleGun = PG;
            }
        }
        else if (PartGenMode == "Script")
        {
            QJsonObject sjs;
            parseJson(js, "GenerationFromScript", sjs);
            if (sjs.isEmpty())
            {
                ErrorString = "Simulation settings do not contain 'from script' generator configuration";
                return false;
            }
            else
            {
                ParticleGun = new AScriptParticleGenerator(*detector.MpCollection, RandGen, ID, &simMan.NumberOfWorkers);
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

    if (PartGenMode == "File")
    {
        totalEventCount = std::min(totalEventCount, static_cast<AFileParticleGenerator*>(ParticleGun)->NumEventsInFile);
    }

    ParticleTracker->configure(&simSettings, fBuildParticleTracks, &tracks, fIgnoreNoDepoEvents, ID);
    ParticleTracker->resetCounter();
    S1generator->setDoTextLog(simSettings.LogsStatOptions.bPhotonGenerationLog);
    S2generator->setDoTextLog(simSettings.LogsStatOptions.bPhotonGenerationLog);
    S2generator->setOnlySecondary(!fDoS1);

    return true;
}

bool AParticleSourceSimulator::finalizeConfig()
{
    const AG4SimulationSettings & G4SimSet = simSettings.G4SimSet;

    if (G4SimSet.bTrackParticles)
    {
        QJsonObject json;
        simMan.generateG4antsConfigCommon(json, this);

        json["NumEvents"] = getEventCount();

        const bool & bBuildTracks = simSettings.TrackBuildOptions.bBuildParticleTracks;
        json["BuildTracks"] = bBuildTracks;
        if (bBuildTracks) json["MaxTracks"] = maxParticleTracks;

        bool bOK = SaveJsonToFile(json, G4SimSet.getConfigFileName(ID));
        if (!bOK)
        {
            ErrorString = "Failed to create Ants2 <-> Geant4 interface files";
            return false;
        }
    }
    return true;
}

void AParticleSourceSimulator::updateGeoManager()
{
    //ParticleTracker->UpdateGeoManager(detector->GeoManager);
    S2generator->UpdateGeoManager(detector.GeoManager);
}

void AParticleSourceSimulator::simulate()
{
    //qDebug() << "Starting simulation, worker #" << ID;

    checkNavigatorPresent();

    if ( !ParticleStack.isEmpty() ) clearParticleStack();

    ReserveSpace(getEventCount());
    if (ParticleGun) ParticleGun->SetStartEvent(eventBegin);
    fStopRequested = false;
    fHardAbortWasTriggered = false;

    std::unique_ptr<QFile> pFile;
    std::unique_ptr<QTextStream> pStream;
    if (simSettings.G4SimSet.bTrackParticles)
    {
        //mode "from G4ants file" requires special treatment
        AFileParticleGenerator * fpg = dynamic_cast<AFileParticleGenerator*>(ParticleGun);
        if (fpg && fpg->IsFormatG4())
        {
            bool bOK = prepareWorkerG4File();
            //qDebug() << "Prepared file for worker #" << ID << "result:"<<bOK;
            fpg->ReleaseResources();

            if (!bOK)
            {
                ErrorString = fpg->GetErrorString();
                fSuccess = false;
                return;
            }

            bOK = geant4TrackAndProcess();
            if (!bOK) fSuccess = false;
            else      fSuccess = !fHardAbortWasTriggered;
            //qDebug() << "tread #" << ID << "reports result of geant4-based tracking:" << bOK << "fSuccess:"<< fSuccess;
            return;
        }

        const QString name = simSettings.G4SimSet.getPrimariesFileName(ID);//   FilePath + QString("primaries-%1.txt").arg(ID);
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
        bool bGenerationSuccessful = choosePrimariesForThisEvent(numPrimaries, eventCurrent);
        if (!bGenerationSuccessful) break; //e.g. in file gen -> end of file reached

        //event prepared
            //qDebug() << "Event composition ready!  Particle stack length:" << ParticleStack.size();

        if (!simSettings.G4SimSet.bTrackParticles)
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

            if (bOnlySavePrimariesToFile) progress = (eventCurrent - eventBegin + 1) * updateFactor;  // *!* obsolete?

            continue; //skip tracking and go to the next event
        }

        //-- only local tracking remains here --
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

        if (!simSettings.fLRFsim) OneEvent->HitsToSignal();

        dataHub->Events.append(OneEvent->PMsignals);
        if (timeRange != 0) dataHub->TimedEvents.append(OneEvent->TimedPMsignals);

        EnergyVectorToScan(); //prepare true position data using deposition data

        progress = (eventCurrent - eventBegin + 1) * updateFactor;
    }

    ParticleTracker->releaseResources();
    if (ParticleGun) ParticleGun->ReleaseResources();
    if (simSettings.G4SimSet.bTrackParticles)
    {
        pStream->flush();
        pFile->close();
    }

    if (simSettings.G4SimSet.bTrackParticles && !bOnlySavePrimariesToFile)
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
    dataHub->GeneratedPhotonsHistory << this->dataHub->GeneratedPhotonsHistory;
    dataHub->ScanNumberOfRuns = 1;
}

void AParticleSourceSimulator::mergeData()
{
    simMan.SeenNonRegisteredParticles += SeenNonRegisteredParticles;
    SeenNonRegisteredParticles.clear();

    simMan.DepoByNotRegistered += DepoByNotRegistered;
    DepoByNotRegistered = 0;

    simMan.DepoByRegistered += DepoByRegistered;
    DepoByRegistered = 0;

    simMan.TrackingHistory.insert(
          simMan.TrackingHistory.end(),
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
    if (bG4isRunning && G4handler) G4handler->abort();
}

void AParticleSourceSimulator::updateMaxTracks(int maxPhotonTracks, int maxParticleTracks)
{
    ParticleTracker->setMaxTracks(maxParticleTracks);
    photonTracker->setMaxTracks(maxPhotonTracks);
}

void AParticleSourceSimulator::EnergyVectorToScan()
{
    AScanRecord * scs = new AScanRecord();
    scs->ScintType = 0;

    if (EnergyVector.isEmpty())
        scs->Points.Reinitialize(0);
    else if (EnergyVector.size() == 1)
    {
        const AEnergyDepositionCell * Node = EnergyVector.at(0);
        for (int i = 0; i < 3; i++)
            scs->Points[0].r[i] = Node->r[i];
        scs->Points[0].energy = Node->dE;
        scs->Points[0].time   = Node->time;
    }
    else
    {
        const int numNodes = EnergyVector.size(); // here it is not equal to 1

        if (!bClusterMerge)
        {
            scs->Points.Reinitialize(numNodes);
            for (int iNode = 0; iNode < numNodes; iNode++)
            {
                const AEnergyDepositionCell * Node = EnergyVector.at(iNode);
                for (int i=0; i<3; i++)
                    scs->Points[iNode].r[i] = Node->r[i];
                scs->Points[iNode].energy = Node->dE;
                scs->Points[iNode].time   = Node->time;
            }
        }
        else
        {
            /*
            qDebug() << "#";
            for (int iCell = 0; iCell < EnergyVector.size(); iCell++)
            {
                const AEnergyDepositionCell * EVcell = EnergyVector.at(iCell);
                qDebug() << EVcell->dE << EVcell->time;
            }
            */

            QVector<APositionEnergyRecord> Depo(numNodes);

            for (int i=0; i<3; i++)
                Depo[0].r[i] = EnergyVector[0]->r[i];
            Depo[0].energy   = EnergyVector[0]->dE;
            Depo[0].time     = EnergyVector[0]->time;

            //first pass is copy from EnergyVector (since cannot modify EnergyVector itself),
            //if next point is within cluster range, merge with previous point
            //if outside, start with a new cluster
            int iPoint = 0;  //merging to this point
            int numMerges = 0;
            for (int iCell = 1; iCell < EnergyVector.size(); iCell++)
            {
                const AEnergyDepositionCell * EVcell = EnergyVector.at(iCell);

                APositionEnergyRecord & Point = Depo[iPoint];
                if (EVcell->isCloser(ClusterMergeRadius2, Point.r) && fabs(EVcell->time - Point.time) < ClusterMergeTimeDif)
                {
                    Point.MergeWith(EVcell->r, EVcell->dE, EVcell->time);
                    numMerges++;
                }
                else
                {
                    //start the next cluster
                    iPoint++;
                    for (int i=0; i<3; i++)
                        Depo[iPoint].r[i] = EVcell->r[i];
                    Depo[iPoint].energy   = EVcell->dE;
                    Depo[iPoint].time     = EVcell->time;
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
                        if (Depo[iThisCluster].isCloser(ClusterMergeRadius2, Depo[iOtherCluster]) && fabs(Depo[iThisCluster].time - Depo[iOtherCluster].time) < ClusterMergeTimeDif)
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
            for (int iDe = 0; iDe < Depo.size(); iDe++)
                scs->Points[iDe] = Depo[iDe];
        }
    }

    dataHub->Scan.append(scs);
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

bool AParticleSourceSimulator::choosePrimariesForThisEvent(int numPrimaries, int iEvent)
{
    for (int iPart = 0; iPart < numPrimaries; iPart++)
    {
        if (!ParticleGun) break; //paranoic
        //generating one event
        bool bGenerationSuccessful = ParticleGun->GenerateEvent(GeneratedParticles, iEvent);
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

#include "asandwich.h"
#include "amonitor.h"
bool AParticleSourceSimulator::geant4TrackAndProcess()
{
    bool bOK = runGeant4Handler();
    if (!bOK) return false;

    // read receipt file, stop if not OK
    QString receipeFileName = simSettings.G4SimSet.getReceitFileName(ID); // FilePath + QString("receipt-%1.txt").arg(ID);
    QJsonObject jrec;
    bOK = LoadJsonFromFile(jrec, receipeFileName);
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
    bOK = processG4DepositionData();
    releaseInputResources();
    if (!bOK) return false;

    //read history/tracks
    if (simSettings.TrackBuildOptions.bBuildParticleTracks || simSettings.LogsStatOptions.bParticleTransportLog)
    {
        ATrackingDataImporter ti(simSettings.TrackBuildOptions,
                                 detector.MpCollection->getListOfParticleNames(),
                                 (simSettings.LogsStatOptions.bParticleTransportLog ? &TrackingHistory : nullptr),
                                 (simSettings.TrackBuildOptions.bBuildParticleTracks ? &tracks : nullptr),
                                 maxParticleTracks);
        const QString TrackingFileName = simSettings.G4SimSet.getTracksFileName(ID);
        ErrorString = ti.processFile(TrackingFileName, eventBegin, simSettings.G4SimSet.BinaryOutput);
        if (!ErrorString.isEmpty()) return false;
    }

    //read monitor data
    int numMon = detector.Sandwich->MonitorsRecords.size();
    if (numMon != 0)
    {
        QString monFileName = simSettings.G4SimSet.getMonitorDataFileName(ID);
        QJsonArray ar;
        bOK = LoadJsonArrayFromFile(ar, monFileName);
        if (bOK)
        {
            for (int i=0; i<ar.size(); i++)
            {
                QJsonObject json = ar[i].toObject();
                int iMon;
                bool bOK = parseJson(json, "MonitorIndex", iMon);
                if (!bOK)
                {
                    ErrorString = "Failed to read monitor data: Monitor index not found";
                    return false;
                }
                if (iMon < 0 || iMon >= numMon)
                {
                    ErrorString = "Failed to read monitor data: Bad monitor index";
                    return false;
                }
                AMonitor * mon = dataHub->SimStat->Monitors[iMon];
                mon->overrideDataFromJson(json);
            }
        }
        else qWarning() << "Failed to read monitor data";
        //if (!bOK)  //restore it later, when compatibility is not an issue with G$ants anymore
        //{
        //    ErrorString = "Failed to read monitor data!";
        //    return false;
        //}
    }
    return true;
}

bool AParticleSourceSimulator::runGeant4Handler()
{
    const QString exe = AGlobalSettings::getInstance().G4antsExec;
    const QString confFile = simSettings.G4SimSet.getConfigFileName(ID); // FilePath + QString("aga-%1.json").arg(ID);
    QStringList ar;
    ar << confFile;

    delete G4handler;
    G4handler = new AExternalProcessHandler(exe, ar);
    //G4handler->setVerbose();
    G4handler->setProgressVariable(&progressG4);

    bG4isRunning = true;
    G4handler->startAndWait();
    bG4isRunning = false;

    QString err = G4handler->ErrorString;

    if (!err.isEmpty())
    {
        ErrorString = "Failed to start G4ants executable\n" + err;
        return false;
    }

    if (fHardAbortWasTriggered) return false;

    progressG4 = 100.0;
    return true;
}

bool AParticleSourceSimulator::processG4DepositionData()
{
    const QString DepoFileName = simSettings.G4SimSet.getDepositionFileName(ID);

    if (simSettings.G4SimSet.BinaryOutput)
    {
        inStream = new std::ifstream(DepoFileName.toLocal8Bit().data(), std::ios::in | std::ios::binary);

        if (!inStream->is_open())
        {
            ErrorString = "Cannot open input file: " + DepoFileName;
            return false;
        }
        eventCurrent = eventBegin - 1;
        bool bOK = readG4DepoEventFromBinFile(true); //only reads the header of the first event
        if (!bOK) return false;
    }
    else
    {
        inTextFile = new QFile(DepoFileName);
        if (!inTextFile->open(QIODevice::ReadOnly | QFile::Text))
        {
            ErrorString = "Cannot open file with energy deposition data:\n" + DepoFileName;
            return false;
        }
        inTextStream = new QTextStream(inTextFile);
        G4DepoLine = inTextStream->readLine();
    }

    for (eventCurrent = eventBegin; eventCurrent < eventEnd; eventCurrent++)
    {
        if (fStopRequested) break;
        if (EnergyVector.size() > 0) clearEnergyVector();

        //Filling EnergyVector for this event
        //  qDebug() << "iEv="<<eventCurrent << "building energy vector...";
        bool bOK = (simSettings.G4SimSet.BinaryOutput ? readG4DepoEventFromBinFile()
                                                      : readG4DepoEventFromTextFile() );
        if (!bOK) return false;
        //  qDebug() << "Energy vector contains" << EnergyVector.size() << "cells";

        bOK = generateAndTrackPhotons();
        if (!bOK) return false;

        if (!simSettings.fLRFsim) OneEvent->HitsToSignal();

        dataHub->Events.append(OneEvent->PMsignals);
        if (timeRange != 0) dataHub->TimedEvents.append(OneEvent->TimedPMsignals);

        EnergyVectorToScan();

        progress = (eventCurrent - eventBegin + 1) * updateFactor;
    }
    return true;
}

bool AParticleSourceSimulator::readG4DepoEventFromTextFile()
{
    //  qDebug() << " CurEv:"<<eventCurrent <<" -> " << G4DepoLine;
    if (!G4DepoLine.startsWith('#') || G4DepoLine.size() < 2)
    {
        ErrorString = "Format error for #event field in G4ants energy deposition file";
        return false;
    }
    G4DepoLine.remove(0, 1);
    //  qDebug() << G4DepoLine << eventCurrent;
    if (G4DepoLine.toInt() != eventCurrent)
    {
        ErrorString = "Missmatch of event number in G4ants energy deposition file";
        return false;
    }

    do
    {
        G4DepoLine = inTextStream->readLine();
        if (G4DepoLine.startsWith('#'))
            break; //next event

        //  qDebug() << ID << "->"<<G4DepoLine;
        //pId mId dE x y z t
        // 0   1   2 3 4 5 6     // pId can be = -1 !
        //populating energy vector data
        QStringList fields = G4DepoLine.split(' ', QString::SkipEmptyParts);
        if (fields.isEmpty()) break; //last event had no depo - end of file reached
        if (fields.size() < 7)
        {
            ErrorString = "Format error in G4ants energy deposition file";
            return false;
        }
        AEnergyDepositionCell* cell = new AEnergyDepositionCell(fields[3].toDouble(), fields[4].toDouble(), fields[5].toDouble(), //x y z
                                                                fields[6].toDouble(), fields[2].toDouble(),  //time dE
                                                                fields[0].toInt(), fields[1].toInt(), 0, eventCurrent); //part mat sernum event
        EnergyVector << cell;
    }
    while (!inTextStream->atEnd());

    return true;
}

bool AParticleSourceSimulator::readG4DepoEventFromBinFile(bool expectNewEvent)
{
    char header = 0;
    while (*inStream >> header)
    {
        if (header == char(0xee))
        {
            inStream->read((char*)&G4NextEventId, sizeof(int));
            //qDebug() << G4NextEventId << "expecting:" << eventCurrent+1;
            if (G4NextEventId != eventCurrent + 1)
            {
                ErrorString = QString("Bad event number in G4ants energy depo file: expected %1 and got %2").arg(eventCurrent).arg(G4NextEventId);
                return false;
            }
            return true; //ready to read this event
        }
        else if (expectNewEvent)
        {
            ErrorString = "New event tag not found in G4ants energy deposition file";
            return false;
        }
        else if (header == char(0xff))
        {
            AEnergyDepositionCell * cell = new AEnergyDepositionCell();

            // format:
            // partId(int) matId(int) DepoE(double) X(double) Y(double) Z(double) Time(double)
            inStream->read((char*)&cell->ParticleId, sizeof(int));
            inStream->read((char*)&cell->MaterialId, sizeof(int));
            inStream->read((char*)&cell->dE,         sizeof(double));
            inStream->read((char*)&cell->r[0],       sizeof(double));
            inStream->read((char*)&cell->r[1],       sizeof(double));
            inStream->read((char*)&cell->r[2],       sizeof(double));
            inStream->read((char*)&cell->time,       sizeof(double));

            EnergyVector << cell;
        }
        else
        {
            ErrorString = "Unexpected format of record header for binary input in G4ants deposition file";
            return false;
        }

        if (inStream->eof() ) return true;
    }

    if (inStream->eof() ) return true;
    else if (inStream->fail())
    {
        ErrorString = "Unexpected format of binary input in G4ants deposition file";
        return false;
    }

    qWarning() << "Error - 'should not be here' reached";
    return true; //should not be here
}

void AParticleSourceSimulator::releaseInputResources()
{
    delete inStream; inStream = nullptr;

    delete inTextStream;  inTextStream = nullptr;
    delete inTextFile;    inTextFile = nullptr;
}

bool AParticleSourceSimulator::prepareWorkerG4File()
{
    AFileParticleGenerator * fpg = static_cast<AFileParticleGenerator*>(ParticleGun);

    const QString FileName = simSettings.G4SimSet.getPrimariesFileName(ID);
    return fpg->generateG4File(eventBegin, eventEnd, FileName);
}

