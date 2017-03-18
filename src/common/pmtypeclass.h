#ifndef PMTYPECLASS_H
#define PMTYPECLASS_H

#include <QVector>
#include <QString>
#include <QJsonObject>

class TGeoVolume;

class PMtypeClass
{
public:
  PMtypeClass(QString Name = "");

  QString name;
  int MaterialIndex;
  int Shape;  //0-square, 1-round, 2-hexa
  double SizeX;
  double SizeY;
  double SizeZ;

  bool SiPM;

  int PixelsX;
  int PixelsY;
  double DarkCountRate;
  double RecoveryTime;

  QVector<double> PDE; //Quantun efficiency & Collection efficiency for PMTs; Photon Detection Efficiency for SiPMs
  QVector<double> PDE_lambda;
  QVector<double> PDEbinned;
  double effectivePDE;

  QVector<double> AngularSensitivity;
  QVector<double> AngularSensitivity_lambda;
  double n1; //refractive index of the medium where PM was positioned to measure the angular response
  QVector<double> AngularSensitivityCosRefracted; //Response vs cos of refracted beam, binned from 0 to 1 in CosBins bins

  QVector< QVector <double> > AreaSensitivity;
  double AreaStepX;
  double AreaStepY;

  TGeoVolume* tmpVol;  //tmp pointer, needed during GDML processing

  void clear();
  void writeToJson(QJsonObject &json);
  void readFromJson(QJsonObject &json);
};


#endif // PMTYPECLASS_H
