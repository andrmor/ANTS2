#include "ascriptparticlegenerator.h"
#include "aparticlesimsettings.h"
#include "aparticlerecord.h"
#include "ajsontools.h"
#include "aparticlegenerator_si.h"
#include "amath_si.h"

#include <QScriptEngine>
#include <QDebug>

AScriptParticleGenerator::AScriptParticleGenerator(const AScriptGenSettings & Settings, const AMaterialParticleCollection & MpCollection, TRandom2 & RandGen) : //, int ThreadID, const int * NumRunningThreads) :
    Settings(Settings), MpCollection(MpCollection), RandGen(RandGen) {}//, ThreadId(ThreadID), NumRunningThreads(NumRunningThreads) {}

AScriptParticleGenerator::~AScriptParticleGenerator()
{
    delete ScriptInterface;
    delete ScriptEngine;
}

bool AScriptParticleGenerator::Init()
{
    //qDebug() << "Init script particle ghenerator";

    if (!ScriptEngine)
    {
            //qDebug() << "Creating script infrastructure";
        ScriptEngine = new QScriptEngine();
        ScriptEngine->setProcessEventsInterval(processInterval);
        ScriptInterface = new AParticleGenerator_SI(MpCollection, &RandGen); //, ThreadId, NumRunningThreads);

        ScriptInterface->setObjectName("gen");
        QScriptValue val = ScriptEngine->newQObject(ScriptInterface, QScriptEngine::QtOwnership);
        ScriptEngine->globalObject().setProperty(ScriptInterface->objectName(), val);

        //QObject::connect(ScriptInterface, &AParticleGeneratorInterface::requestAbort, ScriptEngine, &QScriptEngine::abortEvaluation, Qt::DirectConnection);

        mathInterface = new AMath_SI(&RandGen);
        mathInterface->setObjectName("math");
        val = ScriptEngine->newQObject(mathInterface, QScriptEngine::QtOwnership);
        ScriptEngine->globalObject().setProperty(mathInterface->objectName(), val);
    }

    //TODO: check script syntax
    //TODO: check particles

    return true;
}

bool AScriptParticleGenerator::GenerateEvent(QVector<AParticleRecord*> & GeneratedParticles, int iEvent)
{
    bAbortRequested = false;
    ScriptInterface->configure(&GeneratedParticles, iEvent);
    ScriptEngine->evaluate(Settings.Script).toString();
    return !bAbortRequested;
}

void AScriptParticleGenerator::abort()
{
    bAbortRequested = true;
    if (ScriptEngine) ScriptEngine->abortEvaluation();
}
