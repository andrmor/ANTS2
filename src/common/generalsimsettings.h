#ifndef GENERALSIMSETTINGS_H
#define GENERALSIMSETTINGS_H

#include <QJsonObject>
#include <QJsonArray>
#include <QString>

class GeneralSimSettings
{
public:
  GeneralSimSettings();
  ~GeneralSimSettings() {}      //if (SecScintThetaHist) delete SecScintThetaHist;

  bool   fTimeResolved;
  double TimeFrom;
  double TimeTo;
  int    TimeBins;

  bool   fWaveResolved;
  double WaveFrom;
  double WaveTo;
  double WaveStep;
  int    WaveNodes;

  bool   fAreaResolved;
  bool   fAngResolved;
  int    CosBins;

  bool   fLRFsim;               // true = use LRF for signal evaluation
  int    NumPhotsForLrfUnity;   // the total number of photons per event for unitary LRF
  double NumPhotElPerLrfUnity;  // the number of photoelectrons per unit value LRF

  double MinStep;
  double MaxStep;
  double dE;
  double MinEnergy;
  double Safety;
  int    TrackColorAdd;

  int    MaxNumTrans;
  bool   fTracksOnPMsOnly;
  bool   fQEaccelerator;
  bool   fLogsStat;             //generate logs and statistics of detected photons
  bool   bDoPhotonHistoryLog = false; //detailed photon history, activated by "photon" script!
  int    NumThreads;

  int    DetStatNumBins;        //number of bins in fetection statistics

  int    SecScintGenMode;
  //TH1D *SecScintThetaHist;

  bool   fBuildPhotonTracks;
  int    MaxNumberOfTracks;

  //utility
  QString ErrorString;

  bool readFromJson(QJsonObject &Json);
};


#endif // GENERALSIMSETTINGS_H
