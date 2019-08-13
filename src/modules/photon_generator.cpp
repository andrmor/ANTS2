#include "photon_generator.h"
#include "detectorclass.h"
#include "amaterialparticlecolection.h"
#include "generalsimsettings.h"
#include "aphoton.h"
#include "aoneevent.h"
#include "apmhub.h"
#include "alrfmoduleselector.h"

#include <QDebug>

#include "TRandom2.h"
#include "TMath.h"
#include "TH1D.h"

Photon_Generator::Photon_Generator(const DetectorClass &Detector, TRandom2* RandGen, QObject *parent) :
    QObject(parent), Detector(Detector), RandGen(RandGen) {}

Photon_Generator::~Photon_Generator()
{
   //Photon_Generator::deleteSecScintThetaDistribution();
}

void Photon_Generator::GenerateDirectionPrimary(APhoton* Photon)
{    
    //Sphere function of Root:
    double a=0,b=0,r2=1;
    while (r2 > 0.25) {
          a  = RandGen->Rndm() - 0.5;
          b  = RandGen->Rndm() - 0.5;
          r2 =  a*a + b*b;
       }
    Photon->v[2] = ( -1. + 8.0 * r2 );
    double scale = 8.0 * TMath::Sqrt(0.25 - r2);
    Photon->v[0] = a*scale;
    Photon->v[1] = b*scale;
}

void Photon_Generator::GenerateDirectionSecondary(APhoton *Photon)
{
  /* OBSOLETE
  if (SimSet->SecScintGenMode == 3)
      {
        //custom direction     
        double theta = SimSet->SecScintThetaHist->GetRandom();
        double phi = RandGen->Rndm()*2.*3.1415926535;

        Photon->v[2] = cos(theta);
        double proj = sin(theta);
        Photon->v[1] = proj*cos(phi);
        Photon->v[0] = proj*sin(phi);

        return;
      }
  */

  //Sphere function of Root:
  double a=0,b=0,r2=1;
  while (r2 > 0.25) {
        a  = RandGen->Rndm() - 0.5;
        b  = RandGen->Rndm() - 0.5;
        r2 =  a*a + b*b;
     }
  Photon->v[2] = ( -1. + 8.0 * r2 );
  double scale = 8.0 * TMath::Sqrt(0.25 - r2);
  Photon->v[0] = a*scale;
  Photon->v[1] = b*scale;

  if (SimSet->SecScintGenMode == 0) return;
  if (SimSet->SecScintGenMode == 1)
    {
      if (Photon->v[2]<0) //Photon->v[2] = -Photon->v[2];
        Photon->fSkipThisPhoton = true;
      return;
    }
  if (Photon->v[2]>0) //Photon->v[2] = -Photon->v[2];
      Photon->fSkipThisPhoton = true;
}

void Photon_Generator::GenerateWave(APhoton *Photon, int materialId)
{
    const AMaterial* Material = (*Detector.MpCollection)[materialId];
    //  qDebug()<<"name:"<<Material->name;
    //  qDebug()<<"WaveFrom:"<<SimSet->WaveFrom<<"Wave step:"<<SimSet->WaveStep;

    if (Photon->scint_type == 1) //primary scintillation
    {
        if (SimSet->fWaveResolved && Material->PrimarySpectrumHist)
        {
            double wavelength = Material->PrimarySpectrumHist->GetRandom();
            Photon->waveIndex = (wavelength - SimSet->WaveFrom)/SimSet->WaveStep;
            //  qDebug()<<"prim! lambda "<<wavelength<<" index:"<<Photon->waveIndex;
        }
        else Photon->waveIndex= -1;
    }
    else if (Photon->scint_type == 2) //secondary scintillation
    {
        if (SimSet->fWaveResolved && Material->SecondarySpectrumHist)
        {
            double wavelength = Material->SecondarySpectrumHist->GetRandom();
            Photon->waveIndex = (wavelength - SimSet->WaveFrom)/SimSet->WaveStep;
            //  qDebug()<<"sec! lambda "<<wavelength<<" index:"<<Photon->waveIndex;
        }
        else Photon->waveIndex= -1;
    }
}

void Photon_Generator::GenerateTime(APhoton *Photon, int materialId)
{
    const AMaterial* Material = (*Detector.MpCollection)[materialId];
    //  qDebug()<<"name:"<<Material->name;
    //  qDebug()<<Photon->time;

    if (Photon->scint_type == 1) //primary scintillation
        Photon->time += Material->GeneratePrimScintTime(RandGen);
    else if (Photon->scint_type == 2) //secondary scintillation
        Photon->time += RandGen->Exp(  Material->SecScintDecayTime );

    //  qDebug()<<"Final time"<<Photon->time;
}

void Photon_Generator::GenerateSignalsForLrfMode(int NumPhotons, double* r, AOneEvent* OneEvent)
{
    double energy = 1.0 * NumPhotons / SimSet->NumPhotsForLrfUnity; // NumPhotsForLRFunity corresponds to the total number of photons per event for unitary LRF

    //Generating event
    for (int ipm = 0; ipm < Detector.PMs->count(); ipm++)
      {
        double avSignal = Detector.LRFs->getLRF(ipm, r) * energy;
        double avPhotEl = avSignal * SimSet->NumPhotElPerLrfUnity;
        double numPhotEl = RandGen->Poisson(avPhotEl);

        float signal = numPhotEl / SimSet->NumPhotElPerLrfUnity;  // back to LRF units
        OneEvent->addSignals(ipm, signal);
      }
}

/*
void Photon_Generator::deleteSecScintThetaDistribution()
{
  if (!SecScintThetaDistribution) return;
  delete SecScintThetaDistribution;
  SecScintThetaDistribution = 0;
}
*/
