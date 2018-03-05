#include "ascriptmanager.h"
#include "ainterfacetomessagewindow.h"
#include "coreinterfaces.h"
#include "ascriptinterfacefactory.h"
#include "ainterfacetomultithread.h"

#include <QScriptEngine>
#include <QMetaMethod>
#include <QDebug>
#include <QElapsedTimer>
#include <QScriptValueIterator>
#include <QDialog>

AScriptManager::AScriptManager(TRandom2* RandGen) :
    RandGen(RandGen)
{
    engine = new QScriptEngine();
    engine->setProcessEventsInterval(200);

    fEngineIsRunning = false;
    fAborted = false;
    MiniBestResult = 1e30;
    MiniNumVariables = 0;

    timer = 0;
    timerEvalTookMs = 0;
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

    delete timer;

    for (AScriptMessengerDialog* d : ThreadMessangerDialogs)
    {
        delete d;
        d = 0;
    }
    ThreadMessangerDialogs.clear();
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

    timer = new QElapsedTimer;
    timeOfStart = timer->restart();

    fEngineIsRunning = true;
    QScriptValue scriptreturn = engine->evaluate(Script);
    fEngineIsRunning = false;

    timerEvalTookMs = timer->elapsed();
    delete timer; timer = 0;

    QString result = scriptreturn.toString();
    emit onFinish(result);

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
    qDebug() << "ScriptManager: Abort requested!"<<fAborted<<fEngineIsRunning;

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

void AScriptManager::hideAllMessengerWidgets()
{
    for (AScriptMessengerDialog* d : ThreadMessangerDialogs)
        if (d) d->HideWidget();
}

void AScriptManager::showAllMessengerWidgets()
{
    for (AScriptMessengerDialog* d : ThreadMessangerDialogs)
        if (d) d->RestoreWidget();
}

void AScriptManager::clearUnusedMsgDialogs()
{
    for (int i=0; i<interfaces.size(); i++)
    {
        AInterfaceToMultiThread* t = dynamic_cast<AInterfaceToMultiThread*>(interfaces[i]);
        if (t)
        {
            int numThreads = t->countAll();
            for (int i=ThreadMessangerDialogs.size()-1; i >= numThreads; i--)
            {
                delete ThreadMessangerDialogs[i];
                ThreadMessangerDialogs.removeAt(i);
            }
        }
    }
}

void AScriptManager::closeAllMsgDialogs()
{
    for (AScriptMessengerDialog* d : ThreadMessangerDialogs)
        if (d) d->Hide();
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
        coreObj = new AInterfaceToCore(this);
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

void AScriptManager::deleteMsgDialogs()
{
    for (int i=0; i<interfaces.size(); i++)
    {
        AInterfaceToMessageWindow* t = dynamic_cast<AInterfaceToMessageWindow*>(interfaces[i]);
        if (t)
        {
            // *** !!! t->deleteDialog(); //need by GenScriptWindow ?
            return;
        }
    }
}

void AScriptManager::hideMsgDialogs()
{
    for (int i=0; i<interfaces.size(); i++)
    {
        AInterfaceToMessageWindow* t = dynamic_cast<AInterfaceToMessageWindow*>(interfaces[i]);
        if (t)  t->HideWidget();
    }

    for (AScriptMessengerDialog* d : ThreadMessangerDialogs)
        if (d) d->HideWidget();
}

void AScriptManager::restoreMsgDialogs()
{
    for (int i=0; i<interfaces.size(); i++)
    {
        AInterfaceToMessageWindow* t = dynamic_cast<AInterfaceToMessageWindow*>(interfaces[i]);
        if (t) t->RestorelWidget();
    }

    for (AScriptMessengerDialog* d : ThreadMessangerDialogs)
        if (d)
            if (d->IsShown()) d->RestoreWidget();
}

// ------------ multithreading -------------
//https://stackoverflow.com/questions/5020459/deep-copy-of-a-qscriptvalue-as-global-object
class ScriptCopier
{
public:
    ScriptCopier(QScriptEngine& toEngine)
        : m_toEngine(toEngine) {}

    QScriptValue copy(const QScriptValue& obj);

    QScriptEngine& m_toEngine;
    QMap<quint64, QScriptValue> copiedObjs;
};

QScriptValue ScriptCopier::copy(const QScriptValue& obj)
{
    QScriptEngine& engine = m_toEngine;

    if (obj.isUndefined()) return QScriptValue(QScriptValue::UndefinedValue);
    if (obj.isNull())      return QScriptValue(QScriptValue::NullValue);
    if (obj.isNumber() || obj.isString() || obj.isBool() || obj.isBoolean() || obj.isVariant())
    {
        //  qDebug() << "variant" << obj.toVariant();
        return engine.newVariant(obj.toVariant());
    }

    // If we've already copied this object, don't copy it again.
    QScriptValue copy;
    if (obj.isObject())
    {
        if (copiedObjs.contains(obj.objectId()))
        {
            //qDebug() << "--------------Already have this obj!";
            return copiedObjs.value(obj.objectId());
        }
        copiedObjs.insert(obj.objectId(), copy);
    }

    if (obj.isQObject())
    {
        copy = engine.newQObject(copy, obj.toQObject());
        copy.setPrototype(this->copy(obj.prototype()));
    }
    else if (obj.isQMetaObject())
    {
        copy = engine.newQMetaObject(obj.toQMetaObject());
    }
    else if (obj.isFunction())
    {
        // Calling .toString() on a pure JS function returns
        // the function's source code.
        // On a native function however toString() returns
        // something like "function() { [native code] }".
        // That's why we do a syntax on the code.

        QString code = obj.toString();
        auto syntaxCheck = engine.checkSyntax(code);

        if (syntaxCheck.state() == syntaxCheck.Valid)
        {
            copy = engine.evaluate(QString() + "(" + code + ")");
        }
        else if (code.contains("[native code]"))
        {
            copy.setData(obj.data());
        }
        else
        {
            // Do error handling…
            qDebug() << "-----------------problem---------------";
        }

    }
    else if (obj.isObject() || obj.isArray())
    {
        if (obj.isObject()) {
            if (obj.scriptClass()) {
                copy = engine.newObject(obj.scriptClass(), this->copy(obj.data()));
            } else {
                copy = engine.newObject();
            }
        } else {
            copy = engine.newArray();
        }
        copy.setPrototype(this->copy(obj.prototype()));

        QScriptValueIterator it(obj);
        while ( it.hasNext())
        {
            it.next();

            const QString& name = it.name();
            const QScriptValue& property = it.value();

            copy.setProperty(name, this->copy(property));
        }
    }
    else
    {
        // Error handling…
        qDebug() << "-----------------problem---------------";
    }

    return copy;
}

AScriptManager *AScriptManager::createNewScriptManager(int threadNumber)
{
    AScriptManager* sm = new AScriptManager(RandGen);  // *** !!! make new RandGen one!!!

    for (QObject* io : interfaces)
    {
        AScriptInterface* si = dynamic_cast<AScriptInterface*>(io);
        if (!si) continue;

        if (!si->IsMultithreadCapable()) continue;

        QObject* copy = AScriptInterfaceFactory::makeCopy(io); //cloning script interfaces
        if (copy)
        {
            //  qDebug() << "Making available for multi-thread use: "<<io->objectName();

            //special for core unit
            AInterfaceToCore* core = dynamic_cast<AInterfaceToCore*>(copy);
            if (core)
            {
                //qDebug() << "--this is core";
                core->SetScriptManager(sm);
            }
            AInterfaceToMinimizerScript* mini = dynamic_cast<AInterfaceToMinimizerScript*>(copy);
            if (mini)
            {
                //qDebug() << "--this is mini";
                mini->SetScriptManager(sm);
            }
            AInterfaceToMessageWindow* msg = dynamic_cast<AInterfaceToMessageWindow*>(copy);
            if (msg)
            {
                //  qDebug() << "Handling messanger widget for thread#"<<threadNumber;
                while (threadNumber > ThreadMessangerDialogs.size() )
                    ThreadMessangerDialogs << 0; // paranoic protection

                bool bIsNew = true;
                if (threadNumber < ThreadMessangerDialogs.size()) //will reuse old one if exists
                {
                    if (ThreadMessangerDialogs[threadNumber])
                    {
                        msg->ReplaceDialogWidget( ThreadMessangerDialogs[threadNumber] );
                        bIsNew = false;
                    }
                    else ThreadMessangerDialogs[threadNumber] = msg->GetDialogWidget();
                }
                else ThreadMessangerDialogs << msg->GetDialogWidget();

                if (bIsNew)
                {
                    msg->SetDialogTitle("Thread #"+QString::number(threadNumber));
                    msg->Move(50 + threadNumber*50, 50 + threadNumber*30);
                }
            }

            // connecting the request for abort script
            AScriptInterface* base = dynamic_cast<AScriptInterface*>(copy);
            if (base) connect(base, &AScriptInterface::AbortScriptEvaluation, coreObj, &AInterfaceToCore::abort);

            sm->SetInterfaceObject(copy, io->objectName());
        }
        else
        {
            qDebug() << "Unknown interface object type for unit" << io->objectName();
        }
    }

    QScriptValue global = engine->globalObject();
    ScriptCopier SC(*sm->engine);
    QScriptValueIterator it(global);
    while (it.hasNext())
    {
        it.next();
        //  qDebug() << it.name() << ": " << it.value().toString();

        if (!sm->engine->globalObject().property(it.name()).isValid()) // if resource with this name does not exist...
        {
            //do not copy QObjects - the multi-thread friendly units were already copied
            if (!it.value().isQObject())
            {
                sm->engine->globalObject().setProperty(it.name(), SC.copy(it.value()));
                //  qDebug() << "Registered:"<<it.name() << "-:->" << sm->engine->globalObject().property(it.name()).toVariant();
            }
            else
            {
                //  qDebug() << "Skipping QObject" << it.name();
            }
        }
    }

    //connect core.print() to the ScriptManager of the GUI thread, as queued!
    connect(sm, &AScriptManager::showMessage, this, &AScriptManager::showMessage, Qt::QueuedConnection);

    //  qDebug() << "  Scriptmanager created!"<<sm;
    return sm;
}

void AScriptManager::abortEvaluation()
{
    qDebug() << "!!!Sending abort command to the engine" ;
    engine->abortEvaluation();
}

QScriptValue AScriptManager::getProperty(const QString &properyName) const
{
    QScriptValue global = engine->globalObject();
    return global.property(properyName);
}

QScriptValue AScriptManager::registerNewVariant(const QVariant& Variant)
{
    return engine->toScriptValue(Variant);
}

qint64 AScriptManager::getElapsedTime()
{
    if (timer) return timer->elapsed();
    return timerEvalTookMs;
}
