#ifndef ALRF_SI_H
#define ALRF_SI_H

#include "ascriptinterface.h"

#include <QString>
#include <QVariant>

class AConfiguration;
class EventsDataClass;
class SensorLRFs;

class ALrf_SI : public AScriptInterface
{
  Q_OBJECT

public:
  ALrf_SI(AConfiguration* Config, EventsDataClass* EventsDataHub);

  bool IsMultithreadCapable() const override {return true;}

public slots:
  QString Make();
  double GetLRF(int ipm, double x, double y, double z);
  double GetLRFerror(int ipm, double x, double y, double z);

  QVariant GetAllLRFs(double x, double y, double z);

  //iterations
  int CountIterations();
  int GetCurrent();
  void SetCurrent(int iterIndex);
  void SetCurrentName(QString name);
  void DeleteCurrent();
  QString Save(QString fileName);
  int Load(QString fileName);

private:
  AConfiguration* Config;
  EventsDataClass* EventsDataHub;
  SensorLRFs* SensLRF; //alias

  bool getValidIteration(int &iterIndex);
};

namespace LRF {class ARepository;}
class DetectorClass;

#include <QList>

class ALrfRaim_SI : public AScriptInterface
{
  Q_OBJECT

public:
  ALrfRaim_SI(DetectorClass* Detector, EventsDataClass* EventsDataHub);

public slots:
  QString Make(QString name, QVariantList instructions, bool use_scan_data, bool fit_error, bool scale_by_energy);
  QString Make(int recipe_id, bool use_scan_data, bool fit_error, bool scale_by_energy);
  double GetLRF(int ipm, double x, double y, double z);

  QList<int> GetListOfRecipes();
  int GetCurrentRecipeId();
  int GetCurrentVersionId();
  bool SetCurrentRecipeId(int rid);
  bool SetCurrentVersionId(int vid, int rid = -1);
  void DeleteCurrentRecipe();
  void DeleteCurrentRecipeVersion();

  bool SaveRepository(QString fileName);
  bool SaveCurrentRecipe(QString fileName);
  bool SaveCurrentVersion(QString fileName);
  QList<int> Load(QString fileName); //Returns list of loaded recipes

private:
  DetectorClass* Detector;
  EventsDataClass* EventsDataHub;
  LRF::ARepository *repo; //alias
};

#endif // ALRF_SI_H
