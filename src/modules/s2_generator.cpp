//ANTS
#include "s2_generator.h"
#include "amaterialparticlecolection.h"
#include "photon_generator.h"
#include "aphotontracer.h"
#include "aphoton.h"
#include "aenergydepositioncell.h"
#include "generalsimsettings.h"

//Qt
#include <QDebug>

//Root
#include "TGeoManager.h"
#include "TRandom2.h"

S2_Generator::S2_Generator(Photon_Generator *photonGenerator, APhotonTracer *photonTracker, QVector<AEnergyDepositionCell *> *energyVector, TRandom2 *RandomGenerator, TGeoManager *geoManager, AMaterialParticleCollection *materialCollection, QVector<GeneratedPhotonsHistoryStructure> *PhotonsHistory, QObject *parent)
    : QObject(parent)
{
    PhotonGenerator = photonGenerator;
    PhotonTracker = photonTracker;
    EnergyVector = energyVector;
    RandGen = RandomGenerator;
    GeoManager = geoManager;
    MaterialCollection = materialCollection;
    GeneratedPhotonsHistory = PhotonsHistory;

    DoTextLog = false;
}

bool S2_Generator::Generate() //uses MW->EnergyVector as the input parameter
{
    if (EnergyVector->isEmpty()) return true; //no deposition data -> no secondary to generate

    double Remainer = 0.0;
    double ElRemainer = 0.0;
    double Electrons, Photons;
    int NumElectrons, NumPhotons;

    //what is the first particle to work with?
    //   int Id = EnergyVector->at(0)->ParticleId;

    int RecordNumber; //tracer for photon log index, not used if OnlySecondary
    if (DoTextLog)
    {
        //photon hystory log:
        //if OnlySecondary  log is filled directly (as done in S1)
        //otherwise, adding data to existing log created by S1, we have to find the first index
        if (!OnlySecondary)
         {
           int ThisEvent = EnergyVector->at(0)->eventId;
           int max = GeneratedPhotonsHistory->last().event;
           if (ThisEvent>max)
             {
               qWarning() << "Error in S2 generation: index is out of bounds";
               return false;
             }

           int i = -1;
           do
             {
               i++;
             }
           while (GeneratedPhotonsHistory->at(i).event != ThisEvent);

           RecordNumber = i;
    //       qDebug()<<"record number:"<<RecordNumber;
         }
     }

   int TextLogPhotons = 0;
   double TextLogEnergy = 0.0;
   int LastEvent = EnergyVector->at(0)->eventId;
   int LastIndex = EnergyVector->at(0)->index;
   int LastParticle = EnergyVector->at(0)->ParticleId;
   int LastMaterial = EnergyVector->at(0)->MaterialId;

   //--------------------main cycle--------------------------
   for (int i = 0; i < EnergyVector->size(); i++)
     {
       int ThisId = EnergyVector->at(i)->ParticleId;
       int ThisIndex = EnergyVector->at(i)->index;
       int ThisEvent = EnergyVector->at(i)->eventId;
       //checking are we still tracking the same particle?
       if (LastEvent != ThisEvent || LastIndex != ThisIndex)
         {
           //Id = ThisId;
           //reset remainer
           ElRemainer = 0.0;
           Remainer = 0.0;
         }      
       GeoManager->SetCurrentPoint( (*EnergyVector)[i]->r);
       GeoManager->FindNode();
//       qDebug()<<"starting from:"<<GeoManager->GetCurrentVolume()->GetName();
//       qDebug()<<ElRemainer;
       //int MatIndex = GeoManager->GetCurrentVolume()->GetMaterial()->GetIndex();
       int MatId = EnergyVector->at(i)->MaterialId;
       double W = (*MaterialCollection)[MatId]->W;
       Electrons = EnergyVector->at(i)->dE / W + ElRemainer;
//       qDebug()<<"en, W, el: "<<EnergyVector->at(i).dE<<W<<Electrons;
       NumElectrons = (int)Electrons;
       ElRemainer = Electrons - (double)NumElectrons;
//       qDebug()<<Electrons<<ElRemainer;
//       qDebug()<<"Found"<< NumElectrons <<"electron(s) to drift";

       if (NumElectrons > 0) // !!!cannot skip 0 electrons by "continue" because of the log !!!
       {
         //field is always in z direction, electrons drift up!
         //finding distance to drift and time it will take

         double time =  EnergyVector->at(i)->time;
         //       qDebug()<<"start time: "<<time;
         TString VolName;
         do
         {
            //going up untill enter secondary scintillator ("SecScint")
            int ThisMatIndex = GeoManager->GetCurrentVolume()->GetMaterial()->GetIndex();
            GeoManager->SetCurrentDirection(0,0,1.);  //up
            GeoManager->FindNextBoundaryAndStep();
            double Step = GeoManager->GetStep();
            //double DriftVelocity = 0.01 * (*MaterialCollection)[ThisMatIndex]->e_driftSpeed;//given in cm/us - need in mm/ns
            double DriftVelocity = MaterialCollection->getDriftSpeed(ThisMatIndex);
            if (DriftVelocity != 0)
                time += Step / DriftVelocity;
            VolName = GeoManager->GetCurrentVolume()->GetName();
//            qDebug()<<"Found new volume: "<<VolName<<" drifted: "<<Step<<"new time: "<<time;          
            if (GeoManager->IsOutside()) break;
         }
         while ( VolName != "SecScint" );

         if ( VolName == "SecScint" )
           {
             //       qDebug()<<"found secondary scint";
             double MatIndexSecScint = GeoManager->GetCurrentVolume()->GetMaterial()->GetIndex();
             double PhotonsPerElectron = (*MaterialCollection)[MatIndexSecScint]->SecYield;
      //       qDebug()<<"matIndex of sec scintillator:"<<MatIndexSecScint<<"PhPerEl"<<PhotonsPerElectron;

             double Zstart = GeoManager->GetCurrentPoint()[2];
      //       qDebug()<<"Sec scint starts: "<<Zstart;
             //GeoManager->FindNextBoundaryAndStep();
             GeoManager->FindNextBoundary();
             double Zspan = GeoManager->GetStep();
             double DriftVelocity = 0.01 * (*MaterialCollection)[MatIndexSecScint]->e_driftVelocity; //given in cm/us, need in mm/ns

             //generate photons            
             Photons = NumElectrons * PhotonsPerElectron + Remainer;
             NumPhotons = (int)Photons;
             Remainer   = Photons - (double)NumPhotons;

             if (PhotonGenerator->SimSet->fLRFsim)
                 PhotonGenerator->GenerateSignalsForLrfMode(NumPhotons, (*EnergyVector)[i]->r, PhotonTracker->getEvent());
             else
             {
                 //qDebug()<<"Generate photons: "<<NumPhotons ;
                 APhoton Photon;
                 Photon.r[0] = (*EnergyVector)[i]->r[0];
                 Photon.r[1] = (*EnergyVector)[i]->r[1];
                 Photon.scint_type = 2;
                 Photon.SimStat = PhotonGenerator->DetStat;

                 for (int ii=0; ii<NumPhotons; ii++)
                   {
                     //random z inside secondary scintillator
                     double z = Zspan* RandGen->Rndm();
                     Photon.r[2] = Zstart + z;
                     Photon.time = time + z / DriftVelocity;

                     PhotonGenerator->GenerateDirectionSecondary(&Photon);
                     //catching photons of the wrong direction, resetting the "skip" status!
                     if (Photon.fSkipThisPhoton)
                       {
                         Photon.fSkipThisPhoton = false;
                         continue;
                       }

                     PhotonGenerator->GenerateWave(&Photon, MatIndexSecScint);
                     PhotonGenerator->GenerateTime(&Photon, MatIndexSecScint);
    //                 qDebug()<<time + z / DriftVelocity;
                     PhotonTracker->TracePhoton(&Photon);
                   }
                 //all photons generated
             }
           }
        else
         {
//             qDebug()<<"Secondary scintillator was NOT found!";
             NumPhotons = 0;
         }
       }
       else
         {
           NumPhotons = 0;
         }

       //Text log
       if (DoTextLog)
         {
           if (LastEvent != ThisEvent || LastIndex != ThisIndex || MatId != LastMaterial)
             {
               //changed - saving previous data
               if (OnlySecondary)
                 {
                   GeneratedPhotonsHistoryStructure tmp; // = {};
                   tmp.Energy = TextLogEnergy;
                   tmp.MaterialId = LastMaterial;
                   tmp.ParticleId = LastParticle;
                   tmp.PrimaryPhotons = 0;
                   tmp.SecondaryPhotons = TextLogPhotons;
                   tmp.event = LastEvent;
                   tmp.index = LastIndex;
                   GeneratedPhotonsHistory->append(tmp);
                 }
               else
                 {
//                   qDebug()<<"replacement in"<<RecordNumber<<" value:"<<TextLogPhotons;
                   (*GeneratedPhotonsHistory)[RecordNumber].SecondaryPhotons = TextLogPhotons;
                   RecordNumber++;
                 }

               //updating current data
               LastEvent = ThisEvent;
               LastIndex = ThisIndex;
               LastMaterial = MatId;
               LastParticle = ThisId;               
               TextLogPhotons = NumPhotons;
//               qDebug()<<"--->STARTING log photons:"<<TextLogPhotons;

               TextLogEnergy = EnergyVector->at(i)->dE;
             }
           else
             {
               //continue previous
               TextLogPhotons += NumPhotons;
//               qDebug()<<"     text log photons:"<<TextLogPhotons;
               TextLogEnergy += EnergyVector->at(i)->dE;
             }
         }
     }
   //finished all EnergyVector records

   //last point still needs save history
   if (DoTextLog)
     {       
       if (OnlySecondary)
        {
          GeneratedPhotonsHistoryStructure tmp;// = {};
          tmp.Energy = TextLogEnergy;
          tmp.MaterialId = LastMaterial;
          tmp.ParticleId = LastParticle;
          tmp.PrimaryPhotons = 0;
          tmp.SecondaryPhotons = TextLogPhotons;
          tmp.event = LastEvent;
          tmp.index = LastIndex;
          GeneratedPhotonsHistory->append(tmp);
        }
      else
         {
//           qDebug()<<"after end replacement in"<<RecordNumber<<"  value of "<<TextLogPhotons;
           (*GeneratedPhotonsHistory)[RecordNumber].SecondaryPhotons = TextLogPhotons;
         }
     }

   return true;
}
