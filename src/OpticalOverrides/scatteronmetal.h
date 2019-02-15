#ifndef SCATTERONMETAL_H
#define SCATTERONMETAL_H

#include "aopticaloverride.h"

class APhoton;
class TRandom2;
class AMaterialParticleCollection;
class QJsonObject;
class ATracerStateful;

class ScatterOnMetal : public AOpticalOverride
{
public:    
  ScatterOnMetal(AMaterialParticleCollection* MatCollection, int MatFrom, int MatTo)
    : AOpticalOverride(MatCollection, MatFrom, MatTo) {}
  virtual ~ScatterOnMetal() {}

  virtual OpticalOverrideResultEnum calculate(ATracerStateful& Resources, APhoton* Photon, const double* NormalVector) override; //unitary vectors! iWave = -1 if not wavelength-resolved

  virtual const QString getType() const override {return "DielectricToMetal";}
  virtual const QString getAbbreviation() const override {return "Met";}
  virtual const QString getReportLine() const override;
  virtual const QString getLongReportLine() const override;

  // save/load config is not used for this type!
  virtual void writeToJson(QJsonObject &json) const override;
  virtual bool readFromJson(const QJsonObject &json) override;

#ifdef GUI
  virtual QWidget* getEditWidget(QWidget* caller, GraphWindowClass* GraphWindow) override;
#endif

  virtual const QString checkOverrideData() override;

  //data
  double RealN = 1.07;
  double ImaginaryN = 0.6;

private:
  double calculateReflectivity(double CosTheta, double RealN, double ImaginaryN, int waveIndex);

};

#endif // SCATTERONMETAL_H
