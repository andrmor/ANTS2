#ifndef PARTICLESOURCESCLASS_H
#define PARTICLESOURCESCLASS_H

#include <QString>
#include <QVector>
#include <QObject>
#include "TVector3.h"

class AParticle;
class AMaterialParticleCollection;
class QJsonObject;
class TH1D;
class TRandom2;
class DetectorClass;

struct GunParticleStruct
{
  int     ParticleId = 0;
  double  StatWeight = 1.0;
  double  energy = 100.0; //in keV
  QString PreferredUnits = "keV";
  bool    Individual = true; // true = individual particle; false = linked
  int     LinkedTo = 0; // index of the "parent" particle this one is following
  double  LinkingProbability = 0;  //probability to be emitted after the parent particle
  bool    LinkingOppositeDir = false; // false = random direction; otherwise particle is emitted in the opposite direction in respect to the LinkedTo particle

  TH1D* spectrum = 0; //energy spectrum

  GunParticleStruct * clone() const;

  ~GunParticleStruct();
};

struct AParticleSourceRecord
{
  //name
  QString name = "Underfined";
  //source type (geometry)
  int index = 0; //shape
  //position
  double X0 = 0;
  double Y0 = 0;
  double Z0 = 0;
  //orientation
  double Phi = 0;
  double Theta = 0;
  double Psi = 0;
  //size
  double size1 = 10.0;
  double size2 = 10.0;
  double size3 = 10.0;
  //collimation
  double CollPhi = 0;
  double CollTheta = 0;
  double Spread = 45.0;

  //limit to material
  bool DoMaterialLimited = false;
  QString LimtedToMatName;

  //Relative activity
  double Activity = 1.0;

  //particles
  QVector<GunParticleStruct*> GunParticles;

  ~AParticleSourceRecord(); //deletes records in dynamic GunParticles

  AParticleSourceRecord * clone() const;

  //local variables, used in tracking, calculated autonatically, not to be loaded/saved!
  int LimitedToMat; //automatically calculated if LimtedToMatName matches a material
  bool fLimit = false;
};

class GeneratedParticleStructure
{
public:
    int ParticleId;
    double Energy;
    double Position[3];
    double Direction[3];
};

class LinkedParticleStructure
{
public:
  int iParticle; //indexed according to GunParticles index
  int LinkedTo;  //index of particle it is linked to

  LinkedParticleStructure() {}
  LinkedParticleStructure(int iparticle, int linkedto = -1) {iParticle = iparticle; LinkedTo = linkedto;}
};

class ParticleSourcesClass : public QObject
{
    Q_OBJECT
public:
   ParticleSourcesClass(const DetectorClass* Detector, TRandom2* RandGen, TString nameID = "");
  ~ParticleSourcesClass();

  //MAIN usage
  void Init(); // !!! has to be called before the first use of "GenerateEvent"  NOT thread safe!
  QVector<GeneratedParticleStructure>* GenerateEvent(); //see Init!!!

  //requests
  int size() {return ParticleSourcesData.size();}
  double getTotalActivity();
  AParticleSourceRecord* getSource(int iSource) {return ParticleSourcesData[iSource];}
  //ParticleSourceStructure &operator[] (int iSource) {return *ParticleSourcesData[iSource];}
  AParticleSourceRecord* getLastSource() {return ParticleSourcesData.last();}
  //ParticleSourceStructure &last() {return *ParticleSourcesData.last();}

  //Source handling - after handling is finished, requires Init() !!!
  void append(AParticleSourceRecord* gunParticle);
  void remove(int iSource);
  void clear();

  //check consistency of data
  int CheckSource(int isource); // 0 - no errors
  QString getErrorString(int error);

  // save
  bool writeSourceToJson(int iSource, QJsonObject &json); //only one source
  bool writeToJson(QJsonObject &json); //all config
  // load
  bool readSourceFromJson(int iSource, QJsonObject &json);
  bool readFromJson(QJsonObject &json); // all config
  bool LoadGunEnergySpectrum(int iSource, int iParticle, QString fileName);

  TVector3 GenerateRandomDirection();  
  void checkLimitedToMaterial(AParticleSourceRecord *s);

  //for remove particle from configuration
  void RemoveParticle(int particleId); //should NOT be used to remove one of particles in use! use onIspareticleInUse first
  void IsParticleInUse(int particleId, bool& bInUse, QString& SourceNames);

private:  
  //external resources - pointers
  const DetectorClass* Detector;
  AMaterialParticleCollection* MpCollection;
  TRandom2 *RandGen;

  TString NameID; //use to make unique hist name in multithread environment /// - actually not needed anymore!

  QVector<AParticleSourceRecord*> ParticleSourcesData;
  QVector<double> TotalParticleWeight;  
  double TotalActivity;
  QVector< QVector< QVector<LinkedParticleStructure> > > LinkedPartiles; //[isource] [iparticle] []  (includes the record of the particle iteslf!!!)
                              //full recipe of emission builder (containes particles linked to particles etc up to the top level individual particle)

  QVector<TVector3> CollimationDirection; //[isource] collimation direction 
  QVector<double> CollimationProbability; //[isource] collimation probability: solid angle inside cone / 4Pi
  //QVector<int> CollimationSpreadProduct; //[isource] vector product of collimation and normal (used to check is the new vector inisde collimation cone)

  //utilities
  void CalculateTotalActivity();
  void GeneratePosition(int isource, double *R);
  void AddParticleInCone(int isource, int iparticle, QVector<GeneratedParticleStructure> *GeneratedParticles); //QVector - only pointer is transferred!
};

#endif // PARTICLESOURCESCLASS_H
