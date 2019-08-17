#include "aoneevent.h"
#include "apmhub.h"
#include "apmtype.h"
#include "asimulationstatistics.h"
#include "generalsimsettings.h"
#include "agammarandomgenerator.h"
#include "acustomrandomsampling.h"

#include <QDebug>

#include "TMath.h"
#include "TRandom2.h"
#include "TH1D.h"

AOneEvent::AOneEvent(APmHub *PMs, TRandom2 *RandGen, ASimulationStatistics* SimStat) :
    SimStat(SimStat), PMs(PMs), RandGen(RandGen)
{
  GammaRandomGen = new AGammaRandomGenerator(RandGen);
}

AOneEvent::~AOneEvent()
{
    delete GammaRandomGen;
}

void AOneEvent::configure(const GeneralSimSettings *simSet)
{
    SimSet = simSet;
    numPMs = PMs->count();
    SiPMpixels.resize(numPMs);
    for (int ipm = 0; ipm < numPMs; ipm++)
    {
        QBitArray sipmPixels;
        const APmType *type = PMs->getTypeForPM(ipm);
        if(type->SiPM)
        {
             sipmPixels = QBitArray((SimSet->fTimeResolved ? SimSet->TimeBins : 1) * type->PixelsX * type->PixelsY);
             SiPMpixels[ipm] = sipmPixels;
        }
    }
    AOneEvent::clearHits(); //clears and resizes the hits / signals containers
}

void AOneEvent::clearHits()
{
  if (PMhits.size()    != numPMs)
      PMhits.resize(numPMs);
  if (PMsignals.size() != numPMs)
      PMsignals.resize(numPMs);

  if (SimSet->fTimeResolved)
  {
      if (TimedPMhits.size()    != SimSet->TimeBins)
          TimedPMhits.resize(SimSet->TimeBins);
      if (TimedPMsignals.size() != SimSet->TimeBins)
          TimedPMsignals.resize(SimSet->TimeBins);

      for (int itime = 0; itime < SimSet->TimeBins; itime++)
      {
          if (TimedPMhits.at(itime).size()    != numPMs)
              TimedPMhits[itime].resize(numPMs);
          if (TimedPMsignals.at(itime).size() != numPMs)
              TimedPMsignals[itime].resize(numPMs);
      }
  }

  for (int ipm = 0; ipm < numPMs; ipm++)
  {
      PMhits[ipm] = 0;
      PMsignals[ipm] = 0;

      if (SimSet->fTimeResolved)
          for (int itime = 0; itime < SimSet->TimeBins; itime++)
          {
              TimedPMhits[itime][ipm] = 0;
              TimedPMsignals[itime][ipm] = 0;
          }

      SiPMpixels[ipm].fill(false);
  }
}

bool AOneEvent::CheckPMThit(int ipm, double time, int WaveIndex, double x, double y, double cosAngle, int Transitions, double rnd)
{
  //if time resolved, first check we are inside time window!
  int iTime = 0;
  if (SimSet->fTimeResolved)
    {
      iTime = AOneEvent::TimeToBin(time);
      if (iTime == -1) return false;
    }
  //cheking vs photon detection efficiency
  double DetProbability = PMs->getActualPDE(ipm, WaveIndex);
  //checking vs angular response
  DetProbability *= PMs->getActualAngularResponse(ipm, cosAngle);
  //checking vs area response
  DetProbability *= PMs->getActualAreaResponse(ipm, x, y);
  if (rnd > DetProbability)  //random number is provided by the tracker - done for the accelerator function
      return false;

  if (SimSet->LogsStatOptions.bPhotonDetectionStat) CollectStatistics(WaveIndex, time, cosAngle, Transitions);

  PMhits[ipm] += 1.0f;
  if (SimSet->fTimeResolved) TimedPMhits[iTime][ipm] += 1.0f;

  return true;
}

bool AOneEvent::CheckSiPMhit(int ipm, double time, int WaveIndex, double x, double y, double cosAngle, int Transitions, double rnd)
{
  //if time resolved, first check we are inside time window!
  int iTime = 0;
  if (SimSet->fTimeResolved)
    {
      iTime = AOneEvent::TimeToBin(time);
      if (iTime == -1) return false;
    }
  //cheking vs photon detection efficiency
  double DetProbability = 1;
  DetProbability *= PMs->getActualPDE(ipm, WaveIndex);
  //checking vs angular response
  DetProbability *= PMs->getActualAngularResponse(ipm, cosAngle);
  //checking vs area response
  DetProbability *= PMs->getActualAreaResponse(ipm, x, y);
  //    qDebug()<<"composite detection probability: "<<DetProbability;
  if (rnd > DetProbability) //random number is provided by the tracker - done for the accelerator function
      return false;

  if (SimSet->LogsStatOptions.bPhotonDetectionStat) CollectStatistics(WaveIndex, time, cosAngle, Transitions);

  //    qDebug()<<"Detected!";
  const int itype = PMs->at(ipm).type;
  const APmType* tp = PMs->getType(itype);
  double sizeX = tp->SizeX;
  int pixelsX =  tp->PixelsX;
  //double pixLengthX = sizeX / pixelsX;
  //int binX = (x + sizeX*0.5) / pixLengthX;
  int binX = pixelsX * (x/sizeX + 0.5);
  if (binX<0) binX = 0;
  if (binX>pixelsX-1) binX = pixelsX-1;

  double sizeY = tp->SizeY;//PMtypeProperties[itype].SizeY;
  int pixelsY =  tp->PixelsY;// PMtypeProperties[itype].PixelsY;
  //double pixLengthY = sizeY / pixelsY;
  //int binY = (y + sizeY*0.5) / pixLengthY;
  int binY = pixelsY * (y/sizeY + 0.5);
  if (binY<0) binY = 0;
  if (binY>pixelsY-1) binY = pixelsY-1;

  if (PMs->isDoMCcrosstalk() && PMs->at(ipm).MCmodel==0)
    {
      int num = PMs->at(ipm).MCsampl->sample(RandGen) + 1;
      registerSiPMhit(ipm, iTime, binX, binY, num);
    }
  else
    registerSiPMhit(ipm, iTime, binX, binY);

  return true;
}

void AOneEvent::registerSiPMhit(int ipm, int iTime, int binX, int binY, float numHits)
//numHits != 1 is used 1) for the simplistic model of microcell cross-talk -> then MCmodel = 0
//                     2) to simulate dark counts in advanced model (MCmodel = 1)
{
  const APmType* tp = PMs->getTypeForPM(ipm);

  if (!SimSet->fTimeResolved)
  {
      const int iXY = tp->PixelsX*binY + binX;
      if (SiPMpixels[ipm].testBit(iXY)) return;  //nothing to do - already lit

      //registering hit
      SiPMpixels[ipm].setBit(iXY, true);
      PMhits[ipm] += numHits;

      if (PMs->isDoMCcrosstalk()  &&  PMs->at(ipm).MCmodel == 1)
      {
          //checking 4 neighbours
          const double& trigProb = PMs->at(ipm).MCtriggerProb;
          if (binX > 0             && RandGen->Rndm() < trigProb) registerSiPMhit(ipm, iTime, binX-1, binY, numHits);//left
          if (binX+1 < tp->PixelsX && RandGen->Rndm() < trigProb) registerSiPMhit(ipm, iTime, binX+1, binY, numHits);//right
          if (binY > 0             && RandGen->Rndm() < trigProb) registerSiPMhit(ipm, iTime, binX, binY-1, numHits);//bottom
          if (binY+1 < tp->PixelsY && RandGen->Rndm() < trigProb) registerSiPMhit(ipm, iTime, binX, binY+1, numHits);//top
      }
  }
  else
  {
      const int timeBinInterval = tp->PixelsX * tp->PixelsY;
      const int iTXY = iTime*timeBinInterval + tp->PixelsX*binY + binX;

      //have to check status of pixel during the recovery time from iTime and register hit in each "non-lit" time bins
      const int Tbins = SimSet->TimeBins < 1 ? 1 : SimSet->TimeBins; //protection
      int bins = tp->RecoveryTime * Tbins / (SimSet->TimeTo - SimSet->TimeFrom);  //time bins during which the pixel keeps lit status
      if (bins < 1) bins = 1;

      //Simplified model is used!
      //"later"-emitted photons can appear before "earlier"-emitted photons - overlaps in fired pixels will be wrong at ~saturation conditions!

      const int imax = (bins <= SimSet->TimeBins-iTime) ? bins : SimSet->TimeBins-iTime;
      for (int itime = 0; itime < imax; itime++)
      {
          if (!SiPMpixels[ipm].testBit(iTXY+itime*timeBinInterval)) //not yet lit here
          {
              //registering the hit
              SiPMpixels[ipm].setBit(iTXY+itime*timeBinInterval, true);
              PMhits[ipm] += numHits;
              TimedPMhits[iTime+itime][ipm] += numHits;

              if (PMs->isDoMCcrosstalk()  &&  PMs->at(ipm).MCmodel == 1)
              {
                  //checking 4 neighbours
                  const double& trigProb = PMs->at(ipm).MCtriggerProb;
                  if (binX > 0             && RandGen->Rndm() < trigProb) registerSiPMhit(ipm, iTime, binX-1, binY, numHits);//left
                  if (binX+1 < tp->PixelsX && RandGen->Rndm() < trigProb) registerSiPMhit(ipm, iTime, binX+1, binY, numHits);//right
                  if (binY > 0             && RandGen->Rndm() < trigProb) registerSiPMhit(ipm, iTime, binX, binY-1, numHits);//bottom
                  if (binY+1 < tp->PixelsY && RandGen->Rndm() < trigProb) registerSiPMhit(ipm, iTime, binX, binY+1, numHits);//top
              }
          }
        }
  }
}

bool AOneEvent::isHitsEmpty() const
{
  for (int ipm = 0; ipm < numPMs; ipm++)
    if (PMhits.at(ipm) != 0) return false;  // set to exact zero on init, any non-zero is fine
  return true;
}

void AOneEvent::HitsToSignal()
{
    //PMsignals.resize(numPMs);

    if (PMs->fDoDarkCounts) AddDarkCounts(); //add dark counts for all SiPMs

    if (SimSet->fTimeResolved)
    {
        for (int itime = 0; itime < SimSet->TimeBins; itime++)
        {
            convertHitsToSignal(TimedPMhits.at(itime), TimedPMsignals[itime]);

            for (int ipm = 0; ipm < numPMs; ipm++)
                PMsignals[ipm] += TimedPMsignals.at(itime).at(ipm);
        }
    }
    else convertHitsToSignal(PMhits, PMsignals);
}

void AOneEvent::convertHitsToSignal(const QVector<float> & pmHits, QVector<float> & pmSignals)
{
    for (int ipm = 0; ipm < numPMs; ipm++)
    {
        const APm & pm = PMs->at(ipm);

        // SPE
        if ( PMs->isDoPHS() )
        {
            switch (pm.SPePHSmode)
            {
            case 0:
                pmSignals[ipm] = pm.AverageSigPerPhE * pmHits.at(ipm);
                break;
            case 1:
              {
                double mean =  pm.AverageSigPerPhE * pmHits.at(ipm);
                double sigma = pm.SPePHSsigma * TMath::Sqrt( pmHits.at(ipm) );
                pmSignals[ipm] = RandGen->Gaus(mean, sigma);
                break;
              }
            case 2:
              {
                double k = pm.SPePHSshape;
                double theta = pm.AverageSigPerPhE / k;
                k *= pmHits.at(ipm); //for sum distribution
                pmSignals[ipm] = GammaRandomGen->getGamma(k, theta);
                break;
              }
            case 3:
              {
                pmSignals[ipm] = 0;
                if ( pm.SPePHShist )
                {
                    for (int j = 0; j < pmHits.at(ipm); j++)
                        pmSignals[ipm] += pm.SPePHShist->GetRandom();
                }
              }
            }

            pmSignals[ipm] *= pm.relElStrength;
        }
        else pmSignals[ipm] = pmHits.at(ipm);

        // electronic noise
        if (PMs->isDoElNoise())
        {
            const double& sigma_const = pm.ElNoiseSigma;
            const double& sigma_stat  = pm.ElNoiseSigma_StatSigma;
            const double& sigma_norm  = pm.ElNoiseSigma_StatNorm;

            const double sigma = (sigma_stat == 0 ? sigma_const : sqrt( sigma_const*sigma_const  +  sigma_stat*sigma_stat * pmSignals.at(ipm) / sigma_norm) );
            pmSignals[ipm] += RandGen->Gaus(0, sigma);
        }

        // ADC simulation
        if (PMs->isDoADC())
        {
            if (pmSignals[ipm] < 0) pmSignals[ipm] = 0;
            else
            {
                if (pmSignals[ipm] > pm.ADCmax) pmSignals[ipm] = pm.ADClevels;
                else pmSignals[ipm] = static_cast<int>( pmSignals.at(ipm) / PMs->at(ipm).ADCstep );
            }
        }
    }
}

void AOneEvent::AddDarkCounts() //currently applicable only for SiPMs!
{
  for (int ipm = 0; ipm < numPMs; ipm++)
      if (PMs->isSiPM(ipm))
      {
          const APmType* typ = PMs->getTypeForPM(ipm);
          const int&    pixelsX =  typ->PixelsX;
          const int&    pixelsY =  typ->PixelsY;
          const double& darkRate = typ->DarkCountRate; //in Hz
          //   qDebug() << "SiPM dark rate:" << darkRate << "Hz";

          const int     iTimeBins = SimSet->fTimeResolved ? SimSet->TimeBins : 1;
          const double  TimeInterval = ( iTimeBins == 1 ? PMs->at(ipm).MeasurementTime : (SimSet->TimeTo - SimSet->TimeFrom)/SimSet->TimeBins );
          //   qDebug() << "Time interval:" << TimeInterval << "ns";

          const double  averageDarkCounts = darkRate * TimeInterval * 1.0e-9;
          //   qDebug() << "Average dark counts per time bin:" << averageDarkCounts;

          const double pixelFiringProbability = averageDarkCounts / pixelsX / pixelsY;
          //   qDebug() << "Firing probability of each pixel per time bin:" << pixelFiringProbability;

          if (pixelFiringProbability < 0.05) //if it is less than 5% assuming there will be no overlap in triggered pixels
            {
              //quick procedure
              for (int iTime = 0; iTime < iTimeBins; iTime++)
                {
                  int DarkCounts = RandGen->Poisson(averageDarkCounts);
                  //    qDebug() << "Actual dark counts" << DarkCounts;
                  for (int iev = 0; iev < DarkCounts; iev++)
                    {
                      int iX = pixelsX * RandGen->Rndm();
                      if (iX >= pixelsX) iX = pixelsX-1;//protection
                      int iY = pixelsY * RandGen->Rndm();
                      if (iY >= pixelsY) iY = pixelsY-1;
                      //   qDebug()<<"Pixels:"<<iX<<iY;

                      registerSiPMhit(ipm, iTime, iX, iY, generateDarkHitIncrement(ipm));
                    }
                }
            }
          else
            {
              //slow but accurate procedure
              for (int iTime = 0; iTime<iTimeBins; iTime++)
                {
                  for (int iX = 0; iX<pixelsX; iX++)
                      for (int iY = 0; iY<pixelsY; iY++)
                        {
                          if (RandGen->Rndm() < pixelFiringProbability)
                              registerSiPMhit(ipm, iTime, iX, iY, generateDarkHitIncrement(ipm));
                        }
                }
            }
      }
}

float AOneEvent::generateDarkHitIncrement(int ipm) const
{
    if (PMs->at(ipm).DarkCounts_Model == 0 || PMs->at(ipm).DarkCounts_Distribution.isEmpty()) return 1.0f;

    const int index = RandGen->Uniform(PMs->at(ipm).DarkCounts_Distribution.size());
    return PMs->at(ipm).DarkCounts_Distribution.at(index);
}

void AOneEvent::CollectStatistics(int WaveIndex, double time, double cosAngle, int Transitions)
{
    SimStat->registerWave(WaveIndex);
    SimStat->registerTime(time);
    if (SimSet->fAngResolved)
    {
      double angle = TMath::ACos(cosAngle)*180.0/3.1415926535;
      SimStat->registerAngle(angle);
    }
    SimStat->registerNumTrans(Transitions);
}

int AOneEvent::TimeToBin(double time) const
{
  if (time < SimSet->TimeFrom || time > SimSet->TimeTo) return -1;
  if (SimSet->TimeBins == 0) return -1;
  if (time == SimSet->TimeTo) return SimSet->TimeBins-1; //too large index protection
  return (time - SimSet->TimeFrom)/(SimSet->TimeTo - SimSet->TimeFrom) * SimSet->TimeBins;
}
