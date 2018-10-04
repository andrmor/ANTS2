#ifndef SPECTRALBASICOPTICALOVERRIDE_H
#define SPECTRALBASICOPTICALOVERRIDE_H

#include "aopticaloverride.h"
#include "abasicopticaloverride.h"

#include <QString>
#include <QVector>

class AMaterialParticleCollection;
class ATracerStateful;
class APhoton;
class QJsonObject;
class GraphWindowClass;

class SpectralBasicOpticalOverride : public ABasicOpticalOverride
{
public:
  SpectralBasicOpticalOverride(AMaterialParticleCollection* MatCollection, int MatFrom, int MatTo);
  virtual ~SpectralBasicOpticalOverride() {}

  virtual OpticalOverrideResultEnum calculate(ATracerStateful& Resources, APhoton* Photon, const double* NormalVector) override; //unitary vectors! iWave = -1 if not wavelength-resolved

  virtual const QString getType() const override {return "SimplisticSpectral_model";}
  virtual const QString getAbbreviation() const override {return "SiSp";}
  virtual const QString getReportLine() const override;

  // save/load config is not used for this type!
  virtual void writeToJson(QJsonObject &json) const override;
  virtual bool readFromJson(const QJsonObject &json) override;

  virtual void initializeWaveResolved(bool bWaveResolved, double waveFrom, double waveStep, int waveNodes) override;
  const QString loadData(const QString& fileName);

#ifdef GUI
  virtual QWidget* getEditWidget(QWidget* caller, GraphWindowClass* GraphWindow) override;
#endif

  virtual const QString checkOverrideData() override;

  //parameters
  QVector<double> Wave;
  QVector<double> ProbLoss; //probability of absorption
  QVector<double> ProbLossBinned; //probability of absorption
  QVector<double> ProbRef;  //probability of specular reflection
  QVector<double> ProbRefBinned;  //probability of specular reflection
  QVector<double> ProbDiff; //probability of scattering
  QVector<double> ProbDiffBinned; //probability of scattering
  double effectiveWavelength = 500; //if waveIndex of photon is -1, index correspinding to this wavelength will be used
  double effectiveWaveIndex;

private:
#ifdef GUI
  void loadSpectralData(QWidget *caller);
  void showLoaded(GraphWindowClass *GraphWindow);
  void showBinned(QWidget *widget, GraphWindowClass *GraphWindow);
#endif
};

#endif // SPECTRALBASICOPTICALOVERRIDE_H
