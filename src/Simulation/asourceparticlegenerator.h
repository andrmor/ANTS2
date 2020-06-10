#ifndef ASOURCEPARTICLEGENERATOR_H
#define ASOURCEPARTICLEGENERATOR_H

#include "aparticlegun.h"

#include <QString>
#include <QVector>

#include "TVector3.h"

class  ASourceGenSettings;
class  AMaterialParticleCollection;
class  QJsonObject;
class  TRandom2;
class  DetectorClass;
struct AParticleSourceRecord;

class ALinkedParticle
{
public:
    int iParticle; //indexed according to GunParticles index
    int LinkedTo;  //index of particle it is linked to

    ALinkedParticle() {}
    ALinkedParticle(int iparticle, int linkedto = -1) {iParticle = iparticle; LinkedTo = linkedto;}
};

class ASourceParticleGenerator : public AParticleGun
{
public:
    ASourceParticleGenerator(const ASourceGenSettings & Settings, const DetectorClass & Detector, TRandom2 & RandGen);

    virtual bool Init() override; // !!! has to be called before the first use of "GenerateEvent"!
    virtual bool GenerateEvent(QVector<AParticleRecord*> & GeneratedParticles, int iEvent) override; //see Init!!!  // !*! fix use of detector

    //triggered when remove particle from configuration is attempted
    //virtual bool IsParticleInUse(int particleId, QString& SourceNames) const override;
    //virtual void RemoveParticle(int particleId) override; //should NOT be used to remove one of particles in use! use onIsPareticleInUse first

    virtual void writeToJson(QJsonObject &json) const override;
    virtual bool readFromJson(const QJsonObject &json) override;

    //requests
    //int    countSources() const;  // !*!
    //AParticleSourceRecord* getSource(int iSource); // !*!

    //bool   LoadGunEnergySpectrum(int iSource, int iParticle, QString fileName); //TODO uses load function with message

    //TVector3 GenerateRandomDirection();

private:
    const ASourceGenSettings & Settings;
    const DetectorClass      & Detector;
    TRandom2                 & RandGen;

    //QVector<AParticleSourceRecord*> ParticleSourcesData;
    QVector<double> TotalParticleWeight;
    QVector< QVector< QVector<ALinkedParticle> > > LinkedPartiles; //[isource] [iparticle] []  (includes the record of the particle iteslf!!!)
                              //full recipe of emission builder (containes particles linked to particles etc up to the top level individual particle)

    QVector<TVector3> CollimationDirection; //[isource] collimation direction
    QVector<double> CollimationProbability; //[isource] collimation probability: solid angle inside cone / 4Pi

    //utilities
    void generatePosition(int isource, double *R) const;
    void addParticleInCone(int isource, int iparticle, QVector<AParticleRecord*> & GeneratedParticles) const; //QVector - only pointer is transferred!
};

#endif // ASOURCEPARTICLEGENERATOR_H
