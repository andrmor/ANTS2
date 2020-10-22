#include "ajavascriptmanager_qml.h"
#include "ascriptinterface.h"
#include "acore_si.h"
#include "amath_si.h"

#include <QDebug>
#include <QJSEngine>
#include <QElapsedTimer>
#include <QJsonDocument>

AJavaScriptmanager_qml::AJavaScriptmanager_qml(TRandom2 * RandGen) :
    AScriptManager(RandGen), engine(new QJSEngine()) {}

AJavaScriptmanager_qml::~AJavaScriptmanager_qml()
{
    delete engine;
}

void AJavaScriptmanager_qml::RegisterInterfaceAsGlobal(AScriptInterface *interface)
{

}

void AJavaScriptmanager_qml::RegisterCoreInterfaces(bool bCore, bool bMath)
{
    if (bCore)
    {
        coreObj = new ACore_SI(this);
        QJSValue coreVal = engine->newQObject(coreObj);
        engine->globalObject().setProperty("core", coreVal);
        doRegister(coreObj, "core");
    }

    if (bMath)
    {
        AMath_SI * mathObj = new AMath_SI(RandGen);
        QJSValue mathVal = engine->newQObject(mathObj);
        engine->globalObject().setProperty("math", mathVal);
        doRegister(mathObj, "math");
    }
}

void AJavaScriptmanager_qml::RegisterInterface(AScriptInterface * interface, const QString & name)
{
    //QScriptValue obj = engine->newQObject(interface, QScriptEngine::QtOwnership);
    //engine->globalObject().setProperty(name, obj);

    QJSValue obj = engine->newQObject(interface);
    engine->globalObject().setProperty(name, obj);

    doRegister(interface, name);
}

QString AJavaScriptmanager_qml::Evaluate(const QString & Script)
{
    LastError.clear();
    LastErrorLineNumber = -1;
    fAborted = false;

    bScriptExpanded = false;
    QString ExpandedScript;
    bool bOK = expandScript(Script, ExpandedScript);
    if (!bOK) return LastError;

    emit onStart();

    //running InitOnRun method (if defined) for all defined interfaces
    for (int i=0; i<interfaces.size(); i++)
    {
          AScriptInterface* bi = dynamic_cast<AScriptInterface*>(interfaces[i]);
          if (bi)
          {
              if (!bi->InitOnRun())
              {
                  LastError = "Init failed for unit: "+interfaces.at(i)->objectName();
                  return LastError;
              }
          }
    }

    timer = new QElapsedTimer;
    timeOfStart = timer->restart();

    fEngineIsRunning = true;
    EvaluationResult = engine->evaluate(ExpandedScript);
    fEngineIsRunning = false;

    timerEvalTookMs = timer->elapsed();
    delete timer; timer = nullptr;

    QString result;

    QVariant var = EvaluationResult.toVariant();
    if (var.type() == QVariant::Map || var.type() == QVariant::List)
    {
        QJsonDocument doc = QJsonDocument::fromVariant(var);
        result = doc.toJson(QJsonDocument::Compact);
    }
    else if (var.type() == QVariant::String) result = "\"" + var.toString() + "\"";
    else result = var.toString();

    emit onFinish(result);
    return result;
}

QVariant AJavaScriptmanager_qml::EvaluateScriptInScript(const QString &script)
{
    /*
    int line = FindSyntaxError(script);
    if (line == -1)
      {
          QScriptValue val = engine->evaluate(script);
          return val.toVariant();
      }

    AbortEvaluation("Syntax error in script provided to core.evaluate()");
    */
    return 0;
}

void AJavaScriptmanager_qml::abortEvaluation()
{
    //engine->setInterrupted(true);   //requires Qt 5.14
}

void AJavaScriptmanager_qml::collectGarbage()
{
    engine->collectGarbage();
}

bool AJavaScriptmanager_qml::isUncaughtException() const
{
    return false;
}

int AJavaScriptmanager_qml::getUncaughtExceptionLineNumber() const
{
    return -1;
}

QString AJavaScriptmanager_qml::getUncaughtExceptionString() const
{
    return "test";
}

void AJavaScriptmanager_qml::doRegister(AScriptInterface *interface, const QString &name)
{
    interface->setObjectName(name);
    interfaces.append(interface);
    QObject::connect(interface, &AScriptInterface::AbortScriptEvaluation, this, &AJavaScriptmanager_qml::AbortEvaluation);
}
