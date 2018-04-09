#ifndef ONEEVENTCLASS_H
#define ONEEVENTCLASS_H

#include <QVector>
#include <QBitArray>

class APmHub;
class TRandom2;
class ASimulationStatistics;
class GeneralSimSettings;
class AGammaRandomGenerator;

class OneEventClass
{
public:
  OneEventClass(APmHub* Pms, TRandom2* randGen, ASimulationStatistics* simStat);
  ~OneEventClass();

  //Pm hits info
  QVector<QVector<int> > TimedPMhits;      // PM hits [time][pm]
  QVector<QVector<float> > TimedPMsignals; // convrted to signal  [timeBin][PMnumber]
  QVector<int> PMhits; // PM hits [pm]
  QVector<float> PMsignals; // converted to signal [pm]
  QVector< QBitArray > SiPMpixels; //[PM#] [time] [pixY] [pixX]

  ASimulationStatistics* SimStat;

  //configure
  void configure(const GeneralSimSettings *SimSet);

  //hits processing
  void clearHits();
  bool isHitsEmpty();
  bool CheckPMThit(int ipm, double time, int WaveIndex, double x, double y, double cosAngle, int Transitions, double rnd);
  bool CheckSiPMhit(int ipm, double time, int WaveIndex, double x, double y, double cosAngle, int Transitions, double rnd);
  void HitsToSignal();  //convert hits of PMs to signal using "electronics" settings
  void addHits(int ipm, int hits) {PMhits[ipm] += hits;}
  void addTimedHits(int itime, int ipm, int hits) {TimedPMhits[itime][ipm] += hits;}
  void addSignals(int ipm, float signal) {PMsignals[ipm] += signal;}  //only used in LRF-based sim
  void CollectStatistics(int WaveIndex, double time, double cosAngle, int Transitions);
  int  TimeToBin(double time);

private:
  APmHub* PMs;
  TRandom2* RandGen;  
  AGammaRandomGenerator* GammaRandomGen; // not to use one in pms module - multithread conflict?

  //settings
  const GeneralSimSettings *SimSet;
  int numPMs;

  void registerSiPMhit(int ipm, int iTime, int binX, int binY, int numHits=1); //numHits != 1 is used only for the simplistic model of microcell cross-talk!
  void AddDarkCounts();
  void convertHitsToSignal(const QVector<int> &pmHits, QVector<float> &pmSignals);
};

#endif // ONEEVENTCLASS_H
