#ifndef ASCRIPTOPTICALOVERRIDE_H
#define ASCRIPTOPTICALOVERRIDE_H

#include "aopticaloverride.h"

#include <QString>

class AScriptOpticalOverride : public AOpticalOverride
{
public:
  AScriptOpticalOverride(AMaterialParticleCollection* MatCollection, int MatFrom, int MatTo);
  virtual ~AScriptOpticalOverride();

  virtual OpticalOverrideResultEnum calculate(ATracerStateful& Resources, APhoton* Photon, const double* NormalVector) override; //unitary vectors! iWave = -1 if not wavelength-resolved

  virtual const QString getType() const override {return "CustomScript";}
  virtual const QString getAbbreviation() const override {return "JS";}
  virtual const QString getReportLine() const override;
  virtual const QString getLongReportLine() const;

  virtual void writeToJson(QJsonObject &json) const override;
  virtual bool readFromJson(const QJsonObject &json) override;

#ifdef GUI
  virtual QWidget* getEditWidget(QWidget *caller, GraphWindowClass* GraphWindow) override;
#endif
  virtual const QString checkOverrideData() override;

private:
  QString Script = "if (math.random() < 0.25) photon.LambertForward()\n"
                   "else photon.SpecularReflection()";

#ifdef GUI
  void openScriptWindow(QWidget* caller);
#endif
};

#endif // ASCRIPTOPTICALOVERRIDE_H
