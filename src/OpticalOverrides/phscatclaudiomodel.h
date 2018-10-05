#ifndef PHSCATCLAUDIOMODEL_H
#define PHSCATCLAUDIOMODEL_H

#include "aopticaloverride.h"

class TRandom2;
class APhoton;
class QJsonObject;
class ATracerStateful;

enum HeightDistrEnum {empirical, gaussian, exponential};
enum SlopeDistrEnum {trowbridgereitz, cooktorrance, bivariatecauchy};

class PhScatClaudioModel : public AOpticalOverride //abstract class!
{
public:  
  // constructor
  PhScatClaudioModel(AMaterialParticleCollection* MatCollection, int MatFrom, int MatTo)
    : AOpticalOverride(MatCollection, MatFrom, MatTo) {}

  //virtual OpticalOverrideResultEnum calculate(ATracerStateful& Resources, APhoton* Photon, const double* NormalVector) = 0;
    //unitary vectors! iWave - photon wave index, -1 if no wave-resolved

  //virtual const QString getType() const override = 0;
  virtual const QString getAbbreviation() const override {return "Clau";}
  virtual const QString getReportLine() const override;

  // save/load config
  virtual void writeToJson(QJsonObject &json) const override;
  virtual bool readFromJson(const QJsonObject &json) override;

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
  virtual OpticalOverrideResultEnum calculate(ATracerStateful& Resources, APhoton* Photon, const double* NormalVector) override;
  virtual const QString getType() const override {return "Claudio_Model_V2";}
  virtual const QString getReportLine() const override;

protected:
  virtual double GnFunc(double cost) override;
  virtual double SlopeAngle(double random_num) override;
};

class PhScatClaudioModelV2d1 : public PhScatClaudioModelV2
{
public:
  PhScatClaudioModelV2d1(AMaterialParticleCollection* MatCollection, int MatFrom, int MatTo)
    : PhScatClaudioModelV2(MatCollection, MatFrom, MatTo) {}
  virtual OpticalOverrideResultEnum calculate(ATracerStateful& Resources, APhoton* Photon, const double* NormalVector) override;
  virtual const QString getType() const override {return "Claudio_Model_V2d1";}
  virtual const QString getReportLine() const override;
};

class PhScatClaudioModelV2d2 : public PhScatClaudioModelV2
{
public:
  PhScatClaudioModelV2d2(AMaterialParticleCollection* MatCollection, int MatFrom, int MatTo)
    : PhScatClaudioModelV2(MatCollection, MatFrom, MatTo) {}
  virtual OpticalOverrideResultEnum calculate(ATracerStateful& Resources, APhoton* Photon, const double* NormalVector) override;
  virtual const QString getType() const override {return "ClaudioModel";}
  virtual const QString getReportLine() const override;
};

#endif // PHSCATCLAUDIOMODEL_H
