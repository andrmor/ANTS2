#ifndef AMATERIALPARTICLECOLECTION_H
#define AMATERIALPARTICLECOLECTION_H

#include "amaterial.h"
#include "aparticle.h"

#include <QVector>
#include <QString>
#include <QObject>
#include <QStringList>

class GeneralSimSettings;
class ATracerStateful;
class AGeoObject;

class AMaterialParticleCollection : public QObject
{
  Q_OBJECT

public:
  AMaterialParticleCollection();
  ~AMaterialParticleCollection();

  AMaterial tmpMaterial; //all pointers are 0 on start -see default constructor
  bool fLogLogInterpolation;

private:
  QVector<AParticle*> ParticleCollection;
  QVector<AMaterial*> MaterialCollectionData;
  double WaveFrom, WaveTo, WaveStep;
  int WaveNodes;
  bool WavelengthResolved;

public:
  //configuration
  void SetWave(bool wavelengthResolved, double waveFrom, double waveTo, double waveStep, int waveNodes);

  //hopefully we will get rid of the RandGen after update in NCrystal
  void UpdateRuntimePropertiesAndWavelengthBinning(GeneralSimSettings *SimSet, TRandom2 *RandGen, int numThreads = 1);
  const QString CheckOverrides();
  void updateRandomGenForThread(int ID, TRandom2 *RandGen);

  //for script-based optical override initialization
  bool isScriptOpticalOverrideDefined() const;

  //info requests
    //materials
  AMaterial* operator[](int i) {return MaterialCollectionData[i]; } //get pointer to material with index i
  const AMaterial* operator[](int i) const {return MaterialCollectionData[i]; } //get pointer to material with index i
  int countMaterials() const {return MaterialCollectionData.size();}
  void getFirstOverridenMaterial(int &ifrom, int &ito);
  double convertWaveIndexToWavelength(int index) {return WaveFrom + WaveStep * index;}
  QString getMaterialName(int matIndex);
  const QStringList getListOfMaterialNames() const;

    //particle
  int countParticles() const {return ParticleCollection.size();}
  int getParticleId(QString name) const;
  const QString getParticleName(int particleIndex) const;
  AParticle::ParticleType getParticleType(int particleIndex) const;
  const AParticle* getParticle(int particleIndex) const;
  AParticle* getParticle(int particleIndex);
  const QStringList getListOfParticleNames() const;  
  void ReplaceParticle(int iPart, const AParticle & p);

  //Material handling
  void AddNewMaterial(bool fSuppressChangedSignal = false);
  void AddNewMaterial(QString name, bool fSuppressChangedSignal = false);
  int FindMaterial(QString name); //if not found, returns -1; if found, returns material index
  bool DeleteMaterial(int imat); //takes care of overrides of materials with index larger than imat!
  void UpdateWaveResolvedProperties(int imat); //updates wavelength-resolved material properties
  bool isNCrystalInUse() const;

  //Particles handling
  //bool AddParticle(QString name, AParticle::ParticleType type, int charge, double mass);
  //bool AddParticle(QString name, AParticle::ParticleType type, double mass);
  QVector<AParticle*>* getParticleCollection() {return &ParticleCollection;}
  int getNeutronIndex() const; //returns -1 if not in the collection

  //tmpMaterial - related
  // ***!!! todo: remove ClearTmpMaterial and use clear method of the AMaterial
  void ClearTmpMaterial(); //deletes all objects pointed by the class pointers!!!
  void CopyTmpToMaterialCollection(); //creates a copy of all pointers // true is new material was added to material collection
  void CopyMaterialToTmp(int imat);

  //json write/read handling
  void writeToJson(QJsonObject &json);
  void writeMaterialToJson(int imat, QJsonObject &json);
  void writeParticleCollectionToJson(QJsonObject &json);
  bool readFromJson(QJsonObject &json);
  void AddNewMaterial(QJsonObject &json);

  //general purpose requests
  void   GetWave(bool& wavelengthResolved, double& waveFrom, double& waveTo, double& waveStep, int& waveNodes) const;
  bool   IsWaveResolved() const {return WavelengthResolved;}
  double getDriftSpeed(int iMat) const; //returns in mm / ns
  void   CheckReadyForGeant4Sim(QString & Errors, QString & Warnings, const AGeoObject * World) const;

public:
  void ConvertToStandardWavelengthes(QVector<double> *sp_x, QVector<double> *sp_y, QVector<double> *y);
  int findOrAddParticle(const AParticle & particle);
  int findOrAddParticle(const QJsonObject & json);

  const QString CheckMaterial(const AMaterial *mat, int iPart) const; //"" - check passed, otherwise error
  const QString CheckMaterial(int iMat, int iPart) const;       //"" - check passed, otherwise error
  const QString CheckMaterial(int iMat) const;                  //"" - check passed, otherwise error
  const QString CheckTmpMaterial() const;                       //"" - check passed, otherwise error

  int CheckParticleEnergyInRange(int iPart, double Energy); //check all materials - if this particle is tracable and mat is not-tansparent,
  //check that the particle energy is withing the defined energy range of the total interaction.
  //if not in range, the first material index with such a problem is returned.
  // -1 is returned if there are no errors

  void IsParticleInUse(int particleId, bool& bInUse, QString &MaterialNames) const;
  void RemoveParticle(int particleId);  // should NOT be used directly - use RemoveParticle method of AConfiguration
  int WaveToIndex(double wavelength) const;

private:
  int ConflictingMaterialIndex; //used by CheckMaterial function
  int ConflictingParticleIndex; //used by CheckMaterial function

  //internal kitchen
  void clearMaterialCollection();
  void clearParticleCollection();
  void registerNewParticle(); //called after a particle was added to particle collection. It updates terminations om MatParticles
  bool readParticleCollectionFromJson(QJsonObject &json);
  void generateMaterialsChangedSignal();  
  void ensureMatNameIsUnique(AMaterial *mat);

public slots:
  void OnRequestListOfParticles(QStringList &definedParticles);

signals:
  void MaterialsChanged(const QStringList);
  void ParticleCollectionChanged();

};

#endif // AMATERIALPARTICLECOLECTION_H
