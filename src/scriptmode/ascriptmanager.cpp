#include "ascriptmanager.h"
#include "ainterfacetomessagewindow.h"
#include "coreinterfaces.h"

#include <QScriptEngine>
#include <QMetaMethod>
#include <QDebug>

AScriptManager::AScriptManager(TRandom2* RandGen) :
    RandGen(RandGen)
{
    engine = new QScriptEngine();
    engine->setProcessEventsInterval(200);

    fEngineIsRunning = false;
    MiniBestResult = 1e30;
    MiniNumVariables = 0;
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
                  LastError = "Init failed for unit: "+interfaces.at(i)->objectName();
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

QScriptValue AScriptManager::EvaluateScriptInScript(const QString &script)
{
    int line = FindSyntaxError(script);
    if (line == -1)
      return engine->evaluate(script);

    AbortEvaluation("Syntax error in script provided to core.evaluate()");
    return QScriptValue();
}

bool AScriptManager::isUncaughtException() const
{
    return engine->hasUncaughtException();
}

int AScriptManager::getUncaughtExceptionLineNumber() const
{
    return engine->uncaughtExceptionLineNumber();
}

const QString AScriptManager::getUncaughtExceptionString() const
{
    return engine->uncaughtException().toString();
}

const QString AScriptManager::getFunctionReturnType(const QString& UnitFunction)
{
    QStringList f = UnitFunction.split(".");
    if (f.size() != 2) return "";

    QString unit = f.first();
    int unitIndex = -1;
    for (int i=0; i<interfaces.size(); i++)
        if (interfaces.at(i)->objectName() == unit)
        {
            unitIndex = i;
            break;
        }
    if (unitIndex == -1) return "";

    //qDebug() << "Found unit"<<unit<<" with index"<<unitIndex;
    QString met = f.last();
    //qDebug() << met;
    QStringList skob = met.split("(", QString::SkipEmptyParts);
    if (skob.size()<2) return "";
    QString funct = skob.first();
    QString args = skob[1];
    args.chop(1);
    //qDebug() << funct << args;

    QString insert;
    if (!args.isEmpty())
      {
        QStringList argl = args.split(",");
        for (int i=0; i<argl.size(); i++)
          {
            QStringList a = argl.at(i).simplified().split(" ");
            if (!insert.isEmpty()) insert += ",";
            insert += a.first();
          }
      }
    //qDebug() << insert;

    QString methodName = funct + "(" + insert + ")";
    //qDebug() << "method name" << methodName;
    int mi = interfaces.at(unitIndex)->metaObject()->indexOfMethod(methodName.toLatin1().data());
    //qDebug() << "method index:"<<mi;
    if (mi == -1) return "";

    QString returnType = interfaces.at(unitIndex)->metaObject()->method(mi).typeName();
    //qDebug() << returnType;
    return returnType;
}

void AScriptManager::collectGarbage()
{
    engine->collectGarbage();
}

QScriptValue AScriptManager::getMinimalizationFunction()
{
    return engine->globalObject().property(MiniFunctionName);
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
        coreObj->setObjectName(coreName);
        engine->globalObject().setProperty(coreName, coreVal);
        interfaces.append(coreObj);
        //registering math module
        QObject* mathObj = new AInterfaceToMath(RandGen);
        QScriptValue mathVal = engine->newQObject(mathObj, QScriptEngine::QtOwnership);
        QString mathName = "math";
        mathObj->setObjectName(mathName);
        engine->globalObject().setProperty(mathName, mathVal);
        interfaces.append(mathObj);  //SERVICE OBJECT IS FIRST in interfaces!
      }
    else
      { // name is not empty - this is one of the secondary modules
        engine->globalObject().setProperty(name, obj);
      }

    if (interfaceObject)
      {
        interfaceObject->setObjectName(name);
        interfaces.append(interfaceObject);

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
