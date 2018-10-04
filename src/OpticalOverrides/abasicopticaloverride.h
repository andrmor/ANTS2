#ifndef ABASICOPTICALOVERRIDE_H
#define ABASICOPTICALOVERRIDE_H

#include "aopticaloverride.h"

#include <QString>

class AMaterialParticleCollection;
class ATracerStateful;
class APhoton;
class QJsonObject;
class GraphWindowClass;

class ABasicOpticalOverride : public AOpticalOverride
{
public:
  ABasicOpticalOverride(AMaterialParticleCollection* MatCollection, int MatFrom, int MatTo);
  virtual ~ABasicOpticalOverride() {}

  virtual OpticalOverrideResultEnum calculate(ATracerStateful& Resources, APhoton* Photon, const double* NormalVector) override; //unitary vectors! iWave = -1 if not wavelength-resolved

  virtual const QString getType() const override {return "Simplistic_model";}
  virtual const QString getAbbreviation() const override {return "Simp";}
  virtual const QString getReportLine() const override;

  // save/load config is not used for this type!
  virtual void writeToJson(QJsonObject &json) const override;
  virtual bool readFromJson(const QJsonObject &json) override;

#ifdef GUI
  virtual QWidget* getEditWidget(QWidget *caller, GraphWindowClass* GraphWindow) override;
#endif
  virtual const QString checkOverrideData() override;

  //--parameters--
  double probLoss = 0; //probability of absorption
  double probRef = 0;  //probability of specular reflection
  double probDiff = 0; //probability of scattering
  int    scatterModel = 1; //0 - 4Pi, 1 - 2Pi back, 2 - 2Pi forward
};

#endif // ABASICOPTICALOVERRIDE_H
