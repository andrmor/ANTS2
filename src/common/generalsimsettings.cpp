#include "generalsimsettings.h"
#include "ajsontools.h"

#include <QDebug>

GeneralSimSettings::GeneralSimSettings()
{
  SecScintGenMode = 0;
  ErrorString = "";
  MaxNumTrans = 100;
  fQEaccelerator = false;
  fTracksOnPMsOnly = false;
  fLogsStat = false;
  TrackColorAdd = 0;
  //SecScintThetaHist = 0;
  fTimeResolved = false;
  TimeBins = 1;
  TimeFrom = 0;
  TimeTo = TimeBins;
  WaveStep = 1.0;
  bDoPhotonHistoryLog = false;
  MinEnergyNeutrons = 0.01; //in meV
}

bool GeneralSimSettings::readFromJson(const QJsonObject &Json)
{
  if (!Json.contains("GeneralSimConfig"))
    {
      ErrorString = "Json sent to simulator does not contain general sim config data!";
      return false;
    }
  ErrorString.clear();

  QJsonObject json = Json["GeneralSimConfig"].toObject();
  //reading wavelength options
  QJsonObject wjson = json["WaveConfig"].toObject();
  fWaveResolved = wjson["WaveResolved"].toBool();
  WaveFrom = wjson["WaveFrom"].toDouble();
  WaveTo = wjson["WaveTo"].toDouble();
  WaveStep = wjson["WaveStep"].toDouble();
  WaveNodes = wjson["WaveNodes"].toInt();

  //Time options
  QJsonObject tjson = json["TimeConfig"].toObject();
  fTimeResolved = tjson["TimeResolved"].toBool();
  TimeFrom = tjson["TimeFrom"].toDouble();
  TimeTo = tjson["TimeTo"].toDouble();
  TimeBins = tjson["TimeBins"].toInt();
  //TimeOneMeasurement = tjson["TimeWindow"].toDouble();

  //Angular options
  QJsonObject ajson = json["AngleConfig"].toObject();
  fAngResolved = ajson["AngResolved"].toBool();
  CosBins = ajson["NumBins"].toInt();

  //Area options
  QJsonObject arjson = json["AreaConfig"].toObject();
  fAreaResolved = arjson["AreaResolved"].toBool();

  //LRF-based sim options
  fLRFsim = false;
  NumPhotsForLrfUnity = 20000;
  NumPhotElPerLrfUnity = 1.0;
  if (json.contains("LrfBasedSim"))
  {
      QJsonObject lrfjson = json["LrfBasedSim"].toObject();
      parseJson(lrfjson, "UseLRFs", fLRFsim);
      parseJson(lrfjson, "NumPhotsLRFunity", NumPhotsForLrfUnity);
      parseJson(lrfjson, "NumPhotElLRFunity", NumPhotElPerLrfUnity);
  }
    //qDebug() << "general sim options load - LRF based sim (on/phots/ph.el.):"<<fLRFsim<<NumPhotsForLrfUnity<<NumPhotElPerLrfUnity;

  //Tracking options
  QJsonObject trjson = json["TrackingConfig"].toObject();
  MinStep = trjson["MinStep"].toDouble();
  MaxStep = trjson["MaxStep"].toDouble();
  dE = trjson["dE"].toDouble();
  MinEnergy = trjson["MinEnergy"].toDouble();
  MinEnergyNeutrons = trjson["MinEnergyNeutrons"].toDouble();
  Safety = trjson["Safety"].toDouble();
  TrackColorAdd = trjson["TrackColorAdd"].toInt();

  //Accelerators options
  QJsonObject acjson = json["AcceleratorConfig"].toObject();
  MaxNumTrans = acjson["MaxNumTransitions"].toInt();
  fQEaccelerator = acjson["CheckBeforeTrack"].toBool();
  fTracksOnPMsOnly = acjson["OnlyTracksOnPMs"].toBool();
  fLogsStat = acjson["LogsStatistics"].toBool();
  NumThreads = acjson["NumberThreads"].toInt();

  //Photon tracks
  fBuildPhotonTracks = json["BuildPhotonTracks"].toBool();
  MaxNumberOfTracks = 1000;
  MaxNumberOfTracks = json["MaxNumberOfTracks"].toInt();

  DetStatNumBins = json["DetStatNumBins"].toInt(100);

  //track building options
  QJsonObject tbojs;
    parseJson(json, "TrackBuildingOptions", tbojs);
  TrackBuildOptions.readFromJson(tbojs);

  //Secondary scint options
  QJsonObject scjson = json["SecScintConfig"].toObject();
  SecScintGenMode = scjson["Type"].toInt();  //0-4Pi, 1-2PiUp, 2-2Pidown, // obsolete: 3-custom
  if (SecScintGenMode == 3) SecScintGenMode = 0;
  /*
    if (SecScintGenMode == 3)
      {
        if (!scjson.contains("CustomDistrib"))
          {
             ErrorString = "Config file does not contain secondary scint generation histogram!";
             return false;
          }
        QJsonArray ar = scjson["CustomDistrib"].toArray();
        int size = ar.size();
        double* xx = new double[size];
        double* yy = new double[size];
        for (int i=0; i<size; i++)
            {
              xx[i] = ar[i].toArray()[0].toDouble();
              yy[i] = ar[i].toArray()[1].toDouble();
            }
        SecScintThetaHist = new TH1D("hSecThet","", size-1, xx);
        for (int i = 1; i<size+1; i++)  SecScintThetaHist->SetBinContent(i, yy[i-1]);
        delete[] xx;
        delete[] yy;
      }
    */

  return true;
}

