#ifndef ASCRIPTPARTICLEGENERATOR_H
#define ASCRIPTPARTICLEGENERATOR_H

#include "aparticlegun.h"

#include <QString>

class AScriptGenSettings;
class AMaterialParticleCollection;
class TRandom2;
class QScriptEngine;
class AParticleGenerator_SI;
class AMath_SI;

class AScriptParticleGenerator : public AParticleGun
{
public:
    AScriptParticleGenerator(const AScriptGenSettings & Settings, const AMaterialParticleCollection & MpCollection, TRandom2 & RandGen, int ThreadID, const int * NumRunningThreads);
    virtual ~AScriptParticleGenerator();

    virtual bool Init() override;                   //called before first use
    //virtual void ReleaseResources() override {}   //called after end of operation
    virtual bool GenerateEvent(QVector<AParticleRecord*> & GeneratedParticles, int iEvent) override;

    virtual void writeToJson(QJsonObject& json) const override;
    virtual bool readFromJson(const QJsonObject& json) override;

    void SetProcessInterval(int msOrMinus1) {processInterval = msOrMinus1;}

public slots:
    virtual void abort() override;

private:
    const AScriptGenSettings          & Settings;
    const AMaterialParticleCollection & MpCollection;
    TRandom2                          & RandGen;

    int ThreadId = 0;
    const int * NumRunningThreads;

    //QString Script;
    QScriptEngine         * ScriptEngine    = nullptr;  // creates only on Init - only if from script mode was selected!
                                                        //TODO make external (e.g. Simulator class can host it for sim)
    AParticleGenerator_SI * ScriptInterface = nullptr;  // creates only on Init - only if from script mode was selected!
    AMath_SI              * mathInterface   = nullptr;  // creates only on Init - only if from script mode was selected!

    int processInterval = -1; //ms; if -1 processing of events during evaluation is disabled
};

#endif // ASCRIPTPARTICLEGENERATOR_H
