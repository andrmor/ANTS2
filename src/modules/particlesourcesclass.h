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
  int ParticleId;
  double StatWeight;
  double energy;
  TH1D* spectrum; //energy spectrum
  bool Individual; // true = individual particle; false = linked
  int LinkedTo; // index of the "parent" particle this one is following
  double LinkingProbability;  //probability to be emitted after the parent particle
  bool LinkingOppositeDir; // false = random direction; otherwise particle is emitted in the opposite direction in respect to the LinkedTo particle

  GunParticleStruct()
    {
      spectrum = 0;
      Individual = true;
      LinkedTo = 0;//-1;
      LinkingProbability = 0;
      LinkingOppositeDir = false;
    }

  ~GunParticleStruct();
};

struct ParticleSourceStructure
{
  //name
  QString name;
  //source type (geometry)
  int index; //shape
  //position
  double X0;
  double Y0;
  double Z0;
  //orientation
  double Phi;
  double Theta;
  double Psi;
  //size
  double size1;
  double size2;
  double size3;
  //collimation
  double CollPhi;
  double CollTheta;
  double Spread;

  //limit to material
  bool DoMaterialLimited;
  QString LimtedToMatName;

  //Activity
  double Activity;

  //particles
  QVector<GunParticleStruct*> GunParticles;

  //constructor - creates a default source with _no_ particles!
  ParticleSourceStructure();

  //destructor - takes care of dynamic GunParticles
  ~ParticleSourceStructure();



  //local variables, used in tracking, calculated autonatically, not to be loaded/saved!
  int LimitedToMat; //automatically calculated if LimtedToMatName matches a material
  bool fLimit;
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
   ParticleSourcesClass(const DetectorClass* Detector, TString nameID = "");
  ~ParticleSourcesClass();

  //MAIN usage
  void Init(); // !!! has to be called before the first use of "GenerateEvent"  NOT thread safe!
  QVector<GeneratedParticleStructure>* GenerateEvent(); //see Init!!!

  //requests
  int size() {return ParticleSourcesData.size();}
  double getTotalActivity();
  ParticleSourceStructure* getSource(int iSource) {return ParticleSourcesData[iSource];}
  //ParticleSourceStructure &operator[] (int iSource) {return *ParticleSourcesData[iSource];}
  ParticleSourceStructure* getLastSource() {return ParticleSourcesData.last();}
  //ParticleSourceStructure &last() {return *ParticleSourcesData.last();}

  //Source handling - after handling is finished, requires Init() !!!
  void append(ParticleSourceStructure* gunParticle);
  void remove(int iSource);
  void clear();

  //check consistency of data
  int CheckSource(int isource); // 0 - no errors
  QString getErrorString(int error);

  // save
  bool writeSourceToJson(int iSource, QJsonObject &json); //only one source
  bool writeToJson(QJsonObject &json); //all config
  bool SaveSourceToJsonFile(QString fileName, int iSource);
  // load
  bool readSourceFromJson(int iSource, QJsonObject &json);
  bool readFromJson(QJsonObject &json); // all config
  bool LoadSourceFromJsonFile(QString fileName, int iSource);
  bool LoadGunEnergySpectrum(int iSource, int iParticle, QString fileName);

  TVector3 GenerateRandomDirection();

public slots:
  void onIsParticleInUse(int particleId, bool& fAnswer, QString* SourceName);
  void onRequestRegisterParticleRemove(int particleId); //should NOT be used to remove one of particles in use! use onIspareticleInUse first

signals:
  void RequestUpdateSourcesInConfig(QJsonObject &sourcesJson);

private:  
  //external resources - pointers
  const DetectorClass* Detector;
  AMaterialParticleCollection* MpCollection;
  TRandom2 *RandGen;

  TString NameID; //use to make unique hist name in multithread environment /// - actually not needed anymore!

  QVector<ParticleSourceStructure*> ParticleSourcesData;
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
