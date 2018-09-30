#ifndef SCATTERONMETAL_H
#define SCATTERONMETAL_H

#include "aopticaloverride.h"

class APhoton;
class TRandom2;
class AMaterialParticleCollection;
class QJsonObject;

class ScatterOnMetal : public AOpticalOverride
{
public:    
  ScatterOnMetal(AMaterialParticleCollection* MatCollection, int MatFrom, int MatTo)
    : AOpticalOverride(MatCollection, MatFrom, MatTo) {}
  virtual ~ScatterOnMetal() {}

  virtual OpticalOverrideResultEnum calculate(TRandom2* RandGen, APhoton* Photon, const double* NormalVector); //unitary vectors! iWave = -1 if not wavelength-resolved

  virtual void printConfiguration(int iWave);
  virtual QString getType() const {return "DielectricToMetal";}
  virtual QString getReportLine();

  // save/load config is not used for this type!
  virtual void writeToJson(QJsonObject &json);
  virtual bool readFromJson(QJsonObject &json);

#ifdef GUI
  virtual QWidget* getEditWidget(QWidget* caller, GraphWindowClass* GraphWindow) override;
#endif
  virtual const QString checkOverrideData() const override;

  //data
  double RealN = 1.07;
  double ImaginaryN = 0.6;
private:
  double calculateReflectivity(double CosTheta, double RealN, double ImaginaryN, int waveIndex);

};

#endif // SCATTERONMETAL_H
