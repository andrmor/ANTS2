#ifndef APARTICLEGUN_H
#define APARTICLEGUN_H

#include <QVector>

class AGeneratedParticle
{
public:
    int    ParticleId;
    double Energy;
    double Position[3];
    double Direction[3];
};

class AParticleGun
{
public:
    virtual ~AParticleGun(){}

    virtual void Init() {}               //called before first use
    virtual void ReleaseResources() {}   //called after end of operation
    virtual QVector<AGeneratedParticle>* GenerateEvent() = 0;

    virtual bool CheckConfiguration() {return true;} //check consistency of the configuration

    virtual void RemoveParticle(int particleId) = 0; //should NOT be used to remove one of particles in use! use onIsPareticleInUse first
    virtual bool IsParticleInUse(int particleId, QString& SourceNames) const = 0;
};

#endif // APARTICLEGUN_H
