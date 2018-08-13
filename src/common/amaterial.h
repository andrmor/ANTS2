#ifndef AMATERIAL_H
#define AMATERIAL_H

#include <QVector>
#include <QPair>
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
class TRandom2;
namespace NCrystal { class Scatter; }

class AMaterial
{
public:
  AMaterial();

  QString name;
  double density; //in g/cm3
  double temperature = 298.0; //in K
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
  QVector<QPair<double,double> > PriScint_DecayTimeVector;  //first = stat weight, second = decay time (ns)
  QVector<QPair<double,double> > PriScint__RaiseTimeVector;  //first = stat weight, second = decay time (ns)
  int PriScintModel = 0; //0=sum, 1=Shao

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
  QVector<double> absWaveBinned; //regular step (WaveStep step, WaveNodes bins)
  double getAbsorptionCoefficient(int iWave = -1) const;

  QVector<double> reemisProbWave;
  QVector<double> reemisProbWave_lambda;
  QVector<double> reemissionProbBinned; //regular step (WaveStep step, WaveNodes bins)
  double getReemissionProbability(int iWave = -1) const;

  QVector<double> PrimarySpectrum_lambda;
  QVector<double> PrimarySpectrum;
  TH1D* PrimarySpectrumHist = 0;
  QVector<double> SecondarySpectrum_lambda;
  QVector<double> SecondarySpectrum;
  TH1D* SecondarySpectrumHist = 0;

  TGeoMaterial* GeoMat = 0; // handled by TGEoManager
  TGeoMedium* GeoMed = 0;   // handled by TGEoManager

  double GeneratePrimScintTime(TRandom2* RandGen) const;

  void updateNeutronDataOnCompositionChange(const AMaterialParticleCollection *MPCollection);
  void updateRuntimeProperties(bool bLogLogInterpolation, TRandom2* RandGen, int numThreads = 1);

  void UpdateRandGen(int ID, TRandom2* RandGen);

  void clear();
  void writeToJson (QJsonObject &json, AMaterialParticleCollection* MpCollection);  //does not save overrides!
  bool readFromJson(QJsonObject &json, AMaterialParticleCollection* MpCollection);

  const QString CheckMaterial(int iPart, const AMaterialParticleCollection *MpCollection) const;

  bool isNCrystalInUse() const;

private:
  //run-time properties
  double _PrimScintSumStatWeight_Decay;
  double _PrimScintSumStatWeight__Raise;

private:
  double FT(double td, double tr, double t) const;
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
  QVector<ANeutronInteractionElement> IsotopeRecords;

  void UpdateRunTimeProperties(bool bUseLogLog,  TRandom2 *RandGen, int numThreads, double temp = 298.0);
  void ClearProperties();

  ANeutronInteractionElement* getNeutronInteractionElement(int index);  //0 if wrong index

  void writeToJson (QJsonObject &json, AMaterialParticleCollection* MpCollection) const;
  void readFromJson(const QJsonObject &json, AMaterialParticleCollection* MpCollection);

  bool isParticleOneOfSecondaries(int iPart) const;
  void prepareForParticleRemove(int iPart);

  QString NCrystal_Ncmat;
  double NCrystal_Dcutoff = 0;
  double NCrystal_Packing = 1.0;

#ifdef  __USE_ANTS_NCRYSTAL__
  QVector<const NCrystal::Scatter *> NCrystal_scatters;
#endif
  double getNCrystalCrossSectionBarns(double energy_keV, int threadIndex = 0) const;
  void   generateScatteringNonOriented(double energy_eV, double & angle, double & delta_ekin_keV, int threadIndex = 0) const;
  void   UpdateRandGen(int ID, TRandom2 *RandGen);
};

struct MatParticleStructure  //each paticle have this entry in MaterialStructure
{
  bool TrackingAllowed = true;
  bool MaterialIsTransparent = true;
  double PhYield = 0;         // Photon yield of the primary scintillation
  double IntrEnergyRes = 0; // intrinsic energy resolution

  //for neutrons - separate activation of capture and ellastic scattering is possible
  bool bCaptureEnabled = true;
  bool bEllasticEnabled = false;
  bool bUseNCrystal = false;
  bool bAllowAbsentCsData = false;

  QVector<double> InteractionDataX; //energy in keV
  QVector<double> InteractionDataF; //stopping power (for charged) or total interaction cross-section (neutrals)

  QVector<NeutralTerminatorStructure> Terminators;

  QString DataSource;     //for gamma  - stores XCOM file if was used
  QString DataString;     //for gamma  - can be used to store composition   obsolete!!!

  bool CalculateTotalForGamma();  //true - success, false - mismatch in binning of the data

  void Clear();
};

#endif // AMATERIAL_H
