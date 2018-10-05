#ifndef FSNPOPTICALOVERRIDE_H
#define FSNPOPTICALOVERRIDE_H

#include "aopticaloverride.h"

#include <QString>

class AMaterialParticleCollection;
class ATracerStateful;
class APhoton;
class QJsonObject;
class GraphWindowClass;

class FSNPOpticalOverride : public AOpticalOverride
{
public:
  FSNPOpticalOverride(AMaterialParticleCollection* MatCollection, int MatFrom, int MatTo)
    : AOpticalOverride(MatCollection, MatFrom, MatTo) {}
  virtual ~FSNPOpticalOverride() {}

  virtual OpticalOverrideResultEnum calculate(ATracerStateful& Resources, APhoton* Photon, const double* NormalVector) override; //unitary vectors! iWave = -1 if not wavelength-resolved

  virtual const QString getType() const override {return "FSNP";}
  virtual const QString getAbbreviation() const override {return "FSNP";}
  virtual const QString getReportLine() const override;

  // save/load config is not used for this type!
  virtual void writeToJson(QJsonObject &json) const override;
  virtual bool readFromJson(const QJsonObject &json) override;

#ifdef GUI
  virtual QWidget* getEditWidget(QWidget* caller, GraphWindowClass* GraphWindow) override;
#endif
  virtual const QString checkOverrideData() override;

  //-- parameters --
  double Albedo = 0.95;
};

#endif // FSNPOPTICALOVERRIDE_H
