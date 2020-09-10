#include "s1_generator.h"
#include "amaterialparticlecolection.h"
#include "photon_generator.h"
#include "aphotontracer.h"
#include "aphoton.h"
#include "aenergydepositioncell.h"
#include "ageneralsimsettings.h"
#include "aoneevent.h"

#include <QDebug>

#include "TRandom2.h"

S1_Generator::S1_Generator(Photon_Generator *photonGenerator, APhotonTracer *photonTracker, AMaterialParticleCollection *materialCollection, QVector<AEnergyDepositionCell *> *energyVector, QVector<GeneratedPhotonsHistoryStructure> *PhotonsHistory, TRandom2* RandomGenerator)
{
    PhotonGenerator = photonGenerator;
    PhotonTracker = photonTracker;
    EnergyVector = energyVector;
    MaterialCollection = materialCollection;
    GeneratedPhotonsHistory = PhotonsHistory;
    RandGen = RandomGenerator;
}

bool S1_Generator::Generate() //uses MW->EnergyVector as the input parameter
{
    if (EnergyVector->isEmpty()) return true; //no data

    //what is the first particle to work with?
    int MatId = EnergyVector->at(0)->MaterialId;

    double Remainer = 0.0;
    double Photons;
    int NumPhotons;

    //text log-related inits
    int TextLogPhotons = 0;
    double TextLogEnergy = 0.0;
    int LastEvent = EnergyVector->at(0)->eventId;
    int LastIndex = EnergyVector->at(0)->index;
    int LastParticle = EnergyVector->at(0)->ParticleId;
    int LastMaterial = EnergyVector->at(0)->MaterialId;

    for (int iEv = 0; iEv < EnergyVector->size(); iEv++)
      {
        //checking are we still tracking the same particle?
        int ThisId = EnergyVector->at(iEv)->ParticleId;
        int ThisEvent = EnergyVector->at(iEv)->eventId;
        int ThisIndex = EnergyVector->at(iEv)->index;

        if (LastEvent != ThisEvent || LastIndex != ThisIndex)
          {
            //Id = ThisId;
            //reset remainer
            Remainer = 0;            
          }

        MatId = EnergyVector->at(iEv)->MaterialId;
        //PhotonYield = (*MaterialCollection)[MatId]->MatParticle[ThisId].PhYield;
        double PhotonYield   = (*MaterialCollection)[MatId]->getPhotonYield(ThisId);
        double IntrEnergyRes = (*MaterialCollection)[MatId]->getIntrinsicEnergyResolution(ThisId);

        //if ((*MaterialCollection)[MatId]->MatParticle[ThisId].IntrEnergyRes == 0)
        if (IntrEnergyRes == 0)
           Photons = EnergyVector->at(iEv)->dE * PhotonYield + Remainer;
        else
        {
           double mean =  EnergyVector->at(iEv)->dE * PhotonYield + Remainer;
           //double sigma = (*MaterialCollection)[MatId]->MatParticle[ThisId].IntrEnergyRes * mean /2.35482;
           double sigma = IntrEnergyRes * mean /2.35482;
           Photons = RandGen->Gaus(mean, sigma);
        }

        NumPhotons = (int)Photons;
        Remainer   = Photons - (double)NumPhotons;

        if (PhotonGenerator->SimSet->fLRFsim)
            PhotonGenerator->GenerateSignalsForLrfMode(NumPhotons, (*EnergyVector)[iEv]->r, PhotonTracker->getEvent());
        else
        {
            //generate photons
    //        qDebug()<<"Generate photons: "<<NumPhotons;
            APhoton Photon;
            Photon.r[0] = (*EnergyVector)[iEv]->r[0];
            Photon.r[1] = (*EnergyVector)[iEv]->r[1];
            Photon.r[2] = (*EnergyVector)[iEv]->r[2];
            Photon.scint_type = 1;
            Photon.SimStat = PhotonGenerator->DetStat;
    //        qDebug() << "Photons:"<<NumPhotons;
            for (int ii=0; ii<NumPhotons; ii++)
              {
                Photon.time = (*EnergyVector)[iEv]->time;  //will be changed if there is time dependence
                PhotonGenerator->GenerateDirection(&Photon);
                PhotonGenerator->GenerateWave(&Photon, MatId);
                PhotonGenerator->GenerateTime(&Photon, MatId);
    //            qDebug() << "r, v, wave"<<Photon.r[0]<<Photon.r[1]<<Photon.r[2]<<Photon.v[0]<<Photon.v[1]<<Photon.v[2]<<Photon.WaveIndex;
                PhotonTracker->TracePhoton(&Photon);
              }
    //        qDebug() << "All photons tracked";
        }

        //Text log
        if (DoTextLog)
          {            
            if (LastEvent != ThisEvent || LastIndex != ThisIndex || MatId != LastMaterial)
              {
                //changed - saving previous data
                GeneratedPhotonsHistoryStructure tmp;// = {};
                tmp.Energy = TextLogEnergy;
                tmp.MaterialId = LastMaterial;
                tmp.ParticleId = LastParticle;
                tmp.PrimaryPhotons = TextLogPhotons;
                tmp.SecondaryPhotons = 0;
                tmp.event = LastEvent;
                tmp.index = LastIndex;
                GeneratedPhotonsHistory->append(tmp);

                //updating current data
                LastEvent = ThisEvent;
                LastIndex = ThisIndex;
                LastMaterial = MatId;
                LastParticle = ThisId;
                TextLogPhotons = NumPhotons;
                TextLogEnergy = EnergyVector->at(iEv)->dE;
              }
            else
              {
                //continue previous
                TextLogPhotons += NumPhotons;
                TextLogEnergy += EnergyVector->at(iEv)->dE;
              }
          }    
      }

    if (DoTextLog)
      { //last point save
        GeneratedPhotonsHistoryStructure tmp;// = {};
        tmp.Energy = TextLogEnergy;
        tmp.MaterialId = LastMaterial;
        tmp.ParticleId = LastParticle;
        tmp.PrimaryPhotons = TextLogPhotons;
        tmp.SecondaryPhotons = 0;
        tmp.event = LastEvent;
        tmp.index = LastIndex;
        GeneratedPhotonsHistory->append(tmp);
        //qDebug() << "History:----"<<tmp.PrimaryPhotons;
      }

     return true;
}
