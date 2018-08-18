#ifndef GENERALSIMSETTINGS_H
#define GENERALSIMSETTINGS_H

#include "atrackbuildoptions.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QString>

class GeneralSimSettings
{
public:
  //~GeneralSimSettings() {}      //if (SecScintThetaHist) delete SecScintThetaHist;

  bool   fTimeResolved = false;
  double TimeFrom = 0;
  double TimeTo = 1;
  int    TimeBins = 1;

  bool   fWaveResolved = false;
  double WaveFrom = 200.0;
  double WaveTo = 800.0;
  double WaveStep = 1.0;
  int    WaveNodes = 601;

  bool   fAreaResolved = false;
  bool   fAngResolved = false;
  int    CosBins = 1000;

  bool   fLRFsim = false;               // true = use LRF for signal evaluation
  int    NumPhotsForLrfUnity = 1;    // the total number of photons per event for unitary LRF
  double NumPhotElPerLrfUnity = 1.0;  // the number of photoelectrons per unit value LRF

  double MinStep;
  double MaxStep;
  double dE;
  double MinEnergy;
  double MinEnergyNeutrons = 0.01; //in meV
  double Safety;

  int    MaxNumTrans = 500;
  //bool   fTracksOnPMsOnly;
  bool   fQEaccelerator = false;
  bool   fLogsStat = false;             //generate logs and statistics of detected photons
  bool   bDoPhotonHistoryLog = false; //detailed photon history, activated by "photon" script!
  int    NumThreads;

  int    DetStatNumBins;        //number of bins in fetection statistics

  int    SecScintGenMode = 0;
  //TH1D *SecScintThetaHist;

  //bool   fBuildPhotonTracks; //moved to TrackBuildOptions
  int    MaxNumberOfTracks;

  ATrackBuildOptions TrackBuildOptions;

  //utility
  QString ErrorString;

  bool readFromJson(const QJsonObject &Json);
};


#endif // GENERALSIMSETTINGS_H
