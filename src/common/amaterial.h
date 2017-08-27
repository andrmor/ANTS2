#ifndef AMATERIAL_H
#define AMATERIAL_H

#include <QVector>
#include <QString>

#include "aneutroninteractionelement.h"
#include "amaterialcomposition.h"

class QJsonObject;
class AParticle;
class AOpticalOverride;
class TH1D;
class TGeoMaterial;
class TGeoMedium;
class AMaterialParticleCollection;
struct NeutralTerminatorStructure;
struct MatParticleStructure;

class AMaterial
{
public:
  AMaterial();

  QString name;
  double density; //in g/cm3
  double p1,p2,p3; //parameters for TGeoManager
  double n;   //refractive index for monochrome
  double abs; //exp absorption per mm   for monochrome    (I = I0*exp(-abs*length[mm]))
  double reemissionProb; //for waveshifters: probability that absorbed photon is reemitted
  double rayleighMFP; //0 - no rayleigh scattering
  double rayleighWave;
  QVector<double> rayleighBinned;//regular step (WaveStep step, WaveNodes bins)
  double e_driftVelocity;
  double W; //default W
  double SecYield;  // ph per secondary electron
  double PriScintDecayTime;
  double SecScintDecayTime;
  QString Comments;

  AMaterialComposition ChemicalComposition;

  QVector<MatParticleStructure> MatParticle; //material properties related to individual particles

  QVector<AOpticalOverride*> OpticalOverrides; //NULL - override not defined

  QVector<double> nWave_lambda;
  QVector<double> nWave;
  QVector<double> nWaveBinned; //regular step (WaveStep step, WaveNodes bins)
  double getRefractiveIndex(int iWave = -1) const;

  QVector<double> absWave_lambda;
  QVector<double> absWave;
  QVector<double> absWaveBinned;//regular step (WaveStep step, WaveNodes bins)
  double getAbsorptionCoefficient(int iWave = -1) const;

  QVector<double> PrimarySpectrum_lambda;
  QVector<double> PrimarySpectrum;
  TH1D* PrimarySpectrumHist;    //pointer!
  QVector<double> SecondarySpectrum_lambda;
  QVector<double> SecondarySpectrum;
  TH1D* SecondarySpectrumHist;  //pointer!

  TGeoMaterial* GeoMat; //pointer, but it is taken care of by TGEoManager
  TGeoMedium* GeoMed;   //pointer, but it is taken care of by TGEoManager

  void updateNeutronDataOnCompositionChange(const AMaterialParticleCollection *MPCollection);
  void updateRuntimeProperties(bool bLogLogInterpolation);

  void clear();
  void writeToJson (QJsonObject &json, AMaterialParticleCollection* MpCollection);  //does not save overrides!
  bool readFromJson(QJsonObject &json, AMaterialParticleCollection* MpCollection);

  QString CheckMaterial(int iPart, const AMaterialParticleCollection *MpCollection) const;
};

struct NeutralTerminatorStructure //descriptor for the interaction scenarios for neutral particles
{
  enum ReactionType {Photoelectric = 0,
                    ComptonScattering = 1,
                    Absorption = 2,
                    PairProduction = 3,
                    ElasticScattering = 4,
                    Undefined = 5};  //must keep the numbers - directly used in json config files

  ReactionType Type;  
  QVector<double> PartialCrossSection;
  QVector<double> PartialCrossSectionEnergy;

  // exclusive for neutrons
  QVector<AAbsorptionElement> AbsorptionElements;  // exclusive for capture
  QVector<AElasticScatterElement> ScatterElements; // exclusive for ellastic

  void UpdateNeutronCrossSections(bool bUseLogLog);   //

  AAbsorptionElement* getCaptureElement(int index);  //0 if wrong index
  AElasticScatterElement* getElasticScatterElement(int index);  //0 if wrong index

  void writeToJson (QJsonObject &json, AMaterialParticleCollection* MpCollection) const;
  void readFromJson(const QJsonObject &json, AMaterialParticleCollection* MpCollection);

  bool isParticleOneOfSecondaries(int iPart) const;
  void prepareForParticleRemove(int iPart);
};

struct MatParticleStructure  //each paticle have this entry in MaterialStructure
{
  MatParticleStructure();

  bool TrackingAllowed;
  bool MaterialIsTransparent;
  double PhYield;         // Photon yield of the primary scintillation
  double IntrEnergyRes; // intrinsic energy resolution

  //for neutrons - separate activation of capture and ellastic scattering is possible
  bool bCaptureEnabled;
  bool bEllasticEnabled;
  bool bAllowAbsentCsData;

  QVector<double> InteractionDataX; //energy in keV
  QVector<double> InteractionDataF; //stopping power (for charged) or total interaction cross-section (neutrals)

  QVector<NeutralTerminatorStructure> Terminators;

  QString DataSource;     //for gamma  - stores XCOM file if was used
  QString DataString;     //for gamma  - can be used to store composition   obsolete!!!

  bool CalculateTotalForGamma();  //true - success, false - mismatch in binning of the data
};

#endif // AMATERIAL_H
