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

#endif // ALRF_SI_H
