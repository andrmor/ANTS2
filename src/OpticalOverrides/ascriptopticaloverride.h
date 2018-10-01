#ifndef ASCRIPTOPTICALOVERRIDE_H
#define ASCRIPTOPTICALOVERRIDE_H

#include "aopticaloverride.h"

#include <QString>

//to move away
class QScriptEngine;
class AOpticalOverrideScriptInterface;

class AScriptOpticalOverride : public AOpticalOverride
{
public:
  //AScriptOpticalOverride(AMaterialParticleCollection* MatCollection, int MatFrom, int MatTo);
  AScriptOpticalOverride(AMaterialParticleCollection* MatCollection, int MatFrom, int MatTo);
  virtual ~AScriptOpticalOverride() {}

  //TODO transfer to a separate entry, external pointer to ScriptEngine
  void initializeWaveResolved(bool bWaveResolved, double waveFrom, double waveStep, int waveNodes) override;

  virtual OpticalOverrideResultEnum calculate(TRandom2* RandGen, APhoton* Photon, const double* NormalVector) override; //unitary vectors! iWave = -1 if not wavelength-resolved

  virtual void printConfiguration(int iWave) override;
  virtual QString getType() const override {return "CustomScript";}
  virtual QString getReportLine() override;

  virtual void writeToJson(QJsonObject &json) override;
  virtual bool readFromJson(QJsonObject &json) override;

#ifdef GUI
  virtual QWidget* getEditWidget(QWidget *caller, GraphWindowClass* GraphWindow) override;
#endif
  virtual const QString checkOverrideData() override;

private:
  QString Script = "photon.SetDirection(0, 0, 1)";
  //QString Script = "photon.Fresnel()";
  //QString Script = "photon.Absorb()";

  //will be moved to a separate class and made thread-safe
  QScriptEngine* ScriptEngine;
  AOpticalOverrideScriptInterface* interfaceObject;

#ifdef GUI
  void openScriptWindow(QWidget* parent);
#endif
};

#endif // ASCRIPTOPTICALOVERRIDE_H
