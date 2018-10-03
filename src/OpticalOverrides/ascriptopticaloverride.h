#ifndef ASCRIPTOPTICALOVERRIDE_H
#define ASCRIPTOPTICALOVERRIDE_H

#include "aopticaloverride.h"

#include <QString>

class AOpticalOverrideScriptInterface;

class AScriptOpticalOverride : public AOpticalOverride
{
public:
  AScriptOpticalOverride(AMaterialParticleCollection* MatCollection, int MatFrom, int MatTo);
  virtual ~AScriptOpticalOverride();

  virtual OpticalOverrideResultEnum calculate(ATracerStateful& Resources, APhoton* Photon, const double* NormalVector) override; //unitary vectors! iWave = -1 if not wavelength-resolved

  virtual const QString getType() const override {return "CustomScript";}
  virtual const QString getAbbreviation() const override {return "JS";}
  virtual const QString getReportLine() const override;

  virtual void writeToJson(QJsonObject &json) override;
  virtual bool readFromJson(QJsonObject &json) override;

#ifdef GUI
  virtual QWidget* getEditWidget(QWidget *caller, GraphWindowClass* GraphWindow) override;
#endif
  virtual const QString checkOverrideData() override;

private:
  QString Script = "photon.Absorb()";

#ifdef GUI
  void openScriptWindow(QWidget* parent);
#endif
};

#endif // ASCRIPTOPTICALOVERRIDE_H
