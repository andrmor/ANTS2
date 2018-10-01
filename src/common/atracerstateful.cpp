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

void ATracerStateful::registerAllInterfaceObjects(AMaterialParticleCollection *MPcollection)
{
    MPcollection->registerOpticalOverrideScriptInterfaces(*this);
}

void ATracerStateful::registerInterfaceObject(AOpticalOverrideScriptInterface *interfaceObj)
{
    if (!ScriptEngine)
    {
        qDebug() << "Creating script engine";
        ScriptEngine = new QScriptEngine();
    }

    QScriptValue val = ScriptEngine->newQObject(interfaceObj, QScriptEngine::QtOwnership);
    ScriptEngine->globalObject().setProperty(interfaceObj->objectName(), val);
    interfaceObject = interfaceObj;
}
