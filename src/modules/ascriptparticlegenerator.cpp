#include "ascriptparticlegenerator.h"
#include "aparticlerecord.h"
#include "ajsontools.h"
#include "aparticlegenerator_si.h"
#include "amath_si.h"

#include <QScriptEngine>
#include <QDebug>

AScriptParticleGenerator::AScriptParticleGenerator(const AMaterialParticleCollection &MpCollection, TRandom2 * RandGen) :
    MpCollection(MpCollection), RandGen(RandGen) {}

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
        ScriptInterface = new AParticleGenerator_SI(MpCollection, RandGen);

        ScriptInterface->setObjectName("gen");
        QScriptValue val = ScriptEngine->newQObject(ScriptInterface, QScriptEngine::QtOwnership);
        ScriptEngine->globalObject().setProperty(ScriptInterface->objectName(), val);

        //QObject::connect(ScriptInterface, &AParticleGeneratorInterface::requestAbort, ScriptEngine, &QScriptEngine::abortEvaluation, Qt::DirectConnection);

        mathInterface = new AMath_SI(RandGen);
        mathInterface->setObjectName("math");
        val = ScriptEngine->newQObject(mathInterface, QScriptEngine::QtOwnership);
        ScriptEngine->globalObject().setProperty(mathInterface->objectName(), val);
    }

    //TODO: check script syntax
    //TODO: check particles

    return true;
}

bool AScriptParticleGenerator::GenerateEvent(QVector<AParticleRecord*> & GeneratedParticles)
{
    bAbortRequested = false;
    ScriptInterface->configure(&GeneratedParticles);
    ScriptEngine->evaluate(Script).toString();
    return !bAbortRequested;
}

bool AScriptParticleGenerator::IsParticleInUse(int particleId, QString &SourceNames) const
{
    return false; //TODO
}

void AScriptParticleGenerator::writeToJson(QJsonObject &json) const
{
    json["Script"] = Script;
}

bool AScriptParticleGenerator::readFromJson(const QJsonObject &json)
{
    return parseJson(json, "Script", Script);
}

void AScriptParticleGenerator::abort()
{
    bAbortRequested = true;
    if (ScriptEngine) ScriptEngine->abortEvaluation();
}
