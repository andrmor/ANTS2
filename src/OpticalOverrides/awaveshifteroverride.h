#ifndef AWAVESHIFTEROVERRIDE_H
#define AWAVESHIFTEROVERRIDE_H

#include "aopticaloverride.h"

#include <QString>
#include <QVector>

class AMaterialParticleCollection;
class ATracerStateful;
class APhoton;
class QJsonObject;
class GraphWindowClass;
class TH1D;

class AWaveshifterOverride : public AOpticalOverride
{
public:
  AWaveshifterOverride(AMaterialParticleCollection* MatCollection, int MatFrom, int MatTo);
  virtual ~AWaveshifterOverride();

  void initializeWaveResolved(bool bWaveResolved, double waveFrom, double waveStep, int waveNodes) override;
  virtual OpticalOverrideResultEnum calculate(ATracerStateful& Resources, APhoton* Photon, const double* NormalVector); //unitary vectors! iWave = -1 if not wavelength-resolved

  virtual const QString getType() const override {return "SurfaceWLS";}
  virtual const QString getAbbreviation() const override {return "WLS";}
  virtual const QString getReportLine() const override;

  // save/load config is not used for this type!
  virtual void writeToJson(QJsonObject &json) const;
  virtual bool readFromJson(const QJsonObject &json);

#ifdef GUI
  virtual QWidget* getEditWidget(QWidget *caller, GraphWindowClass* GraphWindow) override;
#endif

  virtual const QString checkOverrideData() override;

  //-- parameters --
  int ReemissionModel = 1; //0-isotropic (4Pi), 1-Lamb back (2Pi), 2-Lamb forward (2Pi)
  QVector<double> ReemissionProbability_lambda;
  QVector<double> ReemissionProbability;
  QVector<double> ReemissionProbabilityBinned;

  QVector<double> EmissionSpectrum_lambda;
  QVector<double> EmissionSpectrum;
  TH1D* Spectrum = 0;

  //tmp parameters
  double WaveFrom;
  double WaveStep;
  int WaveNodes;

private:
#ifdef GUI
  void loadReemissionProbability(QWidget *caller);
  void loadEmissionSpectrum(QWidget *caller);
  void showReemissionProbability(GraphWindowClass* GraphWindow, QWidget *caller);
  void showEmissionSpectrum(GraphWindowClass* GraphWindow, QWidget *caller);
  void showBinnedReemissionProbability(GraphWindowClass* GraphWindow, QWidget *caller);
  void showBinnedEmissionSpectrum(GraphWindowClass* GraphWindow, QWidget *caller);
#endif
};

#endif // AWAVESHIFTEROVERRIDE_H
