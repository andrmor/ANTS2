#include "oneeventclass.h"
#include "pms.h"
#include "pmtypeclass.h"
#include "asimulationstatistics.h"
#include "generalsimsettings.h"
#include "agammarandomgenerator.h"
#include "acustomrandomsampling.h"

#include <QDebug>

#include "TMath.h"
#include "TRandom2.h"
#include "TH1D.h"

OneEventClass::OneEventClass(pms *Pms, TRandom2 *randGen, ASimulationStatistics* simStat)
{
  PMs = Pms;
  RandGen = randGen;
  SimStat = simStat;

  GammaRandomGen = new AGammaRandomGenerator(); 
}

OneEventClass::~OneEventClass()
{
    delete GammaRandomGen;
}

void OneEventClass::configure(const GeneralSimSettings *simSet)
{
  SimSet = simSet; 
  numPMs = PMs->count();
  SiPMpixels.resize(numPMs);
  for (int ipm=0; ipm<numPMs; ipm++)
    {
      QBitArray sipmPixels;
      PMtypeClass *type = PMs->getTypeForPM(ipm);
      if(type->SiPM)
        {
           sipmPixels = QBitArray((SimSet->fTimeResolved ? SimSet->TimeBins : 1) * type->PixelsX * type->PixelsY);
           SiPMpixels[ipm] = sipmPixels;
        }
    }
  OneEventClass::clearHits(); //the rest of object are resized there  
}

void OneEventClass::clearHits()
{
  //prot
  PMhitsTotal.resize(numPMs);
  PMsignals.resize(numPMs);
  if (SimSet->fTimeResolved)
    {
      TimedPMhits.resize(SimSet->TimeBins);
      TimedPMsignals.resize(SimSet->TimeBins);
      for (int itime=0; itime<SimSet->TimeBins; itime++)
        {
          TimedPMhits[itime].resize(numPMs);
          TimedPMsignals[itime].resize(numPMs);
        }
    }

  for (int ipm=0; ipm<numPMs; ipm++)
  {
      //total per PM
      PMhitsTotal[ipm] = 0;
      PMsignals[ipm] = 0;

      //time resolved info
      if (SimSet->fTimeResolved)
          for (int itime=0; itime<SimSet->TimeBins; itime++)
          {
              TimedPMhits[itime][ipm] = 0;
              TimedPMsignals[itime][ipm] = 0;
          }

      //SiPMpixels info
      SiPMpixels[ipm].fill(false);
  }
}

bool OneEventClass::CheckPMThit(int ipm, double time, int WaveIndex, double x, double y, double cosAngle, int Transitions, double rnd)
{
  //if time resolved, first check we are inside time window!
  int iTime = 0;
  if (SimSet->fTimeResolved)
    {
      iTime = OneEventClass::TimeToBin(time);
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

  if (SimSet->fLogsStat) CollectStatistics(WaveIndex, time, cosAngle, Transitions);

  PMhitsTotal[ipm]++;
  if (SimSet->fTimeResolved) TimedPMhits[iTime][ipm]++;

  return true;
}

bool OneEventClass::CheckSiPMhit(int ipm, double time, int WaveIndex, double x, double y, double cosAngle, int Transitions, double rnd)
{
  //if time resolved, first check we are inside time window!
  int iTime = 0;
  if (SimSet->fTimeResolved)
    {
      iTime = OneEventClass::TimeToBin(time);
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

  if (SimSet->fLogsStat) CollectStatistics(WaveIndex, time, cosAngle, Transitions);

  //    qDebug()<<"Detected!";
  const int itype = PMs->at(ipm).type;
  PMtypeClass* tp = PMs->getType(itype);
  double sizeX = tp->SizeX;//PMtypeProperties[typ].SizeX;
  int pixelsX =  tp->PixelsX;//PMtypeProperties[itype].PixelsX;
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
      int num = PMs->at(ipm).MCsampl->sample() + 1;
      registerSiPMhit(ipm, iTime, binX, binY, num);
    }
  else
    registerSiPMhit(ipm, iTime, binX, binY);

  return true;
}

void OneEventClass::registerSiPMhit(int ipm, int iTime, int binX, int binY, int numHits)
//numHits != 1 is used only for the simplistic model of microcell cross-talk!
{
  PMtypeClass *tp = PMs->getTypeForPM(ipm);
  if (!SimSet->fTimeResolved)
  {
      const int iXY = tp->PixelsX*binY + binX;
      if (SiPMpixels[ipm].testBit(iXY)) return;  //nothing to do - already lit

      //registering hit
      SiPMpixels[ipm].setBit(iXY, true);
      PMhitsTotal[ipm] += numHits;

      if (PMs->isDoMCcrosstalk() && PMs->at(ipm).MCmodel==1)
      {
          const int itype = PMs->at(ipm).type;
          PMtypeClass* tp = PMs->getType(itype);
          //checking 4 neighbours
          if (binX>0 && RandGen->Rndm()<PMs->at(ipm).MCtriggerProb) registerSiPMhit(ipm, iTime, binX-1, binY);//left
          if (binX+1<tp->PixelsX && RandGen->Rndm()<PMs->at(ipm).MCtriggerProb) registerSiPMhit(ipm, iTime, binX+1, binY);//right
          if (binY>0 && RandGen->Rndm()<PMs->at(ipm).MCtriggerProb) registerSiPMhit(ipm, iTime, binX, binY-1);//bottom
          if (binY+1<tp->PixelsY && RandGen->Rndm()<PMs->at(ipm).MCtriggerProb) registerSiPMhit(ipm, iTime, binX, binY+1);//top
      }
      return;
  }

  const int timeBinInterval = tp->PixelsX * tp->PixelsY;
  const int iTXY = iTime*timeBinInterval + tp->PixelsX*binY + binX;

  //have to check status of pixel during the recovery time from iTime and register hit in each "non-lit" time bins
  const int Tbins = SimSet->TimeBins < 1 ? 1 : SimSet->TimeBins; //protection
  int bins = tp->RecoveryTime * Tbins / (SimSet->TimeTo - SimSet->TimeFrom);  //time bins during which the pixel keeps lit status
  if (bins < 1) bins = 1;

  //Simplified model is used!
  //"later"-emitted photons can appear before "earlier"-emitted photons - overlaps in fired pixels will be wrong at ~saturation conditions!

  const int imax = (bins <= SimSet->TimeBins-iTime) ? bins : SimSet->TimeBins-iTime;
  for (int i = 0; i < imax; i++)
  {
      if (!SiPMpixels[ipm].testBit(iTXY+i*timeBinInterval)) //not yet lit here
      {
          //registering the hit
          SiPMpixels[ipm].setBit(iTXY+i*timeBinInterval, true);
          PMhitsTotal[ipm] += numHits;
          TimedPMhits[iTime+i][ipm] += numHits;

          if (PMs->isDoMCcrosstalk() && PMs->at(ipm).MCmodel==1)
          {
              const int itype = PMs->at(ipm).type;
              PMtypeClass* tp = PMs->getType(itype);
              //checking 4 neighbours
              if (binX>0 && RandGen->Rndm()<PMs->at(ipm).MCtriggerProb) registerSiPMhit(ipm, iTime, binX-1, binY);//left
              if (binX+1<tp->PixelsX && RandGen->Rndm()<PMs->at(ipm).MCtriggerProb) registerSiPMhit(ipm, iTime, binX+1, binY);//right
              if (binY>0 && RandGen->Rndm()<PMs->at(ipm).MCtriggerProb) registerSiPMhit(ipm, iTime, binX, binY-1);//bottom
              if (binY+1<tp->PixelsY && RandGen->Rndm()<PMs->at(ipm).MCtriggerProb) registerSiPMhit(ipm, iTime, binX, binY+1);//top
          }
      }
    }
}

bool OneEventClass::isHitsEmpty()
{
  for (int i=0; i<numPMs; i++)
    if (PMhitsTotal[i] > 0) return false;
  return true;
}


void OneEventClass::HitsToSignal()
{
  OneEventClass::AddDarkCounts(); //add dark counts for all SiPMs

  PMsignals.resize(numPMs);  //protection
  if (SimSet->fTimeResolved)
  {
      //preparing the container
      TimedPMsignals.resize(SimSet->TimeBins);
      for (int itime =0; itime < SimSet->TimeBins; itime++) TimedPMsignals[itime].resize(numPMs);

      //converting hits to signal
      for (int ipm=0; ipm<numPMs; ipm++)
      {
          if (SimSet->fTimeResolved)
          {
              PMsignals[ipm] = 0; //this will be accumulator
              for (int t=0; t<SimSet->TimeBins; t++)
              {
                  if ( PMs->isDoPHS() )
                  {
                      switch ( PMs->at(ipm).SPePHSmode )
                      {
                        case 0:
                          TimedPMsignals[t][ipm] = PMs->at(ipm).AverageSignalPerPhotoelectron * TimedPMhits[t][ipm];
                          break;
                        case 1:
                        {
                          double mean = PMs->at(ipm).AverageSignalPerPhotoelectron * TimedPMhits[t][ipm];
                          double sigma = TMath::Sqrt(TimedPMhits[t][ipm]) * PMs->getSPePHSsigma(ipm);
                          TimedPMsignals[t][ipm] = RandGen->Gaus(mean, sigma);
                          break;
                        }
                        case 2:
                        {
                          double k = PMs->getSPePHSshape(ipm);
                          double theta = PMs->at(ipm).AverageSignalPerPhotoelectron / k;
                          k *= TimedPMhits[t][ipm]; //for sum distribution
                          TimedPMsignals[t][ipm] = GammaRandomGen->getGamma(k, theta);
                          break;
                        }
                        case 3:
                        {
                          TimedPMsignals[t][ipm] = 0;
                          if (PMs->getSPePHShist(ipm))
                            for (int j=0; j<TimedPMhits[t][ipm]; j++)
                                TimedPMsignals[t][ipm] += PMs->getSPePHShist(ipm)->GetRandom();
                        }
                      }
                  }
                  else TimedPMsignals[t][ipm] = TimedPMhits[t][ipm];

                  //Electronic noise
                  if (PMs->isDoElNoise())
                      //TimedPMsignals[t][ipm] += RandGen->Gaus(0, PMs->getElNoiseSigma(ipm));
                      TimedPMsignals[t][ipm] += RandGen->Gaus(0, PMs->at(ipm).ElNoiseSigma);

                  //ADC sim
                  if (PMs->isDoADC())
                    {
                      if (TimedPMsignals[t][ipm]<0) TimedPMsignals[t][ipm]=0;
                      else
                        {
                          if (TimedPMsignals[t][ipm] > PMs->at(ipm).ADCmax) TimedPMsignals[t][ipm] = PMs->at(ipm).ADClevels;
                          else TimedPMsignals[t][ipm] = (int)( TimedPMsignals[t][ipm] / PMs->at(ipm).ADCstep );
                        }
                    }

                  PMsignals[ipm] += TimedPMsignals[t][ipm];
              } // end cycle by time bins
          }
      } //end cycle by PMs
  } //end time resolved case  -- total signals are calculated later!
  else
  { //not time resolved case
      for (int ipm=0; ipm<numPMs; ipm++)
      {
          if ( PMs->isDoPHS() )
          {
              switch ( PMs->at(ipm).SPePHSmode )
              {
                case 0:
                  PMsignals[ipm] = PMs->at(ipm).AverageSignalPerPhotoelectron * PMhitsTotal[ipm];
                  break;
                case 1:
                {
                  double mean = PMs->at(ipm).AverageSignalPerPhotoelectron * PMhitsTotal[ipm];
                  double sigma = TMath::Sqrt(PMhitsTotal[ipm]) * PMs->getSPePHSsigma(ipm);
                  PMsignals[ipm] = RandGen->Gaus(mean, sigma);
                  break;
                }
                case 2:
                {
                  double k = PMs->getSPePHSshape(ipm);
                  double theta = PMs->at(ipm).AverageSignalPerPhotoelectron / k;
                  k *= PMhitsTotal[ipm]; //for sum distribution
                  PMsignals[ipm] = GammaRandomGen->getGamma(k, theta);
                  break;
                }
                case 3:
                {
                  PMsignals[ipm] = 0;
                  if (PMs->getSPePHShist(ipm))
                    for (int j=0; j<PMhitsTotal[ipm]; j++)
                      PMsignals[ipm] += PMs->getSPePHShist(ipm)->GetRandom();
                }
              }
          }
          else PMsignals[ipm] = PMhitsTotal[ipm];

          //Electronic noise
          if (PMs->isDoElNoise())
              //PMsignals[ipm] += RandGen->Gaus(0, PMs->getElNoiseSigma(ipm));
              PMsignals[ipm] += RandGen->Gaus(0, PMs->at(ipm).ElNoiseSigma);

          //ADC sim
          if (PMs->isDoADC())
          {
              if (PMsignals[ipm]<0) PMsignals[ipm]=0;
              else
                {
                  if (PMsignals[ipm] > PMs->at(ipm).ADCmax) PMsignals[ipm] = PMs->at(ipm).ADClevels;
                  else PMsignals[ipm] = (int)( PMsignals[ipm] / PMs->at(ipm).ADCstep );
                }
          }
      } //end cycle by PMs
    }//end not-time resolved case
}

void OneEventClass::AddDarkCounts()
{
  for (int ipm = 0; ipm < numPMs; ipm++)
    {
      if (PMs->isSiPM(ipm)) //Add dark counts for SiPMs
        {
          const PMtypeClass* typ = PMs->getTypeForPM(ipm);
          const int&    pixelsX =  typ->PixelsX;
          const int&    pixelsY =  typ->PixelsY;
          const double& darkRate = typ->DarkCountRate; //in Hz
          //   qDebug() << "SiPM dark rate:" << darkRate << "Hz";

          const int     iTimeBins = SimSet->fTimeResolved ? SimSet->TimeBins : 1;
          const double  TimeInterval = ( iTimeBins == 1 ? PMs->getMeasurementTime() : (SimSet->TimeTo - SimSet->TimeFrom)/SimSet->TimeBins );
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
                      registerSiPMhit(ipm, iTime, iX, iY);
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
                              registerSiPMhit(ipm, iTime, iX, iY);
                        }
                }
            }
        }
    }
}

void OneEventClass::CollectStatistics(int WaveIndex, double time, double cosAngle, int Transitions)
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

int OneEventClass::TimeToBin(double time)
{
  if (time < SimSet->TimeFrom || time > SimSet->TimeTo) return -1;
  if (SimSet->TimeBins == 0) return -1;
  if (time == SimSet->TimeTo) return SimSet->TimeBins-1; //too large index protection
  return (time - SimSet->TimeFrom)/(SimSet->TimeTo - SimSet->TimeFrom) * SimSet->TimeBins;
}
