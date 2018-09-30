#ifndef PHSCATCLAUDIOMODEL_H
#define PHSCATCLAUDIOMODEL_H

#include "aopticaloverride.h"

class TRandom2;
class APhoton;
class QJsonObject;

enum HeightDistrEnum {empirical, gaussian, exponential};
enum SlopeDistrEnum {trowbridgereitz, cooktorrance, bivariatecauchy};

class PhScatClaudioModel : public AOpticalOverride //abstract class!
{
public:  
  // constructor
  PhScatClaudioModel(AMaterialParticleCollection* MatCollection, int MatFrom, int MatTo)
    : AOpticalOverride(MatCollection, MatFrom, MatTo) {}
  // main method:
  virtual OpticalOverrideResultEnum calculate(TRandom2* RandGen, APhoton* Photon, const double* NormalVector) = 0;
    //unitary vectors! iWave - photon wave index, -1 if no wave-resolved

  virtual void printConfiguration(int iWave);
  virtual QString getType() const = 0;
  virtual QString getReportLine();

  // save/load config
  virtual void writeToJson(QJsonObject &json);
  virtual bool readFromJson(QJsonObject &json);

#ifdef GUI
  virtual QWidget* getEditWidget(QWidget* caller, GraphWindowClass* GraphWindow) override;
#endif
  virtual const QString checkOverrideData() override;

  // interface properties
  double sigma_alpha = 0.18;                          // r.m.s. (or similar) of the slope distribution
  double sigma_h = 1.0;                               // r.m.s (or similar) of the height distribution
  double albedo = 0.97;                               // multiple diffuse albedo
  HeightDistrEnum HeightDistribution = empirical;     // model for heights distribution
  SlopeDistrEnum SlopeDistribution = trowbridgereitz; // model for distribution of slopes

protected:
  double SpikeIntensity(int iWave, double costi);
  double Fresnel(double E1_perp, double E1_parl, double cosinc, double Rinda1, double Rinda2);

  virtual double GnFunc(double cost) = 0; // shadowing functions
  virtual double SlopeAngle(double random_num) = 0;

};

class PhScatClaudioModelV2 : public PhScatClaudioModel
{
public:
  PhScatClaudioModelV2(AMaterialParticleCollection* MatCollection, int MatFrom, int MatTo)
    : PhScatClaudioModel(MatCollection, MatFrom, MatTo) {}
  virtual OpticalOverrideResultEnum calculate(TRandom2* RandGen, APhoton* Photon, const double* NormalVector);
  virtual QString getType() const {return "Claudio_Model_V2";}
  virtual QString getReportLine();

protected:
  virtual double GnFunc(double cost);
  virtual double SlopeAngle(double random_num);
};

class PhScatClaudioModelV2d1 : public PhScatClaudioModelV2
{
public:
  PhScatClaudioModelV2d1(AMaterialParticleCollection* MatCollection, int MatFrom, int MatTo)
    : PhScatClaudioModelV2(MatCollection, MatFrom, MatTo) {}
  virtual OpticalOverrideResultEnum calculate(TRandom2* RandGen, APhoton* Photon, const double* NormalVector);
  virtual QString getType() const {return "Claudio_Model_V2d1";}
  virtual QString getReportLine();
};

class PhScatClaudioModelV2d2 : public PhScatClaudioModelV2
{
public:
  PhScatClaudioModelV2d2(AMaterialParticleCollection* MatCollection, int MatFrom, int MatTo)
    : PhScatClaudioModelV2(MatCollection, MatFrom, MatTo) {}
  virtual OpticalOverrideResultEnum calculate(TRandom2* RandGen, APhoton* Photon, const double* NormalVector);
  virtual QString getType() const {return "Claudio_Model_V2d2";}
  virtual QString getReportLine();
};

#endif // PHSCATCLAUDIOMODEL_H
