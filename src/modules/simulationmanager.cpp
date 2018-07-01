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
#include "particlesourcesclass.h"
#include "amaterialparticlecolection.h"
#include "asandwich.h"
#include "ageoobject.h"
#include "aphotontracer.h"
#include "apositionenergyrecords.h"
#include "aenergydepositioncell.h"
#include "aparticleonstack.h"
#include "amessage.h"
#include "acommonfunctions.h"
#include "ageomarkerclass.h"
#include "atrackrecords.h"
#include "ajsontools.h"
#include "aconfiguration.h"

#include <QVector>
#include <QTime>
#include <QDebug>

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

    //TGeoNavigator *navigator = simulator->getDetector()->GeoManager->AddNavigator();
    simulator->getDetector()->GeoManager->AddNavigator();

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

ASimulatorRunner::ASimulatorRunner(DetectorClass *detector, EventsDataClass *dataHub, QObject *parent) : QObject(parent)
{
    this->detector = detector;
    this->dataHub = dataHub;
    startTime.start(); //can restart be done without start() ?
    simState = SClean;
    //We don't need it to be secret, just random
    //randGen = new TRandom2(QDateTime::currentMSecsSinceEpoch());
    //randGen = detector->RandGen;
}

ASimulatorRunner::~ASimulatorRunner()
{
    clearWorkers();
    for(int i = 0; i < threads.count(); i++)
        delete threads[i];
    threads.clear();
    //delete randGen;
}

void ASimulatorRunner::setup(QJsonObject &json, int threadCount)
{
  //qDebug() << "\n\n is multi?"<< detector->GeoManager->IsMultiThread();

  if (!json.contains("SimulationConfig"))
    {
      message("Json does not contain simulation config!");
      return;
    }
  QJsonObject jsSimSet = json["SimulationConfig"].toObject();
  totalEventCount = 0;
  lastProgress = 0;
  lastEventsDone = 0;
  lastRefreshTime = 0;
  usPerEvent = 0;
  fStopRequested = false;

  bool fRunThreads = threadCount > 0;
  simSettings.readFromJson(jsSimSet);
  dataHub->clear();
  dataHub->initializeSimStat(detector->Sandwich->MonitorsRecords, simSettings.DetStatNumBins, (simSettings.fWaveResolved ? simSettings.WaveNodes : 0) );
  modeSetup = jsSimSet["Mode"].toString();
  //qDebug() << "-------------";
  //qDebug() << "Setup to run with "<<(fRunTThreads ? "TThreads" : "QThread");
  //qDebug() << "Simulation mode:" << modeSetup;
  //qDebug() << "Monitors:"<<dataHub->SimStat->Monitors.size();

  //qDebug() << "Updating PMs module according to sim settings";
  detector->PMs->configure(&simSettings); //Setup pms module and QEaccelerator if needed
  //qDebug() << "Updating MaterialColecftion module according to sim settings";
  detector->MpCollection->UpdateWavelengthBinning(&simSettings); //update wave-resolved properties of materials and runtime properties for neutrons

  clearWorkers(); //just rebuild them all everytime, it's easier
  threadCount = std::max(threadCount, 1);
  for(int i = 0; i < threadCount; i++)
    {
      TString workerName = "simulationWorker"+TString::Itoa(i, 10);
      Simulator *worker;
      if(modeSetup == "PointSim") //Photon simulator
        worker = new PointSourceSimulator(detector, workerName);
      else //Particle simulator
        worker = new ParticleSourceSimulator(detector, workerName);

      worker->setSimSettings(&simSettings);
      int seed = detector->RandGen->Rndm()*100000;
      worker->setRngSeed(seed);
      worker->setup(jsSimSet);
      worker->initSimStat();

      worker->divideThreadWork(i, threadCount);
      int workerEventCount = worker->getEventCount();
      //Let's not create more than the necessary number of workers, but require at least 1
      if(workerEventCount == 0 && i != 0)
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

QString ASimulatorRunner::getErrorMessages() const
{
    QString msg;
    if(simState != SFinished)
        return "Simulation not finished yet";
    if(fStopRequested)
        return "Simulation stopped by user";
    QTextStream msgBuilder(&msg);
    for(int i = 0; i < workers.count(); i++)
    {
        if(workers[i]->wasSuccessful()) continue;
        msgBuilder<<"Thread "<<i<<"/"<<workers.count()<<": "<<workers[i]->getErrorString()<<endl;
    }
    return msg;
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
    emit updateReady(0, 0);
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
            //  qDebug() << "Asking thread"<<(i+1)<<" to join...";
            threads[i]->join();
            //  qDebug() <<"Thread"<<(i+1)<<"joined";
          }
#endif

       for (int i=0; i<threads.size(); i++) delete threads[i];
       threads.clear();
    }
    simState = SFinished;

    progress = 100;
    usPerEvent = startTime.elapsed() / (double)totalEventCount;
    emit updateReady(100.0, usPerEvent);

    //Dump data in dataHub
    dataHub->fSimulatedData = true;
    dataHub->LastSimSet = simSettings;
    for(int i = 0; i < workers.count(); i++)
        workers[i]->appendToDataHub(dataHub);

    //qDebug()<<"Simulation finished!";

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
Simulator::Simulator(const DetectorClass *detector, const TString &nameID)
{
    this->nameID = nameID;
    this->detector = detector;
    this->simSettings = 0;
    eventBegin = 0;
    eventEnd = 0;
    progress = 0;
    RandGen = new TRandom2();    
    dataHub = new EventsDataClass(nameID);    
    OneEvent = new AOneEvent(detector->PMs, RandGen, dataHub->SimStat);
    photonGenerator = new Photon_Generator(detector, RandGen);
    photonTracker = new APhotonTracer(detector->GeoManager, RandGen, detector->MpCollection, detector->PMs, &detector->Sandwich->GridRecords);
}

Simulator::~Simulator()
{
    delete photonTracker;
    delete photonGenerator;
    delete OneEvent;
    dataHub->Events.resize(0);
    dataHub->TimedEvents.resize(0);
    dataHub->Scan.resize(0);
    delete dataHub;
    delete RandGen;
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
    fStopRequested = true;
}

void Simulator::divideThreadWork(int threadId, int threadCount)
{
    int totalEventCount = getTotalEventCount();
    int nodesPerThread = totalEventCount / threadCount;
    int remainingNodes = totalEventCount % threadCount;
    //The first <remainingNodes> threads get an extra node each
    eventBegin = threadId*nodesPerThread + std::min(threadId, remainingNodes);
    eventEnd = eventBegin + nodesPerThread + (threadId >= remainingNodes ? 0 : 1);
    //qDebug()<<"Thread"<<(threadId+1)<<"/"<<threadCount<<": eventBegin"<<eventBegin<<": eventEnd"<<eventEnd<<": eventCount"<<getEventCount();
}

bool Simulator::setup(QJsonObject &json)
{
    fUpdateGUI = json["DoGuiUpdate"].toBool();
    fBuildPhotonTracks = fUpdateGUI && simSettings->fBuildPhotonTracks;

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
    dataHub->Events << this->dataHub->Events;
    dataHub->TimedEvents << this->dataHub->TimedEvents;
    dataHub->Scan << this->dataHub->Scan;

    dataHub->SimStat->AppendSimulationStatistics(this->dataHub->SimStat);
}

void Simulator::ReserveSpace(int expectedNumEvents)
{
    dataHub->Events.reserve(expectedNumEvents);
    if (simSettings->fTimeResolved) dataHub->TimedEvents.reserve(expectedNumEvents);
    dataHub->Scan.reserve(expectedNumEvents);
}

/******************************************************************************\
*                          Point Source Simulator                              |
\******************************************************************************/
PointSourceSimulator::PointSourceSimulator(const DetectorClass *detector, const TString &nameID) :
    Simulator(detector, nameID)
{  
    CustomHist = 0;
    totalEventCount = 0;
    DotsTGeo = new QVector<DotsTGeoStruct>();
}

PointSourceSimulator::~PointSourceSimulator()
{
  if (CustomHist) delete CustomHist;
  delete DotsTGeo;
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
    if (fUpdateGUI) DotsTGeo->clear();

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
        CustomHist = new TH1I("hPhotDistr"+nameID,"Photon distribution", size-1, xx);
        for (int i = 1; i<size+1; i++)  CustomHist->SetBinContent(i, yy[i-1]);
        CustomHist->GetIntegral(); //will be thread safe after this
        delete[] xx;
        delete[] yy;
    }

    //reading wavelength/decay options
    QJsonObject wdjson = js["WaveTimeOptions"].toObject();
    int matMode = wdjson["Direct_Material"].toInt(); //0-direct, 1-from material, 2 - automatic
    fDirectGeneration = ( matMode == 0 );
    fAutomaticWaveTime = ( matMode == 2);

    PhotonOnStart.waveIndex = wdjson["WaveIndex"].toInt();
    //PhotonOnStart.wavelength = simSettings->WaveFrom + simSettings->WaveStep*PhotonOnStart.waveIndex;
    if (!simSettings->fWaveResolved) PhotonOnStart.waveIndex = -1;
    DecayTime = wdjson["DecayTime"].toDouble();
    iMatIndex = wdjson["Material"].toInt();

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

    //Read noise options
    QJsonObject njson = js["BadEventOptions"].toObject();
    fBadEventsOn = njson["BadEvents"].toBool();
    BadEventTotalProbability = njson["Probability"].toDouble();
    SigmaDouble = njson["SigmaDouble"].toDouble();
    QJsonArray njsonArr = njson["NoiseArray"].toArray();
    BadEventConfig.clear();
    for (int iChan=0; iChan<njsonArr.size(); iChan++)
    {
        ScanFloodStructure s;
        QJsonObject chanjson = njsonArr[iChan].toObject();
        s.Active = chanjson["Active"].toBool();
        s.Description = chanjson["Description"].toString();
        s.Probability = chanjson["Probability"].toDouble();
        s.AverageValue = chanjson["AverageValue"].toDouble();
        s.Spread = chanjson["Spread"].toDouble();
        BadEventConfig.append(s);
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
            //totalEventCount = simOptions["NumberNodes"].toInt();//progress reporting knows we do NumRuns per each node
            totalEventCount = simOptions["Nodes"].toArray().size();//progress reporting knows we do NumRuns per each node
            break;
        default:
            ErrorString = "Unknown or not implemented simulation mode: "+QString().number(PointSimMode);
            return false;
    }
    return true;
}

void PointSourceSimulator::simulate()
{
    ReserveSpace(getEventCount());
    fStopRequested = false;
    switch (PointSimMode)
    {
        case 0: fSuccess = SimulateSingle(); break;
        case 1: fSuccess = SimulateRegularGrid(); break;
        case 2: fSuccess = SimulateFlood(); break;
        case 3: fSuccess = SimulateCustomNodes(); break;
        default: fSuccess = false; break;
    }
}

void PointSourceSimulator::appendToDataHub(EventsDataClass *dataHub)
{
    Simulator::appendToDataHub(dataHub);
    dataHub->ScanNumberOfRuns = this->NumRuns;
}

bool PointSourceSimulator::SimulateSingle()
{
    double r[3]; //node position
    r[0] = simOptions["SingleX"].toDouble();
    r[1] = simOptions["SingleY"].toDouble();
    r[2] = simOptions["SingleZ"].toDouble();

    //root bug fix
    if (r[0] == 0 && r[1] == 0 && r[2] == 0)
    {
        r[0] = 0.001;
        r[1] = 0.001;
    }

    int eventsToDo = eventEnd - eventBegin; //runs are split between the threads, since node position is fixed for all
    double updateFactor = (eventsToDo<1) ? 100.0 : 100.0/eventsToDo;
    progress = 0;
    eventCurrent = 0;
    for (int irun = 0; irun<eventsToDo; ++irun)
      {
        OneNode(r);
        eventCurrent = irun;
        progress = irun * updateFactor;
        if(fStopRequested) return false;
      }
    if (fUpdateGUI) addLastScanPointToMarkers();
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
        /*QJsonObject adjson = rsdataArr[ic].toObject();
        RegGridStep[ic][0] = adjson["dX"].toDouble(0);
        RegGridStep[ic][1] = adjson["dY"].toDouble(0);
        RegGridStep[ic][2] = adjson["dZ"].toDouble(0);
        RegGridNodes[ic] = adjson["Nodes"].toInt(1); //1 is disabled axis
        RegGridFlagPositive[ic] = adjson["Option"].toInt(1);*/
    }

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
                double r[3];
                for (int i=0; i<3; i++) r[i] = RegGridOrigin[i];
                //shift from the origin
                for (int axis=0; axis<3; axis++)
                { //going axis by axis
                    double ioffset = 0;
                    if (!RegGridFlagPositive[axis]) ioffset = -0.5*( RegGridNodes[axis] - 1 );
                    for (int i=0; i<3; i++) r[i] += (ioffset + iAxis[axis]) * RegGridStep[axis][i];
                }
                if (fLimitNodesToObject && !isInsideLimitingObject(r))
                  { //shift to unrealistic position to supress this mode
                    r[0] = 1e10;
                    r[1] = 1e10;
                    r[2] = 1e10;
                  }

                //root bug fix
                if (r[0] == 0 && r[1] == 0 && r[2] == 0) r[0] = 0.001;

                //running this node              
                for (int irun = 0; irun<NumRuns; irun++)
                  {
                    OneNode(r);
                    eventCurrent++;
                    progress = eventCurrent * updateFactor;
                    if(fStopRequested) return false;
                  }
                if (fUpdateGUI) addLastScanPointToMarkers(false);
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
    int nodeCount = (eventEnd - eventBegin);
    eventCurrent = 0;
    double updateFactor = 100.0 / (NumRuns*nodeCount);
    for (int inode = 0; inode < nodeCount; inode++)
    {
        if(fStopRequested) return false;

        //choosing node coordinates
        double r[3];
        r[0] = Xfrom + (Xto-Xfrom)*RandGen->Rndm();
        r[1] = Yfrom + (Yto-Yfrom)*RandGen->Rndm();
        //running this node
        if (Shape == 1)
        {
            double r2  = (r[0] - CenterX)*(r[0] - CenterX) + (r[1] - CenterY)*(r[1] - CenterY);
            if ( r2 > Rad2out || r2 < Rad2in )
            {
                inode--;
                continue;
            }
        }
        if (Zoption == 0) r[2] = Zfixed;
        else r[2] = Zfrom + (Zto-Zfrom)*RandGen->Rndm();
        if (fLimitNodesToObject && !isInsideLimitingObject(r))
        {
            inode--;
            continue;
        }

        for (int irun = 0; irun<NumRuns; irun++)
          {
            OneNode(r);            
            eventCurrent++;
            progress = eventCurrent * updateFactor;
            if(fStopRequested) return false;
          }
        if (fUpdateGUI) addLastScanPointToMarkers();
    }
    return true;
}

bool PointSourceSimulator::SimulateCustomNodes()
{
  QJsonArray arr;
  arr = simOptions["Nodes"].toArray();

  int nodeCount = (eventEnd - eventBegin);
  int currentNode = eventBegin;
  eventCurrent = 0;
  double updateFactor = 100.0 / ( NumRuns*nodeCount );
  for (int inode = 0; inode < nodeCount; inode++)
    {
      double r[3];
      QJsonArray el = arr[currentNode].toArray();
      if (el.size()<3)
        {
          r[0] = 1.23456789;
          r[1] = 1.23456789;
          r[2] = 1.23456789;
          qDebug()<<"Custom Node"<<currentNode<<"is not (x,y,z).";
        }
      else
        {
          r[0] = el[0].toDouble();
          r[1] = el[1].toDouble();
          r[2] = el[2].toDouble();
        }

      //root bug fix
      if (r[0] == 0 && r[1] == 0 && r[2] == 0) r[0] = 0.001;

      if (fLimitNodesToObject && !isInsideLimitingObject(r))
        { //shift node coordinates to unrealistic position to suppress it
          r[0] = 1e10;
          r[1] = 1e10;
          r[2] = 1e10;
          qDebug()<<"Point is outside of priScint";
        }

      for (int irun = 0; irun<NumRuns; irun++)
        {
          OneNode(r); //takes care of the number of runs
          eventCurrent++;
          progress = eventCurrent * updateFactor;
          if(fStopRequested) return false;
        }
      if (fUpdateGUI) addLastScanPointToMarkers();
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

void PointSourceSimulator::GenerateTraceNphotons(AScanRecord *scs, int iPoint)
//scs contains coordinates and number of photons to run
//iPoint - number of this scintillation center (can be not zero when e.g. double events are simulated)
{
    //if secondary scintillation, finding the secscint boundaries
    double z1, z2;
    if (scs->ScintType == 2)
    {
        if (!FindSecScintBounds(scs->Points[iPoint].r, &z1, &z2))
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
        PhotonOnStart.time = 0;
        if (fDirectGeneration) //directly given properties
        {
            //Wavelength and index are already set
            if (simSettings->fTimeResolved) PhotonOnStart.time = RandGen->Exp(DecayTime);
        }
        else if (fAutomaticWaveTime) //material according to to emission position
        {
            TGeoNavigator *navigator = detector->GeoManager->GetCurrentNavigator();
            TGeoNode* node = navigator->FindNode(PhotonOnStart.r[0], PhotonOnStart.r[1], PhotonOnStart.r[2]);
            if (!node) PhotonOnStart.waveIndex = -1;
            else
            {
                int thisMatIndex = node->GetVolume()->GetMaterial()->GetIndex();
                photonGenerator->GenerateWaveTime(&PhotonOnStart, thisMatIndex);
            }
        }
        else  //specific material
        {
            photonGenerator->GenerateWaveTime(&PhotonOnStart, iMatIndex);
        }
        if (scs->ScintType == 2) PhotonOnStart.r[2] = z1 + (z2-z1)*RandGen->Rndm();

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

    //if (fUpdateGUI) DotsTGeo->append(DotsTGeoStruct(scs->Points[iPoint].r, kBlack)); //moved to particular generation methods
}

bool PointSourceSimulator::FindSecScintBounds(double *r, double *z1, double *z2)
{
    TString VolName;
    TGeoNavigator *navigator = detector->GeoManager->GetCurrentNavigator();

    navigator->SetCurrentPoint(r);
    navigator->SetCurrentDirection(0, 0, 1.0);
    navigator->FindNode();
    int FoundWhere = 0;  //if found on way up = 1,  if found on way down = -1
    do
    {
        //going up until exit geometry or find SecScint
        navigator->FindNextBoundaryAndStep();
        VolName = navigator->GetCurrentVolume()->GetName();
        //     qDebug()<<VolName;
        if (VolName == "SecScint")
        {
            FoundWhere = 1;
            break;
        }
    }
    while (!navigator->IsOutside());

    if (FoundWhere == 0)
    {
        //      qDebug()<<"Exited geometry on the way up";
        //going down until find SecScint or exit geometry again
        navigator->SetCurrentDirection(0, 0, -1.0);
        do
        {
            navigator->FindNextBoundaryAndStep();
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
    }

    //  qDebug()<<"Found SecSci on the way down";
    if (FoundWhere == 1) *z1 = navigator->GetCurrentPoint()[2];
    else *z2 = navigator->GetCurrentPoint()[2];
    navigator->FindNextBoundary(); //does not step - we are still inside SecScint!
    double SecScintWidth = navigator->GetStep();
    if (FoundWhere == 1) *z2 = *z1 + SecScintWidth;
    else *z1 = *z2 - SecScintWidth;
    return true;
}

void PointSourceSimulator::OneNode(double *r)
{
  if (simSettings->fLRFsim)
    {
      doLRFsimulatedEvent(r); //note: simulation of "bad" events disabled in this case!
      return;
    }

  AScanRecord* scs = new AScanRecord();
  scs->ScintType = ScintType;
  scs->Points[0].r[0] = r[0];
  scs->Points[0].r[1] = r[1];
  scs->Points[0].r[2] = r[2];

  OneEvent->clearHits(); //cleaning the PMhits
  scs->Points[0].energy = PhotonsToRun(); //number of photons to run

  bool fNormal = true;  //true - proceed as it is event without noise
  if (fBadEventsOn) // *** !!! to change hit manipulations to OneEvent
    {
      double random = RandGen->Rndm();
      if ( random > BadEventTotalProbability); //noise was NOT triggered - proceeding with normal event
      else  //noise was triggered
        {
          scs->GoodEvent = false;
          int ichan; //which noise channel?
          //              qDebug()<<"Number of noice mechanisms:"<<ScanFloodNoise.size();
          for (ichan = 0; ichan<BadEventConfig.size(); ichan++)
            {
              if (!BadEventConfig[ichan].Active) continue;
              if (random <= BadEventConfig[ichan].Probability) break; //this one is triggered
              random -= BadEventConfig[ichan].Probability;
            }
          //qDebug()<<"Noise channel chosen:"<<ichan;
          scs->EventType = ichan;

          //doing bad event
          switch (ichan)
            {
            case 0:  //just signal in a random PM
              {
                int ipm = RandGen->Rndm()*detector->PMs->count();
                //                  qDebug()<<"noise in PM#"<<ipm;
                int hits = RandGen->Gaus(BadEventConfig[ichan].AverageValue, BadEventConfig[ichan].Spread);
                OneEvent->addHits(ipm, hits);
                if (simSettings->fTimeResolved)
                  {
                    //int itime = RandGen->Rndm()*detector->PMs->getTimeBins();
                    int itime = RandGen->Rndm() * simSettings->TimeBins;
                    OneEvent->addTimedHits( itime, ipm, hits);
                  }
                fNormal = false;
                scs->Points[0].energy = 0;
                break;
              }
            case 1:  //signal in a random PM on top of normal signal
              {
                int ipm = RandGen->Rndm()*detector->PMs->count();
                //                    qDebug()<<"noise in PM#"<<ipm;
                int hits = RandGen->Gaus(BadEventConfig[ichan].AverageValue, BadEventConfig[ichan].Spread);
                OneEvent->addHits(ipm, hits);
                if (simSettings->fTimeResolved)
                  {
                    //int itime = RandGen->Rndm()*detector->PMs->getTimeBins();
                    int itime = RandGen->Rndm() * simSettings->TimeBins;
                    OneEvent->addTimedHits( itime, ipm, hits);
                  }
                break; //going "normal" after this
              }
            case 2:  //random signals in all PMs
              {
                for (int ipm = 0; ipm < detector->PMs->count(); ipm++)
                  {
                    int hits = RandGen->Gaus(BadEventConfig[ichan].AverageValue, BadEventConfig[ichan].Spread);
                    OneEvent->addHits(ipm, hits);
                    if (simSettings->fTimeResolved)
                      {
                        //int itime = RandGen->Rndm()*detector->PMs->getTimeBins();
                        int itime = RandGen->Rndm() * simSettings->TimeBins;
                        OneEvent->addTimedHits( itime, ipm, hits);
                      }
                  }
                fNormal = false;
                scs->Points[0].energy = 0;
                break;
              }
            case 3:  //random signals in all PMs + normal event
              {
                for (int ipm = 0; ipm < detector->PMs->count(); ipm++)
                  {
                    int hits = RandGen->Gaus(BadEventConfig[ichan].AverageValue, BadEventConfig[ichan].Spread);
                    OneEvent->addHits(ipm, hits);
                    if (simSettings->fTimeResolved)
                      {
                        //int itime = RandGen->Rndm()*detector->PMs->getTimeBins();
                        int itime = RandGen->Rndm() * simSettings->TimeBins;
                        OneEvent->addTimedHits( itime, ipm, hits);
                      }
                  }
                break; //going "normal" after this
              }

            case 4:  //double event
              {
                fNormal = false;
                //first event
                GenerateTraceNphotons(scs);
                if (ScintType == 2)
                  if (scs->Points[0].r[2] == scs->zStop) break; //if was outside of SecScint area, removing bad event status - this will be 0 hits "outside" event

                //second event
                scs->Points.AddPoint(0,0,0, PhotonsToRun()); //adding second point in scan object
                GenerateFromSecond(scs);
                break;
              }

            case 5:  //double event, shared energy
              {
                fNormal = false;
                //splitting #of photons between two positions
                int num1 = scs->Points[0].energy * RandGen->Gaus(BadEventConfig[ichan].AverageValue, BadEventConfig[ichan].Spread);
                if (num1 > scs->Points[0].energy) num1 = scs->Points[0].energy;
                if (num1 <0) num1 = 0;
                int num2 = scs->Points[0].energy - num1;
                //qDebug()<<"num1,num2 ="<<num1<<num2;

                //first point
                scs->Points[0].energy = num1;
                GenerateTraceNphotons(scs);
                if (ScintType == 2)
                  if (scs->Points[0].r[2] == scs->zStop) break;//if was outside of SecScint area, rmoving bad event status - this will be 0 hits "outside" event

                //second point
                scs->Points.AddPoint(0,0,0, num2); //adding second point in scan object
                GenerateFromSecond(scs);
                break;
              }
            }
        }
    }

  if (fNormal) GenerateTraceNphotons(scs); //no noise or additive noise

  OneEvent->HitsToSignal();
  dataHub->Events.append(OneEvent->PMsignals);
  if (simSettings->fTimeResolved) dataHub->TimedEvents.append(OneEvent->TimedPMsignals);
  dataHub->Scan.append(scs);
}

void PointSourceSimulator::doLRFsimulatedEvent(double *r)
{
  // NumPhotElPerLrfUnity - scaling: LRF of 1.0 corresponds to NumPhotElPerLrfUnity photo-electrons

  int numPhots = PhotonsToRun();  // photons (before scaling) to generate at this node
  double energy = 1.0 * numPhots / simSettings->NumPhotsForLrfUnity; // NumPhotsForLRFunity corresponds to the total number of photons per event for unitary LRF

  //Generating events
  QVector<float> ev;
  for (int ipm = 0; ipm < detector->PMs->count(); ipm++)
    {
      double avSignal = detector->LRFs->getLRF(ipm, r) * energy;
      double avPhotEl = avSignal * simSettings->NumPhotElPerLrfUnity;
      double numPhotEl = RandGen->Poisson(avPhotEl);

      double signal = numPhotEl / simSettings->NumPhotElPerLrfUnity;  // back to LRF units
      ev << signal;
    }
  dataHub->Events.append(ev); // *** NO TIMED EVENTS !!!

  //Populating Scan
  AScanRecord* ss = new AScanRecord();
  ss->Points[0].r[0] = r[0];
  ss->Points[0].r[1] = r[1];
  ss->Points[0].r[2] = r[2];
  ss->Points[0].energy = numPhots;
  dataHub->Scan.append(ss);

  if (fUpdateGUI) DotsTGeo->append(DotsTGeoStruct(r));
}

void PointSourceSimulator::GenerateFromSecond(AScanRecord *scs)
{
    do
    {
        do
        {
            scs->Points[1].r[0] = scs->Points[0].r[0] + RandGen->Gaus(0, SigmaDouble);
            scs->Points[1].r[1] = scs->Points[0].r[1] + RandGen->Gaus(0, SigmaDouble);
            scs->Points[1].r[2] = scs->Points[0].r[2] + RandGen->Gaus(0, SigmaDouble);
        }
        while (fLimitNodesToObject && !isInsideLimitingObject(scs->Points[1].r));

        GenerateTraceNphotons(scs, 1); //with secondary, actually no tracing if not found!
        //qDebug()<<scs->r[0]<<scs->r[1]<<scs->r[2]<<"      "<<scs->zStop;
    }
    while (ScintType == 2 && scs->Points[1].r[2] == scs->zStop); //when not equal, SecScint volume was found
}

bool PointSourceSimulator::isInsideLimitingObject(double *r)
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

void PointSourceSimulator::addLastScanPointToMarkers(bool fLimitNumber) //we do not want to limit for scan, can confuse the user
{
  if (dataHub->Scan.isEmpty()) return;
  if (fLimitNumber && DotsTGeo->size()>1000) return; /// *** absolute number!

  AScanRecord* sc = dataHub->Scan.last();
  for (int i=0; i<sc->Points.size(); i++)
    DotsTGeo->append(DotsTGeoStruct(sc->Points[i].r));
}

/******************************************************************************\
*                         Particle Source Simulator                            |
\******************************************************************************/
ParticleSourceSimulator::ParticleSourceSimulator(const DetectorClass *detector, const TString &nameID) :
    Simulator(detector, nameID)
{
    totalEventCount = 0;
    ParticleTracker = new PrimaryParticleTracker(detector->GeoManager,
                                                 RandGen,                                                 
                                                 detector->MpCollection,
                                                 &ParticleStack,
                                                 &EnergyVector,
                                                 &dataHub->EventHistory,
                                                 dataHub->SimStat);
    S1generator = new S1_Generator(photonGenerator, photonTracker, detector->MpCollection, &EnergyVector, &dataHub->GeneratedPhotonsHistory, RandGen);
    S2generator = new S2_Generator(photonGenerator, photonTracker, &EnergyVector, RandGen, detector->GeoManager, detector->MpCollection, &dataHub->GeneratedPhotonsHistory);

    ParticleSources = new ParticleSourcesClass(detector, nameID);
}

ParticleSourceSimulator::~ParticleSourceSimulator()
{
    delete S2generator;
    delete S1generator;
    delete ParticleTracker;
    delete ParticleSources;
    clearParticleStack();
    if (EnergyVector.size() > 0)
    {
        for (int i = 0; i < EnergyVector.size(); i++) delete EnergyVector[i];
        EnergyVector.resize(0);
    }
}

bool ParticleSourceSimulator::setup(QJsonObject &json)
{
    if(!Simulator::setup(json))
        return false;

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
    //QJsonObject cjs = js[controlOptions].toObject();
    QJsonObject cjs = js["SourceControlOptions"].toObject();
    if (cjs.isEmpty())
    {
        ErrorString = "Json sent to simulator does not contain proper sim config data!";
        return false;
    }
    totalEventCount = cjs["EventsToDo"].toInt();
    fAllowMultiple = cjs["AllowMultipleParticles"].toBool();
    AverageNumParticlesPerEvent = cjs["AverageParticlesPerEvent"].toDouble();
    TypeParticlesPerEvent = cjs["TypeParticlesPerEvent"].toInt();
    fDoS1 = cjs["DoS1"].toBool();
    fDoS2 = cjs["DoS2"].toBool();
    fBuildParticleTracks = cjs["ParticleTracks"].toBool();
    fIgnoreNoHitsEvents = false; //compatibility
    parseJson(cjs, "IgnoreNoHitsEvents", fIgnoreNoHitsEvents);
    fIgnoreNoDepoEvents = true; //compatibility
    parseJson(cjs, "IgnoreNoDepoEvents", fIgnoreNoDepoEvents);

    //particle sources
    if (js.contains("ParticleSources"))
    {
        ParticleSources->readFromJson(js);
        ParticleSources->Init();
    }

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
    //watchdogs
    int NumSources = ParticleSources->size();
    if (NumSources == 0)
    {
        ErrorString = "Particle sources are not defined!";
        fSuccess = false;
        return;
    }
    if (ParticleSources->getTotalActivity() == 0)
    {
        ErrorString = "Total activity of sources is zero!";
        fSuccess = false;
        return;
    }
    if (ParticleStack.size()>0) // *** to be moved to "ClearData"?
    {
        for (int i=0; i<ParticleStack.size(); i++) delete ParticleStack[i];
        ParticleStack.resize(0);
    }

    ReserveSpace(getEventCount());
    fStopRequested = false;

    //Simulation
    double updateFactor = 100.0 / ( eventEnd - eventBegin );
    for (eventCurrent = eventBegin; eventCurrent < eventEnd; eventCurrent++)
    {
        if (fStopRequested) break;

        //clear EnergyVector to receive new data
        if (EnergyVector.size() > 0)
        {
            for (int i = 0; i < EnergyVector.size(); i++) delete EnergyVector[i];
            EnergyVector.resize(0);
        }

        //adding particles to stack
        double time = 0;

        //how many particles to run this event?
        int ParticleRunsThisEvent = 1;
        if (fAllowMultiple)
        {
            if (TypeParticlesPerEvent == 0) ParticleRunsThisEvent = std::round(AverageNumParticlesPerEvent);
            else                            ParticleRunsThisEvent = RandGen->Poisson(AverageNumParticlesPerEvent);

            if(ParticleRunsThisEvent < 1) ParticleRunsThisEvent = 1;
        }
        //qDebug()<<"----particle runs this event: "<<ParticleRunsThisEvent;

        //cycle by the particles this event
        for(int iRun = 0; iRun < ParticleRunsThisEvent; iRun++)
        {
            //generating one event
            QVector<GeneratedParticleStructure>* GP = ParticleSources->GenerateEvent();
            //adding particles to the stack
            for (int iPart = 0; iPart < GP->size(); iPart++ )
            {
                if(iRun > 0 && timeRange != 0)
                    time = timeFrom + timeRange*RandGen->Rndm(); //added TimeFrom 05/02/2015

                const GeneratedParticleStructure &part = GP->at(iPart);
                ParticleStack.append(new AParticleOnStack(part.ParticleId,
                                                         part.Position[0], part.Position[1], part.Position[2],
                                                         part.Direction[0], part.Direction[1], part.Direction[2],
                                                         time, part.Energy));
            }
            //clear and delete QVector with generated event
            GP->clear();
            delete GP;
        } //event prepared
        //   qDebug()<<"event!  Particle stack length:"<<ParticleStack.size();

        ParticleTracker->TrackParticlesInStack(eventCurrent);
        //energy vector is ready

        //if it is an empty event, ignore it!
        if (EnergyVector.isEmpty() && fIgnoreNoDepoEvents)
          {
            eventCurrent--;
            continue;
          }

        OneEvent->clearHits();
        if (fDoS1 && !S1generator->Generate())
        {
            ErrorString = "Error executing S1 generation!";
            fSuccess = false;
            return;
        }

        if (fDoS2 && !S2generator->Generate())
        {
            ErrorString = "Error executing S2 generation!";
            fSuccess = false;
            return;
        }

        if (fIgnoreNoHitsEvents)
          if (OneEvent->isHitsEmpty())
            {
              eventCurrent--;
              continue;
            }

        if (simSettings->fLRFsim) ;
        else OneEvent->HitsToSignal();

        dataHub->Events.append(OneEvent->PMsignals);
        if (timeRange != 0) dataHub->TimedEvents.append(OneEvent->TimedPMsignals);

        EnergyVectorToScan(); //save actual positions

        progress = (eventCurrent - eventBegin + 1) * updateFactor;
    } //all events finished

    fSuccess = true;
}

void ParticleSourceSimulator::appendToDataHub(EventsDataClass *dataHub)
{
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

bool ParticleSourceSimulator::standaloneTrackStack(QVector<AParticleOnStack *> *particleStack)
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
    return ParticleTracker->TrackParticlesInStack();
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

    //scanning the energy deposition vector and put info in ScanData
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
}

void ParticleSourceSimulator::clearParticleStack()
{
  for (int i=0; i<ParticleStack.size(); i++) delete ParticleStack[i];
  ParticleStack.clear();
}

ASimulationManager::ASimulationManager(EventsDataClass* EventsDataHub, DetectorClass* Detector)
{
    this->EventsDataHub = EventsDataHub;
    this->Detector = Detector;

    ParticleSources = new ParticleSourcesClass(Detector);
    //qDebug() << "->Container for particle sources created and configured";

    Runner = new ASimulatorRunner(Detector, EventsDataHub);

    QObject::connect(Runner, SIGNAL(simulationFinished()), this, SLOT(onSimulationFinished()));
    QObject::connect(this, SIGNAL(RequestStopSimulation()), Runner, SLOT(requestStop()), Qt::DirectConnection);

    Runner->moveToThread(&simThread); //Move to background thread, as it always runs synchronously even in MT
    QObject::connect(&simThread, SIGNAL(started()), Runner, SLOT(simulate())); //Connect thread to simulation start

    simTimerGuiUpdate.setInterval(1000);
    QObject::connect(&simTimerGuiUpdate, SIGNAL(timeout()), Runner, SLOT(updateGui()), Qt::DirectConnection);

    QObject::connect(Runner, &ASimulatorRunner::updateReady, this, &ASimulationManager::ProgressReport);
}

ASimulationManager::~ASimulationManager()
{
    delete Runner;
    ASimulationManager::Clear();

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

    Runner->setup(json, threads);

    simThread.start();
    simTimerGuiUpdate.start();
}

void ASimulationManager::StopSimulation()
{
    emit RequestStopSimulation();
}

void ASimulationManager::Clear()
{
    clearGeoMarkers();
    clearEnergyVector();
    clearTracks();
    SiPMpixels.clear();
}

void ASimulationManager::clearGeoMarkers()
{
    for (int i=0; i<GeoMarkers.size(); i++) delete GeoMarkers[i];
    GeoMarkers.clear();
}

void ASimulationManager::clearEnergyVector()
{
    for (int i=0; i<EnergyVector.size(); i++) delete EnergyVector[i];
    EnergyVector.clear();
}

void ASimulationManager::clearTracks()
{
    for (int i=0; i<Tracks.size(); i++) delete Tracks[i];
    Tracks.clear();
}

void ASimulationManager::onSimulationFinished()
{
    //qDebug() << "after finish, manager has monitors:"<< EventsDataHub->SimStat->Monitors.size();
    simTimerGuiUpdate.stop();
    QThread::usleep(5000); // to make sure update cycle is finished
    simThread.quit();

    fFinished = true;
    fSuccess = Runner->wasSuccessful();

    //in Raimundo's code on simulation fail workers were not cleared!
    ASimulationManager::Clear();

    if (Runner->modeSetup == "PointSim") LastSimType = 0; //was Point sources sim
    else if (Runner->modeSetup == "SourceSim") LastSimType = 1; //was Particle sources sim
    else LastSimType = -1;

    QVector<Simulator *> simulators = Runner->getWorkers();
    if (!simulators.isEmpty())
      {
        if (LastSimType == 0)
          {
            PointSourceSimulator *lastPointSrcSimulator = static_cast< PointSourceSimulator *>(simulators.last());
            EventsDataHub->ScanNumberOfRuns = lastPointSrcSimulator->getNumRuns();
            LastPhoton = *lastPointSrcSimulator->getLastPhotonOnStart();

            if (fStartedFromGui)
              {
                for (int i = 0; i < simulators.count(); i++)
                  {
                    const PointSourceSimulator *simulator = static_cast<const PointSourceSimulator *>(simulators[i]);
                    const QVector<DotsTGeoStruct>* dots = simulator->getDotsTGeo();
                    GeoMarkerClass* marks = new GeoMarkerClass("Nodes", 6, 2, kBlack);
                    for (int i=0; i<dots->size(); i++)
                      marks->SetNextPoint(dots->at(i).r[0], dots->at(i).r[1], dots->at(i).r[2]);
                    GeoMarkers.append(marks);
                  }
                SiPMpixels = lastPointSrcSimulator->getLastEvent()->SiPMpixels; //only makes sense if there was only 1 event
              }
          }
        if (LastSimType == 1)
          {
             ParticleSourceSimulator *lastPartSrcSimulator = static_cast< ParticleSourceSimulator *>(simulators.last());
             EnergyVector = lastPartSrcSimulator->getEnergyVector();
             lastPartSrcSimulator->ClearEnergyVectorButKeepObjects(); // to avoid clearing the energy vector cells
          }

        for(int iSim = 0; iSim<simulators.count(); iSim++)
          {
             Tracks += simulators[iSim]->tracks;
             simulators[iSim]->tracks.clear();
          }
      }

    //Raimundo's comment: This is part of a hack. Check this function declaration in .h for more details.
    Runner->clearWorkers();

    //before was limited to the mode whenlocations are limited to a certain object in the detector geometry
    //scan and custom nodes might have nodes outside of the limiting object - they are still present but marked with 1e10 X and Y true coordinates
    EventsDataHub->purge1e10events(); //purging events with "true" positions x==1e10 && y==1e10

    Detector->BuildDetector();
    //Detector->Config->UpdateSimSettingsOfDetector(); //inside the rebuild now

    emit SimulationFinished();

    ASimulationManager::Clear();   //clear tmp data - if needed, main window has copied all needed in the slot on SimulationFinished()
    //qDebug() << "SimManager: Sim finished";
}

