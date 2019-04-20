#include "simulationmanager.h"
#include "apmhub.h"
#include "alrfmoduleselector.h"
#include "detectorclass.h"
#include "aoneevent.h"
#include "eventsdataclass.h"
#include "photon_generator.h"
#include "primaryparticletracker.h"
#include "s1_generator.h"
#include "s2_generator.h"
#include "asourceparticlegenerator.h"
#include "afileparticlegenerator.h"
#include "ascriptparticlegenerator.h"
#include "amaterialparticlecolection.h"
#include "asandwich.h"
#include "ageoobject.h"
#include "aphotontracer.h"
#include "apositionenergyrecords.h"
#include "aenergydepositioncell.h"
#include "aparticlerecord.h"
#include "amessage.h"
#include "acommonfunctions.h"
#include "ageomarkerclass.h"
#include "atrackrecords.h"
#include "ajsontools.h"
#include "afiletools.h"
#include "aconfiguration.h"
#include "aglobalsettings.h"
#include "anoderecord.h"

#include <memory>

#include <QVector>
#include <QTime>
#include <QDebug>
#include <QProcess>

#include "TGeoManager.h"
#include "TRandom2.h"
#include "TH1D.h"
#if ROOT_VERSION_CODE < ROOT_VERSION(6,11,1)
    #include "TThread.h"
#else
    #include <thread>
#endif

//static method to give to TThread or std::thread which will run simulation
//Needs to return something, otherwise root always starts detached thread
static void *SimulationManagerRunWorker(void *workerSimulator)
{
    if(!workerSimulator)
    {
        qDebug()<<"ThreadCall: workerSimulatorIsNull!!";
        return NULL;
    }
    Simulator *simulator = (Simulator *)workerSimulator;
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
    int numTracks = (simSettings.TrackBuildOptions.bBuildParticleTracks ? simSettings.TrackBuildOptions.MaxParticleTracks : 0);

    bool bOK = detector->generateG4interfaceFiles(simSettings.G4SimSet, threadCount, numTracks);
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
      if (psc["ControlOptions"].toObject()["Single_Scan_Flood"] == 3)
      {
          QString fileName = psc["CustomNodesOptions"].toObject()["FileWithNodes"].toString();
          //qDebug() << "------------"<<fileName;
          const QString err = simMan->loadNodesFromFile(fileName);
          if (!err.isEmpty())
          {
              ErrorString = err;
              return false;
          }
          qDebug() << "Custom nodes loaded from files, top nodes:"<<simMan->Nodes.size();
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
      Simulator *worker;
      if (modeSetup == "PointSim") //Photon simulator
          worker = new PointSourceSimulator(detector, simMan, i);
      else //Particle simulator
      {
          ParticleSourceSimulator* pss = new ParticleSourceSimulator(detector, simMan, i);
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

/*
void ASimulatorRunner::setWorkersSeed(int rngSeed)
{
    TRandom2 rng(rngSeed);
    for(int i = 0; i < workers.count(); i++)
        workers[i]->setRngSeed(rng.Rndm()*100000);
}
*/

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
    for(const Simulator * sim : workers)
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

    //Dump data in dataHub
    dataHub->fSimulatedData = true;
    dataHub->LastSimSet = simSettings;
    ErrorString.clear();
    //qDebug() << "SimRunner: Collecting data from workers";
    for(int i = 0; i < workers.count(); i++)
    {
        workers[i]->appendToDataHub(dataHub);
        QString err = workers.at(i)->getErrorString();
        if (!err.isEmpty())
            ErrorString += QString("Thread %1 reported error: %2\n").arg(i).arg(err);
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

/******************************************************************************\
*                  Simulator base class (per-thread instance)                  |
\******************************************************************************/
Simulator::Simulator(const DetectorClass *detector, ASimulationManager *simMan, const int ID) :
    detector(detector), simMan(simMan), simSettings(0), ID(ID)
{    
    RandGen = new TRandom2();    
    dataHub = new EventsDataClass(ID);
    OneEvent = new AOneEvent(detector->PMs, RandGen, dataHub->SimStat);
    photonGenerator = new Photon_Generator(detector, RandGen);
    photonTracker = new APhotonTracer(detector->GeoManager, RandGen, detector->MpCollection, detector->PMs, &detector->Sandwich->GridRecords);
}

Simulator::~Simulator()
{
    delete photonTracker;
    delete photonGenerator;
    delete dataHub;
    delete OneEvent;    
    delete RandGen;

    //if transferred, the container will be cleared, need cleaning only in case of abort
    for (TrackHolderClass* t : tracks) delete t;
    tracks.clear();
}

void Simulator::updateGeoManager()
{
    photonTracker->UpdateGeoManager(detector->GeoManager);
}

void Simulator::setSimSettings(const GeneralSimSettings *settings)
{
  simSettings = settings;
}

void Simulator::initSimStat()
{
   dataHub->initializeSimStat(detector->Sandwich->MonitorsRecords, simSettings->DetStatNumBins, (simSettings->fWaveResolved ? simSettings->WaveNodes : 0));
}

void Simulator::setRngSeed(int seed)
{
  this->RandGen->SetSeed(seed);
}

void Simulator::requestStop()
{
    if (fStopRequested)
    {
        //  qDebug() << "Simulator #" << ID << "recived repeated stop request, aborting in hard mode";
        hardAbort();
        fHardAbortWasTriggered = true;
    }
    else
    {
        //  qDebug() << "First stop request was received by simulator #"<<ID;
        fStopRequested = true;
    }
}

void Simulator::divideThreadWork(int threadId, int threadCount)
{
    //  qDebug() <<  "This thread:" << threadId << "Count treads:"<<threadCount;
    int totalEventCount = getTotalEventCount();
    int nodesPerThread = totalEventCount / threadCount;
    int remainingNodes = totalEventCount % threadCount;
    //The first <remainingNodes> threads get an extra node each
    eventBegin = threadId*nodesPerThread + std::min(threadId, remainingNodes);
    eventEnd = eventBegin + nodesPerThread + (threadId >= remainingNodes ? 0 : 1);
    //  qDebug()<<"Thread"<<threadId<<"/"<<threadCount<<": eventBegin"<<eventBegin<<": eventEnd"<<eventEnd<<": eventCount"<<getEventCount()<<"/"<<getTotalEventCount();

    //dividing track building
    int maxPhotonTracks;
    if (simSettings->TrackBuildOptions.bBuildPhotonTracks)
        maxPhotonTracks = ceil( 1.0 * getEventCount() / getTotalEventCount() * simSettings->TrackBuildOptions.MaxPhotonTracks );
    else maxPhotonTracks = 0;
    int maxParticleTracks;
    if (simSettings->TrackBuildOptions.bBuildParticleTracks)
        maxParticleTracks = ceil( 1.0 * getEventCount() / getTotalEventCount() * simSettings->TrackBuildOptions.MaxParticleTracks );
    else maxParticleTracks = 0;
    updateMaxTracks(maxPhotonTracks, maxParticleTracks);
    //  qDebug() << maxPhotonTracks << maxParticleTracks;
}

bool Simulator::setup(QJsonObject &json)
{
    fUpdateGUI = json["DoGuiUpdate"].toBool();
    fBuildPhotonTracks = fUpdateGUI && simSettings->TrackBuildOptions.bBuildPhotonTracks;

    //inits
    dataHub->clear();
    if (fUpdateGUI) detector->GeoManager->ClearTracks();

    //configuring local modules
    OneEvent->configure(simSettings);
    photonGenerator->configure(simSettings, OneEvent->SimStat);
    photonTracker->configure(simSettings, OneEvent, fBuildPhotonTracks, &tracks);
    return true;
}

void Simulator::appendToDataHub(EventsDataClass *dataHub)
{
    dataHub->Events << this->dataHub->Events; //static
    dataHub->TimedEvents << this->dataHub->TimedEvents; //static
    dataHub->Scan << this->dataHub->Scan; //dynamic!
    this->dataHub->Scan.clear();

    dataHub->SimStat->AppendSimulationStatistics(this->dataHub->SimStat); //deep copy
}

void Simulator::hardAbort()
{
    photonTracker->hardAbort();
}

void Simulator::ReserveSpace(int expectedNumEvents)
{
    dataHub->Events.reserve(expectedNumEvents);
    if (simSettings->fTimeResolved) dataHub->TimedEvents.reserve(expectedNumEvents);
    dataHub->Scan.reserve(expectedNumEvents);
}

void Simulator::updateMaxTracks(int maxPhotonTracks, int /*maxParticleTracks*/)
{
    photonTracker->setMaxTracks(maxPhotonTracks);
}

/******************************************************************************\
*                          Point Source Simulator                              |
\******************************************************************************/
PointSourceSimulator::PointSourceSimulator(const DetectorClass *detector, ASimulationManager *simMan, int ID) :
    Simulator(detector, simMan, ID) {}

PointSourceSimulator::~PointSourceSimulator()
{
  if (CustomHist) delete CustomHist;
}

int PointSourceSimulator::getEventCount() const
{
  if (PointSimMode == 0) return eventEnd - eventBegin;
  else return (eventEnd - eventBegin)*NumRuns;
}

bool PointSourceSimulator::setup(QJsonObject &json)
{
    if (!json.contains("PointSourcesConfig"))
      {
        ErrorString = "Json sent to simulator does not contain generation config data!";
        return false;
      }
    if(!Simulator::setup(json)) return false;

    QJsonObject js = json["PointSourcesConfig"].toObject();
    //reading main control options
    QJsonObject cjson = js["ControlOptions"].toObject();
    PointSimMode = cjson["Single_Scan_Flood"].toInt();    
    ScintType = 1 + cjson["Primary_Secondary"].toInt(); //0 - primary(1), 1 - secondary(2)
    NumRuns = cjson["MultipleRunsNumber"].toInt();
    if (!cjson["MultipleRuns"].toBool()) NumRuns = 1;

    fLimitNodesToObject = false;
    if (cjson.contains("GenerateOnlyInPrimary"))  //just in case it is an old config file run directly
    {
        if (detector->Sandwich->World->findObjectByName("PrScint"))
        {
            fLimitNodesToObject = true;
            LimitNodesToObject = "PrScint";
        }
    }
    if (cjson.contains("LimitNodesTo"))
    {
        if (cjson.contains("LimitNodes")) //new system
            fLimitNodesToObject = cjson["LimitNodes"].toBool();
        else
            fLimitNodesToObject = true;  //semi-old
        QString Obj = cjson["LimitNodesTo"].toString();

        if (fLimitNodesToObject && !Obj.isEmpty() && detector->Sandwich->World->findObjectByName(Obj))
        {
            fLimitNodesToObject = true;
            LimitNodesToObject = Obj.toLocal8Bit().data();
        }
    }

    //reading photons per node info
    QJsonObject ppnjson = js["PhotPerNodeOptions"].toObject();
    numPhotMode = ppnjson["PhotPerNodeMode"].toInt();
    if (numPhotMode < 0 || numPhotMode > 3)
    {
        ErrorString = "Unknown photons per node mode!";
        return false;
    }
    //numPhotsConst = ppnjson["PhotPerNodeConstant"].toInt();
    parseJson(ppnjson, "PhotPerNodeConstant", numPhotsConst);
    //numPhotUniMin = ppnjson["PhotPerNodeUniMin"].toInt();
    parseJson(ppnjson, "PhotPerNodeUniMin", numPhotUniMin);
    //numPhotUniMax = ppnjson["PhotPerNodeUniMax"].toInt();
    parseJson(ppnjson, "PhotPerNodeUniMax", numPhotUniMax);
    parseJson(ppnjson, "PhotPerNodeGaussMean", numPhotGaussMean);
    parseJson(ppnjson, "PhotPerNodeGaussSigma", numPhotGaussSigma);

    if (numPhotMode == 3)
    {
        if (!ppnjson.contains("PhotPerNodeCustom"))
        {
            ErrorString = "Generation config does not contain custom photon distribution data!";
            return false;
        }
        QJsonArray ja = ppnjson["PhotPerNodeCustom"].toArray();
        int size = ja.size();
        double* xx = new double[size];
        int* yy    = new int[size];
        for (int i=0; i<size; i++)
        {
            xx[i] = ja[i].toArray()[0].toDouble();
            yy[i] = ja[i].toArray()[1].toInt();
        }
        TString hName = "hPhotDistr";
        hName += ID;
        CustomHist = new TH1I(hName, "Photon distribution", size-1, xx);
        for (int i = 1; i<size+1; i++)  CustomHist->SetBinContent(i, yy[i-1]);
        CustomHist->GetIntegral(); //will be thread safe after this
        delete[] xx;
        delete[] yy;
    }

    //reading wavelength/decay options
    QJsonObject wdjson = js["WaveTimeOptions"].toObject();
    fUseGivenWaveIndex = false; //compatibility
    parseJson(wdjson, "UseFixedWavelength", fUseGivenWaveIndex);
    PhotonOnStart.waveIndex = wdjson["WaveIndex"].toInt();
    if (!simSettings->fWaveResolved) PhotonOnStart.waveIndex = -1;

    //reading direction info
    QJsonObject pdjson = js["PhotonDirectionOptions"].toObject();
    fRandomDirection =  pdjson["Random"].toBool();
    if (!fRandomDirection)
    {
        PhotonOnStart.v[0] = pdjson["FixedX"].toDouble();
        PhotonOnStart.v[1] = pdjson["FixedY"].toDouble();
        PhotonOnStart.v[2] = pdjson["FixedZ"].toDouble();
        NormalizeVector(PhotonOnStart.v);
        //qDebug() << "  ph dir:"<<PhotonOnStart.v[0]<<PhotonOnStart.v[1]<<PhotonOnStart.v[2];
    }
    fCone = false;
    if (pdjson.contains("Fixed_or_Cone"))
      {
        int i = pdjson["Fixed_or_Cone"].toInt();
        fCone = (i==1);
        if (fCone)
          {
            double v[3];
            v[0] = pdjson["FixedX"].toDouble();
            v[1] = pdjson["FixedY"].toDouble();
            v[2] = pdjson["FixedZ"].toDouble();
            NormalizeVector(v);
            ConeDir = TVector3(v[0], v[1], v[2]);
            double ConeAngle = pdjson["Cone"].toDouble()*3.1415926535/180.0;
            CosConeAngle = cos(ConeAngle);
          }
      }

    //calculating eventCount and storing settings specific to each simulation mode
    switch (PointSimMode)
    {
        case 0:
            //fOK = SimulateSingle(json);
            simOptions = js["SinglePositionOptions"].toObject();
            totalEventCount = NumRuns; //the only case when we can split runs between threads
            break;
        case 1:
        {
            //fOK = SimulateRegularGrid(json);
            simOptions = js["RegularScanOptions"].toObject();
            int RegGridNodes[3]; //number of nodes along the 3 axes
            QJsonArray rsdataArr = simOptions["AxesData"].toArray();
            for (int ic=0; ic<3; ic++)
            {
                if (ic >= rsdataArr.size())
                    RegGridNodes[ic] = 1; //disabled axis
                else
                    RegGridNodes[ic] = rsdataArr[ic].toObject()["Nodes"].toInt();
            }
            totalEventCount = 1;
            for (int i=0; i<3; i++) totalEventCount *= RegGridNodes[i]; //progress reporting knows we do NumRuns per each node
            break;
        }
        case 2:
            //fOK = SimulateFlood(json);
            simOptions = js["FloodOptions"].toObject();
            totalEventCount = simOptions["Nodes"].toInt();//progress reporting knows we do NumRuns per each node
            break;
      case 3:
            simOptions = js["CustomNodesOptions"].toObject();
            //totalEventCount = simOptions["Nodes"].toArray().size();//progress reporting knows we do NumRuns per each node
            totalEventCount = simMan->Nodes.size();
            break;
        default:
            ErrorString = "Unknown or not implemented simulation mode: "+QString().number(PointSimMode);
            return false;
    }
    return true;
}

void PointSourceSimulator::simulate()
{
    fStopRequested = false;
    fHardAbortWasTriggered = false;

    ReserveSpace(getEventCount());
    switch (PointSimMode)
    {
        case 0: fSuccess = SimulateSingle(); break;
        case 1: fSuccess = SimulateRegularGrid(); break;
        case 2: fSuccess = SimulateFlood(); break;
        case 3: fSuccess = SimulateCustomNodes(); break;
        default: fSuccess = false; break;
    }
    if (fHardAbortWasTriggered) fSuccess = false;
}

void PointSourceSimulator::appendToDataHub(EventsDataClass *dataHub)
{
    //qDebug() << "Thread #"<<ID << " PhotSim ---> appending data";
    Simulator::appendToDataHub(dataHub);
    dataHub->ScanNumberOfRuns = this->NumRuns;
}

bool PointSourceSimulator::SimulateSingle()
{
    double x = simOptions["SingleX"].toDouble();
    double y = simOptions["SingleY"].toDouble();
    double z = simOptions["SingleZ"].toDouble();
    std::unique_ptr<ANodeRecord> node(ANodeRecord::createS(x, y, z));

    int eventsToDo = eventEnd - eventBegin; //runs are split between the threads, since node position is fixed for all
    double updateFactor = (eventsToDo<1) ? 100.0 : 100.0/eventsToDo;
    progress = 0;
    eventCurrent = 0;
    for (int irun = 0; irun<eventsToDo; ++irun)
    {
        OneNode(*node);
        eventCurrent = irun;
        progress = irun * updateFactor;
        if (fStopRequested) return false;
    }
    return true;
}

bool PointSourceSimulator::SimulateRegularGrid()
{
    //extracting grid parameters
    double RegGridOrigin[3]; //grid origin
    RegGridOrigin[0] = simOptions["ScanX0"].toDouble();
    RegGridOrigin[1] = simOptions["ScanY0"].toDouble();
    RegGridOrigin[2] = simOptions["ScanZ0"].toDouble();
    //
    double RegGridStep[3][3]; //vector [axis] [step]
    int RegGridNodes[3]; //number of nodes along the 3 axes
    bool RegGridFlagPositive[3]; //Axes option
    //
    QJsonArray rsdataArr = simOptions["AxesData"].toArray();
    for (int ic=0; ic<3; ic++)
    {
        if(ic < rsdataArr.count())
        {
            QJsonObject adjson = rsdataArr[ic].toObject();
            RegGridStep[ic][0] = adjson["dX"].toDouble();
            RegGridStep[ic][1] = adjson["dY"].toDouble();
            RegGridStep[ic][2] = adjson["dZ"].toDouble();
            RegGridNodes[ic] = adjson["Nodes"].toInt(); //1 is disabled axis
            RegGridFlagPositive[ic] = adjson["Option"].toInt();
        }
        else
        {
            RegGridStep[ic][0] = 0;
            RegGridStep[ic][1] = 0;
            RegGridStep[ic][2] = 0;
            RegGridNodes[ic] = 1; //1 is disabled axis
            RegGridFlagPositive[ic] = true;
        }
    }

    std::unique_ptr<ANodeRecord> node(ANodeRecord::createS(0, 0, 0));
    int currentNode = 0;
    eventCurrent = 0;
    double updateFactor = 100.0/( NumRuns*(eventEnd - eventBegin) );
    //Do the scan
    int iAxis[3];
    for (iAxis[0]=0; iAxis[0]<RegGridNodes[0]; iAxis[0]++)
        for (iAxis[1]=0; iAxis[1]<RegGridNodes[1]; iAxis[1]++)
            for (iAxis[2]=0; iAxis[2]<RegGridNodes[2]; iAxis[2]++)  //iAxis - counters along the axes!!!
            {
                if (currentNode < eventBegin)
                  { //this node is taken care of by another thread
                    currentNode++;
                    continue;
                  }

                //calculating node coordinates
                for (int i=0; i<3; i++) node->R[i] = RegGridOrigin[i];
                //shift from the origin
                for (int axis=0; axis<3; axis++)
                { //going axis by axis
                    double ioffset = 0;
                    if (!RegGridFlagPositive[axis]) ioffset = -0.5*( RegGridNodes[axis] - 1 );
                    for (int i=0; i<3; i++) node->R[i] += (ioffset + iAxis[axis]) * RegGridStep[axis][i];
                }

                //running this node              
                for (int irun = 0; irun<NumRuns; irun++)
                  {
                    OneNode(*node);
                    eventCurrent++;
                    progress = eventCurrent * updateFactor;
                    if(fStopRequested) return false;
                  }
                currentNode++;
                if(currentNode >= eventEnd) return true;
            }
    return true;
}

bool PointSourceSimulator::SimulateFlood()
{
    //extracting flood parameters
    int Shape = simOptions["Shape"].toInt(); //0-square, 1-ring/circle
    double Xfrom, Xto, Yfrom, Yto, CenterX, CenterY, RadiusIn, RadiusOut;
    double Rad2in, Rad2out;
    if (Shape == 0)
    {
        Xfrom = simOptions["Xfrom"].toDouble();
        Xto =   simOptions["Xto"].toDouble();
        Yfrom = simOptions["Yfrom"].toDouble();
        Yto =   simOptions["Yto"].toDouble();
    }
    else
    {
        CenterX = simOptions["CenterX"].toDouble();
        CenterY = simOptions["CenterY"].toDouble();
        RadiusIn = 0.5*simOptions["DiameterIn"].toDouble();
        RadiusOut = 0.5*simOptions["DiameterOut"].toDouble();

        Rad2in = RadiusIn*RadiusIn;
        Rad2out = RadiusOut*RadiusOut;
        Xfrom = CenterX - RadiusOut;
        Xto =   CenterX + RadiusOut;
        Yfrom = CenterY - RadiusOut;
        Yto =   CenterY + RadiusOut;
    }
    int Zoption = simOptions["Zoption"].toInt(); //0-fixed, 1-range
    double Zfixed, Zfrom, Zto;
    if (Zoption == 0)
    {
        Zfixed = simOptions["Zfixed"].toDouble();
    }
    else
    {
        Zfrom = simOptions["Zfrom"].toDouble();
        Zto =   simOptions["Zto"].toDouble();
    }

    //Do flood
    std::unique_ptr<ANodeRecord> node(ANodeRecord::createS(0, 0, 0));
    int nodeCount = (eventEnd - eventBegin);
    eventCurrent = 0;
    int WatchdogThreshold = 100000;
    double updateFactor = 100.0 / (NumRuns*nodeCount);
    for (int inode = 0; inode < nodeCount; inode++)
    {
        if(fStopRequested) return false;

        //choosing node coordinates
        //double r[3];
        node->R[0] = Xfrom + (Xto-Xfrom)*RandGen->Rndm();
        node->R[1] = Yfrom + (Yto-Yfrom)*RandGen->Rndm();
        //running this node
        if (Shape == 1)
        {
            double r2  = (node->R[0] - CenterX)*(node->R[0] - CenterX) + (node->R[1] - CenterY)*(node->R[1] - CenterY);
            if ( r2 > Rad2out || r2 < Rad2in )
            {
                inode--;
                continue;
            }
        }
        if (Zoption == 0) node->R[2] = Zfixed;
        else node->R[2] = Zfrom + (Zto-Zfrom)*RandGen->Rndm();
        if (fLimitNodesToObject && !isInsideLimitingObject(node->R))
        {
            WatchdogThreshold--;
            if (WatchdogThreshold < 0 && inode == 0)
            {
                ErrorString = "100000 attempts to generate a point inside the limiting object has failed!";
                return false;
            }

            inode--;
            continue;
        }

        for (int irun = 0; irun<NumRuns; irun++)
          {
            OneNode(*node);
            eventCurrent++;
            progress = eventCurrent * updateFactor;
            if(fStopRequested) return false;
          }
    }
    return true;
}

bool PointSourceSimulator::SimulateCustomNodes()
{
  int nodeCount = (eventEnd - eventBegin);
  int currentNode = eventBegin;
  eventCurrent = 0;
  double updateFactor = 100.0 / ( NumRuns*nodeCount );

  for (int inode = 0; inode < nodeCount; inode++)
  {
      ANodeRecord * thisNode = simMan->Nodes.at(currentNode);

      for (int irun = 0; irun<NumRuns; irun++)
      {
          OneNode(*thisNode);
          eventCurrent++;
          progress = eventCurrent * updateFactor;
          if(fStopRequested) return false;
      }
      currentNode++;
  }
  return true;
}

int PointSourceSimulator::PhotonsToRun()
{
    int numPhotons = 0; //protection
    switch (numPhotMode)
    {
        case 0: //constant
            numPhotons = numPhotsConst;
            break;
        case 1:  //uniform
            numPhotons = RandGen->Rndm()*(numPhotUniMax-numPhotUniMin+1) + numPhotUniMin;
            break;
        case 2: //gauss
            numPhotons = RandGen->Gaus(numPhotGaussMean, numPhotGaussSigma);
            break;
        case 3: //custom
            numPhotons = CustomHist->GetRandom();
            break;
    }

    return numPhotons;
}

void PointSourceSimulator::GenerateTraceNphotons(AScanRecord *scs, double time0, int iPoint)
//scs contains coordinates and number of photons to run
//iPoint - number of this scintillation center (can be not zero when e.g. double events are simulated)
{
    //if secondary scintillation, finding the secscint boundaries
    double z1, z2, timeOfDrift, driftSpeedSecScint;
    if (scs->ScintType == 2)
    {
        if (!FindSecScintBounds(scs->Points[iPoint].r, z1, z2, timeOfDrift, driftSpeedSecScint))
        {
            scs->zStop = scs->Points[iPoint].r[2];
            return; //no sec_scintillator for these XY, so no photon generation
        }
    }

    PhotonOnStart.r[0] = scs->Points[iPoint].r[0];
    PhotonOnStart.r[1] = scs->Points[iPoint].r[1];
    PhotonOnStart.r[2] = scs->Points[iPoint].r[2];
    PhotonOnStart.scint_type = scs->ScintType;
    PhotonOnStart.time = 0;

    //================================
    for (int i=0; i<scs->Points[iPoint].energy; i++)
    {
        //photon direction
        if (fRandomDirection)
        {
            if (scs->ScintType == 2)
            {
                photonGenerator->GenerateDirectionSecondary(&PhotonOnStart);
                if (PhotonOnStart.fSkipThisPhoton)
                {
                    PhotonOnStart.fSkipThisPhoton = false;
                    continue;  //skip this photon - direction didn't pass the criterium set for the secondary scintillation
                }
            }
            else photonGenerator->GenerateDirectionPrimary(&PhotonOnStart);
        }
        else if (fCone)
          {
              double z = CosConeAngle + RandGen->Rndm() * (1.0 - CosConeAngle);
              double tmp = sqrt(1.0 - z*z);
              double phi = RandGen->Rndm()*3.1415926535*2.0;
              TVector3 K1(tmp*cos(phi), tmp*sin(phi), z);
              TVector3 Coll(ConeDir);
              K1.RotateUz(Coll);
              PhotonOnStart.v[0] = K1[0];
              PhotonOnStart.v[1] = K1[1];
              PhotonOnStart.v[2] = K1[2];

          }
        //else it is already set

        //configure  wavelength index and emission time
        PhotonOnStart.time = 0;   //reset time offset to zero
        TGeoNavigator *navigator = detector->GeoManager->GetCurrentNavigator();

        int thisMatIndex;
        TGeoNode* node = navigator->FindNode(PhotonOnStart.r[0], PhotonOnStart.r[1], PhotonOnStart.r[2]);
        if (!node)
        {
            //PhotonOnStart.waveIndex = -1;
            thisMatIndex = detector->top->GetMaterial()->GetIndex(); //get material of the world
            //qWarning() << "Node not found when generating photons (photon sources) - assuming material of the world!"<<thisMatIndex;
        }
        else
            thisMatIndex = node->GetVolume()->GetMaterial()->GetIndex();

        if (!fUseGivenWaveIndex) photonGenerator->GenerateWave(&PhotonOnStart, thisMatIndex);//if directly given wavelength -> waveindex is already set in PhotonOnStart
        photonGenerator->GenerateTime(&PhotonOnStart, thisMatIndex);

        if (scs->ScintType == 2)
        {
            const double dist = (z2 - z1) * RandGen->Rndm();
            PhotonOnStart.r[2] = z1 + dist;
            PhotonOnStart.time = time0 + timeOfDrift;
            if (driftSpeedSecScint != 0)
                PhotonOnStart.time += dist / driftSpeedSecScint;
        }

        PhotonOnStart.SimStat = OneEvent->SimStat;

        photonTracker->TracePhoton(&PhotonOnStart);
    }
    //================================

    //filling scs for secondary
    if (scs->ScintType == 2)
    {
        scs->Points[iPoint].r[2] = z1;
        scs->zStop = z2;
    }
}

bool PointSourceSimulator::FindSecScintBounds(double *r, double & z1, double & z2, double & timeOfDrift, double & driftSpeedInSecScint)
{
    TString VolName;
    TGeoNavigator *navigator = detector->GeoManager->GetCurrentNavigator();

    navigator->SetCurrentPoint(r);
    navigator->SetCurrentDirection(0, 0, 1.0);
    navigator->FindNode(); // TODO check node does not exist ***!!!
    int FoundWhere = 0;  //if found on way up = 1,  if found on way down = -1
    double dTime = 0;
    double iMat = navigator->GetCurrentVolume()->GetMaterial()->GetIndex();
    do
    {
        //going up until exit geometry or find SecScint
        navigator->FindNextBoundaryAndStep();
        double Step = navigator->GetStep();
        double DriftVelocity = detector->MpCollection->getDriftSpeed(iMat);
        iMat = navigator->GetCurrentVolume()->GetMaterial()->GetIndex(); //next mat
        if (DriftVelocity != 0)
            dTime += Step / DriftVelocity;
        VolName = navigator->GetCurrentVolume()->GetName();
        //     qDebug()<<VolName;
        if (VolName == "SecScint")
        {
            FoundWhere = 1;
            timeOfDrift = dTime;
            break;
        }
    }
    while (!navigator->IsOutside());

    if (FoundWhere == 0)
    {
        dTime = 0;
        //      qDebug()<<"Exited geometry on the way up";
        //going down until find SecScint or exit geometry again
        navigator->SetCurrentDirection(0, 0, -1.0);
        do
        {
            navigator->FindNextBoundaryAndStep();
            double Step = navigator->GetStep();
            double DriftVelocity = detector->MpCollection->getDriftSpeed(iMat);
            iMat = navigator->GetCurrentVolume()->GetMaterial()->GetIndex(); //next mat
            if (DriftVelocity != 0)
                dTime += Step / DriftVelocity;
            VolName = navigator->GetCurrentVolume()->GetName();
            //         qDebug()<<VolName;
            if (navigator->IsOutside())
            {
                //             qDebug()<<"SecScint not found for this XY!";
                return false;
            }
        }
        while (VolName != "SecScint");
        FoundWhere = -1;
        timeOfDrift = dTime;
    }

    //  qDebug()<<"Found SecSci on the way down";
    driftSpeedInSecScint = detector->MpCollection->getDriftSpeed(iMat);
    if (FoundWhere == 1) z1 = navigator->GetCurrentPoint()[2];
    else z2 = navigator->GetCurrentPoint()[2];
    navigator->FindNextBoundary(); //does not step - we are still inside SecScint!
    double SecScintWidth = navigator->GetStep();
    if (FoundWhere == 1) z2 = z1 + SecScintWidth;
    else z1 = z2 - SecScintWidth;
    return true;
}

void PointSourceSimulator::OneNode(ANodeRecord & node)
{
    ANodeRecord * thisNode = &node;
    std::unique_ptr<ANodeRecord> outNode(ANodeRecord::createS(1e10, 1e10, 1e10)); // if outside will use this instead of thisNode

    const int numPoints = 1 + thisNode->getNumberOfLinkedNodes();
    OneEvent->clearHits();
    AScanRecord* sr = new AScanRecord();
    sr->Points.Reinitialize(numPoints);
    sr->ScintType = ScintType;

    for (int iPoint = 0; iPoint < numPoints; iPoint++)
    {
        const bool bInside = !(fLimitNodesToObject && !isInsideLimitingObject(thisNode->R));
        if (bInside)
        {
            for (int i=0; i<3; i++) sr->Points[iPoint].r[i] = thisNode->R[i];
            sr->Points[iPoint].energy = (thisNode->NumPhot == -1 ? PhotonsToRun() : thisNode->NumPhot);
        }
        else
        {
            for (int i=0; i<3; i++) sr->Points[iPoint].r[i] =  outNode->R[i];
            sr->Points[iPoint].energy = 0;
        }

        if (!simSettings->fLRFsim)
        {
            GenerateTraceNphotons(sr, thisNode->Time);
        }
        else
        {
            if (bInside)
            {
                // NumPhotElPerLrfUnity - scaling: LRF of 1.0 corresponds to NumPhotElPerLrfUnity photo-electrons
                int numPhots = sr->Points[iPoint].energy;  // photons (before scaling) to generate at this node
                double energy = 1.0 * numPhots / simSettings->NumPhotsForLrfUnity; // NumPhotsForLRFunity corresponds to the total number of photons per event for unitary LRF

                //Generating events
                for (int ipm = 0; ipm < detector->PMs->count(); ipm++)
                {
                    double avSignal = detector->LRFs->getLRF(ipm, thisNode->R) * energy;
                    double avPhotEl = avSignal * simSettings->NumPhotElPerLrfUnity;
                    double numPhotEl = RandGen->Poisson(avPhotEl);

                    double signal = numPhotEl / simSettings->NumPhotElPerLrfUnity;  // back to LRF units
                    OneEvent->PMsignals[ipm] += signal;
                }
            }
            //if outside nothing to do
        }

        //if exists, continue to work with the linked node(s)
        thisNode = thisNode->getLinkedNode();
    }
    while (thisNode);

    if (!simSettings->fLRFsim) OneEvent->HitsToSignal();

    dataHub->Events.append(OneEvent->PMsignals);
    if (simSettings->fTimeResolved)
        dataHub->TimedEvents.append(OneEvent->TimedPMsignals);  //LRF sim for time-resolved will give all zeroes!

    dataHub->Scan.append(sr);
}

bool PointSourceSimulator::isInsideLimitingObject(const double *r)
{
//    TGeoNavigator *navigator = detector->GeoManager->GetCurrentNavigator();
//    navigator->SetCurrentPoint(r);
//    navigator->FindNode();
//    TString VolName = navigator->GetCurrentVolume()->GetName();
//    return (VolName == LimitNodesToObject);

    TGeoNavigator *navigator = detector->GeoManager->GetCurrentNavigator();
    TGeoNode* node = navigator->FindNode(r[0], r[1], r[2]);
    return (node && node->GetVolume()->GetName()==LimitNodesToObject);
}

void PointSourceSimulator::ReserveSpace(int expectedNumEvents)
{
    Simulator::ReserveSpace(expectedNumEvents);
    //if (fUpdateGUI) DotsTGeo->reserve(expectedNumEvents); //no need anymore, they are limited in size now to 1000 entries
}

/******************************************************************************\
*                         Particle Source Simulator                            |
\******************************************************************************/
ParticleSourceSimulator::ParticleSourceSimulator(const DetectorClass *detector, ASimulationManager *simMan, int ID) :
    Simulator(detector, simMan, ID)
{
    if (simMan == 0) qDebug() << "simMan is nullptr" ;
    detector->MpCollection->updateRandomGenForThread(ID, RandGen);

    ParticleTracker = new PrimaryParticleTracker(detector->GeoManager,
                                                 RandGen,                                                 
                                                 detector->MpCollection,
                                                 &ParticleStack,
                                                 &EnergyVector,
                                                 &dataHub->EventHistory,
                                                 dataHub->SimStat,
                                                 ID);
    S1generator = new S1_Generator(photonGenerator, photonTracker, detector->MpCollection, &EnergyVector, &dataHub->GeneratedPhotonsHistory, RandGen);
    S2generator = new S2_Generator(photonGenerator, photonTracker, &EnergyVector, RandGen, detector->GeoManager, detector->MpCollection, &dataHub->GeneratedPhotonsHistory);

    //ParticleSources = new ParticleSourcesClass(detector, RandGen); /too early - do not know yet the particle generator mode
}

ParticleSourceSimulator::~ParticleSourceSimulator()
{
    delete S2generator;
    delete S1generator;
    delete ParticleTracker;
    delete ParticleGun;
    clearParticleStack();
    clearGeneratedParticles(); //if something was not transferred
    clearEnergyVector();
}

bool ParticleSourceSimulator::setup(QJsonObject &json)
{
    if ( !Simulator::setup(json) ) return false;

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

void ParticleSourceSimulator::updateGeoManager()
{
    ParticleTracker->UpdateGeoManager(detector->GeoManager);
    S2generator->UpdateGeoManager(detector->GeoManager);
}

void ParticleSourceSimulator::simulate()
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

void ParticleSourceSimulator::appendToDataHub(EventsDataClass *dataHub)
{
    //qDebug() << "Thread #"<<ID << " PartSim ---> appending data";
    Simulator::appendToDataHub(dataHub);
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

bool ParticleSourceSimulator::standaloneTrackStack(QVector<AParticleRecord *> *particleStack)
{
    if (particleStack->isEmpty())
    {
        ErrorString = "Particle stack is empty!";
        return false;
    }
    //qDebug() << ">Standalone particle stack tracker received stack size:"<<particleStack->size();
    clearParticleStack();
    //qDebug() << ">Cleared stack";
    for (int i=0; i<particleStack->size(); i++)
        ParticleStack.append(particleStack->at(i)->clone());
    //qDebug() << ">Cloned";
    ParticleTracker->setRemoveTracksIfNoEnergyDepo(false);
    //qDebug() << ">Start tracking...";
    return ParticleTracker->TrackParticlesOnStack();
}

bool ParticleSourceSimulator::standaloneGenerateLight(QVector<AEnergyDepositionCell *> *energyVector)
{ 
    if (energyVector->isEmpty())
    {
        ErrorString = "No energy deposition data provided!";
        return false;
    }
    EnergyVector = *energyVector; //copy pointers to EnergyDepositionCells

    EnergyVectorToScan();

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

    dataHub->Events.resize(0);
    dataHub->TimedEvents.resize(0);

    //qDebug() << "Total hits recorded:" << OneEvent->PMhits;

    OneEvent->HitsToSignal();
    dataHub->Events.append(OneEvent->PMsignals);
    if(timeRange != 0) dataHub->TimedEvents.append(OneEvent->TimedPMsignals);
    return true;
}

void ParticleSourceSimulator::hardAbort()
{
    //qDebug() << "HARD abort"<<ID;
    Simulator::hardAbort();

    ParticleGun->abort();
    if (bG4isRunning && G4antsProcess) G4antsProcess->kill();
}

void ParticleSourceSimulator::updateMaxTracks(int maxPhotonTracks, int maxParticleTracks)
{
    ParticleTracker->setMaxTracks(maxParticleTracks);
    photonTracker->setMaxTracks(maxPhotonTracks);
}

#include <algorithm>
void ParticleSourceSimulator::EnergyVectorToScan()
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

void ParticleSourceSimulator::clearParticleStack()
{
    for (int i=0; i<ParticleStack.size(); i++) delete ParticleStack[i];
    ParticleStack.clear();
}

void ParticleSourceSimulator::clearEnergyVector()
{
    for (int i = 0; i < EnergyVector.size(); i++) delete EnergyVector[i];
    EnergyVector.clear();
}

void ParticleSourceSimulator::clearGeneratedParticles()
{
    for (AParticleRecord * p : GeneratedParticles) delete p;
    GeneratedParticles.clear();
}

int ParticleSourceSimulator::chooseNumberOfParticlesThisEvent() const
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

bool ParticleSourceSimulator::choosePrimariesForThisEvent(int numPrimaries)
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

bool ParticleSourceSimulator::generateAndTrackPhotons()
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

#include "atrackingdataimporter.h"
bool ParticleSourceSimulator::geant4TrackAndProcess()
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
    QString receipe = simSettings->G4SimSet.getReceitFileName(ID); // FilePath + QString("receipt-%1.txt").arg(ID);
    QString res;
    bool bOK = LoadTextFromFile(receipe, res);
    if (!bOK)
    {
        ErrorString = "Could not read the receipt file";
        return false;
    }
    if (!res.startsWith("OK"))
    {
        ErrorString = res;
        return false;
    }

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
            // 0   1   2 3 4 5 6
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
    if (simSettings->TrackBuildOptions.bBuildParticleTracks)
    {
        ATrackingDataImporter ti(simSettings->TrackBuildOptions, 0, &tracks);
        QString TrackingFileName = simSettings->G4SimSet.getTracksFileName(ID);
        ErrorString = ti.processFile(TrackingFileName, eventBegin);
        if (!ErrorString.isEmpty()) return false;
    }

    return true;
}

ASimulationManager::ASimulationManager(EventsDataClass* EventsDataHub, DetectorClass* Detector)
{
    this->EventsDataHub = EventsDataHub;
    this->Detector = Detector;

    ParticleSources = new ASourceParticleGenerator(Detector, Detector->RandGen);
    FileParticleGenerator = new AFileParticleGenerator(*Detector->MpCollection);
    ScriptParticleGenerator = new AScriptParticleGenerator(*Detector->MpCollection, Detector->RandGen);
    ScriptParticleGenerator->SetProcessInterval(200);

    Runner = new ASimulatorRunner(Detector, EventsDataHub, this);

    QObject::connect(Runner, &ASimulatorRunner::simulationFinished, this, &ASimulationManager::onSimulationFinished);
    QObject::connect(this, &ASimulationManager::RequestStopSimulation, Runner, &ASimulatorRunner::requestStop, Qt::DirectConnection);

    Runner->moveToThread(&simRunnerThread); //Move to background thread, as it always runs synchronously even in MT
    QObject::connect(&simRunnerThread, &QThread::started, Runner, &ASimulatorRunner::simulate); //Connect thread to simulation start

    //GUI update
    simTimerGuiUpdate.setInterval(1000);
    QObject::connect(&simTimerGuiUpdate, &QTimer::timeout, Runner, &ASimulatorRunner::updateGui, Qt::DirectConnection);
    QObject::connect(Runner, &ASimulatorRunner::updateReady, this, &ASimulationManager::ProgressReport);
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

    clearNodes();

    bool bOK = Runner->setup(json, threads, fFromGui);
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

void ASimulationManager::StopSimulation()
{
    emit RequestStopSimulation();
}

void ASimulationManager::onSimFailedToStart()
{
    fFinished = true;
    fSuccess = false;
    Runner->setFinished();

    emit SimulationFinished();
}

void ASimulationManager::clearEnergyVector()
{
    for (int i=0; i<EnergyVector.size(); i++) delete EnergyVector[i];
    EnergyVector.clear();
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

const QString ASimulationManager::loadNodesFromFile(const QString &fileName)
{
    clearNodes();

    QFile file(fileName);
    if(!file.open(QIODevice::ReadOnly | QFile::Text))
        return "Failed to open file "+fileName;

    QTextStream in(&file);
    ANodeRecord * prevNode = nullptr;
    bool bAppendNext = false;
    while (!in.atEnd())
    {
        QString line = in.readLine();
        if (line.startsWith('#')) continue; //comment
        QStringList sl = line.split(' ', QString::SkipEmptyParts);
        if (sl.isEmpty()) continue; //alow empty lines
        // x y z time num *
        // 0 1 2   3   4  5
        int size = sl.size();
        if (size < 4) return "Unexpected format of line: cannot find x y z t information";
        bool bOK;
        double x = sl.at(0).toDouble(&bOK); if (!bOK) return "Bad format of line: conversion to number failed";
        double y = sl.at(1).toDouble(&bOK); if (!bOK) return "Bad format of line: conversion to number failed";
        double z = sl.at(2).toDouble(&bOK); if (!bOK) return "Bad format of line: conversion to number failed";
        double t = sl.at(3).toDouble(&bOK); if (!bOK) return "Bad format of line: conversion to number failed";
        int    n = -1;
        if (sl.size() > 4)
        {
               n = sl.at(4).toInt(&bOK);
               if (!bOK) return "Bad format of line: conversion to number failed";
        }

        ANodeRecord * node = ANodeRecord::createS(x, y, z, t, n);
        if (bAppendNext) prevNode->addLinkedNode(node);
        else Nodes.push_back(node);
        prevNode = node;
        bAppendNext = (sl.size() > 5 && sl.at(5) == '*');
    }

    file.close();
    return "";
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

    QVector<Simulator *> simulators = Runner->getWorkers();
    if (!simulators.isEmpty() && !fHardAborted)
    {
        if (LastSimType == 0)
        {
            PointSourceSimulator *lastPointSrcSimulator = static_cast< PointSourceSimulator *>(simulators.last());
            EventsDataHub->ScanNumberOfRuns = lastPointSrcSimulator->getNumRuns();

            SiPMpixels = lastPointSrcSimulator->getLastEvent()->SiPMpixels; //only makes sense if there was only 1 event
        }
        else if (LastSimType == 1)
        {
            ParticleSourceSimulator *lastPartSrcSimulator = static_cast< ParticleSourceSimulator *>(simulators.last());
            EnergyVector = lastPartSrcSimulator->getEnergyVector();
            lastPartSrcSimulator->ClearEnergyVectorButKeepObjects(); // to avoid clearing the energy vector cells
        }

        for (Simulator * sim : simulators)
        {
            //Tracks += sim->tracks;
            Tracks.insert( Tracks.end(),
                           std::make_move_iterator(sim->tracks.begin()),
                           std::make_move_iterator(sim->tracks.end()) );
            sim->tracks.clear();  //to avoid delete objects on simulator delete
        }
    }

    //Raimundo's comment: This is part of a hack. Check this function declaration in .h for more details.
    Runner->clearWorkers();

    if (fHardAborted)
        EventsDataHub->clear(); //data are not valid!
    else
    {
        //before was limited to the mode whenlocations are limited to a certain object in the detector geometry
        //scan and custom nodes might have nodes outside of the limiting object - they are still present but marked with 1e10 X and Y true coordinates
        EventsDataHub->purge1e10events(); //purging events with "true" positions x==1e10 && y==1e10
    }

    Detector->BuildDetector(true, true);  // <- still needed on Windows
    //Detector->GeoManager->CleanGarbage();

    if (fStartedFromGui) emit SimulationFinished();

    clearEnergyVector(); // main window copied if needed
    SiPMpixels.clear();  // main window copied if needed
    //qDebug() << "SimManager: Sim finished";
}

