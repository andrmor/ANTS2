#ifndef ASCRIPTPARTICLEGENERATOR_H
#define ASCRIPTPARTICLEGENERATOR_H

#include "aparticlegun.h"

#include <QString>

class AMaterialParticleCollection;
class TRandom2;
class QScriptEngine;
class AParticleGeneratorInterface;
class AMathScriptInterface;

class AScriptParticleGenerator : public AParticleGun
{
public:
    AScriptParticleGenerator(const AMaterialParticleCollection& MpCollection, TRandom2 *RandGen);
    virtual ~AScriptParticleGenerator();

    void SetScript(const QString& script) {Script = script;}
    const QString& GetScript() const {return Script;}

    virtual bool Init() override;               //called before first use
    //virtual void ReleaseResources() override {}   //called after end of operation
    virtual bool GenerateEvent(QVector<AParticleRecord*> & GeneratedParticles) override;

    virtual const QString CheckConfiguration() const override {return "";} //check consistency of the configuration

    virtual void RemoveParticle(int) override {} //should NOT be used to remove one of particles in use! use onIsPareticleInUse first
    virtual bool IsParticleInUse(int particleId, QString& SourceNames) const override;

    virtual void writeToJson(QJsonObject& json) const override;
    virtual bool readFromJson(const QJsonObject& json) override;

    void SetProcessInterval(int msOrMinus1) {processInterval = msOrMinus1;}

public slots:
    virtual void abort() override;

private:
    const AMaterialParticleCollection & MpCollection;
    TRandom2 * RandGen;

    QString Script;
    QScriptEngine * ScriptEngine = 0;  // creates only on Init - only if from script mode was selected!
                                       //TODO make external (e.g. Simulator class can host it for sim)
    AParticleGeneratorInterface * ScriptInterface = 0; // creates only on Init - only if from script mode was selected!
    AMathScriptInterface * mathInterface = 0; // creates only on Init - only if from script mode was selected!

    int processInterval = -1; //ms; if -1 processing of events during evaluation is disabled
};

#endif // ASCRIPTPARTICLEGENERATOR_H
