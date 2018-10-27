#ifndef PARTICLESOURCESCLASS_H
#define PARTICLESOURCESCLASS_H

#include "aparticlegun.h"

#include <QString>
#include <QVector>
#include "TVector3.h"

class AMaterialParticleCollection;
class QJsonObject;
class TRandom2;
class DetectorClass;
struct AParticleSourceRecord;

class LinkedParticleStructure
{
public:
  int iParticle; //indexed according to GunParticles index
  int LinkedTo;  //index of particle it is linked to

  LinkedParticleStructure() {}
  LinkedParticleStructure(int iparticle, int linkedto = -1) {iParticle = iparticle; LinkedTo = linkedto;}
};

class ParticleSourcesClass : public AParticleGun
{
public:
    ParticleSourcesClass(const DetectorClass* Detector, TRandom2* RandGen);
    ~ParticleSourcesClass();

    virtual bool Init() override; // !!! has to be called before the first use of "GenerateEvent"!
    virtual QVector<AParticleRecord>* GenerateEvent() override; //see Init!!!

    //triggered when remove particle from configuration is attempted
    virtual bool IsParticleInUse(int particleId, QString& SourceNames) const override;
    virtual void RemoveParticle(int particleId) override; //should NOT be used to remove one of particles in use! use onIsPareticleInUse first

    virtual const QString CheckConfiguration() const;

    virtual void writeToJson(QJsonObject &json) const override;
    virtual bool readFromJson(const QJsonObject &json) override;

    //requests
    int    countSources() const {return ParticleSourcesData.size();}
    double getTotalActivity();
    AParticleSourceRecord* getSource(int iSource) {return ParticleSourcesData[iSource];}
    void   append(AParticleSourceRecord* gunParticle);
    void   forget(AParticleSourceRecord* gunParticle);
    bool   replace(int iSource, AParticleSourceRecord* gunParticle);
    void   remove(int iSource);
    void   clear();

    bool   LoadGunEnergySpectrum(int iSource, int iParticle, QString fileName); //TODO uses load function with message

    TVector3 GenerateRandomDirection();
    void checkLimitedToMaterial(AParticleSourceRecord *s);

private:  
    const DetectorClass* Detector;             //external
    AMaterialParticleCollection* MpCollection; //external
    TRandom2 *RandGen;                         //external

    QVector<AParticleSourceRecord*> ParticleSourcesData;
    QVector<double> TotalParticleWeight;
    double TotalActivity = 0;
    QVector< QVector< QVector<LinkedParticleStructure> > > LinkedPartiles; //[isource] [iparticle] []  (includes the record of the particle iteslf!!!)
                              //full recipe of emission builder (containes particles linked to particles etc up to the top level individual particle)

    QVector<TVector3> CollimationDirection; //[isource] collimation direction
    QVector<double> CollimationProbability; //[isource] collimation probability: solid angle inside cone / 4Pi

    //utilities
    void CalculateTotalActivity();
    void GeneratePosition(int isource, double *R) const;
    void AddParticleInCone(int isource, int iparticle, QVector<AParticleRecord> *GeneratedParticles) const; //QVector - only pointer is transferred!
};

#endif // PARTICLESOURCESCLASS_H
