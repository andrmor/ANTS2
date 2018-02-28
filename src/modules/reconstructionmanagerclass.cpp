#include "reconstructionmanagerclass.h"
#include "eventsdataclass.h"
#include "CorrelationFilters.h"
#include "alrfmoduleselector.h"
#include "sensorlrfs.h"     // TEMPORARY! see cuda
#include "ajsontools.h"
#include "processorclass.h"
#include "ageoobject.h"
#include "apositionenergyrecords.h"
#include "pms.h"
#include "apmgroupsmanager.h"

#ifdef ANTS_FLANN
#include "nnmoduleclass.h"
#endif
#ifdef __USE_ANTS_CUDA__
#include "cudamanagerclass.h"
#endif
#ifdef ANTS_FANN
#include "neuralnetworksmodule.h"
#endif

#include <QThread>
#include <QDebug>
#include <QTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QApplication>

#include "TGeoManager.h"
#include "TError.h"


//ReconstructionManagerClass::ReconstructionManagerClass(EventsDataClass *eventsDataHub, DetectorClass *detector)
ReconstructionManagerClass::ReconstructionManagerClass(EventsDataClass *eventsDataHub,
                                                       pms* PMs,
                                                       APmGroupsManager* PMgroups,
                                                       ALrfModuleSelector *LRFs,
                                                       TGeoManager** PGeoManager) :
    EventsDataHub(eventsDataHub), PMs(PMs), PMgroups(PMgroups), LRFs(LRFs), PGeoManager(PGeoManager)
{
  NumThreads = 1;
  bBusy = false;

#ifdef ANTS_FANN
  ANNmodule = new NeuralNetworksModule(PMs, PMgroups, EventsDataHub);
  qDebug() << "->NeuralNetworks module created";
#endif

#ifdef ANTS_FLANN
  KNNmodule = new NNmoduleClass(EventsDataHub, PMs); //fast nearest neighbour module
  qDebug() << "->Nearest neighbour module created";
#endif
}

ReconstructionManagerClass::~ReconstructionManagerClass()
{
#ifdef ANTS_FLANN
  delete KNNmodule;
  //qDebug() << "  Nearest neighbour module deleted";
#endif

#ifdef ANTS_FANN
  delete ANNmodule;
  //qDebug() << "  NeuralNetworks module deleted";
#endif
}

bool ReconstructionManagerClass::reconstructAll(QJsonObject &json, int numThreads, bool fShow)
{
   //qDebug() << "==> Reconstruct all events triggered";    
  bool fOK = ReconstructionManagerClass::fillSettingsAndVerify(json, true);
  if (!fOK)
    {
      qWarning() << "Reconstruction manager reports fail in processing of configuration:\n"
                 << ErrorString;
      emit ReconstructionFinished(false, fShow);
      return false; //something wrong with configuration
    }
    //qDebug() << "  Sensor groups configured:"<< RecSet.size();
  NumThreads = numThreads;
  if (NumThreads<1) NumThreads = 1;
    //qDebug() << "  Using threads:"<<NumThreads;
  EventsDataHub->resetReconstructionData(RecSet.size()); //does some clear too, keeps EventId

  QTime timer;
  timer.start();

  QList<ProcessorClass*> todo;
  bBusy = true;
  for (CurrentGroup=0; CurrentGroup<RecSet.size(); CurrentGroup++)
  {
      todo.clear();
        //qDebug() << "Starting reconstruction for sensor group:"<<CurrentGroup;
      PMgroups->selectActiveSensorGroup(CurrentGroup); //PMs of other groups will be TMP-passive

      //pre-phase - doing CoG for all events
        //qDebug() << "--> Doing pre-phase CoG";
      distributeWork(0, todo);
      fOK = run(todo); //on finish, todo is empty

      //main algorithm
        //qDebug() << "--> Performing reconstruction with the algorithm:"<<RecSet.at(CurrentGroup).ReconstructionAlgorithm;
      bool fMultiThreaded = (NumThreads>0) && ( RecSet.at(CurrentGroup).ReconstructionAlgorithm == 1 || RecSet.at(CurrentGroup).ReconstructionAlgorithm == 2 );
      if (fMultiThreaded)
      {
          todo.clear();
          distributeWork(RecSet.at(CurrentGroup).ReconstructionAlgorithm, todo);
          fOK = run(todo);
      }
      else
      {
          switch (RecSet.at(CurrentGroup).ReconstructionAlgorithm)
          {
          case 0:
          { //just copy cog data to positions
              for (int iev=0; iev<EventsDataHub->Events.size(); iev++)
              {
                  EventsDataHub->ReconstructionData[CurrentGroup][iev]->Points[0].r[0] = EventsDataHub->ReconstructionData[CurrentGroup][iev]->xCoG;
                  EventsDataHub->ReconstructionData[CurrentGroup][iev]->Points[0].r[1] = EventsDataHub->ReconstructionData[CurrentGroup][iev]->yCoG;
                  EventsDataHub->ReconstructionData[CurrentGroup][iev]->Points[0].r[2] = EventsDataHub->ReconstructionData[CurrentGroup][iev]->zCoG;
                  EventsDataHub->ReconstructionData[CurrentGroup][iev]->Points[0].energy = 1.0; //recently - to protect LRF recon with "energy on" option
                  EventsDataHub->ReconstructionData[CurrentGroup][iev]->chi2 = 0;
              }
              break;
          }
          case 3:
#ifdef ANTS_FANN
              if (RecSet.size()>1)
              {
                  ErrorString = "Neural network reconstruction is not implemented for multi-group configurations";
                  PMgroups->clearActiveSensorGroups();
                  bBusy = false;
                  emit ReconstructionFinished(false, fShow);
                  return false;
              }

              ANNmodule->Configure(&RecSet[CurrentGroup]);
              for (int iev=0; iev<EventsDataHub->Events.size(); iev++)
                  if (EventsDataHub->ReconstructionData[0][iev]->ReconstructionOK) //CoG OK
                      ANNmodule->ReconstructEvent(iev, -1, &EventsDataHub->ReconstructionData[CurrentGroup][iev]->Points[0].r, &EventsDataHub->ReconstructionData[CurrentGroup][iev]->Points[0].energy);
#else
              ErrorString = "FANN was not configured in .pro";
              PMgroups->clearActiveSensorGroups();
              bBusy = false;
              emit ReconstructionFinished(false, fShow);
              return false;
#endif
              break;
          case 4:
          {
#ifdef __USE_ANTS_CUDA__
              CudaManagerClass cm(PMs, PMgroups, LRFs->getOldModule(), EventsDataHub, &RecSet[CurrentGroup], CurrentGroup);
              QString Method = (*LRFs->getOldModule())[0]->type();//  "Axial" "XY" "Sliced3D" "ComprAxial" "Composite"
              bool ok = cm.RunCuda(Method);
              if (!ok)
              {
                  ErrorString = cm.getLastError();//"CUDA module reports an error during reconstruction";
                  PMgroups->clearActiveSensorGroups();
                  bBusy = false;
                  emit ReconstructionFinished(false, fShow);
                  return false;
              }
              qDebug() << "Rate (us/event) reported by cuda manager module:" << cm.getUsPerEvent();
#else
              ErrorString = "CUDA was not configured in .pro";
              PMgroups->clearActiveSensorGroups();
              bBusy = false;
              emit ReconstructionFinished(false, fShow);
              return false;
#endif
             break;
          }
          case 5:
          {
#ifdef ANTS_FLANN
              if (RecSet.size()>1)
              {
                  ErrorString = "kNN reconstruction is not implemented for multi-group configurations";
                  PMgroups->clearActiveSensorGroups();
                  bBusy = false;
                  emit ReconstructionFinished(false, fShow);
                  return false;
              }

              KNNmodule->Reconstructor.readFromJson(RecSet[CurrentGroup].kNNrecSet);
              bool ok = KNNmodule->Reconstructor.reconstructEvents();
              if (!ok)
              {
                  ErrorString = "kNN module reports error in reconstruction";
                  PMgroups->clearActiveSensorGroups();
                  bBusy = false;
                  emit ReconstructionFinished(false, fShow);
                  return false;
              }
              for (int iev=0; iev<EventsDataHub->Events.size(); iev++)
              {
                  EventsDataHub->ReconstructionData[CurrentGroup][iev]->Points[0].r[2] = EventsDataHub->ReconstructionData[CurrentGroup][iev]->zCoG;
                  EventsDataHub->ReconstructionData[CurrentGroup][iev]->Points[0].energy = 1.0; //recently - to protect LRF recon with "energy on" option
                  EventsDataHub->ReconstructionData[CurrentGroup][iev]->chi2 = 0;
              }
#else
              ErrorString = "kNN was not configured in .pro";
              PMgroups->clearActiveSensorGroups();
              bBusy = false;
              emit ReconstructionFinished(false, fShow);
              return false;
#endif
              break;
          }
          case 1:
              //not supported - easy to add but have to insert gui update
          case 2:
              //not supported - easy to add but have to insert gui update: RootMinReconstructorClass rmr(this, 0, numEvents); rmr.execute();
          default:
              ErrorString = "Unsupported algorithm or threading option";
              PMgroups->clearActiveSensorGroups();
              bBusy = false;
              emit ReconstructionFinished(false, fShow);
              return false;
          }
      }
      if (fStopRequested)
      {
          //EventsDataHub->createDefaultReconstructionData(); // to do clear properly
          EventsDataHub->resetReconstructionData(RecSet.size()); // to do clear properly
          PMgroups->clearActiveSensorGroups();
          bBusy = false;
          emit ReconstructionFinished(false, fShow);
          return false;
      }
      int msElapsed = timer.elapsed();
      usPerEvent = (numEvents>0) ? 1000.0*msElapsed/(double)numEvents : 0;

      //chi2 calculation
      bool fDoChi2Calc = LRFs->isAllLRFsDefined()
              && RecSet.at(CurrentGroup).ReconstructionAlgorithm != 0
              && RecSet.at(CurrentGroup).ReconstructionAlgorithm != 4
              && RecSet.at(CurrentGroup).ReconstructionAlgorithm != 5 //might make sense to activate!
              && !(RecSet.at(CurrentGroup).ReconstructionAlgorithm == 2 && RecSet.at(CurrentGroup).MultipleEventOption == 1);
      if (fDoChi2Calc)
      {
          //qDebug() << "--> Calculating Chi2";
          todo.clear();
          distributeWork(10, todo);
          fOK = run(todo);
      }
      //else qDebug() << "--> skipping Chi2 reconstruction";

      //if multiple option = 2, have to do double rec and compare sigma with single
      if (RecSet.at(CurrentGroup).MultipleEventOption == 2)
      {
          qDebug() << "--> Running double event reconstruction";
          todo.clear();
          distributeWork(12, todo);
          fOK = run(todo);
      }
  }

  bBusy = false;

  PMgroups->clearActiveSensorGroups();
  EventsDataHub->RecSettings = RecSet;
  EventsDataHub->fReconstructionDataReady = true;

  //event filtering
    //qDebug() << "--> Running event filters";
  doFilters();

  emit ReconstructionFinished(true, fShow);
  return true;
}

void ReconstructionManagerClass::distributeWork(int Algorithm, QList<ProcessorClass*> &todo)
// Algorithm options:
//0 - CoG reconstruction, 1 - MG, 2 - RootMini
//10 - Calculate Chi2, 11 - process event filters
{ 
  numEvents = EventsDataHub->Events.size();
  int perOne = numEvents / NumThreads;
  if (perOne*NumThreads < numEvents) perOne++;
  int from = 0;
  int to;
  int Id = 0;
  do
    {
      to = from + perOne;
      if (to>numEvents) to = numEvents;
      //qDebug()<<"Adding thread with Id"<<Id<<"to process events from"<<from<<" to "<<to-1;
      switch (Algorithm)
      {
      case 0:
          todo << new CoGReconstructorClass(PMs, PMgroups, LRFs, EventsDataHub, &RecSet[CurrentGroup], CurrentGroup, from, to);
          break;
      case 1:
          todo << new CGonCPUreconstructorClass(PMs, PMgroups, LRFs, EventsDataHub, &RecSet[CurrentGroup], CurrentGroup, from, to);
          break;
      case 2:
          if (RecSet.at(CurrentGroup).fLimitSearchIfTrueIsSet)
               todo << new RootMinRangedReconstructorClass(PMs, PMgroups, LRFs, EventsDataHub, &RecSet[CurrentGroup], CurrentGroup, from, to, RecSet.at(CurrentGroup).RangeForLimitSearchIfTrueSet, RecSet.at(CurrentGroup).LimitSearchGauss);
          else if (RecSet.at(CurrentGroup).MultipleEventOption == 1) //in the case of 2(chose best single or double), first single is calculated
               todo << new RootMinDoubleReconstructorClass(PMs, PMgroups, LRFs, EventsDataHub, &RecSet[CurrentGroup], CurrentGroup, from, to);
          else todo << new RootMinReconstructorClass(PMs, PMgroups, LRFs, EventsDataHub, &RecSet[CurrentGroup], CurrentGroup, from, to);
          break;
      case 10:
          todo << new Chi2calculatorClass(PMs, PMgroups, LRFs, EventsDataHub, &RecSet[CurrentGroup], CurrentGroup, from, to);
          break;
      case 11:
          todo << new EventFilterClass(PMs, PMgroups, LRFs, EventsDataHub, &RecSet[CurrentGroup], &FiltSet[CurrentGroup], CurrentGroup, from, to);
          break;
      case 12:
          todo << new RootMinDoubleReconstructorClass(PMs, PMgroups, LRFs, EventsDataHub, &RecSet[CurrentGroup], CurrentGroup, from, to);
          break;
      }

      todo.last()->Id = Id;
      Id++;
      from += perOne;
    }
  while (from<numEvents);
}

void ReconstructionManagerClass::doFilters()
{
    QList<ProcessorClass*> todo;

    bBusy = true;
    for (CurrentGroup=0; CurrentGroup<FiltSet.size(); CurrentGroup++)
    {
        //qDebug() << "...Running multithread filters for sensor group"<<CurrentGroup;
        todo.clear();
        distributeWork(11, todo);
        //bool fOK =
        run(todo);
        //qDebug() << "...OK?"<<fOK;
        //additional filters which has to be checked in single thread
        //qDebug() << "...single thread filters";
        singleThreadEventFilters();
        //qDebug() << "...done!";
    }
    bBusy = false;
}

bool ReconstructionManagerClass::fillSettingsAndVerify(QJsonObject &json, bool fCheckLRFs)
{
  if (EventsDataHub->isEmpty())
    {
      ErrorString = "No data to reconstruct";
      return false;
    }
  if (!json.contains("ReconstructionConfig"))
    {
      ErrorString = "Reconstruction settings not found in Json file";
      return false;
    }

  QJsonObject jsReconSet = json["ReconstructionConfig"].toObject();

  //LRF module selection
  bool fOld = true;
  if (jsReconSet.contains("LRFmoduleSelected"))
    {
      int sel = 0;
      parseJson(jsReconSet, "LRFmoduleSelected", sel);
      if (sel == 1) fOld = false;
      if (sel > 1)
        {
          ErrorString = "Unknown LRF module selection!";
          return false;
        }
    }
  //qDebug() << "LRF old?"<<fOld;
  if (fOld) LRFs->selectOld();
  else LRFs->selectNew();

  //reading reconstruction settings, for compatibility it could be QJsonObject (old system) or arrays of QObjects, one for each sensor group
  QJsonArray arRC;
  if (jsReconSet["ReconstructionOptions"].isObject())
  {
      //old system - converting to comply with new standard
      QJsonObject jsTmp = jsReconSet["ReconstructionOptions"].toObject();
      arRC << jsTmp;
  }
  else if (jsReconSet["ReconstructionOptions"].isArray())
  {
      //new system
      arRC = jsReconSet["ReconstructionOptions"].toArray();
  }
  else
  {
      ErrorString = "Unknown format of json record in reconstruction settings!";
      return false;
  }

  RecSet.clear(); //clear old vector with settings
  for (int igroup=0; igroup<arRC.size(); igroup++)
  {
        //qDebug() << "  Reading recon settings for sensor group #"<<igroup;
      QJsonObject js = arRC[igroup].toObject();
      ReconstructionSettings RecS;
      bool fOK = RecS.readFromJson(js);
      if (!fOK)
      {
          ErrorString = RecS.ErrorString;
          return false;
      }

      int Alg = RecS.ReconstructionAlgorithm; // 0 - CoG, 1 -CGonCPU, 2-RootMin, 3 - ANN, 4-CUDA

      if (fCheckLRFs)
      {
          if (Alg!=0 && Alg!=3 && Alg!=5) // CoG, ANN and KNN do not need LRFs
              if (!LRFs->isAllLRFsDefined())
              {
                  ErrorString = "LRFs are not defined";
                  return false;
              }
      }

      if (RecS.MultipleEventOption > 0 && (Alg!=2))
      {
          ErrorString = "Multiple events can be reconstructed onnly with Root-based minimizer method";
          return false;
      }
      //if start option is scan, scanData should exist
      if (Alg == 1 && RecS.CGstartOption == 3 && EventsDataHub->isScanEmpty())
      {
          ErrorString = "Start option is set to 'True XY' but scan data are empty";
          return false;
      }
      if (Alg == 2 && RecS.RMstartOption == 2 && EventsDataHub->isScanEmpty())
      {
          ErrorString = "Start option is set to 'True XY' but scan data are empty";
          return false;
      }

      if (RecS.Zstrategy == 1)
      {
          //Scan should contain data for all events!
          if (EventsDataHub->Scan.size() != EventsDataHub->Events.size())
          {
              ErrorString = "Z strategy is set to use loaded/simulated data, but there are no such data!";
              return false;
          }
      }

#ifdef ANTS_FANN
      if (Alg == 3)
      {
          if (!ANNmodule->isFANN() || !ANNmodule->checkPMsDimension())
          {
              ErrorString = "No FANN library is loaded or wrong number of PM inputs";
              return false;
          }
      }
#endif

      if (RecS.fRMsuppressConsole) gErrorIgnoreLevel = kWarning;

      RecSet.append(RecS);
  }

  //filling and checking event filtering settings
  FiltSet.clear();

  bool fOK = ReconstructionManagerClass::configureFilters(jsReconSet);
  if (!fOK) return false;

  if (RecSet.size() != FiltSet.size())
  {
      ErrorString = "Error: mismatch in vector length for reconstruction and filter settings";
      return false;
  }

  ErrorString = "";
  return true;
}

bool ReconstructionManagerClass::configureFilters(QJsonObject &json)
{
  //expecting json to be "ReconstructionConfig" object
  if ( !json.contains("FilterOptions") )
    {
      ErrorString = "||| Json config file does not contain filter settings!";
      return false;
    }

  QJsonArray arFS;
  if (json["FilterOptions"].isArray())
  {
      //new system
      arFS = json["FilterOptions"].toArray();
  }
  else if (json["FilterOptions"].isObject())
  {
      QJsonObject jtmp = json["FilterOptions"].toObject();
      arFS << jtmp;
  }
  else
  {
      ErrorString = "bad format of filter options json";
      return false;
  }

  FiltSet.clear(); //clear old vector with settings
  for (int igroup=0; igroup<arFS.size(); igroup++)
  {
      QJsonObject js = arFS[igroup].toObject();
      AEventFilteringSettings FiltS;
      bool fOK = FiltS.readFromJson(js, PMgroups, igroup, EventsDataHub);
      if (!fOK)
      {
          ErrorString = FiltS.ErrorString;
          return false;
      }
      FiltSet.append(FiltS);
  }
  return true;
}

void ReconstructionManagerClass::onLRFsCopied()
{
    fDoingCopyLRFs.store(false);
}

bool ReconstructionManagerClass::run(QList<ProcessorClass *> reconstructorList)
{    
  fStopRequested = false;
  QList<QThread*> threads;  
  for (int ithread = 0; ithread<reconstructorList.size(); ithread++)
    {
      fDoingCopyLRFs.store(true);
      threads.append(new QThread());
      QObject::connect(threads.last(), &QThread::started, reconstructorList[ithread], &ProcessorClass::copyLrfsAndExecute);
      QObject::connect(reconstructorList[ithread], SIGNAL(lrfsCopied()), this, SLOT(onLRFsCopied()), Qt::DirectConnection);
      QObject::connect(reconstructorList[ithread], SIGNAL(finished()), threads.last(), SLOT(quit()));
      QObject::connect(threads.last(), SIGNAL(finished()), threads.last(), SLOT(deleteLater()));
      reconstructorList[ithread]->moveToThread(threads.last());
      threads.last()->start();

      do qApp->processEvents();
      while (fDoingCopyLRFs.load());
    }

  int StartingTreads = threads.size();
  QTime RefresherTimer;
  RefresherTimer.start();
  int TotalMsPassed = 0;
  while (threads.size()>0)
    {
      for (int i=threads.size()-1; i>-1; i--)
        {
          //finished thread?
          if (reconstructorList[i]->fFinished)
            {
              //qDebug() << "Thread"<< reconstructorList[i]->Id << " reports finished status";
              delete reconstructorList[i];
              reconstructorList.removeAt(i);
              threads.removeAt(i);
            }
        }      
      int msPassed = RefresherTimer.elapsed();
      if (msPassed > 500)
        {
          qApp->processEvents();
          //reporting progress
          float ProgressF = 100.0*(StartingTreads - threads.size()); //of finished threads
          int EventsDone = 0;
          for (int i=0; i<threads.size(); i++)
            {
              int tp = reconstructorList[i]->Progress;
              //qDebug() << "Thread ["<<reconstructorList[i]->Id<< "] progress:"<<tp<<StartingTreads<<threads.size();
              ProgressF += tp;
              EventsDone += reconstructorList[i]->eventsProcessed;
            }
          int TotalProgress = ProgressF/StartingTreads;
          //qDebug() << "Total progress:"<<TotalProgress;

          TotalMsPassed += msPassed;
          //double MsPerEv =  100.0 * 1000 * TotalMsPassed / ( numEvents*(TotalProgress+0.00001));
          double MsPerEv = (EventsDone>0) ? 1000.0 * TotalMsPassed / (double)EventsDone : 0;
          emit UpdateReady(TotalProgress, MsPerEv);

          //"stop" user request handling
          if (fStopRequested)
            {
              for (int i=0; i<reconstructorList.size(); i++)
                reconstructorList[i]->fStopRequested = true;
            }

          RefresherTimer.restart();
        }
    }
  //qDebug() << "All threads finished!";
  return !fStopRequested;
}

void ReconstructionManagerClass::filterEvents(QJsonObject &json, int numThreads)
{ 
    //qDebug() << "==>Filter all events triggered";
  bool fOK = ReconstructionManagerClass::fillSettingsAndVerify(json, false);
  if (!fOK)
      {
         qWarning() << "Reconstruction manager reports fail in processing of configuration:\n"
                    << ErrorString;
         return;
      }

  NumThreads = numThreads;
  if (NumThreads<1) NumThreads = 1;
  //qDebug() << "Filter settings ok, Number of threads:" << NumThreads;
  assureReconstructionDataContainersExist();  //important - can be the first call after new data were simulated/loaded
  //qDebug() << "Rec data containers synchronized";
  doFilters();
  //qDebug() << "Filtering done!";
  EventsDataHub->clearReconstructionTree();

  emit RequestShowStatistics();
}

void ReconstructionManagerClass::assureReconstructionDataContainersExist()
{
    for (CurrentGroup = 0; CurrentGroup<FiltSet.size(); CurrentGroup++)
    {
       if (EventsDataHub->isReconstructionDataEmpty(CurrentGroup))
       {
           //qDebug() << "Creating default reconstruction data container for group"<<CurrentGroup;
           EventsDataHub->createDefaultReconstructionData(CurrentGroup);

           //qDebug() << "Running CoG to fill created ReconstructionData";
           QList<ProcessorClass*> todo;
           distributeWork(0, todo);
           run(todo);
       }
    }
}

void ReconstructionManagerClass::singleThreadEventFilters()
{
  //qDebug() << "Running single thread filters";
  //GoodEvent status already set by multithread filters!

  int numEvents = EventsDataHub->Events.size();
  AEventFilteringSettings* FiltS = &FiltSet[CurrentGroup];

#ifdef ANTS_FLANN  
  if (FiltS->fKNNfilter)
    {
      if (FiltSet.size()==1)
      {
          qDebug() << "  Preparing kNN module..";
          KNNmodule->Filter.prepareNNfilter(FiltS->KNNfilterAverageOver+1); //reuses old data set or calculates new
            //+1 - module assumes 0 is the point itself, therefore incrementing by 1
          qDebug() << "  Done!";
      }
      else qWarning() << "kNN filter is disabled in multi group configuration!";
    }
#endif

  TGeoNavigator* navi = 0;
  if (FiltS->fSpF_LimitToObj && PGeoManager)
      navi = (*PGeoManager)->GetCurrentNavigator();
  else FiltS->fSpF_LimitToObj = false;

  // main cycle
  for (int iev=0; iev<numEvents; iev++)
    {
      AReconRecord* rec = EventsDataHub->ReconstructionData[CurrentGroup][iev];
      if (!rec->GoodEvent) continue;

      //spatial to specific volume     
      if (FiltS->fSpF_LimitToObj)
      {
          //at least one of the points shoud be inside this volume
          APositionEnergyBuffer &Points = (FiltS->SpF_RecOrScan == 0) ? rec->Points : EventsDataHub->Scan.at(iev)->Points;
          bool fNotFound = true;
          for (int iPoint = 0; iPoint<Points.size(); iPoint++)
          {
              TGeoNode* node = navi->FindNode(Points[iPoint].r[0], Points[iPoint].r[1], Points[iPoint].r[2]);
              if (node && TString(node->GetVolume()->GetName()) == FiltS->SpF_LimitToObj)
              {
                  fNotFound = false;
                  break;
              }
          }
          if (fNotFound)
          {
              rec->GoodEvent = false;
              continue;
          }
      }

      //correlation
      if (FiltS->fCorrelationFilters)
        {
          bool pass = true;

          for (int ifilter = 0; ifilter<FiltS->CorrelationFilters.size(); ifilter++)
            if (FiltS->CorrelationFilters.at(ifilter)->Active)
             {
                /*
              qDebug()<<"First:"<<CorrelationFilters->at(ifilter)->getCorrelationUnitX()->getType()<<CorrelationFilters->at(ifilter)->getCorrelationUnitX()->Channels;
              qDebug()<<"Second:"<<CorrelationFilters->at(ifilter)->getCorrelationUnitX()->getType()<<CorrelationFilters->at(ifilter)->getCorrelationUnitX()->Channels;
              qDebug()<<"Cut:"<<CorrelationFilters->at(ifilter)->getCut()->getType()<<CorrelationFilters->at(ifilter)->getCut()->Data;
              qDebug()<<"Current event:"<<CurrentEvent;
              qDebug()<<"Adress of Events in X:"<<CorrelationFilters->at(ifilter)->getCorrelationUnitX()->Events;
              qDebug()<<"Actual Events address:"<<&Events;
              qDebug()<<"Events in X:"<<*(CorrelationFilters->at(ifilter)->getCorrelationUnitX()->Events);
              qDebug()<<"Val_X:"<<CorrelationFilters->at(ifilter)->getCorrelationUnitX()->getValue(CurrentEvent);
              qDebug()<<"Val_Y:"<<CorrelationFilters->at(ifilter)->getCorrelationUnitY()->getValue(CurrentEvent);
              */
              if (!FiltS->CorrelationFilters.at(ifilter)->getCut()->filter( FiltS->CorrelationFilters.at(ifilter)->getCorrelationUnitX()->getValue(iev),
                                                                      FiltS->CorrelationFilters.at(ifilter)->getCorrelationUnitY()->getValue(iev) ))
                {
                  pass = false;
                  break;
                }
//                  qDebug()<<"   #"<<ifilter<<" :"<<pass;
             }
          if (!pass)
          {
              rec->GoodEvent = false;
              continue;
          }
        }

      //kNN
#ifdef ANTS_FLANN
          //kNNfilter - can be activated only if ANTS_FLANN is defined
          if (FiltS->fKNNfilter)
            {
              float dist = KNNmodule->Filter.getAverageDist(iev);
              if (dist<FiltS->KNNfilterMin || dist>FiltS->KNNfilterMax)
              {
                  rec->GoodEvent = false;
                  continue;
              }
            }          
#endif
  }

  //qDebug() << "Single thread filters done!";
}

void ReconstructionManagerClass::onRequestClearKNNfilter()
{
#ifdef ANTS_FLANN
   KNNmodule->Filter.clear();
#endif
}
