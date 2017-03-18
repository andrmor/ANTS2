#ifndef APREPROCESSINGSETTINGS_H
#define APREPROCESSINGSETTINGS_H

#include <QString>

class ManifestItemBaseClass;
class QJsonObject;
class pms;

class APreprocessingSettings
{
public:
  bool fActive;

  bool fHaveLoadedEnergy;
  int EnergyChannel;
  float LoadEnAdd, LoadEnMulti;

  bool fHavePosition;
  int PositionXChannel; //PositionYChannel = PositionXChannel + 1

  bool fHaveZPosition;
  int PositionZChannel;

  bool fIgnoreThresholds;
  float ThresholdMin, ThresholdMax;

  bool fLimitNumber;
  int LimitMax;

  bool fManifest;
  QString ManifestFile;

  ManifestItemBaseClass* ManifestItem;

  QString readFromJson(QJsonObject &json, pms *PMs, QString FileName);
  void writeToJson(QJsonObject &json, pms *PMs);

  void clear();

  APreprocessingSettings();
  ~APreprocessingSettings();
};


#endif // APREPROCESSINGSETTINGS_H
