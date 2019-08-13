#include "atracerstateful.h"
#include "amaterialparticlecolection.h"
#include "aopticaloverridescriptinterface.h"
#include "amath_si.h"

#include <QObject>
#include <QScriptEngine>
#include <QDebug>

#include "TRandom2.h"

ATracerStateful::ATracerStateful(TRandom2 *RandGen) : RandGen(RandGen) {}

ATracerStateful::~ATracerStateful()
{
    delete ScriptEngine; ScriptEngine = 0;
    delete overrideInterface; overrideInterface = 0;
    delete mathInterface; mathInterface = 0;
}

void ATracerStateful::evaluateScript(const QString &Script)
{
        //qDebug() << "Script:"<<Script;
        //QScriptValue res =
    ScriptEngine->evaluate(Script);
        //qDebug() << "eval result:" << res.toString();
}

void ATracerStateful::generateScriptInfrastructureIfNeeded(const AMaterialParticleCollection *MPcollection)
{
    bool bInUse = MPcollection->isScriptOpticalOverrideDefined();

    if (bInUse) generateScriptInfrastructure(MPcollection);
}

void ATracerStateful::generateScriptInfrastructure(const AMaterialParticleCollection *MPcollection)
{
    qDebug() << "Creating script engine";
    ScriptEngine = new QScriptEngine();

    overrideInterface = new AOpticalOverrideScriptInterface(MPcollection, RandGen);
    qDebug() << "Created interface object:"<<overrideInterface;
    overrideInterface->setObjectName("photon");
    QScriptValue val = ScriptEngine->newQObject(overrideInterface, QScriptEngine::QtOwnership);
    ScriptEngine->globalObject().setProperty(overrideInterface->objectName(), val);

    QObject::connect(overrideInterface, &AOpticalOverrideScriptInterface::requestAbort, ScriptEngine, &QScriptEngine::abortEvaluation, Qt::DirectConnection);

    mathInterface = new AMath_SI(RandGen);
    mathInterface->setObjectName("math");
    val = ScriptEngine->newQObject(mathInterface, QScriptEngine::QtOwnership);
    ScriptEngine->globalObject().setProperty(mathInterface->objectName(), val);
}

void ATracerStateful::abort()
{
    if (ScriptEngine)
        ScriptEngine->abortEvaluation();
}
