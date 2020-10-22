#ifndef ASOURCEPARTICLEGENERATOR_H
#define ASOURCEPARTICLEGENERATOR_H

#include "aparticlegun.h"

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

    bool Init() override; // !!! has to be called before the first use of GenerateEvent()!
    bool GenerateEvent(QVector<AParticleRecord*> & GeneratedParticles, int iEvent) override; //see Init!!!  // !*! fix use of detector

private:
    const ASourceGenSettings & Settings;
    const DetectorClass      & Detector;
    TRandom2                 & RandGen;

    //full recipe of emission builder (containes particles linked to particles etc up to the top level individual particle)
    QVector< QVector< QVector<ALinkedParticle> > > LinkedPartiles; //[isource] [iparticle] []  (includes the record of the particle iteslf!!!)

    QVector<double>   TotalParticleWeight;
    QVector<TVector3> CollimationDirection;   //[isource] collimation direction
    QVector<double>   CollimationProbability; //[isource] collimation probability: solid angle inside cone / 4Pi

    void generatePosition(int isource, double *R) const;
    void addParticleInCone(int isource, int iparticle, QVector<AParticleRecord*> & GeneratedParticles) const; //QVector - only pointer is transferred!
};

#endif // ASOURCEPARTICLEGENERATOR_H
