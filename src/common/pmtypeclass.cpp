#include "pmtypeclass.h"
#include "ajsontools.h"
//#include "functions.h"

PMtypeClass::PMtypeClass(QString Name)
{
  //default properties
  name = Name;
  SiPM = false;
  MaterialIndex = 0;
  Shape = 1;
  SizeX = 25.0;
  SizeY = 25.0;
  SizeZ = 0.01;
  PixelsX = 50;
  PixelsY = 50;
  DarkCountRate = 1e7;
  RecoveryTime = 50;
  effectivePDE = 1;
  n1 = 1;
  AreaStepX = 1.0; AreaStepY = 1.0;
  //name has to be unicque, so it has to initialized outside the class
}

void PMtypeClass::clear()
{
  PDE_lambda.resize(0);
  PDE.resize(0);
  AngularSensitivity_lambda.resize(0);
  AngularSensitivity.resize(0);
  AreaSensitivity.resize(0);
  n1=1;
}

void PMtypeClass::writeToJson(QJsonObject &json)
{
  QJsonObject genj;
  genj["Name"] = name;
  genj["MaterialIndex"] = MaterialIndex;
  genj["Shape"] = Shape;
  genj["SizeX"] = SizeX;
  genj["SizeY"] = SizeY;
  genj["SizeZ"] = SizeZ;
  genj["SiPM"] = SiPM;
  json["General"] = genj;

  //if (SiPM)
    {
      QJsonObject sij;
      sij["PixelsX"] = PixelsX;
      sij["PixelsY"] = PixelsY;
      sij["DarkCountRate"] = DarkCountRate;
      sij["RecoveryTime"] = RecoveryTime;
      json["SiPMproperties"] = sij;
    }

  QJsonObject pdej;
  pdej["PDE"] = effectivePDE;
  //if (PDE_lambda.size()>0)
    {
      QJsonArray ar;
      writeTwoQVectorsToJArray(PDE_lambda, PDE, ar);
      pdej["PDEwave"] = ar;
    }
  json["PDEproperties"] = pdej;

  //if (AngularSensitivity_lambda.size()>0)
    {
      QJsonObject angj;
      angj["RefrIndexMeasure"] = n1;
      QJsonArray ar1;
      writeTwoQVectorsToJArray(AngularSensitivity_lambda, AngularSensitivity, ar1);
      angj["ResponseVsAngle"] = ar1;
      json["AngularResponse"] = angj;
    }

  //if (AreaSensitivity.size()>0)
    {
      QJsonObject areaj;
      areaj["AreaStepX"] = AreaStepX;
      areaj["AreaStepY"] = AreaStepY;
      QJsonArray arar;
      write2DQVectorToJArray(AreaSensitivity, arar);
      areaj["ResponseVsXY"] = arar;
      json["AreaResponse"] = areaj;
    }
}

void PMtypeClass::readFromJson(QJsonObject &json)
{
  clear();
  QJsonObject genj = json["General"].toObject();
  parseJson(genj, "Name", name);
  parseJson(genj, "MaterialIndex", MaterialIndex);
  parseJson(genj, "Shape", Shape);
  parseJson(genj, "SizeX", SizeX);
  parseJson(genj, "SizeY", SizeY);
  SizeZ = 0.01; //compatibility
  parseJson(genj, "SizeZ", SizeZ);
  parseJson(genj, "SiPM", SiPM);

  QJsonObject sij = json["SiPMproperties"].toObject();
  parseJson(sij, "PixelsX", PixelsX);
  parseJson(sij, "PixelsY", PixelsY);
  parseJson(sij, "DarkCountRate", DarkCountRate);
  parseJson(sij, "RecoveryTime", RecoveryTime);

  QJsonObject pdej = json["PDEproperties"].toObject();
  parseJson(pdej, "PDE", effectivePDE);
  if (pdej.contains("PDEwave"))
    {
      QJsonArray ar = pdej["PDEwave"].toArray();
      readTwoQVectorsFromJArray(ar, PDE_lambda, PDE);
    }

  if (json.contains("AngularResponse"))
    {
      QJsonObject angj = json["AngularResponse"].toObject();
      parseJson(angj, "RefrIndexMeasure", n1);
      QJsonArray ar =  angj["ResponseVsAngle"].toArray();
      readTwoQVectorsFromJArray(ar, AngularSensitivity_lambda, AngularSensitivity);
    }

  if (json.contains("AreaResponse"))
    {
      QJsonObject areaj = json["AreaResponse"].toObject();
      parseJson(areaj, "AreaStepX", AreaStepX);
      parseJson(areaj, "AreaStepY", AreaStepY);
      QJsonArray ar = areaj["ResponseVsXY"].toArray();
      read2DQVectorFromJArray(ar, AreaSensitivity);
    }
}
