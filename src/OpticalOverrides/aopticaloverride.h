#ifndef OPTICALOVERRIDECLASS_H
#define OPTICALOVERRIDECLASS_H

#include <QString>

class AOpticalOverride;
class APhoton;
class AMaterialParticleCollection;
class QJsonObject;
class GraphWindowClass;
class ATracerStateful;
class QWidget;

//  ----   !!!  ----
// modify these two functions if you want to register a new override type
AOpticalOverride* OpticalOverrideFactory(QString model, AMaterialParticleCollection* MatCollection, int MatFrom, int MatTo);
const QStringList ListOvAllOpticalOverrideTypes();


class AOpticalOverride
{
public:
  enum OpticalOverrideResultEnum {NotTriggered, Absorbed, Forward, Back, _Error_}; //return status for photon tracing:
  enum ScatterStatusEnum {
                          Absorption,
                          SpikeReflection, LobeReflection, LambertianReflection,
                          UnclassifiedReflection,
                          Transmission,
                          Empty, Fresnel, Error
                         }; //detailed status for statistics only - used by override tester only

  AOpticalOverride(AMaterialParticleCollection* MatCollection, int MatFrom, int MatTo)
    : MatCollection(MatCollection), MatFrom(MatFrom), MatTo(MatTo) {}
  virtual ~AOpticalOverride() {}

  virtual OpticalOverrideResultEnum calculate(ATracerStateful& Resources, APhoton* Photon, const double* NormalVector) = 0; //unitary vectors! iWave = -1 if not wavelength-resolved

  virtual const QString getType() const = 0;
  virtual const QString getAbbreviation() const = 0; //for GUI: used to identify - must be short (<= 4 chars) - try to make unique
  virtual const QString getReportLine() const = 0; // for GUI: used to reports override status (try to make as short as possible)
  virtual const QString getLongReportLine() const {return getReportLine();} //for GUI: used in overlap map

  //called automatically before sim start
  virtual void initializeWaveResolved() {}  //override if override has wavelength-resolved data

  // save/load config
  virtual void writeToJson(QJsonObject &json) const;
  virtual bool readFromJson(const QJsonObject &json);

  //used by MatCollection when a material is removed
  void updateMatIndices(int iMatFrom, int iMatTo) {MatFrom = iMatFrom; MatTo = iMatTo;}

#ifdef GUI
  // returns a GUI widget to show / edit parameters
  virtual QWidget* getEditWidget(QWidget* caller, GraphWindowClass* GraphWindow);
#endif

  //called on editing end (widget above) and before sim start to avoid miss-configurations
  virtual const QString checkOverrideData() = 0; //cannot be const - w.resolved needs rebin

  // read-out variables for standalone checker only (not multithreaded)
  ScatterStatusEnum Status;               // type of interaction which happened - use in 1 thread only!

  //misc
  int getMaterialTo() const {return MatTo;}

protected:  
  AMaterialParticleCollection* MatCollection;
  int MatFrom, MatTo;   // material index of material before(from) and after(to) the optical interface
};

#endif // OPTICALOVERRIDECLASS_H
