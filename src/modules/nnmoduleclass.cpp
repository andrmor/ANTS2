#include "nnmoduleclass.h"
#include "eventsdataclass.h"
#include "ajsontools.h"
#include "apmhub.h"
#include "apositionenergyrecords.h"

#include <QDebug>
#include <QVariant>

void KNNfilterClass::clear()
{
  if (dataset) delete[] dataset->ptr();
  dataset = 0;
  if (indices) delete[] indices->ptr();
  indices = 0;
  if (dists)   delete[] dists->ptr();
  dists = 0;
  LastSearchN = -1; 
  LastAveragedN = -1;
}

KNNfilterClass::KNNfilterClass()
{
  EventsDataHub = 0;
  PMs = 0;
  dataset = 0;
  indices = 0;
  dists   = 0;
  LastSearchN = -1;
  LastAveragedN = -1;
}

KNNfilterClass::~KNNfilterClass()
{
  clear();
}

bool KNNfilterClass::Init()
{  
  LastSearchN = -1;
  numEvents = EventsDataHub->Events.size();
  if (numEvents == 0)
    {
      qWarning("kNN filter: No Events!");
      return false;
    }
  numPMs = PMs->count();

  clear();
  dataset = new flann::Matrix<float>(new float[numEvents*numPMs], numEvents, numPMs);
  for (int iev=0; iev<numEvents; iev++)
  {
    float norm = 0;
    for (int ipm=0; ipm<numPMs; ipm++) norm += EventsDataHub->Events[iev][ipm] * EventsDataHub->Events[iev][ipm];
    norm = sqrt(norm);
    if (norm < (float)1.0e-10) norm = (float)1.0e-10;
    for (int ipm=0; ipm<numPMs; ipm++)
      (*dataset)[iev][ipm] = EventsDataHub->Events[iev][ipm]/norm;
  }
  return true;
}

void KNNfilterClass::kNNsearch(int n)
{
  LastSearchN = n;
  if (indices) delete[] indices->ptr();
  if (dists)   delete[] dists->ptr();

  indices = new flann::Matrix<int>(new int[numEvents*n], numEvents, n);
  dists   = new flann::Matrix<float> (new float[numEvents*n], numEvents, n);

  // construct an randomized kd-tree index using 4 kd-trees
  flann::Index<flann::L2<float> > index(*dataset, flann::KDTreeIndexParams(4));
  index.buildIndex();

  // do a knn search
  index.knnSearch(*dataset, *indices, *dists, n, flann::SearchParams(numPMs));
}

bool KNNfilterClass::calculateAverageDists(int n)
{
    //qDebug()<<"Calculating average dists"<<n;
  if (EventsDataHub->Events.size() == 0) return false;
  if (n > LastSearchN || n<2)
    {
      qCritical()<<"Atempt to average over wrong number of neighbours!";
      return false;  //if search is not perfomed, LastSearchN is -1; n cannot be larger than LastSearchN
    }

  averageDistNN.clear();
  for (int iev=0; iev<EventsDataHub->Events.size(); iev++)
    {
      float average = 0;
      for (int i=1; i<n; i++) //skipping the point itself, starting from closest neighbour
          average += (*dists)[iev][i];
      average /= (n-1);
      averageDistNN.append(average);
    }
  LastAveragedN = n;
  return true;
}

bool KNNfilterClass::prepareNNfilter(int KNNfilterAverageOver)
{
  if (!EventsDataHub)
    {
      qWarning() << "Events hub is not connected!";
      return false;
    }
    //qDebug()<< "lastSearchN/LastAverageN/thisAverageN" << LastSearchN<<LastAveragedN<<KNNfilterAverageOver;

  //check if data are already there
  if (LastAveragedN == KNNfilterAverageOver) return true;

  //check if can reuse the search data
  if (LastSearchN > -1  &&  KNNfilterAverageOver < LastSearchN)
    {
      calculateAverageDists(KNNfilterAverageOver);
      return true;
    }

  //else doing both search+calculation
  bool ok = Init();
  if (!ok)
    {
      qCritical()<<"Init of kNN filter failed!";
      clear();
      return false;
    }

  kNNsearch(KNNfilterAverageOver);
  calculateAverageDists(KNNfilterAverageOver);
  return true;
}

// ------------------------ Position reconstructor -------------
KNNreconstructorClass::KNNreconstructorClass()
{  
  Xdataset = 0;
  Ydataset = 0;
  fTrainedXYwithSameEventSet = false;
  Xindex = 0;
  Yindex = 0;
  numNeighbours = 10;
  useNeighbours = 10;
  numTrees = 4;
  weightMode = 0;
}

KNNreconstructorClass::~KNNreconstructorClass()
{
  clear();
}

void KNNreconstructorClass::clear()
{
  if (Xdataset) delete[] Xdataset->ptr();
  Xdataset = 0;
  if (Ydataset) delete[] Ydataset->ptr();
  Ydataset = 0;

  if (Xindex) delete Xindex;
  Xindex = 0;
  if (Yindex) delete Yindex;
  Yindex = 0;

  emit readyXchanged(false, 0);
  emit readyYchanged(false, 0);
}

bool KNNreconstructorClass::calibrate(char option) // x, y, b - (both x and y)
{
  ErrorString = "";
    //qDebug() << "kNN reconstructor calibration triggered with option:" << option;
  if (!EventsDataHub)
    {
      ErrorString = "Events hub is not connected!";
      qWarning() << ErrorString;
      return false;
    }
  if (EventsDataHub->isScanEmpty())
    {
      ErrorString = "Position data are absent!";
      qWarning() << ErrorString;
      return false;
    }
  if (EventsDataHub->countGoodEvents() == 0)
    {
      ErrorString = "kNN reconstructor: There are no events or no events pass all filters!";
      qWarning() << ErrorString;
      return false;
    }

  if (option == 'x')
    {
      emit readyXchanged(false, 0);
      if (Xdataset) delete Xdataset->ptr();
      Xdataset = 0;
      Xpositions.clear();

      bool ok = prepareData(option, &Xdataset);
      if (!ok) return false;

      if (Xindex) delete Xindex;
      Xindex = buildFlannIndex(Xdataset);
      if (!Xindex) return false;

      fTrainedXYwithSameEventSet = false;
      emit readyXchanged(true, Xpositions.size());
    }
  else if (option == 'y')
    {
      emit readyYchanged(false, 0);
      if (Ydataset) delete Ydataset->ptr();
      Ydataset = 0;
      Ypositions.clear();

      bool ok = prepareData(option, &Ydataset);
      if (!ok) return false;

      if (Yindex) delete Yindex;
      Yindex = buildFlannIndex(Ydataset);
      if (!Yindex) return false;

      fTrainedXYwithSameEventSet = false;
      emit readyYchanged(true, Ypositions.size());
    }
  else if (option == 'b')
    {
      emit readyXchanged(false, 0);
      emit readyYchanged(false, 0);
      if (Xdataset) delete Xdataset->ptr();
      Xdataset = 0;
      if (Ydataset) delete Ydataset->ptr();
      Ydataset = 0;
      Xpositions.clear();
      Ypositions.clear();

      bool ok = prepareData(option, &Xdataset);
      if (!ok) return false;

      if (Xindex) delete Xindex;
      Xindex = buildFlannIndex(Xdataset);
      if (!Xindex) return false;
      if (Yindex) delete Yindex;
      Yindex = 0;

      fTrainedXYwithSameEventSet = true;
      emit readyXchanged(true, Xpositions.size());
      emit readyYchanged(false, 0);
    }
  else
    {
      ErrorString = "kNN calibration: unknown option!";
      qWarning() << ErrorString;
      return false;
    }
    //qDebug() << "Calibration: OK";
  return true;
}

bool KNNreconstructorClass::prepareData(char option, flann::Matrix<float> **dataset)
{
  //have to take only those events which do not have "1e10" as corresponding Scan coordinate
  int numEvents = 0;
  for (int iev=0; iev<EventsDataHub->Events.size(); iev++)
    if (EventsDataHub->ReconstructionData[0][iev]->GoodEvent)
      {
        if (option=='x' && EventsDataHub->Scan[iev]->Points[0].r[0]==1e10) continue;
        if (option=='y' && EventsDataHub->Scan[iev]->Points[0].r[1]==1e10) continue;
        if (option=='b' && EventsDataHub->Scan[iev]->Points[0].r[0]==1e10 && EventsDataHub->Scan[iev]->Points[0].r[1]==1e10) continue;
        numEvents++;
      }
  if (numEvents < 100) /// *** absolute
    qWarning() << "Warning: Too few events in the calibration set!";

    //qDebug() << "Making dataset for option"<<option<<"using"<<numEvents<< "events";
  int numPMs = PMs->count();
  try
  {
    *dataset = new flann::Matrix<float> (new float[numEvents*numPMs], numEvents, numPMs);
  }
  catch(...)
  {
    ErrorString = "Failed to reserve space for the calibration dataset";
    return false;
  }

    //qDebug() << "Filling the dataset and the true positions";
  int iev=0; //event index inside dataset
  for (int ievAll=0; ievAll<EventsDataHub->Events.size(); ievAll++)
    if (EventsDataHub->ReconstructionData[0][ievAll]->GoodEvent)
      {
        if (option == 'x')
          {
            double &x = EventsDataHub->Scan[ievAll]->Points[0].r[0];
            if (x == 1e10) continue;
            Xpositions.append(x);
          }
        if (option == 'y')
          {
            double &y = EventsDataHub->Scan[ievAll]->Points[0].r[1];
            if (y == 1e10) continue;
            Ypositions.append(y);
          }
        if (option == 'b')
          {
            double &x = EventsDataHub->Scan[ievAll]->Points[0].r[0];
            double &y = EventsDataHub->Scan[ievAll]->Points[0].r[1];
            if (x == 1e10 && y == 1e10) continue;
            Xpositions.append(x);
            Ypositions.append(y);
          }

        //its a good event, performing normalisation
        float norm = 0;
        for (int ipm=0; ipm<numPMs; ipm++) norm += EventsDataHub->Events[ievAll][ipm] * EventsDataHub->Events[ievAll][ipm];
        norm = sqrt(norm);
        if (norm < (float)1.0e-10) norm = (float)1.0e-10;
        for (int ipm=0; ipm<numPMs; ipm++)
          (**dataset)[iev][ipm] = EventsDataHub->Events[ievAll][ipm]/norm;
        iev++;
      }
  return true;
}

flann::Index<flann::L1<float> >* KNNreconstructorClass::buildFlannIndex(flann::Matrix<float> *dataset)
{
  flann::Index<flann::L1<float> >* index;
  try
  {
    index = new flann::Index<flann::L1<float> > (*dataset, flann::KDTreeIndexParams(numTrees));
    index->buildIndex();
  }
  catch(...)
  {
    ErrorString = "Failed to build knn index";
    return NULL;
  }
  return index;
}

bool KNNreconstructorClass::reconstructEvents()
{  
  ErrorString = "";
    //qDebug() << "Rec uses"<<useNeighbours<<"neighbours";
  if ( !Xindex || (!Yindex && !fTrainedXYwithSameEventSet) )
    {
      ErrorString = "Calibration (index) data are not ready";
      qWarning() << ErrorString;
      return false;
    }
  if (Xpositions.isEmpty() || Ypositions.isEmpty())
    {
      ErrorString = "Calibration (position vectors) are not defined";
      qWarning() << ErrorString;
      return false;
    }

    //qDebug() << "Preparing data";
  int numEvents = EventsDataHub->Events.size();

  int* indicesContainer;

  try
  {
    indicesContainer = new int[numEvents*numNeighbours];
  }
  catch (...)
  {
    ErrorString = "Failed to reserve space for indices";
    return false;
  }

  float* distsContainer;
  try
  {
    distsContainer = new float[numEvents*numNeighbours];
  }
  catch (...)
  {
    ErrorString = "Failed to reserve space for distances";
    delete[] indicesContainer;
    return false;
  }

  flann::Matrix<int> indices(indicesContainer, numEvents, numNeighbours);
  flann::Matrix<float> dists(distsContainer, numEvents, numNeighbours);
  int numPMs = PMs->count();

  float* eventdataContainer;
  try
  {
    eventdataContainer = new float[numEvents*numPMs];
  }
  catch (...)
  {
    ErrorString = "Failed to reserve space for events";
    delete[] indicesContainer;
    delete[] distsContainer;
    return false;
  }

  flann::Matrix<float> eventData(eventdataContainer, numEvents, numPMs);
  for (int iev=0; iev<numEvents; iev++)
    {
      float norm = 0;
      for (int ipm=0; ipm<numPMs; ipm++) norm += EventsDataHub->Events[iev][ipm] * EventsDataHub->Events[iev][ipm];
      norm = sqrt(norm);
      if (norm < (float)1.0e-10) norm = (float)1.0e-10;
      for (int ipm=0; ipm<numPMs; ipm++)
        eventData[iev][ipm] = EventsDataHub->Events[iev][ipm]/norm;
    }

     //qDebug() << "Running search" << (fTrainedXYwithSameEventSet?"in X and Y":"in X");
   Xindex->knnSearch(eventData, indices, dists, numNeighbours, flann::SearchParams(numPMs));
     //qDebug() << "Reconstructing X";
   reconstructPositions(0, &indices, &dists);
   //for (int i=0; i<numNeighbours; i++) qDebug() << dists[0][i];
   if (fTrainedXYwithSameEventSet)
     {
         //qDebug() << "Reconstructing Y";
       reconstructPositions(1, &indices, &dists);
     }
   else
     {
         //qDebug() << "Running search in Y";
       Yindex->knnSearch(eventData, indices, dists, numNeighbours, flann::SearchParams(numPMs));
         //qDebug() << "Reconstructing Y";
       reconstructPositions(1, &indices, &dists);
     }

     //qDebug() << "Cleanup phase";
   delete[] eventData.ptr();
   delete[] indices.ptr();
   delete[] dists.ptr();
   return true;
}

void KNNreconstructorClass::reconstructPositions(int icoord, flann::Matrix<int> *indices, flann::Matrix<float> *dists)
{
  for (int iev=0; iev<EventsDataHub->Events.size(); iev++)
    {
      int tmpWeightMode = weightMode;

gotoSetNoWeightingLabel:
      int numGood = 0;
      float coord = 0;
      float weight = 0;
      float sumweights = 0;

      for (int i=0; i<numNeighbours; i++)
        {
          float thisCoord = ( (icoord==0) ? Xpositions.at( (*indices)[iev][i] ) : Ypositions.at( (*indices)[iev][i] ) );
          if (thisCoord == 1e10) continue;

          switch (tmpWeightMode)
          {
            case 0:
              weight = 1;
              break;
            case 1:
            case 2:
              {
                float &dist = (*dists)[iev][i];
                if (dist <1e-10) goto gotoSetNoWeightingLabel; //reset weighting mode to "no weighting" for this event
                weight = 1.0/dist;
                if (tmpWeightMode == 2) weight/= dist;
              }
              break;
            default:
              qCritical() << "Unknown weighting mode";
              exit(666);
          }

          coord += thisCoord*weight;
          sumweights += weight;

          numGood++;
          if (numGood >= useNeighbours) break;
        }

      if (numGood == 0)
          EventsDataHub->ReconstructionData[0][iev]->ReconstructionOK = false;
      else
        {
          coord /= sumweights;
          EventsDataHub->ReconstructionData[0][iev]->Points[0].r[icoord] = coord;
        }
    }
}

bool KNNreconstructorClass::isXready()
{
  return (Xindex && !Xpositions.isEmpty() && !fTrainedXYwithSameEventSet);
}

bool KNNreconstructorClass::isYready()
{
  return (Yindex && !Ypositions.isEmpty() && !fTrainedXYwithSameEventSet);
}

bool KNNreconstructorClass::isXYready()
{
  return (Xindex && !Xpositions.isEmpty() && !Ypositions.isEmpty() && fTrainedXYwithSameEventSet);
}

bool KNNreconstructorClass::readFromJson(QJsonObject &json)
{
  parseJson(json, "numNeighbours", numNeighbours);
  parseJson(json, "useNeighbours", useNeighbours);
  parseJson(json, "numTrees", numTrees);
  weightMode = 0; //compatibility - can be removed
  parseJson(json, "weightMode", weightMode);
    //qDebug() << "kNNrecSet configured: numneighbours->"<<numNeighbours<<"Use neighbours in rec:"<<useNeighbours<<" Num trees:"<<numTrees<<" Weighting mode:"<<weightMode;
  return true;
}

// ------------------------ NN module --------------------------
NNmoduleClass::NNmoduleClass(EventsDataClass *EventsDataHub, APmHub* PMs) : EventsDataHub(EventsDataHub)
{  
  Filter.configure(EventsDataHub, PMs);
  Reconstructor.configure(EventsDataHub, PMs);

  ScriptInterfacer = new AScriptInterfacer(EventsDataHub, PMs);
}

NNmoduleClass::~NNmoduleClass()
{
  delete ScriptInterfacer;
}

AScriptInterfacer::AScriptInterfacer(EventsDataClass *EventsDataHub, APmHub *PMs) :
  EventsDataHub(EventsDataHub), PMs(PMs), bCalibrationReady(false), NormSwitch(0), CalibrationEvents(0), FlannIndex(0) {}

QVariant AScriptInterfacer::getNeighboursDirect(const QVector<float>& point, int numNeighbours)
{
  if (point.size() != numPMs)
  {
      qDebug() << point.size() << "but expected" << numPMs;
      ErrorString = "Wrong number of dimensions in point dataset for kNN neighhbour serach!";
      return QVariantList();
  }

  if (!bCalibrationReady)
  {
      ErrorString = "Calibration set is empty!";
      return QVariantList();
  }
  ErrorString.clear();

  const QVector<QPair<int, float> > res = neighbours(point, numNeighbours);

   //packing results
   QVariantList vl;
   for (int i=0; i<res.size(); i++)
     {
       QVariantList el;
       el << res.at(i).first << res.at(i).second;
       vl.push_back(el);
     }

   return vl;
}

const QVector<QPair<int, float> > AScriptInterfacer::neighbours(const QVector<float>& point, int numNeighbours)
{
  int* indicesContainer;
  try
  {
    indicesContainer = new int[1*numNeighbours];
  }
  catch (...)
  {
    ErrorString = "Failed to reserve space for indices";
    return QVector<QPair<int, float>>();
  }

  float* distsContainer;
  try
  {
    distsContainer = new float[1*numNeighbours];
  }
  catch (...)
  {
    ErrorString = "Failed to reserve space for distances";
    delete[] indicesContainer;
    return QVector<QPair<int, float>>();
  }

  flann::Matrix<int> indices(indicesContainer, 1, numNeighbours);
  flann::Matrix<float> dists(distsContainer, 1, numNeighbours);

  float* eventdataContainer = new float[numPMs];
  flann::Matrix<float> eventData(eventdataContainer, 1, numPMs);

  // fill and normalisation
  const float norm = ( NormSwitch > 0 ? calculateNorm(point) : 1.0f );
  for (int ipm = 0; ipm < numPMs; ipm++)
    eventData[0][ipm] = point.at(ipm) / norm;

  FlannIndex->knnSearch(eventData, indices, dists, numNeighbours, flann::SearchParams(numPMs));

  //packing results
  QVector<QPair<int, float> > res;
  for (int i=0; i<numNeighbours; i++)
  {
      const QPair<int, float> pair(indices[0][i], dists[0][i]);
      res << pair;
  }

  delete[] eventData.ptr();
  delete[] indices.ptr();
  delete[] dists.ptr();

  return res;
}

QVariant AScriptInterfacer::getNeighbours(int ievent, int numNeighbours)
{
  if (!isValidEventIndex(ievent))
  {
      ErrorString = "Wrong event number";
      return QVariantList();
  }

  return getNeighboursDirect(EventsDataHub->Events.at(ievent), numNeighbours);
}

bool AScriptInterfacer::filterByDistance(int numNeighbours, float maxDistance, bool filterOutEventsWithSmallerDistance)
{
    if (!bCalibrationReady) return false;

    int numEvents = EventsDataHub->Events.size();

    int* indicesContainer;
    try
    {
      indicesContainer = new int[numEvents*numNeighbours];
    }
    catch (...)
    {
      ErrorString = "Failed to reserve space for indices";
      return false;
    }

    float* distsContainer;
    try
    {
      distsContainer = new float[numEvents*numNeighbours];
    }
    catch (...)
    {
      ErrorString = "Failed to reserve space for distances";
      delete[] indicesContainer;
      return false;
    }

    flann::Matrix<int> indices(indicesContainer, numEvents, numNeighbours);
    flann::Matrix<float> dists(distsContainer, numEvents, numNeighbours);

    float* eventdataContainer = new float[numEvents*numPMs];
    flann::Matrix<float> eventData(eventdataContainer, numEvents, numPMs);

    // fill and normalisation
    for (int iev = 0; iev < numEvents; iev++)
    {
       const float norm = ( NormSwitch > 0 ? calculateNorm(EventsDataHub->Events.at(iev)) : 1.0f );
       for (int ipm = 0; ipm < numPMs; ipm++)
            eventData[iev][ipm] = EventsDataHub->Events.at(iev).at(ipm) / norm;
    }

    FlannIndex->knnSearch(eventData, indices, dists, numNeighbours, flann::SearchParams(numPMs));

    for (int iev=0; iev<numEvents; iev++)
    {
        float avDist = 0;
        for (int in=0; in<numNeighbours; in++)
            avDist += dists[iev][in];
        avDist /= numNeighbours;

        if (filterOutEventsWithSmallerDistance)
        {
           if (avDist < maxDistance) EventsDataHub->ReconstructionData[0][iev]->GoodEvent = false;
        }
        else
        {
           if (avDist > maxDistance) EventsDataHub->ReconstructionData[0][iev]->GoodEvent = false;
        }
    }

    //qDebug() << "Cleanup phase";
    delete[] eventData.ptr();
    delete[] indices.ptr();
    delete[] dists.ptr();

    return true;
}

void AScriptInterfacer::clearCalibration()
{
  delete CalibrationEvents; CalibrationEvents = 0;
  delete FlannIndex; FlannIndex = 0;

  X.clear();
  Y.clear();
  Z.clear();
  E.clear();

  numPMs = 0;
  numCalibrationEvents = 0;
  bCalibrationReady = false;
}

bool AScriptInterfacer::setCalibration(bool bUseScan)
{
  if (bUseScan)
    {
      if (EventsDataHub->isScanEmpty())
        {          
          ErrorString = "There are no scan data!";
          return false;
        }
    }
  else
    {
      if (!EventsDataHub->isReconstructionReady())
        {
          ErrorString = "Reconstruction was not performed!";
          return false;
        }
    }

  ErrorString.clear();
  clearCalibration();

  numCalibrationEvents = EventsDataHub->countGoodEvents();
  if (numCalibrationEvents==0)
  {
      qWarning() << "No (good) calibration events provided";
      return false;
  }
  numPMs = PMs->count();
  try
  {
    CalibrationEvents = new flann::Matrix<float> (new float[numCalibrationEvents*numPMs], numCalibrationEvents, numPMs);
  }
  catch(...)
  {
    ErrorString = "Failed to reserve space for the calibration dataset";
    return false;
  }

  //qDebug() << "Filling the dataset and the true positions";
  int iev=0; //event index inside dataset
  for (int ievAll=0; ievAll<EventsDataHub->Events.size(); ievAll++)
    if (EventsDataHub->ReconstructionData.at(0).at(ievAll)->GoodEvent)
      {
        if (bUseScan)
          {
            X << EventsDataHub->Scan.at(ievAll)->Points[0].r[0];
            Y << EventsDataHub->Scan.at(ievAll)->Points[0].r[1];
            Z << EventsDataHub->Scan.at(ievAll)->Points[0].r[2];
            E << EventsDataHub->Scan.at(ievAll)->Points[0].energy;
          }
        else
          {
            X << EventsDataHub->ReconstructionData.at(0).at(ievAll)->Points[0].r[0];
            Y << EventsDataHub->ReconstructionData.at(0).at(ievAll)->Points[0].r[1];
            Z << EventsDataHub->ReconstructionData.at(0).at(ievAll)->Points[0].r[2];
            E << EventsDataHub->ReconstructionData.at(0).at(ievAll)->Points[0].energy;
          }

//        if (NormSwitch > 0)
//        {
//            const float norm = calculateNorm(ievAll);
//            for (int ipm=0; ipm<numPMs; ipm++)
//              (*CalibrationEvents)[iev][ipm] = EventsDataHub->Events[ievAll][ipm] / norm;
//        }
        const float norm = ( NormSwitch > 0 ? calculateNorm(EventsDataHub->Events.at(ievAll)) : 1.0f );
        for (int ipm = 0; ipm < numPMs; ipm++)
           (*CalibrationEvents)[iev][ipm] = EventsDataHub->Events.at(ievAll).at(ipm) / norm;

        iev++;
      }

  //building index
  try
  {
    FlannIndex = new flann::Index<flann::L1<float> > (*CalibrationEvents, flann::KDTreeIndexParams(4));
    FlannIndex->buildIndex();
  }
  catch(...)
  {
    FlannIndex = 0;
    ErrorString = "Failed to build knn index";
    return false;
  }

  bCalibrationReady = true;
  return true;
}

bool AScriptInterfacer::setCalibrationDirect(const QVector< QVector<float>>& data)
{
    ErrorString.clear();
    clearCalibration();

    numCalibrationEvents = data.size();
    if (numCalibrationEvents == 0)
    {
        qWarning() << "No calibration events were provided";
        return false;
    }
    numPMs = data.first().size();
    try
    {
      CalibrationEvents = new flann::Matrix<float> (new float[numCalibrationEvents*numPMs], numCalibrationEvents, numPMs);
    }
    catch(...)
    {
      ErrorString = "Failed to reserve space for the calibration dataset";
      return false;
    }

    //filling data with optional normalization
    for (int iev = 0; iev < numCalibrationEvents; iev++)
        {
          const float norm = ( NormSwitch > 0 ? calculateNorm(data.at(iev)) : 1.0f );
          for (int ipm = 0; ipm < numPMs; ipm++)
             (*CalibrationEvents)[iev][ipm] = data.at(iev).at(ipm) / norm;
        }

    //building index
    try
    {
      FlannIndex = new flann::Index<flann::L1<float> > (*CalibrationEvents, flann::KDTreeIndexParams(4));
      FlannIndex->buildIndex();
    }
    catch(...)
    {
      FlannIndex = 0;
      ErrorString = "Failed to build knn index";
      return false;
    }

    bCalibrationReady = true;
    return true;
}

QVector<float> AScriptInterfacer::evaluatePhPerPhE(int numNeighbours, float upperDistanceLimit, float maxSignal)
{
  if (!bCalibrationReady)
  {
      ErrorString = "Calibration set is empty!";
      return QVector<float>();
  }
  ErrorString.clear();

  QVector<float> avSigma2OverAv(numPMs, 0);
  QVector<int>   avSigma2OverAv_entries(numPMs, 0);

  for (int iEvent = 0; iEvent < numCalibrationEvents; iEvent++)
    {
      const QVector<QPair<int, float> > neighb = neighbours(EventsDataHub->Events.at(iEvent), numNeighbours);

      const float& LastDist = neighb.at(numNeighbours-1).second;
      if (LastDist > upperDistanceLimit) continue;

      for (int iPM = 0; iPM<numPMs; iPM++)
      {
          float sum = 0;
          float sum2 = 0;
          for (int iN = 0; iN < numNeighbours; iN++)
            {
              const int& iEvent = neighb.at(iN).first;
              const float& Sig = EventsDataHub->Events.at(iEvent).at(iPM);
              //const float& Dist = neighb.at(iN).second;

              sum += Sig;
              sum2 += Sig * Sig;
            }
          const float avSig = sum / numNeighbours;
          if (avSig > maxSignal) continue;
          const float sigma2 = (sum2 - 2.0*sum*avSig) / numNeighbours   +   avSig * avSig;
          const float sigma2OverAv = sigma2 / avSig;

          avSigma2OverAv[iPM] += sigma2OverAv;
          avSigma2OverAv_entries[iPM]++;
      }
    }

   for (int ipm=0; ipm<numPMs; ipm++)
       if (avSigma2OverAv_entries.at(ipm) > 1)
           avSigma2OverAv[ipm] /= avSigma2OverAv_entries.at(ipm);

   return avSigma2OverAv;
}

float AScriptInterfacer::calculateNorm(const QVector<float>& data) const
{
    switch (NormSwitch)
    {
    case 1: //sum signal
      {
        float norm = 0;
        for (int ipm = 0; ipm < numPMs; ipm++)
            norm += data.at(ipm);
        if (norm < 1.0e-10f) norm = 1.0e-10f;
        return norm;
      }
    case 2: //quadrature sum
      {
        float norm = 0;
        for (int ipm = 0; ipm < numPMs; ipm++)
            norm += data.at(ipm) * data.at(ipm);
        norm = sqrt(norm);
        if (norm < 1.0e-10f) norm = 1.0e-10f;
        return norm;
      }
    default: qWarning() << "Unknown normalization type!";
    case 0: //no norm
        return 1.0f;
    }
}

double AScriptInterfacer::getCalibrationEventX(int ievent)
{
  if (!isValidEventIndex(ievent)) return std::numeric_limits<double>::quiet_NaN();
  return X.at(ievent);
}

double AScriptInterfacer::getCalibrationEventY(int ievent)
{
  if (!isValidEventIndex(ievent)) return std::numeric_limits<double>::quiet_NaN();
  return Y.at(ievent);
}

double AScriptInterfacer::getCalibrationEventZ(int ievent)
{
  if (!isValidEventIndex(ievent)) return std::numeric_limits<double>::quiet_NaN();
  return Z.at(ievent);
}

double AScriptInterfacer::getCalibrationEventE(int ievent)
{
  if (!isValidEventIndex(ievent)) return std::numeric_limits<double>::quiet_NaN();
  return E.at(ievent);
}

QVariant AScriptInterfacer::getCalibrationEventSignals(int ievent)
{
  if (!isValidEventIndex(ievent)) return QVariantList();
  QVariantList l;
  for (int i=0; i<numPMs; i++)
    l << (*CalibrationEvents)[ievent][i];
  return l;
}

bool AScriptInterfacer::isValidEventIndex(int ievent)
{
  if (ievent < 0 || ievent >= EventsDataHub->Events.size()) return false;
  return true;
}
