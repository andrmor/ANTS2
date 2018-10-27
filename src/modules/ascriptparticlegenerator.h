#ifndef ASCRIPTPARTICLEGENERATOR_H
#define ASCRIPTPARTICLEGENERATOR_H

#include "aparticlegun.h"

#include <QString>

class AMaterialParticleCollection;

class AScriptParticleGenerator : public AParticleGun
{
public:
    AScriptParticleGenerator(const AMaterialParticleCollection& MpCollection);
    virtual ~AScriptParticleGenerator(){}

    void SetScript(const QString& script) {Script = script;}
    const QString& GetScript() const {return Script;}

    virtual bool Init() override;               //called before first use
    //virtual void ReleaseResources() override {}   //called after end of operation
    virtual void GenerateEvent(QVector<AParticleRecord*> & GeneratedParticles) override;

    virtual const QString CheckConfiguration() const override {return "";} //check consistency of the configuration

    virtual void RemoveParticle(int) override {} //should NOT be used to remove one of particles in use! use onIsPareticleInUse first
    virtual bool IsParticleInUse(int particleId, QString& SourceNames) const override;

    virtual void writeToJson(QJsonObject& json) const override;
    virtual bool readFromJson(const QJsonObject& json) override;

private:
    const AMaterialParticleCollection& MpCollection;

    QString Script;
};

#endif // ASCRIPTPARTICLEGENERATOR_H
