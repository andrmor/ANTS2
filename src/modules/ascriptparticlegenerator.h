#ifndef ASCRIPTPARTICLEGENERATOR_H
#define ASCRIPTPARTICLEGENERATOR_H

#include "aparticlegun.h"

class AScriptParticleGenerator : public AParticleGun
{
public:
    AScriptParticleGenerator();
    virtual ~AScriptParticleGenerator(){}

    virtual bool Init() override;               //called before first use
    //virtual void ReleaseResources() override {}   //called after end of operation
    virtual QVector<AParticleRecord>* GenerateEvent();

   // virtual bool CheckConfiguration() override {return true;} //check consistency of the configuration

    virtual void RemoveParticle(int particleId) override; //should NOT be used to remove one of particles in use! use onIsPareticleInUse first
    virtual bool IsParticleInUse(int particleId, QString& SourceNames) const override;
};

#endif // ASCRIPTPARTICLEGENERATOR_H
