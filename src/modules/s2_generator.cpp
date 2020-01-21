#include "s2_generator.h"
#include "amaterialparticlecolection.h"
#include "photon_generator.h"
#include "aphotontracer.h"
#include "aphoton.h"
#include "aenergydepositioncell.h"
#include "generalsimsettings.h"

#include <QDebug>

#include "TGeoManager.h"
#include "TRandom2.h"

S2_Generator::S2_Generator(Photon_Generator *photonGenerator, APhotonTracer *photonTracker, QVector<AEnergyDepositionCell *> *energyVector, TRandom2 *RandomGenerator, TGeoManager *geoManager, AMaterialParticleCollection *materialCollection, QVector<GeneratedPhotonsHistoryStructure> *PhotonsHistory)
{
    PhotonGenerator = photonGenerator;
    PhotonTracker = photonTracker;
    EnergyVector = energyVector;
    RandGen = RandomGenerator;
    GeoManager = geoManager;
    MaterialCollection = materialCollection;
    GeneratedPhotonsHistory = PhotonsHistory;
}

bool S2_Generator::Generate() //uses MW->EnergyVector as the input parameter
{
    if (EnergyVector->isEmpty()) return true; //no deposition data -> no secondary to generate

    if (DoTextLog)
    {
        bool bOK = initLogger();
        if (!bOK) return false;
    }

    double ElectronRemainer = 0.0;
    PhotonRemainer = 0.0;
    
    TextLogPhotons = 0;
    TextLogEnergy  = 0.0;

    const AEnergyDepositionCell * fc = EnergyVector->at(0);
    LastEvent    = fc->eventId;
    LastIndex    = fc->index;
    LastParticle = fc->ParticleId;
    LastMaterial = fc->MaterialId;

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
            BaseTime = cell->time;
            //       qDebug()<<"start time: "<<time;

            TString VolName = GeoManager->GetCurrentVolume()->GetName();
            if (VolName != "SecScint") doDrift(VolName);

            if ( VolName == "SecScint" )
            {
                //       qDebug()<<"found secondary scint";
                generateLight(cell->r);
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

void S2_Generator::doDrift(TString & VolName)
{
    DiffusionRecords.clear();

    GeoManager->SetCurrentDirection(0, 0, 1.0);  //up
    do
    {
        //drifting up until secondary scintillator ("SecScint") or outside of the defined geometry
        int ThisMatIndex = GeoManager->GetCurrentVolume()->GetMaterial()->GetIndex();

        GeoManager->FindNextBoundaryAndStep();
        if (GeoManager->IsOutside()) break;

        const double Step = GeoManager->GetStep();
        const double DriftVelocity = MaterialCollection->getDriftSpeed(ThisMatIndex);
        if (DriftVelocity != 0)
        {
            BaseTime += Step / DriftVelocity;

            double SigmaTime       = MaterialCollection->getDiffusionSigmaTime(ThisMatIndex, Step);
            double SigmaTransverse = MaterialCollection->getDiffusionSigmaTransverse(ThisMatIndex, Step);
            //      qDebug() << VolName << "-ST,STr->"<<SigmaTime << SigmaTransverse;
            if (SigmaTime != 0 || SigmaTransverse != 0)
                DiffusionRecords << DiffSigmas(SigmaTime, SigmaTransverse);
        }

        VolName = GeoManager->GetCurrentVolume()->GetName();
        //      qDebug()<<"Found new volume: "<<VolName<<" drifted: "<<Step<<"new time: "<<time;
    }
    while ( VolName != "SecScint" );
}

void S2_Generator::generateLight(double * DepoPosition)
{
    const int    MatIndexSecScint = GeoManager->GetCurrentVolume()->GetMaterial()->GetIndex();
    const double PhotonsPerElectron = (*MaterialCollection)[MatIndexSecScint]->SecYield;
    //       qDebug()<<"matIndex of sec scintillator:"<<MatIndexSecScint<<"PhPerEl"<<PhotonsPerElectron;
    const double Zstart = GeoManager->GetCurrentPoint()[2];
    //       qDebug()<<"Sec scint starts: "<<Zstart;

    GeoManager->FindNextBoundary();
    const double Zspan = GeoManager->GetStep();

    //generate photons
    if (DiffusionRecords.isEmpty() || PhotonGenerator->SimSet->fLRFsim)
    {
        //reusing old system
        double Photons = NumElectrons * PhotonsPerElectron + PhotonRemainer;
        NumPhotons     = (int)Photons;
        PhotonRemainer = Photons - (double)NumPhotons;

        if (PhotonGenerator->SimSet->fLRFsim)
            PhotonGenerator->GenerateSignalsForLrfMode(NumPhotons, DepoPosition, PhotonTracker->getEvent());
        else
            generateAndTracePhotons(DepoPosition, BaseTime, NumPhotons, MatIndexSecScint, Zstart, Zspan);
    }
    else
    {
        //diffusion is in effect
        for (int iElectron = 0; iElectron < NumElectrons; iElectron++)
        {
            double t = BaseTime;
            double pos[3];
            pos[0] = DepoPosition[0];
            pos[1] = DepoPosition[1];
            pos[2] = Zstart + 0.5*Zspan; // try to be inside the SecScint in Z
            bool bInside = true;
            for (const DiffSigmas & rec : DiffusionRecords)
            {
                t += RandGen->Gaus(0, rec.sigmaTime);
                pos[0] += RandGen->Gaus(0, rec.sigmaX);
                pos[1] += RandGen->Gaus(0, rec.sigmaX);

                GeoManager->SetCurrentPoint(pos);
                GeoManager->FindNode();
                const TString volName = GeoManager->GetCurrentVolume()->GetName();
                if (volName != "SecScint")
                {
                    bInside = false;
                    break;
                }
            }
            if (!bInside)
            {
                //qDebug() << "Left SecScint during diffusion";
                continue;
            }

            double Photons = PhotonsPerElectron + PhotonRemainer;
            int NumPhotonsThisEl = (int)Photons;
            PhotonRemainer = Photons - (double)NumPhotonsThisEl;
            NumPhotons += NumPhotonsThisEl;

            generateAndTracePhotons(pos, t, NumPhotonsThisEl, MatIndexSecScint, Zstart, Zspan);
        }
    }
}

void S2_Generator::generateAndTracePhotons(double * Position, double Time, int NumPhotonsToGenerate, int MatIndexSecScint, double Zstart, double Zspan)
{
    APhoton Photon;
    Photon.r[0] = Position[0];
    Photon.r[1] = Position[1];
    Photon.scint_type = 2;
    Photon.SimStat = PhotonGenerator->DetStat;

    const double DriftVelocity = MaterialCollection->getDriftSpeed(MatIndexSecScint);
    for (int iPhoton = 0; iPhoton < NumPhotonsToGenerate; iPhoton++)
    {
        //random z inside secondary scintillator
        const double z = Zspan * RandGen->Rndm();
        Photon.r[2] = Zstart + z;
        Photon.time = Time + z / DriftVelocity;

        PhotonGenerator->GenerateDirectionSecondary(&Photon);
        //catching photons of the wrong direction, resetting the "skip" status!
        if (Photon.fSkipThisPhoton)
        {
            Photon.fSkipThisPhoton = false;
            continue;
        }

        PhotonGenerator->GenerateWave(&Photon, MatIndexSecScint);
        PhotonGenerator->GenerateTime(&Photon, MatIndexSecScint);
        PhotonTracker->TracePhoton(&Photon);
    }
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
