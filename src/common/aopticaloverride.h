#ifndef OPTICALOVERRIDECLASS_H
#define OPTICALOVERRIDECLASS_H

#include <QVector>
#include <QString>

class AOpticalOverride;
class APhoton;
class AMaterialParticleCollection;
class TRandom2;
class TH1I;
class QJsonObject;
class TH1D;
class QWidget;
class TObject;
class QObject;
class GraphWindowClass;
class ATracerStateful;

//modify these two functions if you want to register a new override type!
AOpticalOverride* OpticalOverrideFactory(QString model, AMaterialParticleCollection* MatCollection, int MatFrom, int MatTo);
const QStringList ListOvAllOpticalOverrideTypes();

class AOpticalOverride
{
public:
  //return status for photon tracing:
  enum OpticalOverrideResultEnum {NotTriggered, Absorbed, Forward, Back, _Error_};
  //detailed status for statistics only - used by override tester only
  enum ScatterStatusEnum {SpikeReflection, LobeReflection, LambertianReflection, Absorption, Transmission, Error, UnclassifiedReflection, Empty, Fresnel};


  AOpticalOverride(AMaterialParticleCollection* MatCollection, int MatFrom, int MatTo)
    : MatCollection(MatCollection), MatFrom(MatFrom), MatTo(MatTo) {}
  virtual ~AOpticalOverride() {}

  virtual OpticalOverrideResultEnum calculate(ATracerStateful& Resources, APhoton* Photon, const double* NormalVector) = 0; //unitary vectors! iWave = -1 if not wavelength-resolved

  virtual void printConfiguration(int iWave) = 0;
  virtual QString getType() const = 0;
  virtual QString getReportLine() = 0; // for GUI: reports override status "to material blabla (#id): properies"
  //TODO: initializeWaveResolved() -> no need to transfer data, MatCollection knows the settings
  virtual void initializeWaveResolved(bool /*bWaveResolved*/, double /*waveFrom*/, double /*waveStep*/, int /*waveNodes*/) {}  //override if override has wavelength-resolved data

  // save/load config
  virtual void writeToJson(QJsonObject &json);
  virtual bool readFromJson(QJsonObject &json);

  //next one is used by MatCollection when a material is removed
  void updateMatIndices(int iMatFrom, int iMatTo) {MatFrom = iMatFrom; MatTo = iMatTo;}

#ifdef GUI
  virtual QWidget* getEditWidget(QWidget* caller, GraphWindowClass* GraphWindow);
#endif
  virtual const QString checkOverrideData() {return "";} //TODO after all are done, make = 0

  // read-out variables for standalone checker only (not multithreaded)
  ScatterStatusEnum Status;               // type of interaction which happened - use in 1 thread only!

protected:  
  AMaterialParticleCollection* MatCollection;
  int MatFrom, MatTo;   // material index of material before(from) and after(to) the optical interface
};

class BasicOpticalOverride : public AOpticalOverride
{
public:
  BasicOpticalOverride(AMaterialParticleCollection* MatCollection, int MatFrom, int MatTo, double probLoss, double probRef, double probDiff, int scatterModel);
  BasicOpticalOverride(AMaterialParticleCollection* MatCollection, int MatFrom, int MatTo);
  virtual ~BasicOpticalOverride() {}

  virtual OpticalOverrideResultEnum calculate(ATracerStateful& Resources, APhoton* Photon, const double* NormalVector); //unitary vectors! iWave = -1 if not wavelength-resolved

  virtual void printConfiguration(int iWave);
  virtual QString getType() const {return "Simplistic_model";}
  virtual QString getReportLine();

  // save/load config is not used for this type!
  virtual void writeToJson(QJsonObject &json);
  virtual bool readFromJson(QJsonObject &json);

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

class FSNPOpticalOverride : public AOpticalOverride
{
public:
  FSNPOpticalOverride(AMaterialParticleCollection* MatCollection, int MatFrom, int MatTo, double albedo)
    : AOpticalOverride(MatCollection, MatFrom, MatTo), Albedo(albedo) {}
  FSNPOpticalOverride(AMaterialParticleCollection* MatCollection, int MatFrom, int MatTo)
    : AOpticalOverride(MatCollection, MatFrom, MatTo) {Albedo = 0.95;}
  virtual ~FSNPOpticalOverride() {}

  virtual OpticalOverrideResultEnum calculate(ATracerStateful& Resources, APhoton* Photon, const double* NormalVector); //unitary vectors! iWave = -1 if not wavelength-resolved

  virtual void printConfiguration(int iWave);
  virtual QString getType() const {return "FS_NP";}
  virtual QString getReportLine();

  // save/load config is not used for this type!
  virtual void writeToJson(QJsonObject &json);
  virtual bool readFromJson(QJsonObject &json);

#ifdef GUI
  virtual QWidget* getEditWidget(QWidget* caller, GraphWindowClass* GraphWindow) override;
#endif
  virtual const QString checkOverrideData() override;

  //-- parameters --
  double Albedo;
};

class AWaveshifterOverride : public AOpticalOverride
{
public:
  AWaveshifterOverride(AMaterialParticleCollection* MatCollection, int MatFrom, int MatTo, int ReemissionModel = 1);
  virtual ~AWaveshifterOverride();

  void initializeWaveResolved(bool bWaveResolved, double waveFrom, double waveStep, int waveNodes) override;
  virtual OpticalOverrideResultEnum calculate(ATracerStateful& Resources, APhoton* Photon, const double* NormalVector); //unitary vectors! iWave = -1 if not wavelength-resolved

  virtual void printConfiguration(int iWave);
  virtual QString getType() const {return "SurfaceWLS";}
  virtual QString getReportLine();

  // save/load config is not used for this type!
  virtual void writeToJson(QJsonObject &json);
  virtual bool readFromJson(QJsonObject &json);

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
  TH1D* Spectrum;

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

class SpectralBasicOpticalOverride : public BasicOpticalOverride
{
public:
  SpectralBasicOpticalOverride(AMaterialParticleCollection* MatCollection, int MatFrom, int MatTo);
  SpectralBasicOpticalOverride(AMaterialParticleCollection* MatCollection, int MatFrom, int MatTo, int ScatterModel, double EffWave);
  virtual ~SpectralBasicOpticalOverride() {}

  virtual OpticalOverrideResultEnum calculate(ATracerStateful& Resources, APhoton* Photon, const double* NormalVector) override; //unitary vectors! iWave = -1 if not wavelength-resolved

  virtual void printConfiguration(int iWave) override;
  virtual QString getType() const override {return "SimplisticSpectral_model";}
  virtual QString getReportLine() override;

  // save/load config is not used for this type!
  virtual void writeToJson(QJsonObject &json) override;
  virtual bool readFromJson(QJsonObject &json) override;

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

#endif // OPTICALOVERRIDECLASS_H
