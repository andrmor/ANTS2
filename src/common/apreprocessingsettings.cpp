#include "apreprocessingsettings.h"

#include "apmhub.h"
#include "ajsontools.h"
#include "manifesthandling.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

APreprocessingSettings::APreprocessingSettings()
{
    ManifestItem = 0;
    clear();
}

APreprocessingSettings::~APreprocessingSettings()
{
    clear();
}

QString APreprocessingSettings::readFromJson(QJsonObject &json, APmHub* PMs, QString FileName)
// returns empty string if loaded OK
// returns "-" is preprocessing data not found in json
// otherwise returns error message
{
  clear();

  QJsonObject js1 = json["DetectorConfig"].toObject();
  if (!js1.contains("LoadExpDataConfig")) return "-";

  QJsonObject js = js1["LoadExpDataConfig"].toObject();
  if (js.isEmpty()) return "-";

  parseJson(js, "Preprocessing", fActive);
  if (js.contains("AddMulti"))
    {
      QJsonArray ar = js["AddMulti"].toArray();
      if (ar.size() != PMs->count())
        {
          QString err = "Mismatch in number of PMs!";
          qWarning() << err << "PMs:"<<PMs->count()<<"In json:"<<ar.size();
          return err;
        }
      for (int i=0; i<PMs->count(); i++)
        {
          QJsonArray el = ar[i].toArray();
          PMs->at(i).PreprocessingAdd = el[0].toDouble();
          PMs->at(i).PreprocessingMultiply = el[1].toDouble();
        }
    }
  else
    {
      for (int i=0; i<PMs->count(); i++)
        {
          PMs->at(i).PreprocessingAdd = 0;
          PMs->at(i).PreprocessingMultiply = 1.0;
          LoadEnAdd = 0;
          LoadEnMulti = 1.0;
        }
    }

  fHaveLoadedEnergy = false;
  QJsonObject enj = js["LoadedEnergy"].toObject();
  parseJson(enj, "Activated", fHaveLoadedEnergy);
  parseJson(enj, "Channel", EnergyChannel);
  parseJson(enj, "Add", LoadEnAdd);
  parseJson(enj, "Multi", LoadEnMulti);

  fHavePosition = false;
  QJsonObject posj = js["LoadedPosition"].toObject();
  parseJson(posj, "Activated", fHavePosition);
  parseJson(posj, "Channel", PositionXChannel);

  fHaveZPosition = false;
  QJsonObject zposj = js["LoadedZPosition"].toObject();
  parseJson(zposj, "Activated", fHaveZPosition);
  parseJson(zposj, "Channel", PositionZChannel);

  fIgnoreThresholds = false;
  QJsonObject ij = js["IgnoreThresholds"].toObject();
  parseJson(ij, "Activated", fIgnoreThresholds);
  parseJson(ij, "Min", ThresholdMin);
  parseJson(ij, "Max", ThresholdMax);

  fLimitNumber = false;
  int lim = -1;
  parseJson(js, "LoadFirst", lim);
  fLimitNumber = ( lim != -1 );
  if (fLimitNumber) LimitMax = lim;

  fManifest = false;
  if (js.contains("ManifestFile"))
    {
      ManifestFile = js["ManifestFile"].toString();
      if (ManifestFile != "")
      {
          //qDebug() << "Manifest file processing";
          fManifest = true;

          if (FileName != "")
            { //FileName == "" when loading preprocessing settings to GUI -> then skip processing
              ManifestProcessorClass mp(ManifestFile);
              QString ErrorMessage;
              ManifestItem = mp.makeManifestItem(FileName, ErrorMessage);
              if (!ManifestItem)
                {
                  fManifest = false;
                  return ErrorMessage;
                }
            }
      }
    }
  return "";
}

void APreprocessingSettings::writeToJson(QJsonObject &json, APmHub *PMs)
{
    json["Preprocessing"] = fActive;
      QJsonArray ar;
      for (int ipm=0; ipm<PMs->count(); ipm++)
      {
        QJsonArray el;
        el.append(PMs->at(ipm).PreprocessingAdd);
        el.append(PMs->at(ipm).PreprocessingMultiply);
        ar.append(el);
      }
    json["AddMulti"] = ar;
        QJsonObject enj;
        enj["Activated"] = fHaveLoadedEnergy;
        enj["Channel"] = EnergyChannel;
        enj["Add"] = LoadEnAdd;
        enj["Multi"] = LoadEnMulti;
    json["LoadedEnergy"] = enj;
      QJsonObject posj;
      posj["Activated"] = fHavePosition;
      posj["Channel"] = PositionXChannel;
    json["LoadedPosition"] = posj;
      QJsonObject zposj;
      zposj["Activated"] = fHaveZPosition;
      zposj["Channel"] = PositionZChannel;
    json["LoadedZPosition"] = zposj;
      QJsonObject ij;
      ij["Activated"] = fIgnoreThresholds;
      ij["Min"] = ThresholdMin;
      ij["Max"] = ThresholdMax;
    json["IgnoreThresholds"] = ij;
    json["LoadFirst"] = ( fLimitNumber ? LimitMax : -1 );
    json["ManifestFile"] = ManifestFile;
}

void APreprocessingSettings::clear()
{
  fActive = false;
  fHaveLoadedEnergy = false;
  LoadEnAdd = 0;
  LoadEnMulti = 1.0;  
  fHavePosition = false;
  fHaveZPosition = false;
  fIgnoreThresholds = false;
  fLimitNumber = false;
  fManifest = false;
  if (ManifestItem)
    {
      delete ManifestItem;
      ManifestItem = NULL;
    }
}
