#include "aphotontracer.h"
#include "apmhub.h"
#include "amaterialparticlecolection.h"
#include "generalsimsettings.h"
#include "aopticaloverride.h"
#include "asimulationstatistics.h"
#include "aoneevent.h"
#include "agridelementrecord.h"
#include "aphoton.h"
#include "atrackrecords.h"
#include "amonitor.h"
#include "atracerstateful.h"

//Qt
#include <QDebug>

//ROOT
#include "TGeoManager.h"
#include "TMath.h"
#include "TRandom2.h"
#include "TH1I.h"

#define c_in_vac 2.997925e2 //speed of light in mm/ns

APhotonTracer::APhotonTracer(TGeoManager *geoManager, TRandom2 *RandomGenerator, AMaterialParticleCollection* materialCollection, APmHub* Pms, const QVector<AGridElementRecord *> *Grids)
{
    RandGen = RandomGenerator;
    GeoManager = geoManager;
    MaterialCollection = materialCollection;
    PMs = Pms;
    grids = Grids;
    fGridShiftOn = false;
    fBuildTracks = false;
    p = new APhoton();
    ResourcesForOverrides = new ATracerStateful(RandGen);

    //for transfer to overrides
    ResourcesForOverrides->generateScriptInfrastructureIfNeeded(MaterialCollection);
}

APhotonTracer::~APhotonTracer()
{
    delete ResourcesForOverrides;
    delete p;
}

void APhotonTracer::configure(const GeneralSimSettings *simSet, AOneEvent* oneEvent, bool fBuildTracks, std::vector<TrackHolderClass *> * tracks)//bool fWave, bool fAngle,  bool fArea, int MaxTrans, bool fTracks, bool fFastTracks, bool fAccelQE, double maxQE, QVector<double> *MaxQEwave)
{
   SimSet = simSet;
   OneEvent = oneEvent;
   MaxTracks = simSet->TrackBuildOptions.MaxPhotonTracks; //default, can be adjusted later (setMaxTracks() methof) if multithread
   this->fBuildTracks = fBuildTracks;
   Tracks = tracks;
   PhotonTracksAdded = 0;
}

void APhotonTracer::TracePhoton(const APhoton* Photon)
{
  if (bAbort) return;
  //qDebug() << "----accel is on?"<<SimSet->fQEaccelerator<< "Build tracks?"<<fBuildTracks;
  //accelerators
  if (SimSet->fQEaccelerator)
    {
      rnd = RandGen->Rndm();
      if (SimSet->fWaveResolved && Photon->waveIndex != -1)
        {
          if (rnd > PMs->getMaxQEvsWave(Photon->waveIndex))
            {
              OneEvent->SimStat->TracingSkipped++;
              return; //no need to trace this photon
            }
        }
      else
        {
           if (rnd > PMs->getMaxQE())
             {
               OneEvent->SimStat->TracingSkipped++;
               return; //no need to trace this photon
             }
        }
    }

   //=====inits=====
   navigator = GeoManager->GetCurrentNavigator();
   if (!navigator)
   {
       qDebug() << "Photon tracer: current navigator does not exist, creating new";
       navigator = GeoManager->AddNavigator();
   }
   navigator->SetCurrentPoint(Photon->r);
   navigator->SetCurrentDirection(Photon->v);
   navigator->FindNode();
   fGridShiftOn = false;

   if (navigator->IsOutside())
     {
//     qDebug()<<"Generated outside geometry!";
       OneEvent->SimStat->GeneratedOutsideGeometry++;
       PhLog.clear();
       PhLog.append( APhotonHistoryLog(p->r, "", p->time, p->waveIndex, APhotonHistoryLog::GeneratedOutsideGeometry) );
       return;
     }

   p->CopyFrom(Photon);
   //qDebug()<<"Photon starts from:";
   //qDebug()<<navigator->GetPath();
   //qDebug()<<"material name: "<<navigator->GetCurrentVolume()->GetMaterial()->GetName();
   //qDebug()<<"material index: "<<navigator->GetCurrentVolume()->GetMaterial()->GetIndex();

   if (fBuildTracks)
     {
       if (PhotonTracksAdded < MaxTracks)
         {
           track = new TrackHolderClass();
           track->Nodes.append(TrackNodeStruct(p->r, p->time));
         }
       else fBuildTracks = false;
     }

   TGeoNode* NodeAfterInterface;
   MatIndexFrom = navigator->GetCurrentVolume()->GetMaterial()->GetIndex();
   fMissPM = true;

   if (SimSet->bDoPhotonHistoryLog)
   {
       PhLog.clear();
       PhLog.append( APhotonHistoryLog(p->r, navigator->GetCurrentVolume()->GetName(), p->time, p->waveIndex, APhotonHistoryLog::Created, MatIndexFrom) );
   }

   Counter = 0; //number of photon transitions - there is a limit on this set by user
   //---------------------------------------------=====cycle=====-----------------------------------------
   while (Counter < SimSet->MaxNumTrans)
   {
     Counter++;

     MaterialFrom = (*MaterialCollection)[MatIndexFrom]; //this is the material where the photon is currently in
     if (SimSet->bDoPhotonHistoryLog) nameFrom = navigator->GetCurrentVolume()->GetName();

     navigator->FindNextBoundary();
     Step = navigator->GetStep();
     //qDebug()<<"Step:"<<Step;

     //----- Bulk Absorption or Rayleigh scattering -----
     switch ( AbsorptionAndRayleigh() )
     {
       case AbsTriggered: goto force_stop_tracing; //finished with this photon
       case RayTriggered: continue; //next step
       case WaveShifted: continue; //next step
       default:; // not triggered
     }

     //----- Making a step towards the interface ------        
     navigator->Step(kTRUE, kFALSE); //stopped right before the crossing

     Double_t R_afterStep[3];
     if (fGridShiftOn && Step>0.001)
     {   //if point after Step is outside of grid bulk, kill this photon
         // it is not realistic in respect of Rayleigh for photons travelling in grid plane
         //also, there is a small chance that photon is rejected by this procedure when it hits the grid wire straight on center
         //chance is very small (~0.1%) even for grids with ~25% transmission value

         const Double_t* rc = navigator->GetCurrentPoint();
         //qDebug() << "...Checking after step of " << Step << "still inside grid bulk?";
         //qDebug() << "...Position in world:"<<rc[0] << rc[1] << rc[2];
         //qDebug() << "...Navigator is supposed to be inside the grid element. Currently in:"<<navigator->GetCurrentNode()->GetVolume()->GetName();

         R_afterStep[0] = rc[0] + FromGridCorrection[0];
         R_afterStep[1] = rc[1] + FromGridCorrection[1];
         R_afterStep[2] = rc[2] + FromGridCorrection[2];
         //qDebug() << "...After correction, coordinates in true global:"<<rc[0] << rc[1] << rc[2];

         double GridLocal[3];
         navigator->MasterToLocal(rc, GridLocal);
         //qDebug() << "...Position in grid element:"<<GridLocal[0] << GridLocal[1] << GridLocal[2];
         GridLocal[0] += FromGridElementToGridBulk[0];
         GridLocal[1] += FromGridElementToGridBulk[1];
         GridLocal[2] += FromGridElementToGridBulk[2];
         //qDebug() << "...Position in grid bulk:"<<GridLocal[0] << GridLocal[1] << GridLocal[2];
         if (!GridVolume->Contains(GridLocal))
         {
             //qDebug() << "...!!!...Photon would be outside the bulk if it takes the Step! -> killing the photon";
             //qDebug() << "Conflicting position:"<<R_afterStep[0] << R_afterStep[1] << R_afterStep[2];
               //if (fGridShiftOn) track->Nodes.append(TrackNodeStruct(R_afterStep, p->time)); //not drawing end of the track for such photons!
             OneEvent->SimStat->LossOnGrid++;
             goto force_stop_tracing;
         }
     }

     //placing this location/state to the stack - it will be restored if photon is reflected
     navigator->PushPoint(); //DO NOT FORGET TO CLEAN IT IF NOT USED!!!

     //can make the track now - the photon made it to the other border in any case
     double refIndex = MaterialFrom->getRefractiveIndex(p->waveIndex);
     p->time += Step/c_in_vac*refIndex;
     if (fBuildTracks && Step>0.001)
     {
         if (fGridShiftOn)
         {             
             //qDebug() << "Added to track using shifted gridnavigator data:"<<R[0]<<R[1]<<R[2];
             track->Nodes.append(TrackNodeStruct(R_afterStep, p->time));
             //track->Nodes.append(TrackNodeStruct(navigator->GetCurrentPoint(), p->time));  //use this to check elementary cell
         }
         else
         {
             //qDebug() << "Added to track using normal navigator:"<<navigator->GetCurrentPoint()[0]<<navigator->GetCurrentPoint()[1]<<navigator->GetCurrentPoint()[2];
             track->Nodes.append(TrackNodeStruct(navigator->GetCurrentPoint(), p->time));
         }
     }

     // ----- Now making a step inside the next material volume on the path -----
     NodeAfterInterface = navigator->FindNextBoundaryAndStep(); //this is the node after crossing the boundary
     //this method MOVES the current position! different from FindNextBoundary method, which only calculates the step
     //now current point is inside the next volume!

     //check if after the border the photon is outside the defined world
     if (navigator->IsOutside()) //outside of geometry - end tracing
       {
         //qDebug() << "Photon escaped!";
         navigator->PopDummy();//clean up the stack
         OneEvent->SimStat->Escaped++;
         if (SimSet->bDoPhotonHistoryLog) PhLog.append( APhotonHistoryLog(navigator->GetCurrentPoint(), nameFrom, p->time, p->waveIndex, APhotonHistoryLog::Escaped) );
         goto force_stop_tracing; //finished with this photon
       }

     //new volume info
     TGeoVolume* ThisVolume = NodeAfterInterface->GetVolume();
     if (SimSet->bDoPhotonHistoryLog) nameTo = navigator->GetCurrentVolume()->GetName();
     MatIndexTo = ThisVolume->GetMaterial()->GetIndex();
     MaterialTo = (*MaterialCollection)[MatIndexTo];
     fHaveNormal = false;

     //qDebug()<<"Found border with another volume: "<<ThisVolume->GetName();
     //qDebug()<<"Mat index after interface: "<<MatIndexTo<<" Mat index before: "<<MatIndexFrom;
     //qDebug()<<"coordinates: "<<navigator->GetCurrentPoint()[0]<<navigator->GetCurrentPoint()[1]<<navigator->GetCurrentPoint()[2];

     //-----Checking overrides-----     
     AOpticalOverride* ov = MaterialFrom->OpticalOverrides[MatIndexTo];
     if (ov)
       {
         //qDebug() << "Overrides defined! Model = "<<ov->getType();
         N = navigator->FindNormal(kFALSE);
         fHaveNormal = true;
         const double* PhPos = navigator->GetCurrentPoint();
         for (int i=0; i<3; i++) p->r[i] = PhPos[i];
         AOpticalOverride::OpticalOverrideResultEnum result = ov->calculate(*ResourcesForOverrides, p, N);
         if (bAbort) return;

         switch (result)
           {
           case AOpticalOverride::Absorbed:
               //qDebug() << "-Override: absorption triggered";
               navigator->PopDummy(); //clean up the stack
               if (SimSet->bDoPhotonHistoryLog)
                   PhLog.append( APhotonHistoryLog(PhPos, nameFrom, p->time, p->waveIndex, APhotonHistoryLog::Override_Loss, MatIndexFrom, MatIndexTo) );
               OneEvent->SimStat->OverrideLoss++;
               goto force_stop_tracing; //finished with this photon
           case AOpticalOverride::Back:
               //qDebug() << "-Override: photon bounced back";
               navigator->PopPoint();  //remaining in the original volume
               navigator->SetCurrentDirection(p->v); //updating direction
               if (SimSet->bDoPhotonHistoryLog)
                   PhLog.append( APhotonHistoryLog(PhPos, nameFrom, p->time, p->waveIndex, APhotonHistoryLog::Override_Back, MatIndexFrom, MatIndexTo) );
               OneEvent->SimStat->OverrideBack++;
               continue; //send to the next iteration
           case AOpticalOverride::Forward:
               navigator->SetCurrentDirection(p->v); //updating direction
               fDoFresnel = false; //stack cleaned afterwards
               if (SimSet->bDoPhotonHistoryLog)
                   PhLog.append( APhotonHistoryLog(PhPos, nameTo, p->time, p->waveIndex, APhotonHistoryLog::Override_Forward, MatIndexFrom, MatIndexTo) );
               OneEvent->SimStat->OverrideForward++;
               break; //switch break
           case AOpticalOverride::NotTriggered:
               fDoFresnel = true;
               break; //switch break
           default:
               qCritical() << "override error - doing fresnel instead!";
               fDoFresnel = true;
           }
       }
     else fDoFresnel = true;


     //-- Override not set or not triggered --

     if (fDoFresnel)
       {
         //qDebug()<<"Performing fresnel"; //Fresnel equations //http://en.wikipedia.org/wiki/Fresnel_equations
         if (!fHaveNormal) N = navigator->FindNormal(kFALSE);
         fHaveNormal = true;
         //qDebug()<<"Normal length is:"<<sqrt(N[0]*N[0] + N[1]*N[1] + N[2]*N[2]);
         //qDebug()<<"Dir vector length is:"<<sqrt(p->v[0]*p->v[0] + p->v[1]*p->v[1] + p->v[2]*p->v[2]);

         const double prob = CalculateReflectionCoefficient(); //reflection probability
         if (RandGen->Rndm() < prob)
           { //-----Reflection-----
             //qDebug()<<"Fresnel - reflection!";
             OneEvent->SimStat->FresnelReflected++;             
             // photon remains in the same volume -> continue to the next iteration
             navigator->PopPoint(); //restore the point before the border
             PerformReflection();
             if (SimSet->bDoPhotonHistoryLog)
               PhLog.append( APhotonHistoryLog(navigator->GetCurrentPoint(), nameFrom, p->time, p->waveIndex, APhotonHistoryLog::Fresnel_Reflection, MatIndexFrom, MatIndexTo) );
             continue;
           }
         //otherwise transmission
         OneEvent->SimStat->FresnelTransmitted++;
       }


     // --------------- Photon entered another volume ----------------------
     navigator->PopDummy(); //clean up the stack

     //if passed overrides and gridNavigator!=0, abandon it - photon is considered to exit the grid
     if (fGridShiftOn && Step >0.001)
     {
         //qDebug() << "++Grid back shift triggered!";
         if (SimSet->bDoPhotonHistoryLog) PhLog.append( APhotonHistoryLog(navigator->GetCurrentPoint(), nameTo, p->time, p->waveIndex, APhotonHistoryLog::Grid_ShiftOut) );
         ReturnFromGridShift();
         navigator->FindNode();
         if (SimSet->bDoPhotonHistoryLog) PhLog.append( APhotonHistoryLog(navigator->GetCurrentPoint(), nameTo, p->time, p->waveIndex, APhotonHistoryLog::Grid_Exit) );
//         qDebug() << "Navigator coordinates: "<<navigator->GetCurrentPoint()[0]<<navigator->GetCurrentPoint()[1]<<navigator->GetCurrentPoint()[2];
     }

     //const char* VolName = ThisVolume->GetName();
     //qDebug()<<"Photon entered new volume:" << VolName;
     const char* VolTitle = ThisVolume->GetTitle();
     //qDebug() << "Title:"<<VolTitle;

     switch (VolTitle[0])
       {
       case 'P': // PM hit
         {
           const int PMnumber = NodeAfterInterface->GetNumber();
           //qDebug()<<"PM hit:"<<ThisVolume->GetName()<<PMnumber<<ThisVolume->GetTitle()<<"WaveIndex:"<<p->waveIndex;
           if (SimSet->bDoPhotonHistoryLog)
             {
               PhLog.append( APhotonHistoryLog(navigator->GetCurrentPoint(), nameTo, p->time, p->waveIndex, APhotonHistoryLog::Fresnel_Transmition, MatIndexFrom, MatIndexTo) );
               PhLog.append( APhotonHistoryLog(navigator->GetCurrentPoint(), nameTo, p->time, p->waveIndex, APhotonHistoryLog::HitPM, -1, -1, PMnumber) );
             }
           PMwasHit(PMnumber);
           OneEvent->SimStat->HitPM++;
           goto force_stop_tracing; //finished with this photon
         }
       case 'p': // dummy PM hit
         {
           //qDebug() << "Dummy PM hit";
           OneEvent->SimStat->HitDummy++;
           if (SimSet->bDoPhotonHistoryLog)
             {
               PhLog.append( APhotonHistoryLog(navigator->GetCurrentPoint(), nameTo, p->time, p->waveIndex, APhotonHistoryLog::Fresnel_Transmition, MatIndexFrom, MatIndexTo) );
               PhLog.append( APhotonHistoryLog(navigator->GetCurrentPoint(), nameTo, p->time, p->waveIndex, APhotonHistoryLog::HitDummyPM, -1, -1, NodeAfterInterface->GetNumber()) );
             }
           goto force_stop_tracing; //finished with this photon
         }
       case 'G': // grid hit
         {
           //qDebug() << "Grid hit!" << ThisVolume->GetName() << ThisVolume->GetTitle()<< "Number:"<<NodeAfterInterface->GetNumber();
           if (SimSet->bDoPhotonHistoryLog) PhLog.append( APhotonHistoryLog(navigator->GetCurrentPoint(), nameTo, p->time, p->waveIndex, APhotonHistoryLog::Grid_Enter) );
           GridWasHit(NodeAfterInterface->GetNumber()); // it is assumed that "empty part" of the grid element will have the same refractive index as the material from which photon enters it
           GridVolume = ThisVolume;
           if (SimSet->bDoPhotonHistoryLog) PhLog.append( APhotonHistoryLog(navigator->GetCurrentPoint(), nameTo, p->time, p->waveIndex, APhotonHistoryLog::Grid_ShiftIn) );
           break;
         }
       case 'M': //monitor
         {
           const int iMon = NodeAfterInterface->GetNumber();
           //qDebug() << "Monitor hit!" << ThisVolume->GetName() << "Number:"<<iMon;// << MatIndexFrom<<MatIndexTo;
           if (p->SimStat->Monitors.at(iMon)->isForPhotons())
             {
               Double_t local[3];
               const Double_t *global = navigator->GetCurrentPoint();
               navigator->MasterToLocal(global, local);
               //qDebug()<<local[0]<<local[1];
               //qDebug() << "Monitors:"<<p->SimStat->Monitors.size();
               if ( (local[2]>0 && p->SimStat->Monitors.at(iMon)->isUpperSensitive()) || (local[2]<0 && p->SimStat->Monitors.at(iMon)->isLowerSensitive()) )
               {
                   //angle?
                   if (!fHaveNormal) N = navigator->FindNormal(kFALSE);
                   double cosAngle = 0;
                   for (int i=0; i<3; i++) cosAngle += N[i] * p->v[i];
                   p->SimStat->Monitors[iMon]->fillForPhoton(local[0], local[1], p->time, 180.0/3.1415926535*TMath::ACos(cosAngle), p->waveIndex);
                   if (p->SimStat->Monitors.at(iMon)->isStopsTracking())
                   {
                       OneEvent->SimStat->KilledByMonitor++;
                       if (SimSet->bDoPhotonHistoryLog) PhLog.append( APhotonHistoryLog(navigator->GetCurrentPoint(), nameTo, p->time, p->waveIndex, APhotonHistoryLog::KilledByMonitor) );
                       goto force_stop_tracing; //finished with this photon
                   }
               }
             }
           break;
         }
       default:
         {
           //other volumes have title '-'
         }
       }


     //if doing fresnel, perform refraction
     if (fDoFresnel)
           {
                 //-----Refraction-----
                 const bool ok = PerformRefraction( RefrIndexFrom/RefrIndexTo); // true - successful
                 // true - successful, false - forbidden -> considered that the photon is absorbed at the surface! Should not happen
                 if (!ok) qWarning()<<"Error in photon tracker: problem with transmission!";
                 if (SimSet->bDoPhotonHistoryLog)
                   PhLog.append( APhotonHistoryLog(navigator->GetCurrentPoint(), nameTo, p->time, p->waveIndex, APhotonHistoryLog::Fresnel_Transmition, MatIndexFrom, MatIndexTo) );
           }

         MatIndexFrom = MatIndexTo;
   } //while cycle: going to the next iteration

   // maximum number of transitions reached
   if (Counter == SimSet->MaxNumTrans) OneEvent->SimStat->MaxCyclesReached++;

   //here all tracing terminators end
force_stop_tracing:
   if (SimSet->bDoPhotonHistoryLog)
     {
       AppendHistoryRecord(); //Add tracks is also there, it has extra filtering
     }
   else
     {
       if (fBuildTracks) AppendTrack();
     }

   //qDebug()<<"Finished with the photon";
   //qDebug() << "Track size:" <<Tracks->size();
}

void APhotonTracer::hardAbort()
{
    bAbort = true;
    ResourcesForOverrides->abort(); //if script engine is there will abort evaluation
}

void APhotonTracer::AppendHistoryRecord()
{
  bool bVeto = false;
  //by process
  if (!p->SimStat->MustNotInclude_Processes.isEmpty())
    {
      for (int i=0; i<PhLog.size(); i++)
        if ( p->SimStat->MustNotInclude_Processes.contains(PhLog.at(i).process) )
          {
            bVeto = true;
            break;
          }
    }
  //by Volume
  if (!bVeto && !p->SimStat->MustNotInclude_Volumes.isEmpty())
        {
          for (int i=0; i<PhLog.size(); i++)
            if ( p->SimStat->MustNotInclude_Volumes.contains(PhLog.at(i).volumeName) )
              {
                bVeto = true;
                break;
              }
        }

  if (!bVeto)
    {
      bool bFound = true;
      //in processes
      for (int im = 0; im<p->SimStat->MustInclude_Processes.size(); im++)
        {
          bool bFoundThis = false;
          for (int i=PhLog.size()-1; i>-1; i--)
            if ( p->SimStat->MustInclude_Processes.at(im) == PhLog.at(i).process)
              {
                bFoundThis = true;
                break;
              }
          if (!bFoundThis)
            {
              bFound = false;
              break;
            }
        }

      //in volumes
      if (bFound)
        {
          for (int im = 0; im<p->SimStat->MustInclude_Volumes.size(); im++)
            {
              bool bFoundThis = false;
              for (int i=PhLog.size()-1; i>-1; i--)
                if ( p->SimStat->MustInclude_Volumes.at(im) == PhLog.at(i).volumeName)
                  {
                    bFoundThis = true;
                    break;
                  }
              if (!bFoundThis)
                {
                  bFound = false;
                  break;
                }
            }
        }

      if (bFound)
        {
          PhLog.squeeze();
          p->SimStat->PhotonHistoryLog.append(PhLog);
          if (fBuildTracks) AppendTrack();
        }
    }
}

#include "atrackbuildoptions.h"
void APhotonTracer::AppendTrack()
{
  //color track according to PM hit status and scintillation type
  //if ( SimSet->fTracksOnPMsOnly && fMissPM ) delete track;
  if ( SimSet->TrackBuildOptions.bSkipPhotonsMissingPMs && fMissPM )
      delete track;
  else
    {
      track->UserIndex = 22;

      ATrackAttributes ta;
      if (!fMissPM && SimSet->TrackBuildOptions.bPhotonSpecialRule_HittingPMs)
          ta = SimSet->TrackBuildOptions.TA_PhotonsHittingPMs;
      else if (p->scint_type == 2 && SimSet->TrackBuildOptions.bPhotonSpecialRule_SecScint)
          ta = SimSet->TrackBuildOptions.TA_PhotonsSecScint;
      else
          ta = SimSet->TrackBuildOptions.TA_Photons;

      track->Color = ta.color;
      track->Width = ta.width;
      track->Style = ta.style;
/*
      track->Width = 1;
      if (fMissPM)
        {
          switch (p->scint_type)
            {
            case 1: track->Color = 7; break; //primary -> kTeal
            case 2: track->Color = kMagenta; break; //secondary -> kMagenta
            default: track->Color = kGray;
            }
        }
      else track->Color = kRed;
*/

      Tracks->push_back(track);
      PhotonTracksAdded++;
    }
}

APhotonTracer::AbsRayEnum APhotonTracer::AbsorptionAndRayleigh()
{
    //Optimized assuming rare use of Rayleigh and very rare use when both abs and Rayleigh are defined!
    //if both active, extract distances at which the processes would trigger, then select the process with the shortest path
    //qDebug() << "Abs and Ray tests";

    //prepare abs
    bool DoAbsorption;
    double AbsPath;

    const double AbsCoeff = (*MaterialCollection)[MatIndexFrom]->getAbsorptionCoefficient(p->waveIndex);
    if (AbsCoeff > 0)
      {
        AbsPath = -log(RandGen->Rndm())/AbsCoeff;
        if (AbsPath < Step) DoAbsorption = true;
        else DoAbsorption = false;
      }
    else DoAbsorption = false;

     //prepare Rayleigh
    bool DoRayleigh;
    double RayleighPath;
    if (MaterialFrom->rayleighMFP == 0) DoRayleigh = false;      // == 0 - means undefined
    else
      {
        double RayleighMFP;
        if (p->waveIndex == -1) RayleighMFP = MaterialFrom->rayleighMFP;
        else RayleighMFP = MaterialFrom->rayleighBinned[p->waveIndex];

        RayleighPath = -RayleighMFP * log(RandGen->Rndm());
        if (RayleighPath < Step) DoRayleigh = true;
        else DoRayleigh = false;
      }

    //checking abs and rayleigh
    //qDebug() << "Abs, Ray, AbsPath, RayPath"<<DoAbsorption<<DoRayleigh<<AbsPath<<RayleighPath;
    if (DoAbsorption || DoRayleigh)
      {
        if (DoAbsorption && DoRayleigh)
          {
            //slecting the one having the shortest path
            if (AbsPath<RayleighPath) DoRayleigh = false;
            else DoAbsorption = false;
          }

        if (DoAbsorption)
          {
            //qDebug()<<"Absorption was triggered!";
            double refIndex = MaterialFrom->getRefractiveIndex(p->waveIndex);
            p->time += AbsPath/c_in_vac*refIndex;
            OneEvent->SimStat->Absorbed++;
            OneEvent->SimStat->BulkAbsorption++;

            if (fBuildTracks || SimSet->bDoPhotonHistoryLog)
              {
                Double_t point[3];
                point[0] = navigator->GetCurrentPoint()[0] + p->v[0]*AbsPath;
                point[1] = navigator->GetCurrentPoint()[1] + p->v[1]*AbsPath;
                point[2] = navigator->GetCurrentPoint()[2] + p->v[2]*AbsPath;
                if (fBuildTracks) track->Nodes.append(TrackNodeStruct(point, p->time));
                if (SimSet->bDoPhotonHistoryLog)
                  PhLog.append( APhotonHistoryLog(point, nameFrom, p->time, p->waveIndex, APhotonHistoryLog::Absorbed, MatIndexFrom) );
               }

            //check if this material is waveshifter
            const double reemissionProb = (*MaterialCollection)[MatIndexFrom]->getReemissionProbability(p->waveIndex);
            if ( reemissionProb > 0 )
              {
                if (RandGen->Rndm() < reemissionProb)
                  {
                    //qDebug() << "Waveshifting! Original index:"<<p->waveIndex;
                    if (p->waveIndex!=-1 && (*MaterialCollection)[MatIndexFrom]->PrimarySpectrumHist)
                      {
                        double wavelength;
                        int waveIndex;
                        int attempts = -1;
                        do
                        {
                            attempts++;
                            if (attempts > 9) return AbsTriggered;  // ***!!! absolute number
                            wavelength = (*MaterialCollection)[MatIndexFrom]->PrimarySpectrumHist->GetRandom();
                            //qDebug() << "   "<<wavelength << " MatIndexFrom:"<< MatIndexFrom;
                            waveIndex = round( (wavelength - SimSet->WaveFrom)/SimSet->WaveStep );
                        }
                        while (waveIndex < p->waveIndex); //conserving energy

                        //qDebug() << "NewIndex:"<<waveIndex;
                        p->waveIndex = waveIndex;                        
                      }
                    else p->waveIndex = -1; // this is to allow to use this procedure to make diffuse medium (including -1 waveIndex)

                    double R[3];
                    R[0] = navigator->GetCurrentPoint()[0] + p->v[0]*AbsPath;
                    R[1] = navigator->GetCurrentPoint()[1] + p->v[1]*AbsPath;
                    R[2] = navigator->GetCurrentPoint()[2] + p->v[2]*AbsPath;
                    navigator->SetCurrentPoint(R);

                    RandomDir();
                    navigator->SetCurrentDirection(p->v);
                    //qDebug() << "After:"<<p->WaveIndex;

                    //if (SimSet->fTimeResolved)
                    //    p->time += RandGen->Exp(  MaterialFrom->PriScintDecayTime );
                    p->time += (*MaterialCollection)[MatIndexFrom]->GeneratePrimScintTime(RandGen);

                    OneEvent->SimStat->Reemission++;
                    if (SimSet->bDoPhotonHistoryLog)
                      PhLog.append( APhotonHistoryLog(R, nameFrom, p->time, p->waveIndex, APhotonHistoryLog::Reemission, MatIndexFrom) );
                    return WaveShifted;
                }
              }
            //else absorption
            return AbsTriggered;
          }

        if (DoRayleigh)
          {
            //qDebug()<<"Scattering was triggered";
            //interaction position
            double R[3];
            R[0] = navigator->GetCurrentPoint()[0] + p->v[0]*RayleighPath;
            R[1] = navigator->GetCurrentPoint()[1] + p->v[1]*RayleighPath;
            R[2] = navigator->GetCurrentPoint()[2] + p->v[2]*RayleighPath;
            navigator->SetCurrentPoint(R);
            //navigator->FindNode();  /// not nreeded - removed

              //new direction
            double v_old[3];
            v_old[0] = p->v[0]; v_old[1] = p->v[1]; v_old[2] = p->v[2];
            double dotProduct;
            do
              {
                RandomDir();
                dotProduct = p->v[0]*v_old[0] + p->v[1]*v_old[1] + p->v[2]*v_old[2];
              }
            while ( (dotProduct*dotProduct + 1.0) < RandGen->Rndm(2.0));
            navigator->SetCurrentDirection(p->v);            

            double refIndex = MaterialFrom->getRefractiveIndex(p->waveIndex);
            p->time += RayleighPath/c_in_vac*refIndex;
            OneEvent->SimStat->Rayleigh++;

            //updating track if needed
            if (fBuildTracks) track->Nodes.append(TrackNodeStruct(R, p->time));
            if (SimSet->bDoPhotonHistoryLog)
              PhLog.append( APhotonHistoryLog(R, nameFrom, p->time, p->waveIndex, APhotonHistoryLog::Rayleigh, MatIndexFrom) );

            return RayTriggered;
          }
      }

    //qDebug() << "Abs and Ray - not triggered";
    return AbsRayNotTriggered;
}

double APhotonTracer::CalculateReflectionCoefficient()
{
    const double NK = N[0]*p->v[0] + N[1]*p->v[1] + N[2]*p->v[2]; // NK = cos of the angle of incidence = cos1
    const double cos1 = fabs(NK);
    //qDebug() << "Cos of incidence:"<<cos1;

    RefrIndexFrom = MaterialFrom->getRefractiveIndex(p->waveIndex);
    RefrIndexTo   = MaterialTo->getRefractiveIndex(p->waveIndex);

    //qDebug()<<"Photon wavelength"<<p->wavelength<<"WaveIndex:"<<p->WaveIndex<<"n1 and n2 are: "<<RefrIndexFrom<<RefrIndexTo;
    const double sin1 = sqrt(1.0 - NK*NK);
    const double sin2 = RefrIndexFrom/RefrIndexTo*sin1;

//         qDebug()<<"cos1 sin1 sin2 are:"<<cos1<<sin1<<sin2;
    if (fabs(sin2)>1.0)
      {
        // qDebug()<<"Total internal reflection, RefCoeff = 1.0";
        return 1.0;
      }
    else
      {
        double cos2 = sqrt(1-sin2*sin2);
        double Rs = (RefrIndexFrom*cos1-RefrIndexTo*cos2) / (RefrIndexFrom*cos1+RefrIndexTo*cos2);
        Rs *= Rs;
        double Rp = (RefrIndexFrom*cos2-RefrIndexTo*cos1) / (RefrIndexFrom*cos2+RefrIndexTo*cos1);
        Rp *= Rp;
        return 0.5*(Rs + Rp);
      }
}

void APhotonTracer::PMwasHit(int PMnumber)
{
    const bool fSiPM = PMs->isSiPM(PMnumber);

    Double_t local[3];//if no area dep or not SiPM - local[0] and [1] are undefined!
    if (SimSet->fAreaResolved || fSiPM)
      {
        const Double_t *global = navigator->GetCurrentPoint();
        navigator->MasterToLocal(global, local);
        //qDebug()<<local[0]<<local[1];
      }

    double cosAngle;  //undefined for not AngResolved!
    if (SimSet->fAngResolved)
      {
        //since we check vs cos of _refracted_:
        if (fDoFresnel) PerformRefraction( RefrIndexFrom / RefrIndexTo); // true - successful
        if (!fHaveNormal) N = navigator->FindNormal(kFALSE);
//       qDebug()<<N[0]<<N[1]<<N[2]<<"Normal length is:"<<sqrt(N[0]*N[0]+N[1]*N[1]+N[2]*N[2]);
//       qDebug()<<K[0]<<K[1]<<K[2]<<"Dir vector length is:"<<sqrt(K[0]*K[0]+K[1]*K[1]+K[2]*K[2]);
        cosAngle = 0;
        for (int i=0; i<3; i++)
            cosAngle += N[i] * p->v[i];
//       qDebug()<<"cos() = "<<cosAngle;
      }

    if (!SimSet->fQEaccelerator) rnd = RandGen->Rndm(); //else already calculated

    bool bDetected;
    if (fSiPM)
      bDetected = OneEvent->CheckSiPMhit(PMnumber, p->time, p->waveIndex, local[0], local[1], cosAngle, Counter, rnd);
    else
      bDetected = OneEvent->CheckPMThit(PMnumber, p->time, p->waveIndex, local[0], local[1], cosAngle, Counter, rnd);

    if (SimSet->bDoPhotonHistoryLog)
      PhLog.append( APhotonHistoryLog(navigator->GetCurrentPoint(), navigator->GetCurrentVolume()->GetName(), p->time, p->waveIndex, (bDetected ? APhotonHistoryLog::Detected : APhotonHistoryLog::NotDetected), -1, -1, PMnumber) );

    fMissPM = false;
}

bool APhotonTracer::PerformRefraction(Double_t nn) // nn = nFrom / nTo
{
    //qDebug()<<"refraction triggered, n1/n2 ="<<nn;
    //N - normal vector, K - origial photon direction vector
    // nn = n(in)/n(tr)
    // (NK) - scalar product
    // T = -( nn*(NK) - sqrt(1-nn*nn*[1-(NK)*(NK)]))*N + nn*K

    if (!fHaveNormal) N = navigator->FindNormal(kFALSE);
    const Double_t NK = N[0]*p->v[0] + N[1]*p->v[1] + N[2]*p->v[2];

    const Double_t UnderRoot = 1.0 - nn*nn*(1.0 - NK*NK);
    if (UnderRoot<0)
      {
//        qDebug()<<"total internal detected - will assume this photon is abosrbed at the surface";
        return false;    //total internal reflection! //previous reflection test should catch this situation
      }
    const Double_t tmp = nn*NK - sqrt(UnderRoot);

    p->v[0] = -tmp*N[0] + nn*p->v[0];
    p->v[1] = -tmp*N[1] + nn*p->v[1];
    p->v[2] = -tmp*N[2] + nn*p->v[2];

    navigator->SetCurrentDirection(p->v);
    return true;
}

void APhotonTracer::PerformReflection()
{
  if (!fHaveNormal)
    {
      N = navigator->FindNormal(kFALSE);
      fHaveNormal = true;
    }
  //qDebug() << "Normal:"<<N[0]<<N[1]<<N[2];
  //qDebug() << "Vector before:"<<p->v[0]<<p->v[1]<<p->v[2];

  //rotating the vector
  //K = K - 2*(NK)*N    where K is the photon direction vector
  const double NK = N[0]*p->v[0] + N[1]*p->v[1] + N[2]*p->v[2];
  p->v[0] -= 2.0 * NK * N[0];
  p->v[1] -= 2.0 * NK * N[1];
  p->v[2] -= 2.0 * NK * N[2];

  //qDebug() << "Vector after:"<<p->v[0]<<p->v[1]<<p->v[2];
  //qDebug() << "Photon position:"<<navigator->GetCurrentPoint()[0]<<navigator->GetCurrentPoint()[1]<<navigator->GetCurrentPoint()[2];

  navigator->SetCurrentDirection(p->v);
}

void APhotonTracer::RandomDir()
{
  //Sphere function of Root:
  double a=0, b=0, r2=1;
  while (r2 > 0.25)
    {
        a  = RandGen->Rndm() - 0.5;
        b  = RandGen->Rndm() - 0.5;
        r2 =  a*a + b*b;
    }
  p->v[2] = ( -1.0 + 8.0 * r2 );
  double scale = 8.0 * TMath::Sqrt(0.25 - r2);
  p->v[0] = a*scale;
  p->v[1] = b*scale;
}

bool APhotonTracer::GridWasHit(int GridNumber)
{
    //calculating where we are in respect to the grid element
    const Double_t *global = navigator->GetCurrentPoint();
    //init only:
    FromGridCorrection[0] = global[0];
    FromGridCorrection[1] = global[1];
    FromGridCorrection[2] = global[2];

    if (GridNumber<0 || GridNumber>grids->size()-1)
    {
        qCritical() << "Grid hit: Bad grid number!";
        return false;
    }
    //qDebug() << "------------grid record number"<<GridNumber << "Shape, dx, dy:"<<(*grids)[GridNumber]->shape<<(*grids)[GridNumber]->dx<<(*grids)[GridNumber]->dy;

    //qDebug() << "-->Global coordinates where entered the grid bulk:"<<global[0]<<global[1]<<global[2];

    double local[3]; // to local coordinates in the grid
    navigator->MasterToLocal(global,local);
    //qDebug() << "-->Local in grid bulk:" <<local[0]<<local[1]<<local[2];
    FromGridElementToGridBulk[0] = local[0];
    FromGridElementToGridBulk[1] = local[1];
    FromGridElementToGridBulk[2] = local[2];

    if ((*grids)[GridNumber]->shape < 2)
    {
        //rectangular grid
        double& dx = (*grids)[GridNumber]->dx;
        double& dy = (*grids)[GridNumber]->dy;
        //qDebug() << "-->grid cell half size in x and y"<<dx<<dy;

        int periodX = 0.5*fabs(local[0])/dx + 0.5;
        //qDebug() << "-->periodX"<<periodX;
        if (local[0]>0) local[0] -= periodX*2.0*dx;
        else local[0] += periodX*2.0*dx;
        //qDebug() << "-->local x"<<local[0];

        int periodY = 0.5*fabs(local[1])/dy + 0.5;
        //qDebug() << "-->periodY"<<periodX;
        if (local[1]>0) local[1] -= periodY*2.0*dy;
        else local[1] += periodY*2.0*dy;
        //qDebug() << "-->local y"<<local[0];
    }
    else
    {
        //hexagonal grid - the size is given in radius of circle inside!
        double dx = (*grids)[GridNumber]->dx*1.1547; // 1.0/cos(30)
        double dy = dx*0.866; //sqrt(3)*2
//        qDebug() << "-->dx,dy:"<<dx<<dy;
        int ix = fabs(local[0]/dx/1.5);
        int iy = fabs(local[1]/dy);
        bool fXisEven = (ix % 2 == 0);
        bool fYisEven = (iy % 2 == 0);
//        qDebug() << "-->ix, iy:" << ix << iy;
        double x = fabs(local[0]) - ix*1.5*dx;
        double y = fabs(local[1]) - iy*dy;
//        qDebug() << "-->x,y"<<x<<y<<"odd/even for ix and iy"<<fXisEven<<fYisEven;
        double CenterX, CenterY;
        if ( (fXisEven && fYisEven) || (!fXisEven && !fYisEven) )
        {
            CenterX = ix*1.5*dx;
            CenterY = iy*dy;
//            qDebug() << "-->aaaaaa"<<y<<1.7320508*(dx-x);
            if (y > 1.7320508*(dx-x) ) //sqrt(3)
              {
                CenterX += 1.5*dx;
                CenterY += dy;
              }
        }
        else
        {
            CenterX = ix*1.5*dx;
            CenterY = (iy+1)*dy;
            if (y < 1.7320508*(x-0.5*dx) ) //sqrt(3)
              {
                CenterX += 1.5*dx;
                CenterY -= dy;
              }
        }
        // for negative coordinates - just inversion
        if (local[0]<0) CenterX = -CenterX;
        if (local[1]<0) CenterY = -CenterY;
//        qDebug() << "-->Closest center:"<<CenterX << CenterY;
        // to coordinates inside an elementary block of the grid
        local[0] -= CenterX;
        local[1] -= CenterY;
    }

    //  qDebug() << "-->new local inside grid elementary block:" << local[0] << local[1] << local[2];
    FromGridElementToGridBulk[0] -= local[0];
    FromGridElementToGridBulk[1] -= local[1];
    FromGridElementToGridBulk[2] -= local[2];

    double master[3];
    navigator->LocalToMaster(local, master);
    //  qDebug() << "-->global for the elementary block:"<<master[0]<<master[1]<<master[2];
    navigator->SetCurrentPoint(master);
    navigator->FindNode();

    FromGridCorrection[0] -= master[0];
    FromGridCorrection[1] -= master[1];
    FromGridCorrection[2] -= master[2];

    fGridShiftOn = true;

    //qDebug() << "-->Grid shift performed!";
    //qDebug() << "-->  Current node:"<<navigator->GetCurrentNode()->GetName();
    //qDebug() << "-->  Current point:"<<navigator->GetCurrentPoint()[0]<<navigator->GetCurrentPoint()[1]<<navigator->GetCurrentPoint()[2];
    //qDebug() << "-->  Shifts in XYZ to get to normal:"<<FromGridCorrection[0]<<FromGridCorrection[1]<<FromGridCorrection[2];
    return true;
}

void APhotonTracer::ReturnFromGridShift()
{
    //qDebug() << "<--Returning from grid shift!";
    //qDebug() << "<--Shifted coordinates:"<<navigator->GetCurrentPoint()[0]<<navigator->GetCurrentPoint()[1]<<navigator->GetCurrentPoint()[2];
    //qDebug() << "<--Currently in"<<navigator->FindNode()->GetName();
    const Double_t* r = navigator->GetCurrentPoint();
    Double_t R[3];
    R[0] = r[0] + FromGridCorrection[0];
    R[1] = r[1] + FromGridCorrection[1];
    R[2] = r[2] + FromGridCorrection[2];
    navigator->SetCurrentPoint(R);
    navigator->FindNode();
    fGridShiftOn = false;
    //qDebug() << "<--True coordinates:"<<navigator->GetCurrentPoint()[0]<<navigator->GetCurrentPoint()[1]<<navigator->GetCurrentPoint()[2];
    //qDebug() << "<--After back shift in"<<navigator->FindNode()->GetName();
}
