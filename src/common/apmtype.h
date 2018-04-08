#ifndef APMTYPE_H
#define APMTYPE_H

#include <QVector>
#include <QString>

class TGeoVolume;
class QJsonObject;

class APmType
{
public:
  APmType(const QString &Name = "") : Name(Name) {}

  QString Name;
  int     MaterialIndex = 0;
  int     Shape = 1;  //0-square, 1-round, 2-hexa
  double  SizeX = 25.0;
  double  SizeY = 25.0;
  double  SizeZ = 0.01;

  bool    SiPM = false;

  int     PixelsX = 50;
  int     PixelsY = 50;
  double  DarkCountRate = 3.0e5;
  double  RecoveryTime = 50.0;

  QVector<double> PDE;
  QVector<double> PDE_lambda;
  QVector<double> PDEbinned;
  double  EffectivePDE = 1.0;

  QVector<double> AngularSensitivity;
  QVector<double> AngularSensitivity_lambda;
  double  AngularN1 = 1.0;                        // refractive index of the medium where PM was positioned to measure the angular response
  QVector<double> AngularSensitivityCosRefracted; // response vs cos of refracted beam, binned from 0 to 1 in CosBins bins

  QVector<QVector<double> > AreaSensitivity;
  double  AreaStepX = 1.0;
  double  AreaStepY = 1.0;

  TGeoVolume* tmpVol;                             // tmp pointer, needed during GDML processing

  void    clear();
  void    writeToJson(QJsonObject &json) const;
  void    readFromJson(const QJsonObject &json);
};


#endif // APMTYPE_H
