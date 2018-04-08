#ifdef __USE_ANTS_CUDA__

#include "cudamanagerclass.h"
#include "apmhub.h"
#include "apmgroupsmanager.h"
#include "sensorlrfs.h"
#include "eventsdataclass.h"
#include "lrfaxial.h"
#include "lrfcaxial.h"
#include "lrfxy.h"
#include "bspline3.h"
#ifdef TPS3M
#include "tpspline3m.h"
#else
#include "tpspline3.h"
#endif
#include "lrfsliced3d.h"
#include "apositionenergyrecords.h"
#include "ajsontools.h"
#include "amessage.h"

#include <QTextStream>
#include <QDebug>

extern "C" bool cuda_run(const int Method,  //0 - Axial 2D; 1 - XY; 2 - Slices; 3 - compressed axial; 4 - Composite
                         const int blockSizeXY,
                         const int iterations,
                         const float scale,
                         const float scaleReductionFactor,               
                         const int mlORchi2,
                         const bool ignoreLowSigPMs,
                         const float ignoreThresholdLow,
                         const float ignoreThresholdHigh,
                         const bool ignoreFarPMs,
                         const float ignoreDistance,
                         const float comp_r0,
                         const float comp_a,
                         const float comp_b,
                         const float comp_lam2,
                         const float* const PMx,
                         const float* const PMy,              
                         const int numPMs,
                         const float* lrfdata,
                         const int lrfFloatsPerPM,
                         const int p1,
                         const int p2,
                         const int lrfFloatsAxialPerPM,
                         const float* EventsData,
                         const int numEvents,
                         float *RecX,
                         float *RecY,
                         float *RecEnergy,
                         float *Chi2,
                         float *Proability,
                         float *ElapsedTime);

extern "C" const char* getLastCUDAerror();

CudaManagerClass::CudaManagerClass(APmHub* PMs, APmGroupsManager* PMgroups, SensorLRFs* SensLRF, EventsDataClass *eventsDataHub, ReconstructionSettings *RecSet, int currentGroup) :
   PMs(PMs), PMgroups(PMgroups), SensLRF(SensLRF), EventsDataHub(eventsDataHub), CurrentGroup(currentGroup)
{  
  eventsData = 0; 
  lrfData = 0;  
  recX = recY = recEnergy = chi2 = probability = 0;
  numEvents = 0;
  numPMs = 0;
  Method = 0;
  p1 = 0;
  p2 = 0;

  QJsonObject js = RecSet->CGonCUDAsettings;
  parseJson(js, "ThreadBlockXY", BlockSizeXY);
  parseJson(js, "Iterations", Iterations);
  parseJson(js, "StartX", OffsetX);
  parseJson(js, "StartY", OffsetY);
  parseJson(js, "OptimizeMLChi2", MLorChi2);
  //have to invert - GUI has changed:
  if (MLorChi2 == 0) MLorChi2 = 1;
  else MLorChi2 = 0;
  parseJson(js, "ScaleReduction", ScaleReductionFactor);
  parseJson(js, "StartStep", Scale);
  parseJson(js, "StartOption", OffsetOption);
  parseJson(js, "StarterZ", StarterZ);

  //Dynamic passives
  if (js.contains("Passives"))
  {
      //old system, compatibility
      bool fPassives;
      parseJson(js, "Passives", fPassives);
      IgnoreLowSigPMs = false;
      IgnoreFarPMs = false;
      if (fPassives)
        {
          int pasOpt;
          parseJson(js, "PassiveOption", pasOpt);
          if (pasOpt == 0) IgnoreLowSigPMs = true;
          else IgnoreFarPMs = true;
        }
       parseJson(js, "Threshold", IgnoreThresholdLow);
       IgnoreThresholdHigh = 1.0e10;
       parseJson(js, "MaxDistance", IgnoreDistance);
  }
  else
  {
      //new system
      IgnoreLowSigPMs = RecSet->fUseDynamicPassivesSignal;
      IgnoreThresholdLow = RecSet->SignalThresholdLow;
      IgnoreThresholdHigh = RecSet->SignalThresholdHigh;
      IgnoreFarPMs = RecSet->fUseDynamicPassivesDistance;
      IgnoreDistance = RecSet->MaxDistance;
  }

  //qDebug() << "Doing ML=0/LS=1 :"<<MLorChi2;
  //qDebug() << BlockSizeXY << Iterations<< Scale<<ScaleReductionFactor<<OffsetOption<<OffsetX<<OffsetY<<MLorChi2<<IgnoreLowSigPMs<<IgnoreThresholdLow<<IgnoreThresholdHigh<<IgnoreFarPMs<<IgnoreDistance<<StarterZ;
}

CudaManagerClass::~CudaManagerClass()
{
  Clear();
}

bool CudaManagerClass::RunCuda(int bufferSize)
{
  //watchdogs
  numEvents = EventsDataHub->Events.size(); //num events defined
  if (numEvents == 0)
    {
      message("There are no events to reconstruct!");
      return false;
    }
  if (OffsetOption == 3 && EventsDataHub->isScanEmpty() )
    {
      message("Start from true XY selected, but scan data are empty!");
      return false;
    }
  if (!SensLRF->isAllLRFsDefined())
    {
      message("LRFs are not defined!");
      return false;
    }
  numPMs = PMs->count(); //num PMs defined
  numPMsStaticActive = PMgroups->countActives(); //num PMs for CUDA: all active PMs (dynamic were cleared!)
  if (numPMsStaticActive<2)
    {
      message("Not enough active PMs: "+QString::number(numPMsStaticActive)+" are defined!");
      return false;
    }

  //selecting reconstruction method
    //find the first active PM - it defined the method
  int iFirstActivePM = 0;
  for (;iFirstActivePM<numPMs; iFirstActivePM++)
    if (PMgroups->isActive(iFirstActivePM)) break;
  const LRF2* lrf =  (*SensLRF)[iFirstActivePM];
  const QString method = lrf->type();
  //Defining the method
  if (method == "Axial") Method = 0;
  else if (method == "XY") Method = 1;
  else if (method == "Sliced3D") Method = 2;
  else if (method == "ComprAxial") Method = 3;
  else if (method == "Composite") Method = 4;
  else
    {
      message("Unknown/non-implemented LRF parametrization method.\nCurrently implemented: Axial, Compessed_Axial, XY, Sliced3D");
      return false;
    }

  //confirming that all LRFs (active pms!) have the same type and setting (nodes number, compression parameters...)
  QJsonObject json0 = lrf->reportSettings();
  for (int ipm=iFirstActivePM+1; ipm<numPMs; ipm++)
    if (PMgroups->isActive(ipm))
     {
        if (json0 != (*SensLRF)[ipm]->reportSettings())
          {
            message("All lrfs have to have the same settings!");
            return false;
          }
     }
  //no need?:
  for (int ipm=0; ipm<numPMs; ipm++)
   if (PMgroups->isActive(ipm))
    {
      const LRF2* lrf =  (*SensLRF)[0];

      if ((Method == 0) && (lrf->type() != QString("Axial")))
        {
          message("All PMs should have axial LRFs!");
          return false;
        }
      if ((Method == 1) && (lrf->type() != QString("XY")))
        {
          message("All PMs should have XY LRFs!");
          return false;
        }
      if ((Method == 2) && (lrf->type() != QString("Sliced3D")))
        {
          message("All PMs should have Sliced3D LRFs!");
          return false;
        }
      if ((Method == 3) && (lrf->type() != QString("ComprAxial")))
        {
          message("All PMs should have ComprAxial LRFs!");
          return false;
        }
      if ((Method == 4) && (lrf->type() != QString("Composite")))
        {
          message("All PMs should have Composite LRFs!");
          return false;
        }
    }

  qDebug() << "->Cuda reconstruction";
  qDebug() << "-->Method:"<<method<< "Total PMs:"<<numPMs<<"Active PMs:"<<numPMsStaticActive;

  qDebug()<<"-->Starting cuda inits";
  qDebug()<<"--->Clear...";
  Clear(); //clear all containers
  qDebug()<<"--->PM centers...";
  ConfigurePMcenters();
  qDebug()<<"--->LRFs...";
  ConfigureLRFs(); //also defines slices data for Slice3D
  qDebug()<<"--->Outputs...";
  bool bOK = ConfigureOutputs(bufferSize);  //needs Slices defined!
  if (!bOK)
    {
      message("CUDA manager: Failed to reserve memory on host for the results");
      Clear();
      return false;
    }

  int from = 0;
  int to = bufferSize;
  float FullElapsedTime = 0;
  do
  {
      if (to > numEvents) to = numEvents;
      if (from == to) break;
      qDebug() << "--->Preparing reconstruction for events from"<< from << "to" << to;

      qDebug()<<"---->configure events";
      bool bOK = ConfigureEvents(from, to);
      if (!bOK)
        {
          message("CUDA manager: Failed to reserve memory on host to store events");
          Clear();
          return false;
        }


      if (Method == 2) bOK = CudaManagerClass::PerformSliced(from, to);
      else bOK = CudaManagerClass::Perform2D(from, to);
      qDebug()<<"---->Cuda run completed. Success:"<<bOK;
      FullElapsedTime += ElapsedTime;
      if (!bOK)
      {
          qDebug() << "Fail!";
          LastError = getLastCUDAerror();
          Clear();
          return false;
      }

      from += bufferSize;
      to   += bufferSize;
  }
  while (to <= numEvents);

  qDebug() << "---->GPU kernel cumulative execution time: " << FullElapsedTime << "ms";

  usPerEvent = (FullElapsedTime>0) ? 1000.0*FullElapsedTime/numEvents : 0;
  LastError = "";
  return true;
}

double CudaManagerClass::GetUsPerEvent() const
{
    return usPerEvent;
}

const QString &CudaManagerClass::GetLastError() const
{
    return LastError;
}

bool CudaManagerClass::Perform2D(int from, int to)
{
    int numEventsInBuffer = to - from;

    bool ok = cuda_run(Method,
            BlockSizeXY,
            Iterations,
            Scale,
            ScaleReductionFactor,       
            MLorChi2,
            IgnoreLowSigPMs,
            IgnoreThresholdLow,
            IgnoreThresholdHigh,
            IgnoreFarPMs,
            IgnoreDistance,
            comp_r0,
            comp_a,
            comp_b,
            comp_lam2,
            PMsX.data(),
            PMsY.data(),
            numPMsStaticActive,
            lrfData,
            lrfFloatsPerPM,
            p1,
            p2,
            lrfFloatsAxialPerPM,
            eventsData,
            numEventsInBuffer,
            recX,
            recY,
            recEnergy,
            chi2,
            probability,
            &ElapsedTime);

  if (ok) DataToAntsContainers(from, to);
  return ok;
}

bool CudaManagerClass::PerformSliced(int from, int to)
{
    int numEventsInBuffer = to - from;
    float ElapsedTimeDelta = 0;

    for (int iz = 0; iz<zSlices; iz++)
      {
        qDebug()<<"==>Running cuda, slice="<<iz;
        bool ok = cuda_run(1,  //reusing XY-lrf method
                      BlockSizeXY,
                      Iterations,
                      Scale,
                      ScaleReductionFactor,             
                      MLorChi2,
                      IgnoreLowSigPMs,
                      IgnoreThresholdLow,
                      IgnoreThresholdHigh,
                      IgnoreFarPMs,
                      IgnoreDistance,
                      0.0,     //compression cannot be used with XY-sym lrfs
                      0.0,
                      0.0,
                      0.0,
                      PMsX.data(),
                      PMsY.data(),
                      numPMsStaticActive,
                      lrfSplineData[iz].data(),
                      lrfFloatsPerPM,
                      p1,
                      p2,
                      lrfFloatsAxialPerPM,
                      eventsData,
                      numEventsInBuffer,
                      recX,
                      recY,
                      recEnergy,
                      chi2,
                      probability,
                      &ElapsedTimeDelta);

        if (!ok) return false;
        qDebug()<<"==> done!";

        //storing temporary data
        for (int iev=0; iev<numEventsInBuffer; iev++)
          {
           RecData[iz][iev].X = recX[iev];
           RecData[iz][iev].Y = recY[iev];
           RecData[iz][iev].Chi2 = chi2[iev];
           RecData[iz][iev].Energy = recEnergy[iev];
           if (MLorChi2 == 0) RecData[iz][iev].Probability = probability[iev];
          }

        //if (MLorChi2 == 0) qDebug()<<iz<<"First event> Prob:"<<probability[0]<<"  x,y,E,chi2:"<<recX[0]<<recY[0]<<recEnergy[0]<<chi2[0];
        //else qDebug()<<iz<<"First event> Chi2:"<<chi2[0]<<"  x,y,E:"<<recX[0]<<recY[0]<<recEnergy[0];
        ElapsedTime += ElapsedTimeDelta;
      }

    //success! - processing results
    for (int iev = 0; iev<numEventsInBuffer; iev++)
      {
        //finding best z
        int imax = 0;
        float maxProb, minChi2;

        if (MLorChi2 == 0) maxProb = RecData[0][iev].Probability;
        else minChi2 = RecData[0][iev].Chi2;

        for (int iz=1; iz<zSlices; iz++)
          {
            if (MLorChi2 == 0)
              {
                const float& Prob = RecData[iz][iev].Probability;
                if (Prob > maxProb)
                  {
                    maxProb = Prob;
                    imax = iz;
                  }
              }
            else
              {
                const float& Chi2 = RecData[iz][iev].Chi2;
                if (Chi2 < minChi2)
                  {
                    minChi2 = Chi2;
                    imax = iz;
                  }
              }
          }

        AReconRecord* rec = EventsDataHub->ReconstructionData[CurrentGroup][from + iev];
        rec->Points[0].r[0] = RecData[imax][iev].X;
        rec->Points[0].r[1] =  RecData[imax][iev].Y;
        const LRFsliced3D *lrf = dynamic_cast<const LRFsliced3D*>( (*SensLRF)[0] );
        if (lrf)
           rec->Points[0].r[2] = lrf->getSliceMedianZ(imax); //like in CPU based
        else
           rec->Points[0].r[2] = Slice0Z + imax * SliceThickness;

        rec->Points[0].energy = RecData[imax][iev].Energy;
        rec->chi2 = RecData[imax][iev].Chi2;
        bool bGoodEvent = ( rec->chi2 != 1.0e10 );
        rec->ReconstructionOK = bGoodEvent;
        rec->GoodEvent = bGoodEvent;

        //qDebug()<<"Imax:"<<imax<<"  prob:"<<maxProb<<" z0="<<Slice0Z<<"  z="<<Slice0Z + imax * SliceThickness;
      }
    return true;
}

void CudaManagerClass::ConfigurePMcenters()
{  
   for (int ipm=0; ipm<numPMs; ipm++)
    if (PMgroups->isActive(ipm))
      {
        PMsX.append(PMs->X(ipm));
        PMsY.append(PMs->Y(ipm));
      }
}

bool CudaManagerClass::ConfigureEvents(int from, int to)
{
  int eventSize = numPMsStaticActive + 2; //PM signals of active PMs + XY coordinates of grid center
  int numEventsInBuffer = to - from;

  //allocating event data container
  if (eventsData) delete [] eventsData;
  eventsData = 0;
  try
    {
      eventsData = new float[numEventsInBuffer * eventSize];
    }
  catch (...)
    {
      return false;
    }

  //filling events (only active PMs!)
  for (int iev = 0; iev<numEventsInBuffer; iev++)
    {
      int ipm = 0;
      for (int iActualPM = 0; iActualPM < numPMs; iActualPM++) //iActualPM - acual PM index, ipm - pm index for CUDA
        if (PMgroups->isActive(iActualPM))
          { //adding only active PMs! - note - dynamic status was cleared for all PMs!
              eventsData[iev*eventSize + ipm] = EventsDataHub->Events.at(from + iev)[iActualPM];
              ipm++;
          }
    }

  //filling centers of grid for all events
  switch (OffsetOption)
  {
  case 0: //CoG XY -> center of grid
      for (int iev = 0; iev<numEventsInBuffer; iev++)
      {          
          float *cuEvent = &eventsData[iev*eventSize];          
          const AReconRecord* thisEv = EventsDataHub->ReconstructionData.at(0).at(from + iev);
          if (thisEv->ReconstructionOK)
          {
              cuEvent[numPMsStaticActive]   = thisEv->xCoG;
              cuEvent[numPMsStaticActive+1] = thisEv->yCoG;
          }
          else
          {
              //             qDebug() << "Cog failed, event# "<<iev;
              cuEvent[numPMsStaticActive]   = 1e10;
              cuEvent[numPMsStaticActive+1] = 1e10;
          }
      }
      break;
  case 1: //PM with maximum signal (corrected for gain) -> center of grid
      for (int iev = 0; iev<numEventsInBuffer; iev++)
      {
          float *cuEvent = &eventsData[iev*eventSize];
          const AReconRecord* thisEv = EventsDataHub->ReconstructionData.at(0).at(from + iev);
          if (thisEv->ReconstructionOK)
            {
              cuEvent[numPMsStaticActive]   = PMs->X(thisEv->iPMwithMaxSignal);
              cuEvent[numPMsStaticActive+1] = PMs->Y(thisEv->iPMwithMaxSignal);
            }
          else
            {
              //             qDebug() << "Cog failed, event# "<<iev;
              cuEvent[numPMsStaticActive]   = 1e10;
              cuEvent[numPMsStaticActive+1] = 1e10;
            }
      }
      break;
  case 2: //fixed initial grid center
      for (int iev = 0; iev<numEventsInBuffer; iev++)
      {          
          eventsData[iev*eventSize + numPMsStaticActive]   = OffsetX;
          eventsData[iev*eventSize + numPMsStaticActive+1] = OffsetY;
      }
      break;
  case 3: //true XY position
      for (int iev = 0; iev<numEventsInBuffer; iev++)
      {          
          eventsData[iev*eventSize + numPMsStaticActive]   = EventsDataHub->Scan[from + iev]->Points[0].r[0];
          eventsData[iev*eventSize + numPMsStaticActive+1] = EventsDataHub->Scan[from + iev]->Points[0].r[1];
      }
      break;
  default:
      {
        qCritical()<<"Unknown option for start center of grid!";
        return false;
      }
  }
  return true;
}

bool CudaManagerClass::ConfigureOutputs(int bufferSize)
{
  try
    {
      recX = new float[bufferSize];
      recY = new float[bufferSize];
      recEnergy = new float[bufferSize];
      chi2 = new float[bufferSize];
      probability = new float[bufferSize];
    }
  catch(...)
    {
      return false;
    }

  if (Method == 2)
    {
      //additional tmp storage
      RecData.resize(zSlices);
      for (int i=0; i<zSlices; i++) RecData[i].resize(bufferSize);
    }
  return true;
}

void CudaManagerClass::ConfigureLRFs()
{
  int mem = 0;
  switch (Method)
    {
    case 0:
    case 3:
      ConfigureLRFs_Axial();
      mem = numPMsStaticActive * lrfFloatsPerPM * 4;
      break;
    case 1:
      ConfigureLRFs_XY();
      mem = numPMsStaticActive * lrfFloatsPerPM * 4;
      break;
    case 2:
      ConfigureLRFs_Sliced3D();
      mem = lrfSplineData[0].size()*lrfSplineData.size() * 4;
      break;
    case 4:
      ConfigureLRFs_Composite();
      mem = numPMsStaticActive * lrfFloatsPerPM * 4;
      break;
    default:      
      break;
    }  
  qDebug() << "==>Cuda: LRF memory size requested:" << mem << "bytes";
}

void CudaManagerClass::ConfigureLRFs_Axial()
{
  //looking for the first active PM
  int iFirstActive=0;
  for (;iFirstActive<numPMs; iFirstActive++)
    if (PMgroups->isActive(iFirstActive)) break;
  LRFaxial *lrf = (LRFaxial*)(*SensLRF)[iFirstActive];
  int RadialNodes = lrf->getNint();
  //qDebug()<<"nodes:"<<RadialNodes;
  //qDebug()<<"LRF[0] at 0:"<<lrf->eval(0,0,0);
  //lrf = (LRFaxial*)(*Detector->SensLRF)[1];
  //qDebug()<<"LRF[1] at 0:"<<lrf->eval(0,0,0);
  //qDebug()<<"nint"<<lrf->GetNint();

  //if (lrfData) delete[] lrfData; //already is in Clear()
  lrfFloatsPerPM = RadialNodes+3+1; // (nodes+3) spline coefficients + maxRadius
  lrfData = new float[numPMsStaticActive * lrfFloatsPerPM];
  int ipm = 0;
  for (int iActualPM = 0; iActualPM<numPMs; iActualPM++)
   if (PMgroups->isActive(iActualPM))
    {
      LRFaxial *lrf = (LRFaxial*)(*SensLRF)[iActualPM]; //iActualPM - acual PM index, ipm - pm index for CUDA
      double gain = SensLRF->getIteration()->sensor(iActualPM)->GetGain();
      std::vector <Bspline3::value_type> coef = lrf->getSpline()->GetCoef();
      for (int i=0; i<lrfFloatsPerPM-1; i++)
         lrfData[lrfFloatsPerPM*ipm + i] = coef[i]*gain;

      float max = lrf->getRmax();
      lrfData[lrfFloatsPerPM*ipm + lrfFloatsPerPM-1] = max;
      ipm++;

     //qDebug()<< iActualPM << " x max:"<<max<<"    to"<<lrfFloatsPerPM*ipm + lrfFloatsPerPM-1;
    }

  p1 = 0; //does not use additional parameters
  p2 = 0;

  if (Method == 3)
    {
      //Compressed axial
      LRFcAxial *lrf = (LRFcAxial*)(*SensLRF)[iFirstActive];
      //comp_k = lrf->getCompr_k();
      comp_r0 = lrf->getCompr_r0();
      //comp_lam = lrf->getCompr_lam();
      comp_a = lrf->getCompr_a();
      comp_b = lrf->getCompr_b();
      comp_lam2 = lrf->getCompr_lam2();      
    }
  else
    {
      comp_r0 = comp_a = comp_b = comp_lam2 = 0;
    }
  //qDebug() << "Comp data:"  << comp_r0 << comp_a << comp_b << comp_lam2;
}

void CudaManagerClass::ConfigureLRFs_Composite()
{
  //looking for the first active PM
  int iFirstActive=0;
  for (;iFirstActive<numPMs; iFirstActive++)
    if (PMgroups->isActive(iFirstActive)) break;

  const LRFcomposite* lrfComposite = dynamic_cast<const LRFcomposite*>( (*SensLRF)[iFirstActive] );

  //--------------- Init Axial part -----------------
  const LRFaxial *lrfAxial = dynamic_cast<const LRFaxial*>( lrfComposite->lrfdeck.at(0) );
  int RadialNodes = lrfAxial->getNint();
    //qDebug()<<"Radial nodes:"<<RadialNodes;
    //qDebug()<<"First active PM#:"<<iFirstActive<<"AxialLRF at 0:"<<lrfAxial->eval(0,0,0);
  lrfFloatsAxialPerPM = RadialNodes+3+1; // (nodes+3) spline coefficients + maxRadius
  if (lrfAxial->type() == "ComprAxial") //Compressed axial
    {
      const LRFcAxial *lrfCA = dynamic_cast<const LRFcAxial*>( lrfComposite->lrfdeck.at(0) );
      comp_r0 = lrfCA->getCompr_r0();
      comp_a = lrfCA->getCompr_a();
      comp_b = lrfCA->getCompr_b();
      comp_lam2 = lrfCA->getCompr_lam2();
    }
  else
    {
      comp_r0 = comp_a = comp_b = comp_lam2 = 0;
    }
  //qDebug() << "Comp data:"  << comp_r0 << comp_a << comp_b << comp_lam2;
  //---------------- Init XY part -----------------
  const LRFxy *lrfXY = dynamic_cast<const LRFxy*>( lrfComposite->lrfdeck.at(1) );
  int xNodes = lrfXY->getNintX();
  p1 = xNodes;
  int yNodes = lrfXY->getNintY();
  p2 = yNodes;
    //qDebug()<<"XY nodes:"<<xNodes<<yNodes;
    //qDebug()<<"XY LRF at 0,0,0:"<<lrfXY->eval(0,0,0);
  int XYnodesDataSize = (xNodes+3) * (yNodes+3);  //(nodes+3) coeff in x * (nodes+3) coeff in y
  int lrfFloatsXYPerPM = XYnodesDataSize + 9; // adding:   dx, dy, sinphi, cosphi, flip,  minX, maxX, minY, maxY

    //qDebug() << "SumLRF at 0,0,0:"<<lrfComposite->eval(0,0,0);

  //reserving lrf coeff container
  lrfFloatsPerPM = lrfFloatsAxialPerPM + lrfFloatsXYPerPM;
  lrfData = new float[numPMsStaticActive * lrfFloatsPerPM];

  //filling LRF coeff data
  int ipm = 0;  //CUDA PM number - only static-active ones!
  for (int iActualPM = 0; iActualPM<numPMs; iActualPM++)
   if (PMgroups->isActive(iActualPM))
    {
      const LRFcomposite *lrfComposite = dynamic_cast<const LRFcomposite*>( (*SensLRF)[iActualPM] ); //iActualPM - acual PM index, ipm - pm index for CUDA
      const LRFaxial *lrfAxial = dynamic_cast<const LRFaxial*>( lrfComposite->lrfdeck.at(0) );
      const LRFxy *lrfXY = dynamic_cast<const LRFxy*>( lrfComposite->lrfdeck.at(1) );
      PMsensor *sensor = SensLRF->getIteration()->sensor(iActualPM);
      double gain = sensor->GetGain();

      //axial part
      {
        std::vector<Bspline3::value_type> coef = lrfAxial->getSpline()->GetCoef();
        for (int i=0; i<lrfFloatsAxialPerPM-1; i++)
          lrfData[lrfFloatsPerPM*ipm + i] = coef[i]*gain;
        float max = lrfAxial->getRmax();
        lrfData[lrfFloatsPerPM*ipm + lrfFloatsAxialPerPM-1] = max;
      }

      //xy part
      std::vector<double> coef = lrfXY->getSpline()->GetCoef();
      for (int i=0; i<XYnodesDataSize; i++)
         lrfData[lrfFloatsPerPM*ipm + lrfFloatsAxialPerPM + i] = coef[i] * gain;

      double dx, dy, phi;
      bool fFlip;
      sensor->GetTransform(&dx, &dy, &phi, &fFlip);
      float dFlip;
      if (fFlip) dFlip = 1.0; else dFlip = -1.0;

      lrfData[lrfFloatsPerPM*ipm + lrfFloatsPerPM-9] = dx;
      lrfData[lrfFloatsPerPM*ipm + lrfFloatsPerPM-8] = dy;
      lrfData[lrfFloatsPerPM*ipm + lrfFloatsPerPM-7] = sin(phi);
      lrfData[lrfFloatsPerPM*ipm + lrfFloatsPerPM-6] = cos(phi);
      lrfData[lrfFloatsPerPM*ipm + lrfFloatsPerPM-5] = dFlip;
      float minX = lrfAxial->getXmin();
      lrfData[lrfFloatsPerPM*ipm + lrfFloatsPerPM-4] = minX;
      float maxX = lrfAxial->getXmax();
      lrfData[lrfFloatsPerPM*ipm + lrfFloatsPerPM-3] = maxX;
      float minY = lrfAxial->getYmin();
      lrfData[lrfFloatsPerPM*ipm + lrfFloatsPerPM-2] = minY;
      float maxY = lrfAxial->getYmax();
      lrfData[lrfFloatsPerPM*ipm + lrfFloatsPerPM-1] = maxY;

      ipm++;
    }
}

void CudaManagerClass::ConfigureLRFs_XY()
{
  //looking for the first active PM
  int i=0;
  for (;i<numPMs; i++)
    //if (Detector->PMs->at(i).passive == 0) break;
    if (PMgroups->isActive(i)) break;
  LRFxy *lrf = (LRFxy*)(*SensLRF)[i];

  int xNodes = lrf->getNintX();
  p1 = xNodes;
  int yNodes = lrf->getNintY();
  p2 = yNodes;
//  qDebug()<<"nodes:"<<xNodes<<yNodes;
//  qDebug()<<"LRF[0] at 0,0,0:"<<lrf->Eval(0,0,0);
//  lrf = (LRFxy*)Detector->LRF->operator [](1);
//  qDebug()<<"LRF[1] at 0,0,0:"<<lrf->Eval(0,0,0);

  int nodesDataSize = (xNodes+3) * (yNodes+3);  //(nodes+3) coeff in x * (nodes+3) coeff in y
  lrfFloatsPerPM = nodesDataSize + 9; // adding:   dx, dy, sinphi, cosphi, flip,  minX, maxX, minY, maxY
  lrfData = new float[numPMsStaticActive * lrfFloatsPerPM];

  int ipm = 0;
  for (int iActualPM = 0; iActualPM<numPMs; iActualPM++)
   //if (Detector->PMs->at(iActualPM).passive == 0)
   if (PMgroups->isActive(iActualPM))
    {
       PMsensor *sensor = SensLRF->getIteration()->sensor(iActualPM);
      //LRFxy *lrf = (LRFxy*)(*Detector->SensLRF)[iActualPM];//iActualPM - acual PM index, ipm - pm index for CUDA
      LRFxy *lrf = (LRFxy*)sensor->GetLRF();//iActualPM - acual PM index, ipm - pm index for CUDA
      double gain = sensor->GetGain();
      std::vector <double> coef = lrf->getSpline()->GetCoef();
      //for (int i=0; i<lrfFloatsPerPM-4; i++)
      for (int i=0; i<nodesDataSize; i++)
         lrfData[lrfFloatsPerPM*ipm + i] = coef[i] * gain;

      double dx, dy, phi;
      bool fFlip;
      sensor->GetTransform(&dx, &dy, &phi, &fFlip);

      float dFlip;
      if (fFlip) dFlip = 1.0; else dFlip = -1.0;

      lrfData[lrfFloatsPerPM*ipm + lrfFloatsPerPM-9] = dx;
      lrfData[lrfFloatsPerPM*ipm + lrfFloatsPerPM-8] = dy;
      lrfData[lrfFloatsPerPM*ipm + lrfFloatsPerPM-7] = sin(phi);
      lrfData[lrfFloatsPerPM*ipm + lrfFloatsPerPM-6] = cos(phi);
      lrfData[lrfFloatsPerPM*ipm + lrfFloatsPerPM-5] = dFlip;

      float minX = lrf->getXmin();
      lrfData[lrfFloatsPerPM*ipm + lrfFloatsPerPM-4] = minX;
      float maxX = lrf->getXmax();
      lrfData[lrfFloatsPerPM*ipm + lrfFloatsPerPM-3] = maxX;
      float minY = lrf->getYmin();
      lrfData[lrfFloatsPerPM*ipm + lrfFloatsPerPM-2] = minY;
      float maxY = lrf->getYmax();
      lrfData[lrfFloatsPerPM*ipm + lrfFloatsPerPM-1] = maxY;
 /*
      double minmax[4];
      sensor->getGlobalMinMax(minmax);
      int dataIndex = lrfFloatsPerPM*ipm + lrfFloatsPerPM-4;
      for(int i = 0; i < 4; i++)
          lrfData[dataIndex+i] = minmax[i];
*/

      ipm++;

   //  qDebug()<<"y max:"<<maxY<<"    to"<<lrfFloatsPerPM*ipm + lrfFloatsPerPM-1;
    }
//  qDebug()<<"xy minmax:"<<lrfData[lrfFloatsPerPM-4]<<lrfData[lrfFloatsPerPM-3]<<lrfData[lrfFloatsPerPM-2]<<lrfData[lrfFloatsPerPM-1]; 
}

void CudaManagerClass::ConfigureLRFs_Sliced3D()
{
  //looking for the first active PM
  int i=0;
  for (;i<numPMs; i++)
    //if (Detector->PMs->at(i).passive == 0) break;
    if (PMgroups->isActive(i)) break;
  LRFsliced3D *lrf = (LRFsliced3D*)(*SensLRF)[i];

  int xNodes = lrf->getNintX();
  p1 = xNodes;
  int yNodes = lrf->getNintY();
  p2 = yNodes;
  //qDebug()<<"nodes:"<<xNodes<<yNodes;
  zSlices = lrf->getNintZ();
  //qDebug()<<"Z slices:"<<zSlices;
  double Zmin = lrf->getZmin();
  double Zmax = lrf->getZmax();
  double dZ = Zmax - Zmin;
  //qDebug()<<"Zmin, Zmax, dz"<<Zmin<<Zmax<<dZ;
  SliceThickness = dZ/zSlices;
  Slice0Z = -0.5*(zSlices-1)*SliceThickness;
  //qDebug()<<"0th slice mid is at z="<<Slice0Z<<"  slice thickness="<<SliceThickness;

  int nodesDataSize = (xNodes+3) * (yNodes+3);  //(nodes+3) coeff in x * (nodes+3) coeff in y
  lrfFloatsPerPM = nodesDataSize + 9; // adding:   dx, dy, sinphi, cosphi, flip,  minX, maxX, minY, maxY
  lrfSplineData.resize(zSlices);

  for (int iz=0; iz<zSlices; iz++)
    {
      //qDebug() << " Prepare LRF data for slice#"<<iz;
      lrfSplineData[iz].resize( numPMsStaticActive * lrfFloatsPerPM );

      int ipm = 0;
      for (int iActualPM = 0; iActualPM<numPMs; iActualPM++)
       //if (Detector->PMs->at(iActualPM).passive == 0)
       if (PMgroups->isActive(iActualPM))
        {
          //qDebug() << "  Extracting coefficients for PM#"<< iActualPM;
          LRFsliced3D *lrf = (LRFsliced3D*)(*SensLRF)[iActualPM];//iActualPM - acual PM index, ipm - pm index for CUDA
          const PMsensor *sensor = SensLRF->getIteration()->sensor(iActualPM);
          double gain = sensor->GetGain();
          //qDebug() << "  Gain = "<<gain;
          //qDebug() << "  Ponter to spline:" <<lrf->getSpline(iz);
          std::vector <double> coef = lrf->getSpline(iz)->GetCoef();
          for (int i=0; i<nodesDataSize; i++) lrfSplineData[iz][lrfFloatsPerPM*ipm + i] = coef[i] * gain;

          //qDebug() << "  Filling transform data";
          double dx, dy, phi;
          bool fFlip;
          sensor->GetTransform(&dx, &dy, &phi, &fFlip);
          float dFlip;
          if (fFlip) dFlip = 1.0; else dFlip = -1.0;

          lrfSplineData[iz][lrfFloatsPerPM*ipm + lrfFloatsPerPM-9] = dx;
          lrfSplineData[iz][lrfFloatsPerPM*ipm + lrfFloatsPerPM-8] = dy;
          lrfSplineData[iz][lrfFloatsPerPM*ipm + lrfFloatsPerPM-7] = sin(phi);
          lrfSplineData[iz][lrfFloatsPerPM*ipm + lrfFloatsPerPM-6] = cos(phi);
          lrfSplineData[iz][lrfFloatsPerPM*ipm + lrfFloatsPerPM-5] = dFlip;

          lrfSplineData[iz][lrfFloatsPerPM*ipm + lrfFloatsPerPM-4] = lrf->getXmin();;
          lrfSplineData[iz][lrfFloatsPerPM*ipm + lrfFloatsPerPM-3] = lrf->getXmax();
          lrfSplineData[iz][lrfFloatsPerPM*ipm + lrfFloatsPerPM-2] = lrf->getYmin();
          lrfSplineData[iz][lrfFloatsPerPM*ipm + lrfFloatsPerPM-1] = lrf->getYmax();

          /*
          double minmax[4];
          sensor->getGlobalMinMax(minmax);
          int dataIndex = lrfFloatsPerPM*ipm + lrfFloatsPerPM-4;
          for(int i = 0; i < 4; i++)
              lrfData[dataIndex+i] = minmax[i];
          */

          ipm++;
        }
    }
}

void CudaManagerClass::DataToAntsContainers(int from, int to)
{
  int numEventsInBuffer = to - from;

  for (int iev = 0; iev<numEventsInBuffer; iev++)
    {
      //qDebug()<<iev<<"> "<<recX[iev]<<recY[iev];
      if (recX[iev] == 1.0e10) continue; //CoG failed, no need to process
      AReconRecord* rec = EventsDataHub->ReconstructionData[CurrentGroup][from + iev];
      rec->chi2 = chi2[iev];
      if (rec->chi2 != 1.0e10)
      {
          rec->Points[0].r[0] = recX[iev];
          rec->Points[0].r[1] = recY[iev];
          rec->Points[0].r[2] = rec->zCoG; //StarterZ or loaded - already in zCoG
          rec->Points[0].energy = recEnergy[iev];
          rec->ReconstructionOK = true;
          rec->GoodEvent = true;
      }
      else
        {
          rec->ReconstructionOK = false;
          rec->GoodEvent = false;
        }
    }
}

void CudaManagerClass::Clear()
{
  if (eventsData) delete [] eventsData;
  eventsData = 0;
  if (lrfData) delete [] lrfData;
  lrfData = 0;
  if (recX) delete [] recX;
  recX = 0;
  if (recY) delete [] recY;
  recY = 0;
  if (recEnergy) delete [] recEnergy;
  recEnergy = 0;
  if (chi2) delete [] chi2;
  chi2 = 0;
  if (probability) delete [] probability;
  probability = 0;

  lrfSplineData.clear();
  lrfSplineData.squeeze();
  RecData.clear();
  RecData.squeeze();
  PMsX.clear();
  PMsX.squeeze();
  PMsY.clear();
  PMsY.squeeze();
}

#endif
