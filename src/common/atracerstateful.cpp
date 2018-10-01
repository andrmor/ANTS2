#include "atracerstateful.h"
#include "amaterialparticlecolection.h"
//#include "coreinterfaces.h"

#include <QObject>
#include <QScriptEngine>
#include <QDebug>

#include "TRandom2.h"

ATracerStateful::~ATracerStateful()
{
    delete ScriptEngine; ScriptEngine = 0;
    qDebug() << "Deleting" << interfaces.size() << "ov script interface(s)";
    for (QObject* o : interfaces) delete o;
    interfaces.clear();
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

void ATracerStateful::registerInterfaceObject(QObject *interfaceObj)
{
    if (!ScriptEngine)
    {
        qDebug() << "Creating script engine";
        ScriptEngine = new QScriptEngine();
    }

    QScriptValue val = ScriptEngine->newQObject(interfaceObj, QScriptEngine::QtOwnership);
    ScriptEngine->globalObject().setProperty(interfaceObj->objectName(), val);
    interfaces << interfaceObj;
}
