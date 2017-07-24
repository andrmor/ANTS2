#include "eventsdataclass.h"
#include "ajsontools.h"
#include "apositionenergyrecords.h"
#include "apreprocessingsettings.h"
#include "pms.h"

//Root
#include "TTree.h"
#include "TFile.h"
#include "TParameter.h"
#include "TRandom2.h"

#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QApplication>

EventsDataClass::EventsDataClass(const TString nameID) //nameaddon to make unique hist names in multithread
 : QObject()
{
#ifdef SIM
  SimStat = new ASimulationStatistics(nameID);
  //SimStat->initialize();
#endif
  fReconstructionDataReady = false;
  ReconstructionTree = 0;
  ResolutionTree = 0;
  fLoadedEventsHaveEnergyInfo = false;  
}

EventsDataClass::~EventsDataClass()
{
  clear();
#ifdef SIM
  delete SimStat;
#endif
}

void EventsDataClass::clearReconstructionTree()
{
  if (ReconstructionTree)
    {
      delete ReconstructionTree;
      ReconstructionTree = 0;
      qDebug() << "  Reconstruction tree deleted";
    }
}

void EventsDataClass::clearResolutionTree()
{
  if (ResolutionTree)
    {
      delete ResolutionTree;
      ResolutionTree = 0;
      qDebug() << "  Resolution tree deleted";
    }
}

void EventsDataClass::clear()
{
  //qDebug() << "--->EventsDatHub: Clear";
  Events.clear();
  TimedEvents.clear();  
  fLoadedEventsHaveEnergyInfo = false;
#ifdef SIM
  for (int i=0; i<EventHistory.size(); i++) delete EventHistory[i];
  EventHistory.clear();
  GeneratedPhotonsHistory.clear();
  SimStat->initialize();
#endif
  clearScan();
  clearReconstruction();
  clearManifest();
  emit requestClearKNNfilter(); //save in any configuration
  squeeze();
  //preprocessing settings clear is triggered when detector is made with different number of PMs than before

  emit cleared();                      //to MainWindow and tmpHub
  emit requestEventsGuiUpdate();       //to ReconWindow
}

void EventsDataClass::onRequestStopLoad()
{
    fStopLoadRequested = true;
}

void EventsDataClass::clearReconstruction()
{
  for (int igroup=0; igroup<ReconstructionData.size(); igroup++)
     clearReconstruction(igroup);
  ReconstructionData.clear();
}

void EventsDataClass::clearReconstruction(int igroup)
{
    if (igroup > ReconstructionData.size()-1)
    {
        ReconstructionData.resize(igroup+1);
        return;
    }

    for (int ievent=0; ievent<ReconstructionData[igroup].size(); ievent++)
        delete ReconstructionData[igroup][ievent];
    ReconstructionData[igroup].clear();

    fReconstructionDataReady = false;
    clearReconstructionTree();
    clearResolutionTree();
}

void EventsDataClass::clearScan()
{
  for (int i=0; i<Scan.size(); i++) delete Scan[i];
  Scan.clear();
  ScanNumberOfRuns = 1;  //on MW was 0
  fSimulatedData = false;
}

void EventsDataClass::clearManifest()
{
  for (int i=0; i<Manifest.size(); i++) delete Manifest[i];
  Manifest.clear();
}

#ifdef SIM
void EventsDataClass::initializeSimStat(QVector<const AGeoObject*> monitorRecords, int numBins, int waveNodes)
{
  SimStat->initialize(monitorRecords, numBins, waveNodes);
}
#endif

bool EventsDataClass::isReconstructionDataEmpty(int igroup)
{
    if (ReconstructionData.isEmpty()) return true;
    if (igroup > ReconstructionData.size()-1) return true;
    if (ReconstructionData[igroup].isEmpty()) return true;
    return false;
}

void EventsDataClass::purge1e10events()
{
  if (Scan.size() != Events.size()) return;
  bool fTime = ( TimedEvents.size() == Events.size() );

  int i = 0;
  while (i<Scan.size())
    {
      //qDebug() << Scan.at(i)->Points[0].r[0] << Scan.at(i)->Points[0].r[1];
      if (Scan.at(i)->Points[0].r[0] == 1e10 && Scan.at(i)->Points[0].r[1] == 1e10 )
        {
          //qDebug() << "Found one";
          Scan.removeAt(i);
          Events.removeAt(i);
          if (fTime) TimedEvents.removeAt(i);
        }
      else
        i++;
    }
}

bool EventsDataClass::BlurReconstructionData(int type, double sigma, TRandom2* RandGen, int igroup)
{
  if (igroup > ReconstructionData.size()-1)
  {
      ErrorString = "Incorrect igroup number!";
      qWarning() << ErrorString;
      return false;
  }
  if (type<0 || type>1)
  {
      ErrorString = "EventsDataHub: Undefined blur type";
      qWarning() << ErrorString;
      return false;
  }

  int istart = 0;
  int istop = ReconstructionData.size();
  if (igroup>-1)
  {
      //just one group
      istart = igroup;
      istop = igroup+1;
  }

  for (int igr=istart; igr<istop; igr++)
  {
      if (type == 0)
        {
          //uniform
          for (int iev=0; iev<ReconstructionData[igr].size(); iev++)
            for (int ip=0; ip<ReconstructionData[igr][iev]->Points.size(); ip++)
            {
              ReconstructionData[igr][iev]->Points[ip].r[0] += -sigma + 2.0*sigma*RandGen->Rndm();
              ReconstructionData[igr][iev]->Points[ip].r[1] += -sigma + 2.0*sigma*RandGen->Rndm();
            }
        }
      else if (type == 1)
        {
          //Gauss
          for (int iev=0; iev<ReconstructionData[igr].size(); iev++)
            for (int ip=0; ip<ReconstructionData[igr][iev]->Points.size(); ip++)
            {
              ReconstructionData[igr][iev]->Points[ip].r[0] += RandGen->Gaus(0, sigma);
              ReconstructionData[igr][iev]->Points[ip].r[1] += RandGen->Gaus(0, sigma);
            }
        }
    }
  return true;
}

bool EventsDataClass::BlurReconstructionDataZ(int type, double sigma, TRandom2 *RandGen, int igroup)
{
  if (igroup > ReconstructionData.size()-1)
  {
      ErrorString = "Incorrect igroup number!";
      qWarning() << ErrorString;
      return false;
  }
  if (type<0 || type>1)
  {
      ErrorString = "EventsDataHub: Undefined blur type";
      qWarning() << ErrorString;
      return false;
  }

  int istart = 0;
  int istop = ReconstructionData.size();
  if (igroup>-1)
  {
      //just one group
      istart = igroup;
      istop = igroup+1;
  }

  for (int igr=istart; igr<istop; igr++)
  {
      if (type == 0)
        {
          //uniform
          for (int iev=0; iev<ReconstructionData[igr].size(); iev++)
            for (int ip=0; ip<ReconstructionData[igr][iev]->Points.size(); ip++)
              ReconstructionData[igr][iev]->Points[ip].r[2] += -sigma + 2.0*sigma*RandGen->Rndm();
        }
      else if (type == 1)
        {
          //Gauss
          for (int iev=0; iev<ReconstructionData[igr].size(); iev++)
            for (int ip=0; ip<ReconstructionData[igr][iev]->Points.size(); ip++)
              ReconstructionData[igr][iev]->Points[ip].r[2] += RandGen->Gaus(0, sigma);
        }
    }
  return true;
}

void EventsDataClass::PurgeFilteredEvents(int igroup)
{
  if (igroup > ReconstructionData.size()-1)
  {
      qWarning() << "Bad group number!";
      return;
  }

  int NumEvents = Events.size();
  if (NumEvents == 0) return;
  if (countGoodEvents() == NumEvents) return;

  bool fDoTimed = isTimed();
  bool fDoScan = !isScanEmpty();
  qDebug() << "Timed, scan:"<<fDoTimed << fDoScan;

  int iposition = 0;
  for (int iev = 0; iev<NumEvents; iev++)
    {
      //if bad event, continue
      if (!ReconstructionData[igroup][iev]->GoodEvent) continue;

      //found good event, moving to iposition if not != iev
      if (iev != iposition)
        {
          //move
          Events[iposition] = Events[iev];
          if (fDoTimed) TimedEvents[iposition] = TimedEvents[iev];
          if (fDoScan)  Scan[iposition] = Scan[iev];
          for (int ig=0; ig<ReconstructionData.size(); ig++)
            ReconstructionData[ig][iposition] = ReconstructionData[ig][iev];
        }
      iposition++;
    }

  //resizing
  Events.resize(iposition);
  if (fDoTimed) TimedEvents.resize(iposition);
  if (fDoScan)  Scan.resize(iposition);
  for (int ig=0; ig<ReconstructionData.size(); ig++)
    ReconstructionData[ig].resize(iposition);

  squeeze();
  emit requestEventsGuiUpdate();
}

void EventsDataClass::Purge(int OnePer, int igroup)
{
  if (igroup > ReconstructionData.size()-1)
    {
        qWarning() << "Bad group number!";
        return;
    }

  int NumEvents = Events.size();
  if (NumEvents == 0) return;
  bool fDoTimed = isTimed();
  bool fDoScan = !isScanEmpty();
  qDebug() << "Timed, scan:"<<fDoTimed << fDoScan;

  int iposition = 0;
  int counter = 0;
  for (int iev = 0; iev<NumEvents; iev++)
    {
      counter++;
      if (counter != OnePer) continue;
      counter = 0;

      //found good event, moving to iposition if not != iev
      if (iev != iposition)
        {
          //move
          Events[iposition] = Events[iev];
          if (fDoTimed) TimedEvents[iposition] = TimedEvents[iev];
          if (fDoScan)  Scan[iposition] = Scan[iev];
          for (int ig=0; ig<ReconstructionData.size(); ig++)
            ReconstructionData[ig][iposition] = ReconstructionData[ig][iev];
        }
      iposition++;
    }

  //resizing
  Events.resize(iposition);
  if (fDoTimed) TimedEvents.resize(iposition);
  if (fDoScan)  Scan.resize(iposition);
  for (int ig=0; ig<ReconstructionData.size(); ig++)
    ReconstructionData[ig].resize(iposition);

  squeeze();
  emit requestEventsGuiUpdate();
}

bool EventsDataClass::isReconstructionReady(int igroup)
{
    if (igroup<0 || igroup > ReconstructionData.size()-1)
    {
        //qWarning() << "bad group number!" << igroup;
        return false;
    }
    return (!ReconstructionData[igroup].isEmpty() && fReconstructionDataReady);
}

void EventsDataClass::squeeze()
{
    Events.squeeze();
    TimedEvents.squeeze();
    Scan.squeeze();
    for (int i=0; i<ReconstructionData.size(); i++) ReconstructionData[i].squeeze();
    ReconstructionData.squeeze();
#ifdef SIM
    EventHistory.squeeze();
    GeneratedPhotonsHistory.squeeze();
#endif
}

void EventsDataClass::createDefaultReconstructionData(int igroup)
{
  if (igroup == 0)
  {
      // have to reset all
      clearReconstruction();
      ReconstructionData.resize(1);
  }
  else
  {
      // adding a new container for non-first sensor group
      if (igroup > ReconstructionData.size()-1) ReconstructionData.resize(igroup+1);
  }

  try   //added in July 2017 - attempt to find memory leak
  {
    ReconstructionData[igroup].reserve(Events.size());
    //filling default values
    for (int ievent=0; ievent<Events.size(); ievent++)
    {
        AReconRecord* r = new AReconRecord();
        r->EventId = ievent;
        r->chi2 = 0;
        r->ReconstructionOK = false;
        ReconstructionData[igroup].append(r);
    }
  }
  catch (...)
  {
    qCritical() << "Failed allocate space for reconstruction data";
    exit(888);
  }
}

void EventsDataClass::resetReconstructionData(int numGroups)
{
  for (int igroup = 0; igroup<numGroups; igroup++)
  {
      if (isReconstructionDataEmpty(igroup) || ReconstructionData[igroup].size() != Events.size())
        EventsDataClass::createDefaultReconstructionData(igroup);
      else
        {
          for (int i=0; i<Events.size(); i++)
            {
              ReconstructionData[igroup][i]->Points.Reinitialize(1);  //in case it was reconstructed before as a double event
              ReconstructionData[igroup][i]->chi2 = 0; //obsolete?
            }
        }
      ReconstructionData[igroup].squeeze();
  }

  fReconstructionDataReady = false;
  clearReconstructionTree();
}

int EventsDataClass::getTimeBins()
{
  if (TimedEvents.isEmpty()) return 0;
  return TimedEvents[0].size();
}

int EventsDataClass::getNumPMs()
{
    if (Events.isEmpty()) return 0;
    return Events[0].size();
}

const QVector<float> *EventsDataClass::getEvent(int iev)
{
  return &Events.at(iev);
}

const QVector<QVector<float> > *EventsDataClass::getTimedEvent(int iev)
{
  return &TimedEvents.at(iev);
}

int EventsDataClass::countGoodEvents(int igroup) //counts events passing all filters
{
    if (igroup > ReconstructionData.size()-1)
    {
        qWarning() << "bad group number!";
        return false;
    }
   int count = 0;
   for (int i=0; i<ReconstructionData[igroup].size(); i++)
       if (ReconstructionData[igroup][i]->GoodEvent) count++;
   return count;
}

void EventsDataClass::prepareStatisticsForEvents(const bool isAllLRFsDefined, int &GoodEvents, double &AvChi2, double &AvDeviation, int igroup)
{  /// *** not handling multiple events propelrly!
  //qDebug()<<"   Preparing statistics of EventsDataHub triggered";
  if (igroup > ReconstructionData.size()-1)
  {
      GoodEvents = 0;
      AvChi2 = 0;
      AvDeviation = 0;
      return;
  }

  GoodEvents = 0;
  AvChi2 = 0;
  AvDeviation = 0;
  bool DoDeviation = (isScanEmpty() || !isReconstructionReady(igroup)) ?  false : true;
  bool fIsCoG = true;
  if (igroup>-1 && igroup<RecSettings.size())
      if (RecSettings.at(igroup).ReconstructionAlgorithm != 0)
          fIsCoG = false;
  bool fDoChi2 = (!isReconstructionReady(igroup) || fIsCoG || !isAllLRFsDefined) ? false : true;

  for (int iev=0; iev<ReconstructionData[igroup].size(); iev++)
    if (ReconstructionData[igroup][iev]->GoodEvent)
      {
        if (ReconstructionData[igroup][iev]->chi2 != ReconstructionData[igroup][iev]->chi2)
          qWarning() << "nan Chi2 detected for event"<<iev<<"RecOK?"<<ReconstructionData[igroup][iev]->ReconstructionOK;
        GoodEvents++;
        if (fDoChi2) AvChi2 += ReconstructionData[igroup][iev]->chi2;
        if (DoDeviation)
          {
            double r2 = 0;
            for (int i=0; i<2; i++) r2 += (ReconstructionData[igroup][iev]->Points[0].r[i] - Scan[iev]->Points[0].r[i]) *
                (ReconstructionData[igroup][iev]->Points[0].r[i] - Scan[iev]->Points[0].r[i]);
            AvDeviation += sqrt(r2);
          }
      }

  if (fDoChi2 && GoodEvents>0) AvChi2 /= GoodEvents;
  else AvChi2 = -1;

  if (DoDeviation && GoodEvents>0) AvDeviation /= GoodEvents;
  else AvDeviation = -1;
}

void EventsDataClass::copyTrueToReconstructed(int igroup)
{
  if (Scan.isEmpty()) return;
  if (igroup > ReconstructionData.size()-1)
  {
      qWarning() << "bad group number!";
      return;
  }

  clearReconstruction(igroup);
  for (int iEvent=0; iEvent<Scan.size(); iEvent++)
    {
      AReconRecord* rec = new AReconRecord();
      rec->Points.Reinitialize(0);

      for (int i=0; i<Scan.at(iEvent)->Points.size(); i++)
         rec->Points.AddPoint(Scan.at(iEvent)->Points[i].r, Scan.at(iEvent)->Points[i].energy);

      rec->chi2 = 1;
      rec->EventId = iEvent;
      rec->GoodEvent = true;
      rec->ReconstructionOK = true;

      ReconstructionData[igroup].append(rec);
      //rec->report();
    }
  fReconstructionDataReady = true;
}

void EventsDataClass::copyReconstructedToTrue(int igroup)
{
    if (!isReconstructionReady(igroup)) return;

    clearScan();
    for (int iEvent=0; iEvent<ReconstructionData.at(igroup).size(); iEvent++)
      {
        AScanRecord* rec = new AScanRecord();
        rec->ScintType = 0;
        rec->GoodEvent = true;

        rec->Points.Reinitialize(0);
        for (int i=0; i<ReconstructionData.at(igroup).at(iEvent)->Points.size(); i++)
           rec->Points.AddPoint(ReconstructionData.at(igroup).at(iEvent)->Points[i].r, ReconstructionData.at(igroup).at(iEvent)->Points[i].energy);

        Scan.append(rec);
      }
}

bool EventsDataClass::createReconstructionTree(pms* PMs, bool fIncludePMsignals, bool fIncludeRho, bool fIncludeTrue, int igroup)
{
  if (igroup > ReconstructionData.size()-1)
  {
      qWarning() << "Bad sensor group index!";
      return false;
  }

  bool fRecReady = isReconstructionReady(igroup);
  if (!fRecReady)
  {
      qWarning() << "Reconstruction is not ready, not full set of data was used to build the tree";
  }

    //qDebug()<<"--> Building reconstruction tree";
  if (ReconstructionTree) delete ReconstructionTree;
  ReconstructionTree = new TTree("T","Reconstruction data");
  int size = ReconstructionData[igroup].size();

  //double x, y, z, energy;
  std::vector <double> x; //Can be multiple point reconstruction!
  std::vector <double> y;
  std::vector <double> z;
  std::vector <double> energy;
  double chi2;
  double ssum;
  int ievent; //event number
  int good, recOK;

  ReconstructionTree->Branch("i",&ievent, "i/I");
  ReconstructionTree->Branch("ssum",&ssum, "ssum/D");

  if (fRecReady)
  {
      ReconstructionTree->Branch("x", &x);
      ReconstructionTree->Branch("y", &y);
      ReconstructionTree->Branch("z", &z);
      ReconstructionTree->Branch("energy", &energy);

      ReconstructionTree->Branch("chi2",&chi2, "chi2/D");

      ReconstructionTree->Branch("good", &good, "good/I");
      ReconstructionTree->Branch("recOK", &recOK, "recOK/I");
  }
  else fIncludeRho = false;

  //Pm signals and rho
  int numPMs = PMs->count();
  char buf[32];
  float* signal = 0;
  float* rho = 0;
  if (fIncludePMsignals)
     {
       signal = new float[numPMs];
       sprintf(buf, "signal[%d]/F", numPMs);
       ReconstructionTree->Branch("signal", signal, buf);
     }
  if (fIncludeRho)
   {
       rho = new float[numPMs];
       sprintf(buf, "rho[%d]/F", numPMs);
       ReconstructionTree->Branch("rho", rho, buf);
   }

  //tmp
  //int abA;
  //ReconstructionTree->Branch("abA",&abA, "abA/I");
  //double frac1;
  //ReconstructionTree->Branch("frac1",&frac1, "frac1/D");

  //scan data
  std::vector <double> xScan;
  std::vector <double> yScan;
  std::vector <double> zScan;
  std::vector <int> numPhotons;
  double zStop;
  int ScintType, GoodEvent, EventType;

  if (!isScanEmpty() && fIncludeTrue)
    {
      ReconstructionTree->Branch("xScan", &xScan);
      ReconstructionTree->Branch("yScan", &yScan);
      ReconstructionTree->Branch("zScan", &zScan);
      ReconstructionTree->Branch("numPhotons", &numPhotons);

      ReconstructionTree->Branch("zStop",&zStop, "zStop/D");
      ReconstructionTree->Branch("ScintType",&ScintType, "ScintType/I");
      ReconstructionTree->Branch("GoodEvent",&GoodEvent, "GoodEvent/I");
      ReconstructionTree->Branch("EventType",&EventType, "EventType/I");
    }

  //========= building tree =========
  for (int iev=0; iev<size; iev++)
    {
      ievent = ReconstructionData[igroup][iev]->EventId;

      if (fRecReady)
      {
          int Points = ReconstructionData[igroup][iev]->Points.size();
          x.resize(Points);
          y.resize(Points);
          z.resize(Points);
          energy.resize(Points);
          for (int iP=0; iP<Points; iP++)
            {
              x[iP] = ReconstructionData[igroup][iev]->Points[iP].r[0];
              y[iP] = ReconstructionData[igroup][iev]->Points[iP].r[1];
              z[iP] = ReconstructionData[igroup][iev]->Points[iP].r[2];
              energy[iP] = ReconstructionData[igroup][iev]->Points[iP].energy;
            }
          chi2 = ReconstructionData[igroup][iev]->chi2;
          good = ReconstructionData[igroup][iev]->GoodEvent;
          recOK = ReconstructionData[igroup][iev]->ReconstructionOK;
      }

      ssum = 0;
      for (int ipm=0; ipm<numPMs; ipm++)
        {
          if (fIncludePMsignals) signal[ipm] = Events[iev][ipm];
          if (fIncludeRho)
            {
              double X = PMs->X(ipm);
              double Y = PMs->Y(ipm);
              rho[ipm] = sqrt((x[0]-X)*(x[0]-X)+(y[0]-Y)*(y[0]-Y));
            }
          ssum += Events[iev][ipm];// * PMgroups->Groups.at(igroup)->PMS.at(ipm).gain;
        }

      //tmp
      //abA = 0;
      //double avSignal = ssum/numPMs;
      //double threshold = (maxSig - avSignal)*0.15 + avSignal;
      //for (int ipm=0; ipm<numPMs; ipm++)
      //    if ( Events[iev][ipm]* PMs->at(ipm).relGain > threshold) abA++;
      //frac1 = maxSig/ssum;

      if (!isScanEmpty() && fIncludeTrue)
        {
          int Points = Scan[iev]->Points.size();
          xScan.resize(Points);
          yScan.resize(Points);
          zScan.resize(Points);
          numPhotons.resize(Points);
          for (int iP=0; iP<Points; iP++)
            {
              xScan[iP] = Scan[iev]->Points[iP].r[0];
              yScan[iP] = Scan[iev]->Points[iP].r[1];
              zScan[iP] = Scan[iev]->Points[iP].r[2];
              numPhotons[iP] = Scan[iev]->Points[iP].energy;
            }
          zStop = Scan[iev]->zStop;
          ScintType = Scan[iev]->ScintType;
          GoodEvent = Scan[iev]->GoodEvent;
          EventType = Scan[iev]->EventType;
        }

      ReconstructionTree->Fill();
    }

  ReconstructionTree->ResetBranchAddresses();
  if (signal) delete [] signal;
  if (rho) delete [] rho;
    qDebug()<<"   Tree created with "<<ReconstructionTree->GetEntries()<<" entries";
  return true;
}

bool EventsDataClass::createResolutionTree(int igroup)
{
  if (!isReconstructionReady(igroup))
  {
      qWarning() << "||| Reconstruction is not ready!";
      return false;
  }
  //  qDebug()<<"Creating a tree with sigma/distortion data";
  if (ResolutionTree)
    {
      delete ResolutionTree;
      ResolutionTree = 0;
    }
  if (ReconstructionData[igroup].size() != Scan.size())
    {
      qWarning() << "||| Cannot build scan tree: reconstruction and simulated data length mismatch";
      return false;
    }

  int events = Scan.size();
  int nodes = events / ScanNumberOfRuns;

  if (nodes*ScanNumberOfRuns != events)
    {
      qWarning() << "||| Cannot build resolution tree - data size is not consistent with the number of runs";
      return false;
    }

  ResolutionTree = new TTree("ResolutionTree","ScanTree");
  float x, y, z;  //actual position
  float rx, ry, rz;  //mean reconstructed position
  float sigmax, sigmay, sigmaz;
  int node;

  ResolutionTree->Branch("node",&node, "node/I");
  ResolutionTree->Branch("x",&x, "x/F");
  ResolutionTree->Branch("y",&y, "y/F");
  ResolutionTree->Branch("z",&z, "z/F");
  ResolutionTree->Branch("rx",&rx, "rx/F");
  ResolutionTree->Branch("ry",&ry, "ry/F");
  ResolutionTree->Branch("rz",&rz, "rz/F");
  ResolutionTree->Branch("sigmax",&sigmax, "sigmax/F");
  ResolutionTree->Branch("sigmay",&sigmay, "sigmay/F");
  ResolutionTree->Branch("sigmaz",&sigmaz, "sigmaz/F");

  for ( node = 0; node<nodes; node++ )
    {
      int i0this = node * ScanNumberOfRuns; //set has the same coordinates for ScanNumberOfRuns events, using 0th
      x = Scan[i0this]->Points[0].r[0];
      y = Scan[i0this]->Points[0].r[1];
      z = Scan[i0this]->Points[0].r[2];
      //qDebug()<<"x, y, z:"<<x<<y<<z;
      //averages
      double sum[3] = {0.0, 0.0, 0.0};
      int goodEvents = 0;
      for (int ievRelative = 0; ievRelative < ScanNumberOfRuns; ievRelative++)
        {
          int iev = ievRelative + node*ScanNumberOfRuns;
          if (iev > ReconstructionData[igroup].size()-1) break;
          if (!ReconstructionData[igroup][iev]->GoodEvent) continue;
          goodEvents++;
          for (int j=0;j<3; j++) sum[j] += ReconstructionData[igroup][iev]->Points[0].r[j];
        }
      float factor = (goodEvents == 0) ? 0 : 1.0/goodEvents;
      for (int j=0;j<3; j++) sum[j] *= factor;
      rx = sum[0];
      ry = sum[1];
      rz = sum[2];
      //qDebug()<<"rx, ry, rz:"<<rx<<ry<<rz;

      //deltas
      float asigma[3] = {0.0, 0.0, 0.0};
      for (int ievRelative = 0; ievRelative < ScanNumberOfRuns; ievRelative++)
        {
          int iev = ievRelative + node*ScanNumberOfRuns;
          if (!ReconstructionData[igroup][iev]->GoodEvent) continue;
          for (int j=0;j<3; j++)
            asigma[j] += (ReconstructionData[igroup][iev]->Points[0].r[j] - sum[j]) * (ReconstructionData[igroup][iev]->Points[0].r[j] - sum[j]);
        }
      factor = (goodEvents < 2) ? 0 : 1.0/(goodEvents-1);
      for (int j=0;j<3; j++) asigma[j] = sqrt(asigma[j]*factor);
      sigmax = asigma[0];
      sigmay = asigma[1];
      sigmaz = asigma[2];
      //qDebug()<<"sigx, sigy, sigz, sig:"<<sigmax<<sigmay<<sigmaz;

      ResolutionTree->Fill();
    }
  //qDebug()<<"Tree is ready";
  return true;
}

bool EventsDataClass::saveReconstructionAsTree(QString fileName, pms *PMs, bool fIncludePMsignals, bool fIncludeRho, bool fIncludeTrue, int igroup)
{
  clearReconstructionTree();
  QByteArray ba = fileName.toLocal8Bit();
  const char *c_str = ba.data();

  TFile* f = new TFile(c_str,"recreate");
  bool fOK = createReconstructionTree(PMs, fIncludePMsignals, fIncludeRho, fIncludeTrue, igroup);
  if (!fOK) return false;
  int result = f->Write();
  f->Close();
  delete f;
  ReconstructionTree = 0; //Tree does not exist after save!  
  return (result == 0) ? false : true;
}

bool EventsDataClass::saveReconstructionAsText(QString fileName, int igroup)
{
  if (igroup > ReconstructionData.size()-1)
  {
      qWarning() << "bad sensor group index!";
      return false;
  }

  QFile outputFile(fileName);
  outputFile.open(QIODevice::WriteOnly);
  if(!outputFile.isOpen())
    {
      qWarning() << "Unable to open file " +fileName+ " for writing!";
      return false;
    }
  int size = ReconstructionData[igroup].size();
  QTextStream outStream(&outputFile);

  for (int iev=0; iev<size; iev++)
    if (ReconstructionData[igroup][iev]->GoodEvent)
      {       
        int Points = ReconstructionData[igroup][iev]->Points.size();
        for (int iP = 0; iP<Points; iP++)
          {            
            outStream<<ReconstructionData[igroup][iev]->Points[iP].r[0]<<" ";
            outStream<<ReconstructionData[igroup][iev]->Points[iP].r[1]<<" ";
            outStream<<ReconstructionData[igroup][iev]->Points[iP].r[2]<<"  "; //2 spaces
            outStream<<ReconstructionData[igroup][iev]->Points[iP].energy<<"   "; //3 spaces
          }
        outStream<<"  "<<ReconstructionData[igroup][iev]->chi2<<" "; //5 spaces including trailing
        outStream<<"    "<<ReconstructionData[igroup][iev]->EventId<<" "; //event id
        outStream<<"\r\n";
      }
  outputFile.close();
  return true;
}

bool EventsDataClass::saveSimulationAsTree(QString fileName)
{
  QByteArray ba = fileName.toLocal8Bit();
  const char *c_str = ba.data();
  TFile* f = new TFile(c_str,"recreate");
  TTree *tree = new TTree("SimulationTree","Simulation data");

  int numPMs = (Events.size()>0) ? Events[0].size() : 0;
  int tbins = 1;
  if (isTimed())
    if (Events.size()>0)
      tbins = TimedEvents[0].size();
  //qDebug()<<"time bins:"<<tbins;
  std::vector <double> x;
  std::vector <double> y;
  std::vector <double> z;
  double zStop;
  std::vector <int> numPhotons; //int numPhots;
  int ScintType, GoodEvent, EventType;
  int iev;

  tree->Branch("i",&iev, "i/I");
  if (!Scan.isEmpty())
    {
      tree->Branch("x", &x);
      tree->Branch("y", &y);
      tree->Branch("z", &z);
      tree->Branch("zStop",&zStop, "zStop/D");
      tree->Branch("numPhotons", &numPhotons);
      tree->Branch("ScintType",&ScintType, "ScintType/I");
      tree->Branch("GoodEvent",&GoodEvent, "GoodEvent/I");
      tree->Branch("EventType",&EventType, "EventType/I");
    }
  float* signal;
  signal = new float[numPMs];
  char buf[32];
  sprintf(buf, "signal[%d]/F", numPMs);
  tree->Branch("signal", signal, buf);
  std::vector< std::vector <float> > signalTimed;  //  [timebin][ipm]
  signalTimed.resize(tbins);
  for (int itime=0; itime<tbins; itime++) signalTimed[itime].resize(numPMs);
  if (isTimed()) tree->Branch("signalTimed", &signalTimed);

  for (iev=0; iev<Events.size(); iev++)
    {
      for (int ipm=0; ipm<numPMs; ipm++) signal[ipm] = Events[iev][ipm];

      if (isTimed())
        for (int ipm=0; ipm<numPMs; ipm++)
          for (int itime=0; itime<tbins; itime++)
            signalTimed[itime][ipm] = TimedEvents[iev][itime][ipm];

      if (!Scan.isEmpty())
        {
          int Points = Scan[iev]->Points.size();
          x.resize(Points);
          y.resize(Points);
          z.resize(Points);
          numPhotons.resize(Points);
          for (int iP=0; iP<Points; iP++)
            {
              x[iP] = Scan[iev]->Points[iP].r[0];
              y[iP] = Scan[iev]->Points[iP].r[1];
              z[iP] = Scan[iev]->Points[iP].r[2];
              numPhotons[iP] = Scan[iev]->Points[iP].energy;
            }
          zStop = Scan[iev]->zStop;
          ScintType = Scan[iev]->ScintType;
          GoodEvent = Scan[iev]->GoodEvent;
          EventType = Scan[iev]->EventType;
        }
      tree->Fill();
    }

  TParameter<int> *numRuns = new TParameter<int>("numRuns",ScanNumberOfRuns);
  tree->GetUserInfo()->Add(numRuns);

  int result = f->Write() ;
  f->Close();
  delete f;
  tree = 0;
  delete [] signal;
  return (result == 0) ? false : true;
}

bool EventsDataClass::saveSimulationAsText(QString fileName)
{
  QFile outputFile(fileName);
  outputFile.open(QIODevice::WriteOnly);
  if(!outputFile.isOpen())
    {
      qWarning()<<"Unable to open file"<<fileName<<"for writing!";
      return false;
    }
  int size = Events.size();
  bool fTimed = isTimed();
  int numPMs = (size>0) ? Events[0].size() : 0;
  int tbins = (fTimed) ? TimedEvents[0].size() : 1;

  QTextStream outStream(&outputFile);
  if (fTimed) outStream<<"Time bins "<<tbins<<"\r\n";  //header!

  for (int iev=0; iev<size; iev++)
    for (int itime=0; itime<tbins; itime++)
      {
        if (fTimed)
          for (int ipm=0; ipm<numPMs; ipm++)
            outStream << TimedEvents[iev][itime][ipm]<<" ";
        else
          for (int ipm=0; ipm<numPMs; ipm++)
            outStream << Events[iev][ipm]<<" ";

        if (!Scan.isEmpty())
          {
            outStream << "    "; //5 spaces including trailing one
            for (int ip=0; ip<Scan[iev]->Points.size(); ip++)
              {
                outStream << (int)Scan[iev]->Points[ip].energy << " ";
                outStream << Scan[iev]->Points[ip].r[0] << " "
                          << Scan[iev]->Points[ip].r[1] << " "
                          << Scan[iev]->Points[ip].r[2] << "   "; //3 spaces
              }
          }
        outStream<<"\r\n";
      }
  outputFile.close();
  return true;
}

int EventsDataClass::loadEventsFromTxtFile(QString fileName, QJsonObject &jsonPreprocessJson, pms *PMs)
{
  ErrorString = "";
  fStopLoadRequested = false;
  QFile file(fileName);
  if(!file.open(QIODevice::ReadOnly | QFile::Text))
    {
      ErrorString = "Could not open: "+fileName;
      qWarning() << ErrorString;
      return -1;
    }

  APreprocessingSettings PreprocessingSettings;
  QString result = PreprocessingSettings.readFromJson(jsonPreprocessJson, PMs, QFileInfo(fileName).fileName());
  if (result == "-")
      ;// qDebug() << "Do not perform any preprocessing: settings are not provided";
  else if (!result.isEmpty())
    {
      ErrorString = result;
      //qWarning() << result; //already reported
      return -1;
    }
  //else qDebug() << "Preprocessing settings processed";

  if (PreprocessingSettings.fManifest && PreprocessingSettings.fHavePosition)
    {
      ErrorString = "Preprocessing cannot contain both manifest and true positions!";
      qWarning() << ErrorString;
      return -1;
    }


  clearReconstruction();

  QTextStream in(&file);
  QRegExp rx("(\\ |\\,|\\:|\\t)"); //separators: ' ' or ',' or ':' or '\t'
  QString line = in.readLine();
  //  qDebug()<<"Looking for the code line 'Time bins' in the first line of the file";
  // qDebug()<<"Read line:"<<line;
  QStringList fields = line.split(rx, QString::SkipEmptyParts);

  int tBins=1; //if tBins == 1 - no timed data will be kept!
  bool ok;
  if (fields.count() > 2)
     if (!fields[0].compare("Time"))
        if (!fields[1].compare("bins"))
            tBins = fields[2].toInt(&ok);

  //First loaded file defined Time-related strategy:
  //if no time - all timed data will be ignored in later files
  //if timed, all files with different number of time bits will be discarded
  int Forced_tBins;
  if (Events.isEmpty())
    { //this is the first file
      Forced_tBins = tBins;
      //qDebug()<<"tBins fored for all following files to "<<Forced_tBins;
    }
  else
    {
      if (TimedEvents.isEmpty()) Forced_tBins = 1;
      else Forced_tBins = TimedEvents[0].size();
      //qDebug()<<"Forced tBins is "<<Forced_tBins;
      if (Forced_tBins == 1)
        {
          //qDebug()<<"Ignoring timed info!";
        }
      else if (Forced_tBins != tBins)
        {
          ErrorString = "Mismatch in number of time bins detected, the file " + fileName + " is discarded!";
          qWarning() << ErrorString;
          file.close();
          return -1;
        }
    }

  //  qDebug()<<"Bins per PM: "<<tBins << PreprocessingSettings.fActive;
  if (tBins == 1) in.seek(0); //return to file start

  bool LoadEnergy = PreprocessingSettings.fActive && PreprocessingSettings.fHaveLoadedEnergy;
  int EnergyChannel = PreprocessingSettings.EnergyChannel;
  if (LoadEnergy && EnergyChannel < PMs->count())
    {
      ErrorString = "Error! Energy channel number should be >= the number of PMs";
      qWarning() << ErrorString;
      file.close();
      return -1;
    }
  bool LoadPosition = PreprocessingSettings.fActive && PreprocessingSettings.fHavePosition;
  int PositionXChannel = PreprocessingSettings.PositionXChannel;
  int PositionYChannel = PositionXChannel + 1;
  if (LoadPosition)
    {
      if (PositionXChannel < PMs->count() )
        {
          ErrorString = "Error! X position channel number should be >= the number of PMs";
          qWarning() << ErrorString;
          file.close();
          return -1;
        }
      if (LoadEnergy && (EnergyChannel==PositionXChannel || EnergyChannel==PositionYChannel) )
        {
          ErrorString = "Error! XY position channels overlaps energy channel";
          qWarning() << ErrorString;
          file.close();
          return -1;
        }
    }
  bool LoadZPosition = PreprocessingSettings.fActive && PreprocessingSettings.fHaveZPosition;
  int PositionZChannel = PreprocessingSettings.PositionZChannel;
  if (LoadZPosition)
    {
      if (PositionZChannel < PMs->count() )
        {
          ErrorString = "Error! Z position channel number should be >= the number of PMs";
          qWarning() << ErrorString;
          file.close();
          return -1;
        }
      if (LoadEnergy && EnergyChannel==PositionZChannel )
        {
          ErrorString = "Error! Z position channel overlaps energy channel";
          qWarning() << ErrorString;
          file.close();
          return -1;
        }
      if (LoadPosition && (PositionZChannel==PositionXChannel || PositionZChannel==PositionYChannel) )
        {
          ErrorString = "Error! Z position channel overlaps XY position channels";
          qWarning() << ErrorString;
          file.close();
          return -1;
        }
    }


  int UpperBound = PMs->count();
  if (LoadEnergy) UpperBound = EnergyChannel+1;
  if (LoadPosition) UpperBound = std::max(UpperBound, PositionYChannel+1);
  if (LoadZPosition) UpperBound = std::max(UpperBound, PositionZChannel+1);
  int DataSize = PMs->count();
  if (LoadEnergy) DataSize++;
  //position data are NOT added to tmp[][], so they do not influence DataSize

  int eventNumber = 0;

  int AppendedFrom = Events.size(); //data from this file will be appended from this position
  int AppendedTimedFrom = TimedEvents.size(); //will be mismatch with AppendedFrom if tBins == 0
  qint64 fileSize = file.size();
  qint64 alreadyRead = 0;
  double tmpEnergy;
  double positionX = 0, positionY = 0, positionZ = 0;

  while(!in.atEnd())
     {
        //Stop from GUI?
        if (fStopLoadRequested)
        {
            //user triggered stop
            ErrorString = "User requested abort";
            //clear data loaded from this file
            TimedEvents.resize(AppendedTimedFrom);
            if (LoadEnergy || LoadPosition || LoadZPosition)
              {
                for (int i=AppendedTimedFrom; i<Scan.size(); i++) delete Scan[i];
                Scan.resize(AppendedTimedFrom);
              }
            file.close();
            return -1;
        }

        // reading and storing data to LoadedTimedEvents
        bool error = false;
        //creating and shaping tmp storage   [timeBin][PM]
        QVector< QVector <float> > tmp;
        tmp.resize(tBins);
        tmp.squeeze();
        for (int t=0; t<tBins; t++)
          {
            tmp[t].resize(DataSize);
            tmp[t].squeeze();
          }

        tmpEnergy = 0; //accumulating loaded energy over all time bins
        for (int t=0; t<tBins; t++)
         {
          do  //this way we allow empty lines!
              {
                line = in.readLine();
                //qDebug() << line;
                alreadyRead += line.size();
                fields = line.split(rx, QString::SkipEmptyParts);
                if (in.atEnd()) break;
              }
          while (fields.isEmpty());

          if (fields.count() < UpperBound)
            {
              if (in.atEnd())
                {
                  error=true;
                  break;
                }
              QString str;
              str.setNum(fields.count());
              if (LoadEnergy || LoadPosition || LoadZPosition) ErrorString = fileName+" - found a line with number of channels ( "+str+" ) less than required to extract the energy or position information!";
              else ErrorString = fileName+" - found a line with number of PMs ( "+str+" ) which is less than the number of PMs defined in the current geometry!";
              qWarning() << ErrorString;
              //clear data loaded from this file
              TimedEvents.resize(AppendedTimedFrom);
              if (LoadEnergy || LoadPosition || LoadZPosition)
                {
                  for (int i=AppendedTimedFrom; i<Scan.size(); i++) delete Scan[i];
                  Scan.resize(AppendedTimedFrom);
                }
              file.close();
              return -1;
            }

          for (int i=0; i<UpperBound; i++)
            {
              bool ok;
              float val = fields[i].toFloat(&ok);
              //  qDebug()<<i<<ok<<val;
              if (!ok)
                {
                  ErrorString = fileName+" - wrong format - expecting float datatype";
                  qWarning() << ErrorString;
                  //clear data loaded from this file
                  TimedEvents.resize(AppendedTimedFrom);
                  if (LoadEnergy || LoadPosition || LoadZPosition)
                    {
                      for (int i=AppendedTimedFrom; i<Scan.size(); i++) delete Scan[i];
                      Scan.resize(AppendedTimedFrom);
                    }
                  file.close();
                  return -1;
                }

              if (i >= PMs->count())
                {
                  //can be only energy or position info here
                  if (LoadEnergy && i==EnergyChannel)
                    {
                      //energy channel
                      double energy;
                      if (PreprocessingSettings.fActive) energy = (val + PreprocessingSettings.LoadEnAdd) * PreprocessingSettings.LoadEnMulti;
                      else  energy = val;
                      tmp[t][PMs->count()] = energy;
                      tmpEnergy += energy;
                    }
                  else if (LoadZPosition && i==PositionZChannel) positionZ = val;
                  else if (LoadPosition)
                    {
                      if (i == PositionXChannel) positionX = val;
                      else if (i == PositionYChannel) positionY = val;
                    }
                  //otherwise do nothing
                }
              else
                {
                  //PM channels
                  if (PreprocessingSettings.fActive)
                    { //saturation control
                      if (PreprocessingSettings.fIgnoreThresholds)
                        {
                          if (val < PreprocessingSettings.ThresholdMin ) error = true;
                          if (val > PreprocessingSettings.ThresholdMax ) error = true;
                        }
                      tmp[t][i] = (val + PMs->at(i).PreprocessingAdd) * PMs->at(i).PreprocessingMultiply;                      
                    }
                  else tmp[t][i] = val;
                }

            } //end cycle by channels
         } // end cycle by time bins
        if (!error)
          {
            eventNumber++;
            //checking event number selecton
            if (PreprocessingSettings.fActive)
              {
                if (PreprocessingSettings.fLimitNumber && eventNumber > PreprocessingSettings.LimitMax) break;
                if (PreprocessingSettings.fManifest && PreprocessingSettings.ManifestItem->LimitEvents != -1)
                  if (eventNumber > PreprocessingSettings.ManifestItem->LimitEvents) break;
              }

            TimedEvents.append(tmp); //adding event

            if (LoadEnergy || LoadPosition || LoadZPosition)
              {
                // *** !!! for timed events: the last time_bin's XYZ is copied to Scan!
                AScanRecord* sc = new AScanRecord();
                sc->Points[0].r[0] = positionX;
                sc->Points[0].r[1] = positionY;
                sc->Points[0].r[2] = positionZ;
                sc->Points[0].energy = tmpEnergy;
                sc->ScintType = 1;
                Scan.append(sc);
              }

            //indication / app update
            int numAdded = TimedEvents.size()-AppendedTimedFrom;
            if (numAdded % 5000 == 0)
              {
                //qDebug() << numAdded;
                emit loaded(numAdded, 100.0/fileSize*alreadyRead);
                qApp->processEvents();
              }
          }
     }

   int numEvents = TimedEvents.size() - AppendedTimedFrom;
   // qDebug()<<"Done! Events found: "<<numEvents;
   file.close();

   //calculating total, deleting LoadedEventsTimed if tBins = 1
   Events.resize(AppendedFrom + numEvents);
   //   qDebug()<<"      numEvents, tBins, DataSize:"<<numEvents<<tBins<<DataSize;
   for (int iev=0; iev<numEvents; iev++)
     {
       Events[AppendedFrom + iev].resize(DataSize);
       Events[AppendedFrom + iev].squeeze();
       for (int ipm = 0; ipm<DataSize; ipm++)
         {
            Events[AppendedFrom + iev][ipm]=0;
            for (int t=0; t<tBins; t++) Events[AppendedFrom + iev][ipm] += TimedEvents[AppendedTimedFrom + iev][t][ipm];
         }

       if (PreprocessingSettings.fManifest) // this mode can not be together with LoadPositions
         {
           AScanRecord* sc = new AScanRecord();
           sc->Points[0].r[2] = 0;
           sc->Points[0].energy = 1.0;
           sc->GoodEvent = true;
           sc->ScintType = 0;

           if (PreprocessingSettings.ManifestItem->getType() == "hole")
             {
               sc->Points[0].r[0] = PreprocessingSettings.ManifestItem->X;
               sc->Points[0].r[1] = PreprocessingSettings.ManifestItem->Y;                          
             }
           else if (PreprocessingSettings.ManifestItem->getType() == "slit")
             {
               double &Angle = dynamic_cast<ManifestItemSlitClass*>(PreprocessingSettings.ManifestItem)->Angle; //angle with X axis in degrees
               if ( Angle == 0)
                 {
                   sc->Points[0].r[0] = 1e10; //undefined
                   sc->Points[0].r[1] = PreprocessingSettings.ManifestItem->Y;
                 }
               else if (Angle == 90)
                 {
                   sc->Points[0].r[0] = PreprocessingSettings.ManifestItem->X;
                   sc->Points[0].r[1] = 1e10; //undefined
                 }
               else
                 {
                   sc->Points[0].r[0] = 1e10; //undefined
                   sc->Points[0].r[1] = 1e10; //undefined
                 }
             }
           else
             {
               sc->Points[0].r[0] = 1e10; //undefined
               sc->Points[0].r[1] = 1e10; //undefined
             }

           Scan.append(sc);
         }
     }
   if (Forced_tBins == 1)
     {
       // qDebug()<<"  Deleting unused time-resolved data...";
       TimedEvents.resize(0);
     }
   fLoadedEventsHaveEnergyInfo = LoadEnergy;
   if (PreprocessingSettings.fManifest)
     {
       //transferring the ManifestItem to the EventsDataHub storage
       Manifest.append(PreprocessingSettings.ManifestItem);
       PreprocessingSettings.ManifestItem = 0;
     }
   fSimulatedData = false;
   //qDebug() << "File size:"<<fileSize << "read:"<<alreadyRead;   

   return numEvents;
}

bool EventsDataClass::overlayAsciiFile(QString fileName, bool fAddMulti, pms* PMs)
{
  qDebug() << fAddMulti<< PMs->at(0).PreprocessingAdd << PMs->at(0).PreprocessingMultiply;
  if (Events.isEmpty())
    {
      ErrorString = "Cannot overlay - events are empty";
      qWarning() << ErrorString;
      return false;
    }

  QFile file(fileName);
  if(!file.open(QIODevice::ReadOnly | QFile::Text))
    {
      ErrorString = "Could not open file "+fileName;
      qWarning() << ErrorString;
      return false;
    }

  QTextStream in(&file);
  QRegExp rx("(\\ |\\,|\\:|\\t)"); //separators: ' ' or ',' or ':' or '\t'
  QString line = in.readLine();
  //  qDebug()<<"Looking for the code line 'Time bins' in the first line of the file";
  // qDebug()<<"Read line:"<<line;
  QStringList fields = line.split(rx, QString::SkipEmptyParts);

  if (fields.count() > 2)
     if (fields[0] == "Time" && fields[1] == "bins")
       {
         ErrorString = "Time resolved files are not currently supported";
         qWarning() << ErrorString;
         return false;
       }
  in.seek(0); //return to file start

  //check that it is enough channels and events
  int numPMs = PMs->count();
  int iEvent = 0;
  while(!in.atEnd())
    {
      line = in.readLine();
      fields = line.split(rx, QString::SkipEmptyParts);
      if (fields.isEmpty()) continue; //allow empty lines
      if (fields.count() < numPMs)
        {
          ErrorString = "Found event line with data channels less than PMs";
          qWarning() << ErrorString;
          return false;
        }
      bool ok;
      for (int i=0; i<fields.count(); i++)
        {
          fields[i].toFloat(&ok);
          if (!ok)
            {
              ErrorString = "Failed to convert text to float";
              qWarning() << ErrorString;
              return false;
            }
        }
      iEvent++;
      if (iEvent == Events.size()) break;
    }
  if (iEvent != Events.size())
    {
      ErrorString = "Not enough data lines in file to cover all events";
      qWarning() << ErrorString;
      return false;
    }

  //all is fine, can read and overlay
  in.seek(0); //return to file start
  iEvent = 0;
  while(!in.atEnd())
    {
      line = in.readLine();
      fields = line.split(rx, QString::SkipEmptyParts);
      if (fields.isEmpty()) continue; //allow empty lines

      for (int i=0; i<fields.count(); i++)
        {
          float delta = fields[i].toFloat();
          if (fAddMulti) delta = (delta + PMs->at(i).PreprocessingAdd) * PMs->at(i).PreprocessingMultiply;
          Events[iEvent][i] += delta;
        }

      iEvent++;
      if (iEvent == Events.size()) break;
    }

  ErrorString = "";
  return true;
}

int EventsDataClass::loadSimulatedEventsFromTree(QString fileName, pms *PMs, int maxEvents)
{
  ErrorString = "";
  bool limitNumEvents = (maxEvents>0);
  QByteArray ba = fileName.toLocal8Bit();
  const char *c_str = ba.data();
  TFile* f = new TFile(c_str);
  TTree* T = 0;
  T = (TTree*)f->Get("SimulationTree");
  if (!T)
    {
      ErrorString = "Tree with simulated data not found!";
      qWarning() << ErrorString;
      f->Close();
      return -1;
    }

  bool FirstDataSet = Events.isEmpty();
  //if it is the first data set:
  //if there are no scan data - in any later files scan data is ignored
  //if scan data present - any later files without scan data will NOT be loaded

  int numPMs = PMs->count();
  int numEv = T->GetEntries();
  //qDebug() << "Number of events found: "+QString::number(numEv);

  bool ScanDataPresent = false; //Scan data exists?
  TBranch* xBranch = 0;
  xBranch = T->FindBranch("x");
  if (xBranch)
    {
      //qDebug()<<"ScanDataFound!";
      ScanDataPresent = true;
      if (!FirstDataSet && Scan.isEmpty())
        {
          qWarning()<<"Ignoring scan data in this file!";
          ScanDataPresent = false;
        }
    }
  else
    {
      //qDebug()<<"No scan data in the tree!";
      if (!FirstDataSet && !Scan.isEmpty())
        {
          ErrorString = "Sim tree file cannot be loaded - no required scan data present!";
          qWarning() << ErrorString;
          f->Close();
          return -1;
        }
    }

  //num of PMs in the Tree data?
  TBranch* signalBranch = T->FindBranch("signal");
  TString title = signalBranch->GetTitle();  //  "signal[%d]/D"    or "signal[%d]/F"
  bool fDoubleSignals = (title.Contains("D")) ? true : false;  //(title.Contains("F"))

  title.ReplaceAll("signal[","");
  title.ReplaceAll("]/D","");
  title.ReplaceAll("]/F","");

  int numPMsInTree = ((QString)title).toInt();
  //qDebug()<<"PMs in tree file="<<numPMsInTree;
  if (numPMsInTree != numPMs)
    {
      ErrorString = "Error: File contains less PMs than in this detector configuration!";
      qWarning() << ErrorString;
      f->Close();
      return -1;
    }

  //timed data?
  bool TimedData = false;
  TBranch* timeBranch = 0;
  timeBranch = T->FindBranch("signalTimed");
  if (timeBranch)
    {
      //qDebug() << "Data is time-resolved!";
      TimedData = true;
    }

  double zStop;
  std::vector <double> *x = 0, *y = 0, *z = 0;
  std::vector <int> *numPhotons = 0;
  int ScintType, GoodEvent, EventType;
  TBranch *bx = 0;

  if (ScanDataPresent)
    {
      T->SetBranchAddress("x", &x, &bx);
      T->SetBranchAddress("y", &y);
      T->SetBranchAddress("z", &z);
      T->SetBranchAddress("numPhotons", &numPhotons);

      T->SetBranchAddress("zStop", &zStop);
      T->SetBranchAddress("ScintType", &ScintType);
      T->SetBranchAddress("GoodEvent", &GoodEvent);
      T->SetBranchAddress("EventType", &EventType);
    }

  float *signalF = new float[numPMs];
  std::vector< std::vector <float> > *signalTimedF = 0;  //  [timebin][ipm]
  double *signalD = new double[numPMs];
  std::vector< std::vector <double> > *signalTimedD = 0;  //  [timebin][ipm]

  if (fDoubleSignals)
    {
      T->SetBranchAddress("signal", signalD);
      if (TimedData) T->SetBranchAddress("signalTimed", &signalTimedD);
    }
  else
    {
      T->SetBranchAddress("signal", signalF);
      if (TimedData) T->SetBranchAddress("signalTimed", &signalTimedF);
    }

  for (int iev=0; iev<numEv; iev++)
    {
      if (limitNumEvents && iev==maxEvents) break;

      Long64_t tentry = T->LoadTree(iev);
      T->GetEntry(tentry);

      if (ScanDataPresent)
        {
          AScanRecord* scs = new AScanRecord();
          int Points = x->size();
          //                        qDebug()<<"Points this event:"<<Points;
          if (Points != 1) scs->Points.Reinitialize(Points);
          for (int iP = 0; iP<Points; iP++)
            {
              scs->Points[iP].r[0] = x->at(iP);
              scs->Points[iP].r[1] = y->at(iP);
              scs->Points[iP].r[2] = z->at(iP);
              scs->Points[iP].energy = numPhotons->at(iP);
            }
          scs->zStop = zStop;
          scs->GoodEvent = GoodEvent;
          scs->EventType = EventType;
          scs->ScintType = ScintType;

          Scan.append(scs);          
        }

      QVector<float> PMsignals(numPMs);
      if (fDoubleSignals)
        {
         for (int ipm=0; ipm<numPMs; ipm++) PMsignals[ipm] = signalD[ipm];
         Events.append(PMsignals);

         if (TimedData)
           {
             int timeBins = signalTimedD->size();
             QVector< QVector <float> > PMsignalsTimed(timeBins);
             for (int itime=0; itime<timeBins; itime++)
               {
                 PMsignalsTimed[itime].resize(numPMs);

                 for (int ipm=0; ipm<numPMs; ipm++)
                   PMsignalsTimed[itime][ipm] = (*signalTimedD)[itime][ipm];
               }
             TimedEvents.append(PMsignalsTimed);
             delete signalTimedD;
           }
        }
      else
        {
         for (int ipm=0; ipm<numPMs; ipm++) PMsignals[ipm] = signalF[ipm];
         Events.append(PMsignals);

         if (TimedData)
           {
             int timeBins = signalTimedF->size();
             QVector< QVector <float> > PMsignalsTimed(timeBins);
             for (int itime=0; itime<timeBins; itime++)
               {
                 PMsignalsTimed[itime].resize(numPMs);

                 for (int ipm=0; ipm<numPMs; ipm++)
                   PMsignalsTimed[itime][ipm] = (*signalTimedF)[itime][ipm];
               }
             TimedEvents.append(PMsignalsTimed);
             delete signalTimedF;
           }
        }      
    }

  TObject* tmp = T->GetUserInfo()->FindObject("numRuns");
  TParameter<int> *numRuns = dynamic_cast<TParameter<int>*>(tmp);
  if (numRuns)
    {
      int NumRuns = numRuns->GetVal();
      //qDebug() << "Found numRuns info:"<< NumRuns;
      ScanNumberOfRuns = NumRuns;
    }
  fSimulatedData = true;

  T->ResetBranchAddresses();
  // qDebug()<<"SimulatedScan Object contains now"<<SimulatedScan.size()<<"entries";
  //qDebug()<<"Events object contains now"<<Events.size()<<"entries";
  // qDebug()<<"TimedEvents object contains now"<<TimedEvents.size()<<"entries";
  f->Close();
  delete[] signalD;
  delete[] signalF;
  return (limitNumEvents ? maxEvents : numEv);
}
