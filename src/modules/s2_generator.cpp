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

    if (DoTextLog)
    {
        bool bOK = initLogger();
        if (!bOK) return false;
    }

    double PhotonRemainer = 0.0;
    double ElectronRemainer = 0.0;
    
    TextLogPhotons = 0;
    TextLogEnergy  = 0.0;

    LastEvent = EnergyVector->at(0)->eventId;
    LastIndex = EnergyVector->at(0)->index;
    LastParticle = EnergyVector->at(0)->ParticleId;
    LastMaterial = EnergyVector->at(0)->MaterialId;

    for (int i = 0; i < EnergyVector->size(); i++)
    {
        AEnergyDepositionCell * cell = EnergyVector->at(i);
        ThisId    = cell->ParticleId;
        ThisIndex = cell->index;
        ThisEvent = cell->eventId;
        MatId     = cell->MaterialId;

        DepositedEnergy = cell->dE;

        //checking are we still tracking the same particle?
        if (LastEvent != ThisEvent || LastIndex != ThisIndex)
        {
            //reset remainer
            ElectronRemainer = 0.0;
            PhotonRemainer = 0.0;
        }
        GeoManager->SetCurrentPoint(cell->r);
        GeoManager->FindNode();
        //       qDebug()<<"starting from:"<<GeoManager->GetCurrentVolume()->GetName();

        const double W = (*MaterialCollection)[MatId]->W;
        double Electrons = ( W != 0 ? DepositedEnergy/W : 0 );
        Electrons += ElectronRemainer;
        //       qDebug()<<"en, W, el: "<<cell.dE<<W<<Electrons;
        NumElectrons = (int)Electrons;
        ElectronRemainer = Electrons - (double)NumElectrons;
        //       qDebug()<<Electrons<<ElRemainer;
        //       qDebug()<<"Found"<< NumElectrons <<"electron(s) to drift";

        if (NumElectrons > 0)
        {
            //field is always in z direction, electrons drift up!
            //finding distance to drift and time it will take
            double time = cell->time;
            //       qDebug()<<"start time: "<<time;
            TString VolName = GeoManager->GetCurrentVolume()->GetName();
            if (VolName != "SecScint")
            {
                do
                {
                    //going up untill enter secondary scintillator ("SecScint")
                    int ThisMatIndex = GeoManager->GetCurrentVolume()->GetMaterial()->GetIndex();
                    GeoManager->SetCurrentDirection(0, 0, 1.0);  //up
                    GeoManager->FindNextBoundaryAndStep();
                    const double Step = GeoManager->GetStep();
                    const double DriftVelocity = MaterialCollection->getDriftSpeed(ThisMatIndex);
                    if (DriftVelocity != 0) time += Step / DriftVelocity;
                    VolName = GeoManager->GetCurrentVolume()->GetName();
                    //            qDebug()<<"Found new volume: "<<VolName<<" drifted: "<<Step<<"new time: "<<time;
                    if (GeoManager->IsOutside()) break;
                }
                while ( VolName != "SecScint" );
            }

            if ( VolName == "SecScint" )
            {
                //       qDebug()<<"found secondary scint";
                const int    MatIndexSecScint = GeoManager->GetCurrentVolume()->GetMaterial()->GetIndex();
                const double PhotonsPerElectron = (*MaterialCollection)[MatIndexSecScint]->SecYield;
                //       qDebug()<<"matIndex of sec scintillator:"<<MatIndexSecScint<<"PhPerEl"<<PhotonsPerElectron;

                const double Zstart = GeoManager->GetCurrentPoint()[2];
                //       qDebug()<<"Sec scint starts: "<<Zstart;
                GeoManager->FindNextBoundary();
                const double Zspan = GeoManager->GetStep();
                const double DriftVelocity = MaterialCollection->getDriftSpeed(MatIndexSecScint);

                //generate photons
                double Photons = NumElectrons * PhotonsPerElectron + PhotonRemainer;
                NumPhotons = (int)Photons;
                PhotonRemainer   = Photons - (double)NumPhotons;

                if (PhotonGenerator->SimSet->fLRFsim)
                    PhotonGenerator->GenerateSignalsForLrfMode(NumPhotons, (*EnergyVector)[i]->r, PhotonTracker->getEvent());
                else
                {
                    //      qDebug()<<"Generate photons: "<<NumPhotons ;
                    APhoton Photon;
                    Photon.r[0] = cell->r[0];
                    Photon.r[1] = cell->r[1];
                    Photon.scint_type = 2;
                    Photon.SimStat = PhotonGenerator->DetStat;

                    for (int ii=0; ii<NumPhotons; ii++)
                    {
                        //random z inside secondary scintillator
                        double z = Zspan * RandGen->Rndm();
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
                        //      qDebug()<<time + z / DriftVelocity;
                        PhotonTracker->TracePhoton(&Photon);
                    }
                    //all photons were generated
                }
            }
            else
            {
                //      qDebug()<<"Secondary scintillator was NOT found!";
                NumPhotons = 0;
            }
        }
        else
        {
            //no electrons were generated
            NumPhotons = 0;
        }

        if (DoTextLog) updateLogger();
    }

    if (DoTextLog) storeLog();    //save history for the last event
    return true;
}

bool S2_Generator::initLogger()
{
    //photon hystory log:
    //if OnlySecondary = true, log is filled directly (same as in S1)
    //otherwise, add data to existing log created by S1, in this case need to find the first index
    if (!OnlySecondary)
    {
       int ThisEvent = EnergyVector->at(0)->eventId;
       int max = GeneratedPhotonsHistory->last().event;
       if (ThisEvent > max)
       {
           qWarning() << "Error in S2 generation: index is out of bounds";
           return false;
       }

       int i = -1;
       do ++i;
       while (GeneratedPhotonsHistory->at(i).event != ThisEvent);

       RecordNumber = i;
    }

    return true;
}

void S2_Generator::updateLogger()
{
    if (LastEvent != ThisEvent || LastIndex != ThisIndex || MatId != LastMaterial)
    {
        //new event -> saving previous data
        storeLog();

        //updating current data
        LastEvent = ThisEvent;
        LastIndex = ThisIndex;
        LastMaterial = MatId;
        LastParticle = ThisId;
        TextLogPhotons = NumPhotons;
        TextLogEnergy = DepositedEnergy;
    }
    else
    {
        //continue previous
        TextLogPhotons += NumPhotons;
        TextLogEnergy += DepositedEnergy;
    }
}

void S2_Generator::storeLog()
{
    if (OnlySecondary)
    {
        GeneratedPhotonsHistoryStructure tmp;
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
        (*GeneratedPhotonsHistory)[RecordNumber].SecondaryPhotons = TextLogPhotons;
        RecordNumber++;
    }
}
