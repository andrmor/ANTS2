#include "ascriptparticlegenerator.h"
#include "aparticlerecord.h"
#include "ajsontools.h"
#include "aparticlegeneratorinterface.h"
#include "amathscriptinterface.h"

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
    qDebug() << "Init script particle ghenerator";

    if (!ScriptEngine)
    {
        qDebug() << "Creating script infrastructure";
        ScriptEngine = new QScriptEngine();
        ScriptInterface = new AParticleGeneratorInterface(MpCollection, RandGen);

        ScriptInterface->setObjectName("gen");
        QScriptValue val = ScriptEngine->newQObject(ScriptInterface, QScriptEngine::QtOwnership);
        ScriptEngine->globalObject().setProperty(ScriptInterface->objectName(), val);

        //QObject::connect(ScriptInterface, &AParticleGeneratorInterface::requestAbort, ScriptEngine, &QScriptEngine::abortEvaluation, Qt::DirectConnection);

        mathInterface = new AMathScriptInterface(RandGen);
        mathInterface->setObjectName("math");
        val = ScriptEngine->newQObject(mathInterface, QScriptEngine::QtOwnership);
        ScriptEngine->globalObject().setProperty(mathInterface->objectName(), val);
    }

    return true;
}

void AScriptParticleGenerator::GenerateEvent(QVector<AParticleRecord*> & GeneratedParticles)
{
    ScriptInterface->configure(&GeneratedParticles);
    ScriptEngine->evaluate(Script).toString();
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
