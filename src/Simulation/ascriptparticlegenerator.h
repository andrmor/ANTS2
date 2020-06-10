#ifndef ASCRIPTPARTICLEGENERATOR_H
#define ASCRIPTPARTICLEGENERATOR_H

#include "aparticlegun.h"

#include <QString>

class AMaterialParticleCollection;
class TRandom2;
class QScriptEngine;
class AParticleGenerator_SI;
class AMath_SI;

class AScriptParticleGenerator : public AParticleGun
{
public:
    AScriptParticleGenerator(const AMaterialParticleCollection& MpCollection, TRandom2 *RandGen, int ThreadID, const int * NumRunningThreads);
    virtual ~AScriptParticleGenerator();

    void SetScript(const QString& script) {Script = script;}
    const QString& GetScript() const {return Script;}

    virtual bool Init() override;               //called before first use
    //virtual void ReleaseResources() override {}   //called after end of operation
    virtual bool GenerateEvent(QVector<AParticleRecord*> & GeneratedParticles, int iEvent) override;

    virtual void writeToJson(QJsonObject& json) const override;
    virtual bool readFromJson(const QJsonObject& json) override;

    void SetProcessInterval(int msOrMinus1) {processInterval = msOrMinus1;}

public slots:
    virtual void abort() override;

private:
    const AMaterialParticleCollection & MpCollection;
    TRandom2 * RandGen;
    int ThreadId = 0;
    const int * NumRunningThreads;

    QString Script;
    QScriptEngine * ScriptEngine = 0;  // creates only on Init - only if from script mode was selected!
                                       //TODO make external (e.g. Simulator class can host it for sim)
    AParticleGenerator_SI * ScriptInterface = 0; // creates only on Init - only if from script mode was selected!
    AMath_SI * mathInterface = 0; // creates only on Init - only if from script mode was selected!

    int processInterval = -1; //ms; if -1 processing of events during evaluation is disabled
};

#endif // ASCRIPTPARTICLEGENERATOR_H
