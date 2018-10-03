#include "atracerstateful.h"
#include "amaterialparticlecolection.h"
#include "aopticaloverridescriptinterface.h"
#include "amathscriptinterface.h"

#include <QObject>
#include <QScriptEngine>
#include <QDebug>

#include "TRandom2.h"

ATracerStateful::~ATracerStateful()
{
    delete ScriptEngine; ScriptEngine = 0;
    if (overrideInterface)
    {
        qDebug() << "Deleting ov script interface";
        delete overrideInterface;
    }
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

    overrideInterface = new AOpticalOverrideScriptInterface(MPcollection);
    qDebug() << "Created interface object:"<<overrideInterface;
    overrideInterface->setObjectName("photon");
    QScriptValue val = ScriptEngine->newQObject(overrideInterface, QScriptEngine::QtOwnership);
    ScriptEngine->globalObject().setProperty(overrideInterface->objectName(), val);

    mathInterface = new AMathScriptInterface(RandGen);
    mathInterface->setObjectName("math");
    val = ScriptEngine->newQObject(mathInterface, QScriptEngine::QtOwnership);
    ScriptEngine->globalObject().setProperty(mathInterface->objectName(), val);
}
