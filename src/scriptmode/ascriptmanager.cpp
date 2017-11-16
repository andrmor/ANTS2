#include "ascriptmanager.h"
#include "ainterfacetomessagewindow.h"
#include "coreinterfaces.h"

#include <QScriptEngine>
#include <QDebug>

AScriptManager::AScriptManager(TRandom2* RandGen) :
    RandGen(RandGen)
{
    engine = new QScriptEngine();
    engine->setProcessEventsInterval(200);

    fEngineIsRunning = false;
    bestResult = 1e30;
    numVariables = 0;
}

AScriptManager::~AScriptManager()
{
    for (int i=0; i<interfaces.size(); i++) delete interfaces[i];
    interfaces.clear();

    if (engine)
    {
        engine->abortEvaluation();
        engine->deleteLater();
        engine = 0;
    }
}

QString AScriptManager::Evaluate(QString Script)
{
    LastError = "";
    fAborted = false;

    emit onStart();

    //running InitOnRun method (if defined) for all defined interfaces
    for (int i=0; i<interfaces.size(); i++)
      {
          AScriptInterface* bi = dynamic_cast<AScriptInterface*>(interfaces[i]);
          if (bi)
            {
              if (!bi->InitOnRun())
                {
                  LastError = "Init failed for unit: "+interfaceNames.at(i);
                  return LastError;
                }
            }
      }

    fEngineIsRunning = true;
    QScriptValue scriptreturn = engine->evaluate(Script);
    fEngineIsRunning = false;

    QString result = scriptreturn.toString();
    emit success(result); //bad name here :) could be not successful at all, but still want to trigger

    return result;
}

void AScriptManager::CollectGarbage()
{
    engine->collectGarbage();
}

void AScriptManager::AbortEvaluation(QString message)
{
    //qDebug() << "ScriptManager: Abort requested!"<<fAborted<<fEngineIsRunning;

    if (fAborted || !fEngineIsRunning) return;
    fAborted = true;

    engine->abortEvaluation();
    fEngineIsRunning = false;

    // going through registered units and requesting abort
    for (int i=0; i<interfaces.size(); i++)
      {
        AScriptInterface* bi = dynamic_cast<AScriptInterface*>(interfaces[i]);
        if (bi) bi->ForceStop();
      }

    message = "<font color=\"red\">"+ message +"</font><br>";
    emit showMessage(message);
    emit onAbort();
}

void AScriptManager::SetInterfaceObject(QObject *interfaceObject, QString name)
{
    //qDebug() << "Registering:" << interfaceObject << name;
    QScriptValue obj = ( interfaceObject ? engine->newQObject(interfaceObject, QScriptEngine::QtOwnership) : QScriptValue() );

    if (name.isEmpty())
      { // empty name means the main module
        if (interfaceObject)
           engine->setGlobalObject(obj); //do not replace the global object for global script - in effect (non zero pointer) only for local scripts
        // registering service object
        QObject* coreObj = new AInterfaceToCore(this);
        QScriptValue coreVal = engine->newQObject(coreObj, QScriptEngine::QtOwnership);
        QString coreName = "core";
        engine->globalObject().setProperty(coreName, coreVal);
        interfaces.append(coreObj);  //CORE OBJECT IS FIRST in interfaces!
        interfaceNames.append(coreName);
        //registering math module
        QObject* mathObj = new AInterfaceToMath(RandGen);
        QScriptValue mathVal = engine->newQObject(mathObj, QScriptEngine::QtOwnership);
        QString mathName = "math";
        engine->globalObject().setProperty(mathName, mathVal);
        interfaces.append(mathObj);  //SERVICE OBJECT IS FIRST in interfaces!
        interfaceNames.append(mathName);
      }
    else
      { // name is not empty - this is one of the secondary modules
        engine->globalObject().setProperty(name, obj);
      }

    if (interfaceObject)
      {
        interfaces.append(interfaceObject);
        interfaceNames.append(name);

        //connecting abort request from main interface to serviceObj
        int index = interfaceObject->metaObject()->indexOfSignal("AbortScriptEvaluation(QString)");
        if (index != -1)
            QObject::connect(interfaceObject, "2AbortScriptEvaluation(QString)", this, SLOT(AbortEvaluation(QString)));  //1-slot, 2-signal
      }
}

int AScriptManager::FindSyntaxError(QString script)
{
    QScriptSyntaxCheckResult check = QScriptEngine::checkSyntax(script);
    if (check.state() == QScriptSyntaxCheckResult::Valid) return -1;
    else
      {
        int lineNumber = check.errorLineNumber();
        qDebug()<<"Syntax error at line"<<lineNumber;
        return lineNumber;
      }
}

void AScriptManager::deleteMsgDialog()
{
    for (int i=0; i<interfaces.size(); i++)
    {
        AInterfaceToMessageWindow* t = dynamic_cast<AInterfaceToMessageWindow*>(interfaces[i]);
        if (t)
        {
            t->deleteDialog();
            return;
        }
    }
}

void AScriptManager::hideMsgDialog()
{
    for (int i=0; i<interfaces.size(); i++)
    {
        AInterfaceToMessageWindow* t = dynamic_cast<AInterfaceToMessageWindow*>(interfaces[i]);
        if (t)
        {
            t->hide();
            return;
        }
    }
}

void AScriptManager::restoreMsgDialog()
{
    for (int i=0; i<interfaces.size(); i++)
    {
        AInterfaceToMessageWindow* t = dynamic_cast<AInterfaceToMessageWindow*>(interfaces[i]);
        if (t)
        {
            if (t->isActive()) t->restore();
            return;
        }
    }
}
