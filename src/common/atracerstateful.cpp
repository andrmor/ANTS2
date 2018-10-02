#include "atracerstateful.h"
#include "amaterialparticlecolection.h"
#include "aopticaloverridescriptinterface.h"

#include <QObject>
#include <QScriptEngine>
#include <QDebug>

#include "TRandom2.h"

ATracerStateful::~ATracerStateful()
{
    delete ScriptEngine; ScriptEngine = 0;
    if (interfaceObject)
    {
        qDebug() << "Deleting ov script interface";
        delete interfaceObject;
    }
}

void ATracerStateful::evaluateScript(const QString &Script)
{
    qDebug() << "Script:"<<Script;
    QScriptValue res = ScriptEngine->evaluate(Script);
    qDebug() << "eval result:" << res.toString();
}

void ATracerStateful::generateScriptInfrastructureIfNeeded(AMaterialParticleCollection *MPcollection)
{
    bool bInUse = MPcollection->isScriptOpticalOverrideDefined();

    if (bInUse)
    {
        qDebug() << "Creating script engine";
        ScriptEngine = new QScriptEngine();

        interfaceObject = new AOpticalOverrideScriptInterface();
        qDebug() << "Created interface object:"<<interfaceObject;
        interfaceObject->setObjectName("photon");
        QScriptValue val = ScriptEngine->newQObject(interfaceObject, QScriptEngine::QtOwnership);
        ScriptEngine->globalObject().setProperty(interfaceObject->objectName(), val);
    }
}

void ATracerStateful::registerInterfaceObject(AOpticalOverrideScriptInterface *interfaceObj)
{

}
