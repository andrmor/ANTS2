#include "processorclass.h"
#include "pms.h"
#include "apmgroupsmanager.h"
#include "eventsdataclass.h"
#include "reconstructionsettings.h"
#include "dynamicpassiveshandler.h"
#include "apositionenergyrecords.h"
#include "aeventfilteringsettings.h"

#include <QDebug>

#include "TMath.h"
#include "Math/Functor.h"
#include "Minuit2/Minuit2Minimizer.h"

ProcessorClass::ProcessorClass(pms* PMs,
               APmGroupsManager* PMgroups,
               ALrfModuleSelector *LRFs,
               EventsDataClass *EventsDataHub,
               ReconstructionSettings *RecSet,
               int ThisPmGroup,
               int EventsFrom, int EventsTo) :
    PMs(PMs), PMgroups(PMgroups), LRFs(*LRFs), EventsDataHub(EventsDataHub), RecSet(RecSet), ThisPmGroup(ThisPmGroup), EventsFrom(EventsFrom), EventsTo(EventsTo)
{ 
  Progress = 0;
  eventsProcessed = 0;
  fFinished = false;
  fStopRequested = false;  
  DynamicPassives = new DynamicPassivesHandler(PMs, PMgroups, EventsDataHub);
  DynamicPassives->init(RecSet, ThisPmGroup);
}

ProcessorClass::~ProcessorClass()
{
  delete DynamicPassives;
}

void ProcessorClass::copyLrfsAndExecute()
{
  LRFs = LRFs.copyToCurrentThread();  
  emit lrfsCopied();
  execute();
}

void CoGReconstructorClass::execute()
{
  fFinished = false;
  float Factor = 100.0/(EventsTo-EventsFrom);
  eventsProcessed = 0;
  //qDebug() << Id<<"> Starting CoG reconstruction. Events from"<<EventsFrom<<" to "<<EventsTo-1;
  for (int iev=EventsFrom; iev<EventsTo; iev++)
    {
      AReconRecord* rec = EventsDataHub->ReconstructionData[ThisPmGroup][iev];
      const QVector< float >* PMsignals = &EventsDataHub->Events[iev];

      //first have to get PM with max signal, it might be used for PM selection (fCoGIgnoreFar)
      rec->iPMwithMaxSignal = 0;
      double maxSignal = -1.0e10;
      for (int i=0; i<PMs->count(); i++)
       if (!PMgroups->isTMPPassive(i))
        if (!PMgroups->isStaticPassive(i) || RecSet->fIncludePassive) //if fIncludePassive is true, max signal PM can be passive
          {
            //double sig = PMsignals->at(i) / Detector->PMs->at(i).relGain;
            double sig = PMsignals->at(i) / PMgroups->Groups.at(ThisPmGroup)->PMS.at(i).gain;
            if (sig > maxSignal)
              {
                maxSignal = sig;
                rec->iPMwithMaxSignal = i;
              }
          }

      //calculating sum signal of active PMs
      double SumHits = 0;
      for (int i=0; i<PMs->count(); i++)
        if (PMgroups->isActive(i))
          {
            //double sig = PMsignals->at(i) / Detector->PMs->at(i).relGain;
            double sig = PMsignals->at(i) / PMgroups->Groups.at(ThisPmGroup)->PMS.at(i).gain;
            if (RecSet->fCoGIgnoreBySignal)
              {
                if (sig < RecSet->CoGIgnoreThresholdLow) continue; //ignore this PM
                if (sig > RecSet->CoGIgnoreThresholdHigh) continue; //ignore this PM
              }
            if (RecSet->fCoGIgnoreFar)
              {
                double dx = PMs->X(i) - PMs->X(rec->iPMwithMaxSignal);
                double dy = PMs->Y(i) - PMs->Y(rec->iPMwithMaxSignal);
                if (dx*dx+dy*dy > RecSet->CoGIgnoreDistance2) continue; //ignore this PM
              }
            SumHits += sig;
          }

      if (SumHits <= 0.0)
        {
    //      qDebug()<<"CoG failed!";
          rec->ReconstructionOK = false;
          rec->GoodEvent = false;
          continue;
        }
            
      //doing reconstruction
      double SumX = 0;
      double SumY = 0;
      double SumZ = 0;
      if (RecSet->fReconstructZ && !RecSet->fCogForceFixedZ)
          {
             for (int ipm=0; ipm<PMs->count(); ipm++)
               {
                 //double sig = PMsignals->at(ipm) / Detector->PMs->at(ipm).relGain;
                 double sig = PMsignals->at(ipm) / PMgroups->Groups.at(ThisPmGroup)->PMS.at(ipm).gain;

                 //bool fActive = !Detector->PMs->at(ipm).isStaticPassive();
                 bool fActive = PMgroups->isActive(ipm);
                 if (fActive)
                  {
                    if (RecSet->fCoGIgnoreBySignal)
                      {
                        if (sig < RecSet->CoGIgnoreThresholdLow) continue; //ignore this PM
                        if (sig > RecSet->CoGIgnoreThresholdHigh) continue; //ignore this PM
                      }
                    if (RecSet->fCoGIgnoreFar)
                      {
                        double dx = PMs->X(ipm) - PMs->X(rec->iPMwithMaxSignal);
                        double dy = PMs->Y(ipm) - PMs->Y(rec->iPMwithMaxSignal);
                        if (dx*dx+dy*dy > RecSet->CoGIgnoreDistance2) continue; //ignore this PM
                      }
                    SumX += PMs->X(ipm) * sig;
                    SumY += PMs->Y(ipm) * sig;
                    SumZ += PMs->Z(ipm) * sig;
                  }                 
               }

             if (RecSet->fCoGStretch)
               {
                 rec->xCoG = SumX/SumHits * RecSet->CoGStretchX;
                 rec->yCoG = SumY/SumHits * RecSet->CoGStretchY;
                 rec->zCoG = SumZ/SumHits * RecSet->CoGStretchZ;
               }
             else
               {
                 rec->xCoG = SumX/SumHits;
                 rec->yCoG = SumY/SumHits;
                 rec->zCoG = SumZ/SumHits;
               }
          }
        else
          {
            SumX = 0;
            SumY = 0;
            for (int ipm=0; ipm<PMs->count(); ipm++)
              {
                //double sig = PMsignals->at(ipm) / Detector->PMs->at(ipm).relGain;
                double sig = PMsignals->at(ipm) / PMgroups->Groups.at(ThisPmGroup)->PMS.at(ipm).gain;

                //bool fActive = !Detector->PMs->at(ipm).isStaticPassive();
                bool fActive = PMgroups->isActive(ipm);
                if (fActive)
                 {
                   if (RecSet->fCoGIgnoreBySignal)
                     {
                       if (sig < RecSet->CoGIgnoreThresholdLow) continue; //ignore this PM
                       if (sig > RecSet->CoGIgnoreThresholdHigh) continue; //ignore this PM
                     }
                   if (RecSet->fCoGIgnoreFar)
                     {
                       double dx = PMs->X(ipm) - PMs->X(rec->iPMwithMaxSignal);
                       double dy = PMs->Y(ipm) - PMs->Y(rec->iPMwithMaxSignal);
                       if (dx*dx+dy*dy > RecSet->CoGIgnoreDistance2) continue; //ignore this PM
                     }
                   SumX += PMs->X(ipm) * sig;
                   SumY += PMs->Y(ipm) * sig;
                 }                
              }

            if (RecSet->fCoGStretch)
              {
                rec->xCoG = SumX/SumHits * RecSet->CoGStretchX;
                rec->yCoG = SumY/SumHits * RecSet->CoGStretchY;
              }
            else
              {
                rec->xCoG = SumX/SumHits;
                rec->yCoG = SumY/SumHits;
              }

            if (RecSet->Zstrategy == 0) rec->zCoG = RecSet->SuggestedZ;
            else rec->zCoG = EventsDataHub->Scan.at(iev)->Points[0].r[2];
          }
      //  qDebug()<<rec->xCoG<<rec->yCoG<<rec->zCoG;
      rec->ReconstructionOK = true;
      rec->GoodEvent = true;

      eventsProcessed++;
      Progress = eventsProcessed*Factor;
    }
  emit finished();
  fFinished = true;
}

RootMinReconstructorClass::RootMinReconstructorClass(pms* PMs,
                                                     APmGroupsManager* PMgroups,
                                                     ALrfModuleSelector *LRFs,
                                                     EventsDataClass *EventsDataHub,
                                                     ReconstructionSettings *RecSet,
                                                     int CurrentGroup,
                                                     int EventsFrom, int EventsTo,
                                                     bool UseGauss)
  : ProcessorClass(PMs, PMgroups, LRFs, EventsDataHub, RecSet, CurrentGroup, EventsFrom, EventsTo)
{    
  switch (RecSet->RMminuitOption)
    {
    case 0:
        //RootMinimizer = ROOT::Math::Factory::CreateMinimizer("Minuit2", "Migrad");
        RootMinimizer = new ROOT::Minuit2::Minuit2Minimizer(ROOT::Minuit2::kMigrad);
        break;
    default: qDebug() << "Error: unknown or not-supported minuit minimizer";
    case 1:
        //RootMinimizer = ROOT::Math::Factory::CreateMinimizer("Minuit2", "Simplex");
        RootMinimizer = new ROOT::Minuit2::Minuit2Minimizer(ROOT::Minuit2::kSimplex);
        break;
    }
  RootMinimizer->SetMaxFunctionCalls(RecSet->RMmaxCalls);
  RootMinimizer->SetMaxIterations(1000);
  RootMinimizer->SetTolerance(0.001);
  if (UseGauss)
    {
      if (RecSet->fRMuseML) FunctorLSML = new ROOT::Math::Functor(&MLstaticGauss, 5);
      else FunctorLSML = new ROOT::Math::Functor(&Chi2staticGauss, 5);
    }
  else
    {
      if (RecSet->fRMuseML) FunctorLSML = new ROOT::Math::Functor(&MLstatic, 5);
      else FunctorLSML = new ROOT::Math::Functor(&Chi2static, 5);
    }
  RootMinimizer->SetFunction(*FunctorLSML);
}

RootMinReconstructorClass::~RootMinReconstructorClass()
{
  delete RootMinimizer;
  delete FunctorLSML;
}

void RootMinReconstructorClass::execute()
{
  fFinished = false;
  float Factor = 100.0/(EventsTo-EventsFrom);
  eventsProcessed = 0;
  //qDebug() << Id<<"> Starting RootMin reconstruction. Events from"<<EventsFrom<<" to "<<EventsTo-1;

  for (int iev=EventsFrom; iev<EventsTo; iev++)
    {
      if (fStopRequested) break;
      AReconRecord *rec = EventsDataHub->ReconstructionData[ThisPmGroup][iev];
      if (rec->ReconstructionOK)
        {
          PMsignals = &EventsDataHub->Events[iev];
          if (RecSet->fUseDynamicPassives) DynamicPassives->calculateDynamicPassives(iev, rec);

          if (RecSet->fRMuseML)
            {
              LastMiniValue = 1.e100; // reset for the new event
              //RootMinimizer->SetFunction(f_ML); //moved to constructor
            }
          else
            {
              LastMiniValue = 1.e6; //reset for the new event
              //RootMinimizer->SetFunction(*f_LS);  //moved to constructor
            }
          //set variables to minimize
          if (RecSet->RMstartOption == 1)
            {
              //starting from XY of the centre of the PM with max signal
              RootMinimizer->SetVariable(0, "x", PMs->X(rec->iPMwithMaxSignal), RecSet->RMstepX);
              RootMinimizer->SetVariable(1, "y", PMs->Y(rec->iPMwithMaxSignal), RecSet->RMstepY);
            }
          else if (RecSet->RMstartOption == 2 && !EventsDataHub->isScanEmpty())
            {
              //start from true XY position
              RootMinimizer->SetVariable(0, "x", EventsDataHub->Scan[iev]->Points[0].r[0], RecSet->RMstepX);
              RootMinimizer->SetVariable(1, "y", EventsDataHub->Scan[iev]->Points[0].r[1], RecSet->RMstepY);
            }
          else
            {
              //else start from CoG data
              RootMinimizer->SetVariable(0, "x", rec->xCoG, RecSet->RMstepX);
              RootMinimizer->SetVariable(1, "y", rec->yCoG, RecSet->RMstepY);
            }
          if (RecSet->fReconstructZ) RootMinimizer->SetVariable(2, "z", rec->zCoG, RecSet->RMstepZ);
          else RootMinimizer->SetFixedVariable(2, "z", rec->zCoG);
          if (RecSet->fReconstructEnergy) RootMinimizer->SetLowerLimitedVariable(3, "e", RecSet->SuggestedEnergy, RecSet->RMstepEnergy, 0.);
          else RootMinimizer->SetFixedVariable(3, "e", RecSet->SuggestedEnergy);
          //prepare to transfer pointer to this object (Raimundo changed to memcpy at march 7th)
          //int intPoint = reinterpret_cast<int>(this);
          double dPoint;// = intPoint;
          void *thisvalue = this;
          memcpy(&dPoint, &thisvalue, sizeof(void *));
          //We need to fix for the possibility that double isn't enough to store void*
          RootMinimizer->SetFixedVariable(4, "p", dPoint);

          // do the minimization
          bool fOK = RootMinimizer->Minimize();
          //double MinValue = RootMinimizer->MinValue();
          //if (MinValue != MinValue)
          //  qDebug()<<"nan detected! Minimization success? "<<fOK;

          if (fOK)
            {
              const double *xs = RootMinimizer->X();
              rec->Points[0].r[0] = xs[0];
              rec->Points[0].r[1] = xs[1];
              rec->Points[0].r[2] = xs[2];
              rec->Points[0].energy = xs[3];
              //alread have "OK and Good" status from CoG
            }
          else
            {
              rec->chi2 = -1.0; //signal for double event recon that cog rec was allright (default chi2=0)
              rec->ReconstructionOK = false;
              rec->GoodEvent = false;
            }
        }
      //else CoG failed event

      eventsProcessed++;
      Progress = eventsProcessed*Factor;
    }
  emit finished();
  fFinished = true;
}

// -----Experimental-----
RootMinRangedReconstructorClass::RootMinRangedReconstructorClass(pms *PMs, APmGroupsManager *PMgroups, ALrfModuleSelector *LRFs, EventsDataClass *EventsDataHub, ReconstructionSettings *RecSet, int ThisPmGroup, int EventsFrom, int EventsTo, double Range, bool UseGauss) :
    RootMinReconstructorClass(PMs, PMgroups, LRFs, EventsDataHub, RecSet, ThisPmGroup, EventsFrom, EventsTo, UseGauss), Range(Range)
{
    qDebug() << "Warning! Experimental feature activated - reconstruction by RootMin for ranged X or Y\nScanX or ScanY of 1e10 -> free search, otherwise within"<<Range<<"from ScanX or ScanY value";
}

RootMinRangedReconstructorClass::~RootMinRangedReconstructorClass() {}

void RootMinRangedReconstructorClass::execute()
{
    fFinished = false;
    float Factor = 100.0/(EventsTo-EventsFrom);
    eventsProcessed = 0;
    //qDebug() << Id<<"> Starting RootMinRanged reconstruction. Events from"<<EventsFrom<<" to "<<EventsTo-1;
    //qDebug() << "Range:"<<Range<<"StepXY"<<RecSet->RMstepX<<RecSet->RMstepY<<"Start option:"<<RecSet->RMstartOption;
    //qDebug() << "ML?"<<RecSet->fRMuseML<<"Z?"<<RecSet->fReconstructZ<<"En?"<<RecSet->fReconstructEnergy;

    for (int iev=EventsFrom; iev<EventsTo; iev++)
      {
        if (fStopRequested) break;
        AReconRecord *rec = EventsDataHub->ReconstructionData[ThisPmGroup][iev];
        if (rec->ReconstructionOK)
          {
            PMsignals = &EventsDataHub->Events[iev];
            if (RecSet->fUseDynamicPassives) DynamicPassives->calculateDynamicPassives(iev, rec);

            // reset for the new event
            if (RecSet->fRMuseML) LastMiniValue = 1.e100;
            else LastMiniValue = 1.e6;

            //set variables to minimize
            // if Scan X or Y is 1e10, variable if free, else it is confined to the range of +-Range around Scan value
            RangedX = RangedY = false;
            for (int ia=0; ia<2; ia++)
            {
                const double& val = EventsDataHub->Scan[iev]->Points[0].r[ia];
                //qDebug() << ia << val << (val == 1.0e10);
                if (val == 1.0e10)
                {
                    //free variable
                    if (RecSet->RMstartOption == 1)
                      {  //starting from the centre of the PM with max signal
                        if (ia==0) RootMinimizer->SetVariable(0, "x", PMs->X(rec->iPMwithMaxSignal), RecSet->RMstepX);
                        else       RootMinimizer->SetVariable(1, "y", PMs->Y(rec->iPMwithMaxSignal), RecSet->RMstepY);
                      }
                    else if (RecSet->RMstartOption == 2)
                      {
                        //start from true XY position
                        if (ia==0) RootMinimizer->SetVariable(0, "x", EventsDataHub->Scan[iev]->Points[0].r[0], RecSet->RMstepX);
                        else       RootMinimizer->SetVariable(1, "y", EventsDataHub->Scan[iev]->Points[0].r[1], RecSet->RMstepY);
                      }
                    else
                      {
                        //else start from CoG data
                        if (ia==0) RootMinimizer->SetVariable(0, "x", rec->xCoG, RecSet->RMstepX);
                        else       RootMinimizer->SetVariable(1, "y", rec->yCoG, RecSet->RMstepY);
                      }
                }
                else
                {
                    // range variable
                    if (ia==0)
                      {
                        RootMinimizer->SetLimitedVariable(0, "x", val, RecSet->RMstepX, val-Range, val+Range);
                        RangedX = true;
                        CenterX = val;
                      }
                    else
                      {
                        RootMinimizer->SetLimitedVariable(1, "y", val, RecSet->RMstepY, val-Range, val+Range);
                        RangedY = true;
                        CenterY = val;
                      }
                }
            }

            if (RecSet->fReconstructZ) RootMinimizer->SetVariable(2, "z", rec->zCoG, RecSet->RMstepZ);
            else RootMinimizer->SetFixedVariable(2, "z", rec->zCoG);
            if (RecSet->fReconstructEnergy) RootMinimizer->SetLowerLimitedVariable(3, "e", RecSet->SuggestedEnergy, RecSet->RMstepEnergy, 0.);
            else RootMinimizer->SetFixedVariable(3, "e", RecSet->SuggestedEnergy);
            //prepare to transfer pointer to this object (Raimundo changed to memcpy at march 7th)
            //int intPoint = reinterpret_cast<int>(this);
            double dPoint;// = intPoint;
            void *thisvalue = this;
            memcpy(&dPoint, &thisvalue, sizeof(void *));
            //We need to fix for the possibility that double isn't enough to store void*
            RootMinimizer->SetFixedVariable(4, "p", dPoint);

            // do the minimization
            bool fOK = RootMinimizer->Minimize();
            //double MinValue = RootMinimizer->MinValue();
            //if (MinValue != MinValue)
            //  qDebug()<<"nan detected! Minimization success? "<<fOK;

            if (fOK)
              {
                const double *xs = RootMinimizer->X();
                rec->Points[0].r[0] = xs[0];
                rec->Points[0].r[1] = xs[1];
                rec->Points[0].r[2] = xs[2];
                rec->Points[0].energy = xs[3];
                //alread have "OK and Good" status from CoG
              }
            else
              {
                rec->chi2 = -1.0; //signal for double event recon that cog rec was allright (default chi2=0)
                rec->ReconstructionOK = false;
                rec->GoodEvent = false;
              }
          }
        //else CoG failed event

        eventsProcessed++;
        Progress = eventsProcessed*Factor;
      }
    emit finished();
    fFinished = true;
}
// -------------------------

RootMinDoubleReconstructorClass::RootMinDoubleReconstructorClass(pms* PMs,
                                                                 APmGroupsManager* PMgroups,
                                                                 ALrfModuleSelector *LRFs,
                                                                 EventsDataClass *EventsDataHub,
                                                                 ReconstructionSettings *RecSet,
                                                                 int CurrentGroup,
                                                                 int EventsFrom, int EventsTo)
    : ProcessorClass(PMs, PMgroups, LRFs, EventsDataHub, RecSet, CurrentGroup, EventsFrom, EventsTo)
{
  switch (RecSet->RMminuitOption)
    {
    case 0:
        //RootMinimizer = ROOT::Math::Factory::CreateMinimizer("Minuit2", "Migrad");
        RootMinimizer = new ROOT::Minuit2::Minuit2Minimizer(ROOT::Minuit2::kMigrad);
        break;
    default: qDebug() << "Error: unknown or not-supported minuit minimizer";
    case 1:
        //RootMinimizer = ROOT::Math::Factory::CreateMinimizer("Minuit2", "Simplex");
        RootMinimizer = new ROOT::Minuit2::Minuit2Minimizer(ROOT::Minuit2::kSimplex);
        break;
    }
  RootMinimizer->SetMaxFunctionCalls(RecSet->RMmaxCalls);
  RootMinimizer->SetMaxIterations(1000);
  RootMinimizer->SetTolerance(0.001);

  if (RecSet->fRMuseML) FunctorLSML = new ROOT::Math::Functor(&MLstaticDouble, 9);
  else FunctorLSML = new ROOT::Math::Functor(&Chi2staticDouble, 9);
  RootMinimizer->SetFunction(*FunctorLSML);
}
RootMinDoubleReconstructorClass::~RootMinDoubleReconstructorClass()
{
  delete RootMinimizer;
  delete FunctorLSML;
}

void RootMinDoubleReconstructorClass::execute()
{
  fFinished = false;
  float Factor = 100.0/(EventsTo-EventsFrom);
  eventsProcessed = 0;
  //qDebug() << Id<<"> Starting RootMin_2 reconstruction. Events from"<<EventsFrom<<" to "<<EventsTo-1;

  for (int iev=EventsFrom; iev<EventsTo; iev++)
    {
      if (fStopRequested) break;
      AReconRecord *rec = EventsDataHub->ReconstructionData[ThisPmGroup][iev];
      if (rec->ReconstructionOK || rec->chi2 == -1) //chi2=-1 if single rec failed, but CoG was ok
        {
          PMsignals = &EventsDataHub->Events[iev];
          if (RecSet->fUseDynamicPassives) DynamicPassives->calculateDynamicPassives(iev, rec);

          //=== Reconstructing as double event ===
          LastMiniValue = 1.0e6; //reset for the new event

          // for the moment, have a weak procedure to find the second starting coordinate - XY of PM with the second strongest signal
          int ipmWithSecondMaxSignal = 0;
          double SecondMaxSignal = -1.0e10;
          for (int ipm=0; ipm < PMsignals->count(); ipm++)
            if (DynamicPassives->isStaticActive(ipm))
             {
               if (ipm == rec->iPMwithMaxSignal) continue;
               //double sig = PMsignals->at(ipm) / Detector->PMs->at(ipm).relGain;
               double sig = PMsignals->at(ipm) / PMgroups->Groups.at(ThisPmGroup)->PMS.at(ipm).gain;
               if (sig > SecondMaxSignal)
                 {
                   SecondMaxSignal = sig;
                   ipmWithSecondMaxSignal = ipm;
                 }
              }
           //qDebug() << iPMwithMaxSignal << ipmWithSecondMaxSignal;

           RootMinimizer->SetVariable(0, "x1", PMs->X(rec->iPMwithMaxSignal), RecSet->RMstepX);
           RootMinimizer->SetVariable(1, "y1", PMs->Y(rec->iPMwithMaxSignal), RecSet->RMstepY);
           if (RecSet->fReconstructZ) RootMinimizer->SetVariable(2, "z1", rec->zCoG, RecSet->RMstepZ);
           else RootMinimizer->SetFixedVariable(2, "z1", rec->zCoG);
           if (RecSet->fReconstructEnergy) RootMinimizer->SetLowerLimitedVariable(3, "e1", 0.5*RecSet->SuggestedEnergy, RecSet->RMstepEnergy, 0.);
           else RootMinimizer->SetFixedVariable(3, "e1", 1.0);
           RootMinimizer->SetVariable(4, "x2", PMs->X(ipmWithSecondMaxSignal), RecSet->RMstepX);
           RootMinimizer->SetVariable(5, "y2", PMs->Y(ipmWithSecondMaxSignal), RecSet->RMstepY);
           if (RecSet->fReconstructZ) RootMinimizer->SetVariable(6, "z2", rec->zCoG, RecSet->RMstepZ);
           else RootMinimizer->SetFixedVariable(6, "z2", rec->zCoG);
           if (RecSet->fReconstructEnergy) RootMinimizer->SetLowerLimitedVariable(7, "e2", 0.5*RecSet->SuggestedEnergy, RecSet->RMstepEnergy, 0.);
           else RootMinimizer->SetFixedVariable(7, "e2", 1.0);
           //prepare to transfer pointer to this object (Raimundo changed to memcpy at march 7th)
           //int intPoint = reinterpret_cast<int>(this);
           double dPoint;// = intPoint;
           void *thisvalue = this;
           //We need to fix for the possibility that double isn't enough to store void*
           memcpy(&dPoint, &thisvalue, sizeof(void *));
           RootMinimizer->SetFixedVariable(8, "p", dPoint);

           bool fOK = RootMinimizer->Minimize();
           //qDebug()<<"-------------Minimization success? "<<fOK;
           //double MinValue = RootMinimizer->MinValue();
           //qDebug() << "MinValue:" << MinValue;
           if (fOK)
             {
               //reconstruction success
               const double *xs = RootMinimizer->X(); //results
               //calculating Chi2 for double events
               //qDebug() << "Starting chi2  calc for double";
               double chi2double = calculateChi2DoubleEvent(xs);
               //qDebug() << "done!";

               if (RecSet->MultipleEventOption == 1 || chi2double<rec->chi2) // == 2 - comparing two chi2
                 {
                   rec->Points[0].r[0] = xs[0];
                   rec->Points[0].r[1] = xs[1];
                   rec->Points[0].r[2] = xs[2];
                   rec->Points[0].energy = xs[3];
                   rec->Points.AddPoint(xs[4], xs[5], xs[6], xs[7]);
                   rec->chi2 = chi2double;
                   rec->ReconstructionOK = true;
                 }
               //else leaving the old record
             }
           else rec->ReconstructionOK = false;

        }
      //else CoG failed event

      eventsProcessed++;
      Progress = eventsProcessed*Factor;
    }
  emit finished();
  fFinished = true;
}

double RootMinDoubleReconstructorClass::calculateChi2DoubleEvent(const double *result)
{
  double x1 = result[0];
  double y1 = result[1];
  double z1 = result[2];
  double energy1 = result[3];
  double x2 = result[4];
  double y2 = result[5];
  double z2 = result[6];
  double energy2 = result[7];
  double sum = 0;

  for (int ipm = 0; ipm < PMsignals->count(); ipm++)
    if (DynamicPassives->isActive(ipm))
     {
       double LRFhere1 = LRFs.getLRF(ipm, x1, y1, z1) * energy1;
       double LRFhere2 = LRFs.getLRF(ipm, x2, y2, z2) * energy2;
       if (LRFhere1 == 0 || LRFhere2 == 0) return 1.0e20;

       double LRFhere = LRFhere1 + LRFhere2;
       double delta = LRFhere - PMsignals->at(ipm);
       if (RecSet->fWeightedChi2calculation)
         {
           double sigma2;
           double err1 = LRFs.getLRFErr(ipm, x1, y1, z1) * energy1;
           double err2 = LRFs.getLRFErr(ipm, x2, y2, z2) * energy2;
           sigma2 = LRFhere + err1*err1 + err2*err2; // if err is not calculated, 0 is returned
           sum += delta*delta/sigma2;
         }
       else sum += delta*delta;
     }
  int DegFreedomDouble = DynamicPassives->countActives()-1 -4; //sigma, 2xX, 2xY
  if (RecSet->fReconstructEnergy) {DegFreedomDouble--; DegFreedomDouble--;}
  if (RecSet->fReconstructZ) {DegFreedomDouble--; DegFreedomDouble--;}
  if (DegFreedomDouble<1) DegFreedomDouble = 1; //protection

  return sum / DegFreedomDouble;
}

double ProcessorClass::calculateChi2NoDegFree(int iev, AReconRecord *rec)
{
  if (RecSet->fUseDynamicPassives) DynamicPassives->calculateDynamicPassives(iev, rec);
  double sum = 0;

  for (int ipm = 0; ipm < PMs->count(); ipm++)
    if (DynamicPassives->isActive(ipm))
     {
       double LRFhere = LRFs.getLRF(ipm, rec->Points[0].r) * rec->Points[0].energy;
       if (LRFhere <= 0) return 1.0e20;

       double delta = LRFhere - EventsDataHub->Events.at(iev).at(ipm);
       if (RecSet->fWeightedChi2calculation)
         {
           double err = LRFs.getLRFErr(ipm, rec->Points[0].r)*rec->Points[0].energy;
           double sigma2 = LRFhere + err*err; // if err is not calculated, 0 is returned
           sum += delta*delta/sigma2;
         }
       else sum += delta*delta;
     }
  return sum;
}

double ProcessorClass::calculateMLfactor(int iev, AReconRecord *rec)
{
  if (RecSet->fUseDynamicPassives) DynamicPassives->calculateDynamicPassives(iev, rec);
  double sum = 0;

  for (int ipm = 0; ipm < PMs->count(); ipm++)
    if (DynamicPassives->isActive(ipm))
      {
       double LRFhere = LRFs.getLRF(ipm, rec->Points[0].r) * rec->Points[0].energy;
       if (LRFhere <= 0.) return 1.0e20;

       sum += EventsDataHub->Events.at(iev).at(ipm) * log(LRFhere) - LRFhere; //"probability"
     }
    //qDebug() << "Log Likelihood = " << sum;
  return -sum; //-probability, since look for minimum in the caller code
}

double Chi2static(const double *p) //0-x, 1-y, 2-z, 3-energy, 4-pointer to RootMinReconstructorClass
{
  double sum = 0;
  //Raimundo changed to memcpy at march 7th
  //int intPoint = p[4];
  void *thisvalue;
  memcpy(&thisvalue, &p[4], sizeof(void *));
  //RootMinReconstructorClass* Reconstructor = reinterpret_cast<RootMinReconstructorClass*>(intPoint);
  RootMinReconstructorClass* Reconstructor = (RootMinReconstructorClass*)thisvalue;
    //qDebug() << "point:"<<X<< Y<< Z<< energy;
  for (int ipm = 0; ipm < Reconstructor->PMs->count(); ipm++)
    if (Reconstructor->DynamicPassives->isActive(ipm))
     {
       double LRFhere = Reconstructor->LRFs.getLRF(ipm, p)*p[3];//X, Y, Z) * energy;
         //       qDebug() << "PM " << ipm << ": " << LRFhere;
       if (LRFhere <= 0)
           return Reconstructor->LastMiniValue *= 1.25; //if LRFs are not defined for this coordinates

       double delta = (LRFhere - Reconstructor->PMsignals->at(ipm));
       if (Reconstructor->RecSet->fWeightedChi2calculation)
         {
            double sigma2;
            double err = Reconstructor->LRFs.getLRFErr(ipm, p)*p[3];//X, Y, Z) * energy;
            sigma2 = LRFhere + err*err; // if err is not calculated, 0 is returned
            sum += delta*delta/sigma2;
         }
       else sum += delta*delta;
     }
    // qDebug() << "LS chi2 = " << sum;
  return Reconstructor->LastMiniValue = sum;
}

double Chi2staticGauss(const double *p)  //0-x, 1-y, 2-z, 3-energy, 4-pointer to RootMinReconstructorClass
{
  double sum = 0;
  void *thisvalue;
  memcpy(&thisvalue, &p[4], sizeof(void *));
  RootMinRangedReconstructorClass* Reconstructor = (RootMinRangedReconstructorClass*)thisvalue;
  for (int ipm = 0; ipm < Reconstructor->PMs->count(); ipm++)
    if (Reconstructor->DynamicPassives->isActive(ipm))
     {
       double LRFhere = Reconstructor->LRFs.getLRF(ipm, p)*p[3];
       if (LRFhere <= 0) return Reconstructor->LastMiniValue *= 1.25; //if LRFs are not defined for this coordinates

       double delta = (LRFhere - Reconstructor->PMsignals->at(ipm));
       if (Reconstructor->RecSet->fWeightedChi2calculation)
         {
            double sigma2;
            double err = Reconstructor->LRFs.getLRFErr(ipm, p)*p[3];//X, Y, Z) * energy;
            sigma2 = LRFhere + err*err; // if err is not calculated, 0 is returned
            sum += delta*delta/sigma2;
         }
       else sum += delta*delta;
     }

  const double sigma = Reconstructor->Range/2.35482;
  if ( fabs(sigma) > 1.0e-10)
    {
      if (Reconstructor->RangedX)
        {
          double po = (Reconstructor->CenterX - p[0]) / sigma;
          po *= po;
          double gauss = exp(-0.5*po);
          if (gauss < 0.0001) gauss = 0.0001;
          sum /= gauss;
        }
      if (Reconstructor->RangedY)
        {
          double po = (Reconstructor->CenterY - p[1]) / sigma;
          po *= po;
          double gauss = exp(-0.5*po);
          if (gauss < 0.0001) gauss = 0.0001;
          sum /= gauss;
        }
    }

  return Reconstructor->LastMiniValue = sum;
}

double Chi2staticDouble(const double *p)
{
  double sum = 0;
  double X1 = p[0];
  double Y1 = p[1];
  double Z1 = p[2];
  double energy1 = p[3];
  double X2 = p[4];
  double Y2 = p[5];
  double Z2 = p[6];
  double energy2 = p[7];
  //Raimundo changed to memcpy at march 7th
  //int intPoint = p[8];
  void *thisvalue;
  memcpy(&thisvalue, &p[8], sizeof(void *));
  //RootMinDoubleReconstructorClass* Reconstructor = reinterpret_cast<RootMinDoubleReconstructorClass*>(intPoint);
  RootMinDoubleReconstructorClass* Reconstructor = (RootMinDoubleReconstructorClass*)thisvalue;
  //  qDebug() << X1 << Y1<< Z1 << energy1<< "    "<< X2 << Y2<<Z2<<energy2;
  //for (int ipm = 0; ipm < Reconstructor->PMsignals->count(); ipm++)
  for (int ipm = 0; ipm < Reconstructor->PMs->count(); ipm++)
    if (Reconstructor->DynamicPassives->isActive(ipm))
     {
       double LRFhere1 = Reconstructor->LRFs.getLRF(ipm, X1, Y1, Z1) * energy1;
       double LRFhere2 = Reconstructor->LRFs.getLRF(ipm, X2, Y2, Z2) * energy2;
       if (LRFhere1 <= 0 || LRFhere2 <= 0)
         return Reconstructor->LastMiniValue *= 1.25;  // if LRFs are not defined for these coordinates

       double LRFhere = LRFhere1 + LRFhere2;
       double delta = (LRFhere - Reconstructor->PMsignals->at(ipm));
       if (Reconstructor->RecSet->fWeightedChi2calculation)
         {
           double sigma2;
           double err1 = Reconstructor->LRFs.getLRFErr(ipm, X1, Y1, Z1) * energy1;
           double err2 = Reconstructor->LRFs.getLRFErr(ipm, X2, Y2, Z2) * energy2;
           sigma2 = LRFhere + err1*err1 + err2*err2; // if err is not calculated, 0 is returned
           sum += delta*delta/sigma2;
         }
       else sum += delta*delta;
     }
  //qDebug()<<"Chi2:"<< sum;
  return Reconstructor->LastMiniValue = sum;
}

double MLstatic(const double *p) //0-x, 1-y, 2-z, 3-energy, 4-pointer to RootMinReconstructorClass
{
  double sum = 0;
  //Raimundo changed to memcpy at march 7th
  //int intPoint = p[4];
  void *thisvalue;
  memcpy(&thisvalue, &p[4], sizeof(void *));
  //RootMinReconstructorClass* Reconstructor = reinterpret_cast<RootMinReconstructorClass*>(intPoint);
  RootMinReconstructorClass* Reconstructor = (RootMinReconstructorClass*)thisvalue;

  //for (int ipm = 0; ipm < Reconstructor->PMsignals->count(); ipm++)
  for (int ipm = 0; ipm < Reconstructor->PMs->count(); ipm++)
    if (Reconstructor->DynamicPassives->isActive(ipm))
      {
       double LRFhere = Reconstructor->LRFs.getLRF(ipm, p)*p[3];//X, Y, Z) * energy;
       if (LRFhere <= 0.)
           return Reconstructor->LastMiniValue += fabs(Reconstructor->LastMiniValue) * 0.25;

       sum += Reconstructor->PMsignals->at(ipm)*log(LRFhere) - LRFhere; //measures probability
     }
    //qDebug() << "Log Likelihood = " << sum;
  return Reconstructor->LastMiniValue = -sum; //-probability, since we use minimizer
}

double MLstaticGauss(const double *p)
{
  double sum = 0;
  void *thisvalue;
  memcpy(&thisvalue, &p[4], sizeof(void *));
  RootMinRangedReconstructorClass* Reconstructor = (RootMinRangedReconstructorClass*)thisvalue;

  for (int ipm = 0; ipm < Reconstructor->PMs->count(); ipm++)
    if (Reconstructor->DynamicPassives->isActive(ipm))
      {
       double LRFhere = Reconstructor->LRFs.getLRF(ipm, p)*p[3];//X, Y, Z) * energy;
       if (LRFhere <= 0.)
           return Reconstructor->LastMiniValue += fabs(Reconstructor->LastMiniValue) * 0.25;

       sum += Reconstructor->PMsignals->at(ipm)*log(LRFhere) - LRFhere; //measures probability
     }

  const double sigma = Reconstructor->Range/2.35482;
  if ( fabs(sigma) > 1.0e-10)
    {
      if (Reconstructor->RangedX)
        {
          double po = (Reconstructor->CenterX - p[0]) / sigma;
          po *= po;
          sum *= exp(-0.5*po);
        }
      if (Reconstructor->RangedY)
        {
          double po = (Reconstructor->CenterY - p[1]) / sigma;
          po *= po;
          sum *= exp(-0.5*po);
        }
    }

  return Reconstructor->LastMiniValue = -sum; //-probability, since we use minimizer
}

double MLstaticDouble(const double *p) //0-x, 1-y, 2-z, 3-energy, 4567 x1y1z1en1 8-pointer to RootMinDoubleReconstructorClass
{
  double sum = 0;
  double X1 = p[0];
  double Y1 = p[1];
  double Z1 = p[2];
  double energy1 = p[3];
  double X2 = p[4];
  double Y2 = p[5];
  double Z2 = p[6];
  double energy2 = p[7];
  //Raimundo changed to memcpy at march 7th
  //int intPoint = p[8];
  void *thisvalue;
  memcpy(&thisvalue, &p[8], sizeof(void *));
  //RootMinDoubleReconstructorClass* Reconstructor = reinterpret_cast<RootMinDoubleReconstructorClass*>(intPoint);
  RootMinDoubleReconstructorClass* Reconstructor = (RootMinDoubleReconstructorClass*)thisvalue;

  //for (int ipm = 0; ipm < Reconstructor->PMsignals->count(); ipm++)
  for (int ipm = 0; ipm < Reconstructor->PMs->count(); ipm++)
    if (Reconstructor->DynamicPassives->isActive(ipm))
      {
       double LRFhere1 = Reconstructor->LRFs.getLRF(ipm, X1, Y1, Z1)*energy1;
       double LRFhere2 = Reconstructor->LRFs.getLRF(ipm, X2, Y2, Z2)*energy2;
       if (LRFhere1 <= 0.0 || LRFhere2 <= 0.0 )
           return Reconstructor->LastMiniValue += fabs(Reconstructor->LastMiniValue) * 0.25;
       double LRFhere = LRFhere1 + LRFhere2;

       sum += Reconstructor->PMsignals->at(ipm)*log(LRFhere) - LRFhere; //measures probability
     }
    //qDebug() << "Log Likelihood = " << sum;
  return Reconstructor->LastMiniValue = -sum; //-probability, since we use minimizer
}

void Chi2calculatorClass::execute()
{
  fFinished = false;
    //qDebug() << Id<<"> Calculating Chi2 for events from"<<EventsFrom<<" to "<<EventsTo-1;
  for (int iev=EventsFrom; iev<EventsTo; iev++)
    {
      AReconRecord *rec = EventsDataHub->ReconstructionData[ThisPmGroup][iev];
      if (!rec->ReconstructionOK) continue;
      double chi2 = calculateChi2NoDegFree(iev, rec);

      int DegFreedom = DynamicPassives->countActives()-1 -2; //sigma, X, Y
      if (RecSet->fReconstructEnergy) DegFreedom--;
      if (RecSet->fReconstructZ) DegFreedom--;
      if (DegFreedom<1) DegFreedom = 1; //protection       
      rec->chi2 = chi2 / DegFreedom;
    }
  emit finished();
  fFinished = true;
}

void EventFilterClass::execute()
{
    fFinished = false;
    //qDebug() << Id<<"> Applying basic filters for events from"<<EventsFrom<<" to "<<EventsTo-1;

    const bool fMultipleScanEvents = FiltSet->fMultipleScanEvents && !EventsDataHub->isScanEmpty();
    const bool fDoSpatialFilter = FiltSet->fSpF_custom &&
                            (
                               (FiltSet->SpF_RecOrScan == 0 && EventsDataHub->fReconstructionDataReady)
                                ||
                               (FiltSet->SpF_RecOrScan == 1 && !EventsDataHub->isScanEmpty())
                            );
    const bool fDoRecEnergyFilter = FiltSet->fEnergyFilter && EventsDataHub->fReconstructionDataReady;
    const bool fDoLoadedEnergyFilter = FiltSet->fLoadedEnergyFilter && !EventsDataHub->isScanEmpty();
    const bool fDoChi2Filter = FiltSet->fChi2Filter && EventsDataHub->fReconstructionDataReady;

    for (int iev=EventsFrom; iev<EventsTo; iev++)
    {
        AReconRecord *rec = EventsDataHub->ReconstructionData[ThisPmGroup][iev];
        const QVector< float >* PMsignals = &EventsDataHub->Events[iev];

        //reconstruction performed and failed -> definitely bad event
        if (EventsDataHub->fReconstructionDataReady && !rec->ReconstructionOK) goto BadEventLabel;

        if (FiltSet->fEventNumberFilter)
            if (iev < FiltSet->EventNumberFilterMin || iev > FiltSet->EventNumberFilterMax) goto BadEventLabel;

        if (FiltSet->fCutOffFilter || FiltSet->fSumCutOffFilter)
        {
            double sum=0;
            for (int ipm=0; ipm<PMs->count(); ipm++)
                if (PMgroups->isPmBelongsToGroupFast(ipm, ThisPmGroup))
                    if (!PMgroups->isStaticPassive(ipm) || FiltSet->fCutOffsForPassivePMs)
                    {
                        double sig = PMsignals->at(ipm);
                        if (FiltSet->fSumCutUsesGains) sig /= PMgroups->getGainFast(ipm, ThisPmGroup);//  Groups.at(ThisPmGroup)->PMS.at(ipm).gain;
                        if (FiltSet->fCutOffFilter)
                            //if (sig < PMgroups->Groups.at(ThisPmGroup)->PMS.at(ipm).cutOffMin || sig > PMgroups->Groups.at(ThisPmGroup)->PMS.at(ipm).cutOffMax) goto BadEventLabel;
                            if (sig < PMgroups->getCutOffMinFast(ipm, ThisPmGroup) || sig > PMgroups->getCutOffMaxFast(ipm, ThisPmGroup)) goto BadEventLabel;
                        sum += sig;
                    }
            if (FiltSet->fSumCutOffFilter)
                if (sum < FiltSet->SumCutOffMin || sum > FiltSet->SumCutOffMax) goto BadEventLabel;
        }

        if (fDoLoadedEnergyFilter)
        {
            //double energy = PMsignals->at(PMs->count());  //energy channel is always the last one, pm numbering starts with 0
            const double& energy = EventsDataHub->Scan.at(iev)->Points[0].energy;
            if (energy < FiltSet->LoadedEnergyFilterMin || energy > FiltSet->LoadedEnergyFilterMax) goto BadEventLabel;
        }

        if (fMultipleScanEvents)
            if (EventsDataHub->Scan.at(iev)->Points.size() > 1) goto BadEventLabel;

        if (fDoSpatialFilter)
        {
            double xx, yy, zz;
            if (FiltSet->SpF_RecOrScan == 0)
            {
                //reconstructed positions
                xx = rec->Points[0].r[0];
                yy = rec->Points[0].r[1];
                zz = rec->Points[0].r[2];
            }
            else
            {
                xx = EventsDataHub->Scan.at(iev)->Points[0].r[0];
                yy = EventsDataHub->Scan.at(iev)->Points[0].r[1];
                zz = EventsDataHub->Scan.at(iev)->Points[0].r[2];
            }
            //first checking Z
            if (!FiltSet->fSpF_allZ && (zz<FiltSet->SpF_Zfrom || zz>FiltSet->SpF_Zto) ) goto BadEventLabel;

            bool fPass = true; //migh be inverted below if the spatial filter setting is "inside"!
            switch (FiltSet->SpF_shape)
            {
              case 0:
                //square
                xx -= FiltSet->SpF_X0;
                yy -= FiltSet->SpF_Y0;
                if (xx<-FiltSet->SpF_halfSizeX || xx>FiltSet->SpF_halfSizeX || yy<-FiltSet->SpF_halfSizeY || yy>FiltSet->SpF_halfSizeY) fPass = false;
                break;
              case 1:
              {
                //round
                xx -= FiltSet->SpF_X0;
                yy -= FiltSet->SpF_Y0;
                double dist = xx*xx + yy*yy;
                if (dist > FiltSet->SpF_radius2) fPass = false;
                break;
              }
              case 2://common for hexagonal and custom
              case 3:
                if (FiltSet->SpF_polygon.containsPoint( QPointF(xx,yy), Qt::OddEvenFill )) fPass = false;
                break;
              default:
              {
                qCritical() << "Error: unknown type of spatial filter!";
                exit(666);
              }
            }
            if (FiltSet->SpF_CutOutsideInside == 1) fPass = !fPass; //if cut inside, invert the filter results
            if (!fPass) goto BadEventLabel;
        }

        if (fDoRecEnergyFilter)
        {
            double TotalEnergy = 0;
            for (int iPo=0; iPo<rec->Points.size(); iPo++) TotalEnergy += rec->Points[iPo].energy;
            if (TotalEnergy < FiltSet->EnergyFilterMin || TotalEnergy > FiltSet->EnergyFilterMax) goto BadEventLabel;
        }

        if (fDoChi2Filter)
            if (rec->chi2 < FiltSet->Chi2FilterMin || rec->chi2 > FiltSet->Chi2FilterMax) goto BadEventLabel;

        //if come to this point, its a good event
        rec->GoodEvent = true;
        continue;
        //goto for bad event collected here:
BadEventLabel:
        rec->GoodEvent = false;
    }

    emit finished();
    fFinished = true;
}

void CGonCPUreconstructorClass::execute()
{
  //qDebug() << "CConCPU starting";
  if (LRFs.isAllSlice3Dold())
  {
      //qDebug() << "Sliced3D old module - special procedure!";
      executeSliced3Dold();
      return;
  }

  fFinished = false;
  float Factor = 100.0/(EventsTo-EventsFrom);
  double r2max = (RecSet->fLimitNodes) ? RecSet->LimitNodesSize1*RecSet->LimitNodesSize1 : 0;  //round only uses size1
  eventsProcessed = 0;

  for (int iev=EventsFrom; iev<EventsTo; iev++)
    {      
      if (fStopRequested) break;
      AReconRecord *rec = EventsDataHub->ReconstructionData[ThisPmGroup][iev];
      if (rec->ReconstructionOK)
        {
          if (RecSet->fUseDynamicPassives) DynamicPassives->calculateDynamicPassives(iev, rec);

          double CenterX, CenterY;
          //starting coordinates
          if (RecSet->CGstartOption == 1)
            { //starting from XY of the centre of the PM with max signal
             CenterX = PMs->X(rec->iPMwithMaxSignal);
             CenterY = PMs->Y(rec->iPMwithMaxSignal);
            }
          else if (RecSet->CGstartOption == 2 && !EventsDataHub->isScanEmpty())
            {
              //starting from true XY
              CenterX = EventsDataHub->Scan[iev]->Points[0].r[0];
              CenterY = EventsDataHub->Scan[iev]->Points[0].r[1];
            }
          else
            { //start from CoG data
              CenterX = rec->xCoG;
              CenterY = rec->yCoG;
            }
          //qDebug() << "Center option:"<<RecSet->CGstartOption<<"Center xy:"<< CenterX<<CenterY;

          int Nodes = RecSet->CGnodesXY;
          double Step = RecSet->CGinitialStep;
          int numPMs = PMs->count();
          double bestResult = 1.e20;
          double bestX, bestY;         
          double bestEnergy = -1;
          rec->Points[0].r[2] = rec->zCoG;

          for (int iter = 0; iter<RecSet->CGiterations; iter++)
            {
              for (int ix = 0; ix<Nodes; ix++)
                for (int iy = 0; iy<Nodes; iy++)
                  {
                    //coordinates of this node
                    rec->Points[0].r[0] = CenterX - (0.5*(Nodes-1) - ix)*Step;
                    rec->Points[0].r[1] = CenterY - (0.5*(Nodes-1) - iy)*Step;

                    //if we have some spatial filtering:
                    if (RecSet->fLimitNodes)
                      {
                        if (RecSet->LimitNodesShape == 0)
                          {
                            //rectangular
                            if ( fabs(rec->Points[0].r[0]) > RecSet->LimitNodesSize1 ) continue;
                            if ( fabs(rec->Points[0].r[1]) > RecSet->LimitNodesSize2 ) continue;
                          }
                        else
                          {
                            //round
                            double r2 = rec->Points[0].r[0]*rec->Points[0].r[0] + rec->Points[0].r[1]*rec->Points[0].r[1];
                            if (r2 > r2max) continue;                            
                          }
                      }

                    //energy assuming event is in this node
                    if (RecSet->fReconstructEnergy)
                      {
                        bool fBadLRFfound = false;
                        double sumLRFs = 0;
                        double SumSignal = 0;
                        for (int ipm = 0; ipm < numPMs; ipm++)
                          if (DynamicPassives->isActive(ipm))
                            {
                             double LRFhere = LRFs.getLRF(ipm, rec->Points[0].r);
                             if (LRFhere <= 0.0)
                               {
                                 fBadLRFfound = true;
                                 break;
                               }
                             sumLRFs += LRFhere;
                             //SumSignal += EventsDataHub->Events[iev].at(ipm) / Detector->PMs->at(ipm).relGain;
                             SumSignal += EventsDataHub->Events[iev].at(ipm) / PMgroups->Groups.at(ThisPmGroup)->PMS.at(ipm).gain;
                            }
                        if (fBadLRFfound) continue; //skip this location
                        rec->Points[0].energy = SumSignal/sumLRFs;//energy cannot be zero here
                      }
                    else rec->Points[0].energy = RecSet->SuggestedEnergy;

                    double result = (RecSet->CGoptimizeWhat == 0) ? calculateChi2NoDegFree(iev, rec) : calculateMLfactor(iev, rec);
                    if (result == 1.0e20) continue; //fail - one of LRFs is undefined for these coordinates
                    if (result < bestResult)
                      { //current best
                        bestResult = result;
                        bestX = rec->Points[0].r[0];
                        bestY = rec->Points[0].r[1];
                        bestEnergy = rec->Points[0].energy;
                      }
                  }
              //preparing for new iteration
              CenterX = bestX;
              CenterY = bestY;
              Step /= RecSet->CGreduction;
            }

          if (bestEnergy != -1)
            {
              rec->Points[0].r[0] = bestX;
              rec->Points[0].r[1] = bestY;
              rec->Points[0].r[2] = rec->zCoG;
              rec->Points[0].energy = bestEnergy;
              //alread have "OK and Good" status from CoG
            }
          else
            {  //not a single good node wos found
              rec->ReconstructionOK = false;
              rec->GoodEvent = false;
            }
        }
      //else CoG failed event

      eventsProcessed++;
      Progress = eventsProcessed*Factor;
    }

  emit finished();
  fFinished = true;
}

void CGonCPUreconstructorClass::executeSliced3Dold()
{
    fFinished = false;
    float Factor = 100.0/(EventsTo-EventsFrom);

    eventsProcessed = 0;
    const LRFsliced3D* lrf = dynamic_cast<const LRFsliced3D*>( (*LRFs.getOldModule())[0] );
    if (!lrf) return;
    //qDebug() << "Slices:"<<lrf->getNintZ();

    for (int iev=EventsFrom; iev<EventsTo; iev++)
      {
        BestSlResult = 1.0e20;
        AReconRecord *rec = EventsDataHub->ReconstructionData[ThisPmGroup][iev];
        if (!rec->ReconstructionOK) continue; //cog failed!
        if (RecSet->fUseDynamicPassives) DynamicPassives->calculateDynamicPassives(iev, rec);

        for (int isl=0; isl<lrf->getNintZ(); isl++)
        {
            if (fStopRequested) break;
            oneSlice(iev, isl);
        }

        if (BestSlResult == 1.0e20)
        {  //not a single good node was found
            rec->ReconstructionOK = false;
            rec->GoodEvent = false;
        }
        else
        {
            rec->Points[0].r[0] = BestSlX;
            rec->Points[0].r[1] = BestSlY;
            rec->Points[0].r[2] = BestSlZ;
            rec->Points[0].energy = BestSlEnergy;
            //alread have "OK and Good" status from CoG
        }

        eventsProcessed++;
        Progress = eventsProcessed*Factor;
      }

    emit finished();
    fFinished = true;
}

void CGonCPUreconstructorClass::oneSlice(int iev, int iSlice)
{
    double r2max = (RecSet->fLimitNodes) ? RecSet->LimitNodesSize1*RecSet->LimitNodesSize1 : 0;  //round only uses size1
    AReconRecord *rec = EventsDataHub->ReconstructionData[ThisPmGroup][iev];

    double CenterX, CenterY;
    //starting coordinates
    if (RecSet->CGstartOption == 1)
    { //starting from XY of the centre of the PM with max signal
        CenterX = PMs->X(rec->iPMwithMaxSignal);
        CenterY = PMs->Y(rec->iPMwithMaxSignal);
    }
    else if (RecSet->CGstartOption == 2 && !EventsDataHub->isScanEmpty())
    {
        //starting from true XY
        CenterX = EventsDataHub->Scan[iev]->Points[0].r[0];
        CenterY = EventsDataHub->Scan[iev]->Points[0].r[1];
    }
    else
    { //start from CoG data
        CenterX = rec->xCoG;
        CenterY = rec->yCoG;
    }
    //qDebug() << "Center option:"<<RecSet->CGstartOption<<"Center xy:"<< CenterX<<CenterY;

    int Nodes = RecSet->CGnodesXY;
    double Step = RecSet->CGinitialStep;
    int numPMs = PMs->count();
    double bestResult = 1.0e20;
    double bestX, bestY, bestZ;
    double bestEnergy = -1;
    const LRFsliced3D* lrf = dynamic_cast<const LRFsliced3D*>( (*LRFs.getOldModule())[0] );
    rec->Points[0].r[2] = lrf->getSliceMedianZ(iSlice);

    for (int iter = 0; iter<RecSet->CGiterations; iter++)
          {
            for (int ix = 0; ix<Nodes; ix++)
              for (int iy = 0; iy<Nodes; iy++)
                {
                  //coordinates of this node
                  rec->Points[0].r[0] = CenterX - (0.5*(Nodes-1) - ix)*Step;
                  rec->Points[0].r[1] = CenterY - (0.5*(Nodes-1) - iy)*Step;

                  //if we have some spatial filtering:
                  if (RecSet->fLimitNodes)
                    {
                      if (RecSet->LimitNodesShape == 0)
                        {
                          //rectangular
                          if ( fabs(rec->Points[0].r[0]) > RecSet->LimitNodesSize1 ) continue;
                          if ( fabs(rec->Points[0].r[1]) > RecSet->LimitNodesSize2 ) continue;
                        }
                      else
                        {
                          //round
                          double r2 = rec->Points[0].r[0]*rec->Points[0].r[0] + rec->Points[0].r[1]*rec->Points[0].r[1];
                          if (r2 > r2max) continue;
                        }
                    }

                  //energy assuming event is in this node
                  if (RecSet->fReconstructEnergy)
                    {
                      bool fBadLRFfound = false;
                      double sumLRFs = 0;
                      double SumSignal = 0;
                      for (int ipm = 0; ipm < numPMs; ipm++)
                        if (DynamicPassives->isActive(ipm))
                          {
                           double LRFhere = LRFs.getLRF(ipm, rec->Points[0].r);
                           if (LRFhere <= 0.0)
                             {
                               fBadLRFfound = true;
                               break;
                             }
                           sumLRFs += LRFhere;
                           SumSignal += EventsDataHub->Events[iev].at(ipm) / PMgroups->Groups.at(ThisPmGroup)->PMS.at(ipm).gain;
                          }
                      if (fBadLRFfound) continue; //skip this location
                      rec->Points[0].energy = SumSignal/sumLRFs;//energy cannot be zero here
                    }
                  else rec->Points[0].energy = RecSet->SuggestedEnergy;

                  double result = (RecSet->CGoptimizeWhat == 0) ? calculateChi2NoDegFree(iev, rec) : calculateMLfactor(iev, rec);
                  if (result == 1.0e20) continue; //fail - one of LRFs is undefined for these coordinates

                  if (result < bestResult)
                    { //current best
                      bestResult = result;
                      bestX = rec->Points[0].r[0];
                      bestY = rec->Points[0].r[1];
                      bestZ = rec->Points[0].r[2];
                      bestEnergy = rec->Points[0].energy;
                    }
                }
            //preparing for new iteration
            CenterX = bestX;
            CenterY = bestY;
            Step /= RecSet->CGreduction;
          }
        //qDebug() << "Slice:"<<iSlice<<"Result, xyzenergy:"<<bestResult<<bestX<<bestY<<bestZ<<bestEnergy;

        if (bestResult < BestSlResult)
          {
            BestSlResult = bestResult;
            BestSlX = bestX;
            BestSlY = bestY;
            BestSlZ = bestZ;
            BestSlEnergy = bestEnergy;
          }
}
