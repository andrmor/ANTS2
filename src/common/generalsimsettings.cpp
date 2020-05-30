#include "generalsimsettings.h"
#include "ajsontools.h"

#include <QDebug>

bool GeneralSimSettings::readFromJson(const QJsonObject & Json)
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

  //Accelerators options
  QJsonObject acjson = json["AcceleratorConfig"].toObject();
  MaxNumTrans = acjson["MaxNumTransitions"].toInt();
  fQEaccelerator = acjson["CheckBeforeTrack"].toBool();  

  DetStatNumBins = json["DetStatNumBins"].toInt(100);

  //track building options
  QJsonObject tbojs;
    parseJson(json, "TrackBuildingOptions", tbojs);
  TrackBuildOptions.readFromJson(tbojs);

  QJsonObject lsjs;
    parseJson(json, "LogStatOptions", lsjs);
  LogsStatOptions.readFromJson(lsjs);

  //Geant4 simulation options
  QJsonObject g4js;
    parseJson(json, "Geant4SimulationSettings", g4js);
  G4SimSet.readFromJson(g4js);

  ExitParticleSettings.SaveParticles = false;
  {
      QJsonObject js;
        bool bOK = parseJson(json, "ExitParticleSettings", js);
      if (bOK) ExitParticleSettings.readFromJson(js);
  }

  //Secondary scint options
  //QJsonObject scjson = json["SecScintConfig"].toObject();

  return true;
}

bool GeneralSimSettings::writeToJson(QJsonObject & Json) const
{
    QJsonObject json;

    //wavelength-resolved
    {
        QJsonObject js;
            js["WaveResolved"] = fWaveResolved;
            js["WaveFrom"]     = WaveFrom;
            js["WaveTo"]       = WaveTo;
            js["WaveStep"]     = WaveStep;
            js["WaveNodes"]    = WaveNodes;
        json["WaveConfig"] = js;
    }

    //time-resolved
    {
        QJsonObject js;
            js["TimeResolved"] = fTimeResolved;
            js["TimeFrom"]     = TimeFrom;
            js["TimeTo"]       = TimeTo;
            js["TimeBins"]     = TimeBins;
        json["TimeConfig"] = js;
    }

    //angle-resolved
    {
        QJsonObject js;
            js["AngResolved"] = fAngResolved;
            js["NumBins"]     = CosBins;
        json["AngleConfig"] = js;
    }

    //area-resolved
    {
        QJsonObject js;
            js["AreaResolved"] = fAreaResolved;
        json["AreaConfig"] = js;
    }

    //LRF-based simulations
    {
        QJsonObject js;
            js["UseLRFs"] = fLRFsim;
            js["NumPhotsLRFunity"]  = NumPhotsForLrfUnity;
            js["NumPhotElLRFunity"] = NumPhotElPerLrfUnity;
        json["LrfBasedSim"] = js;
    }

    //tracking options
    {
        QJsonObject js;
            js["MinStep"]           = MinStep;
            js["MaxStep"]           = MaxStep;
            js["dE"]                = dE;
            js["MinEnergy"]         = MinEnergy;
            js["MinEnergyNeutrons"] = MinEnergyNeutrons;
            js["Safety"]            = Safety;
        json["TrackingConfig"] = js;
    }

    //accelerators options
    {
        QJsonObject js;
            js["MaxNumTransitions"] = MaxNumTrans;
            js["CheckBeforeTrack"]  = fQEaccelerator;
        json["AcceleratorConfig"] = js;
    }

    //number of bins in detection statistics
    json["DetStatNumBins"] = DetStatNumBins;  // !*! why not in Logs?

    //track building options
    {
        QJsonObject js;
            TrackBuildOptions.writeToJson(js);
        json["TrackBuildingOptions"] = js;
    }

    //log settings
    {
        QJsonObject js;
            LogsStatOptions.writeToJson(js);
        json["LogStatOptions"] = js;
    }

    //geant4 simulation options
    {
        QJsonObject js;
            G4SimSet.writeToJson(js);
        json["Geant4SimulationSettings"] = js;
    }

    //exit particles to file
    {
        QJsonObject js;
            ExitParticleSettings.writeToJson(js);
        json["ExitParticleSettings"] = js;
    }

    Json["GeneralSimConfig"] = json;

    return true;
}
