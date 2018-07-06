#include "ajavascriptmanager.h"
#ifdef GUI
#include "ainterfacetomessagewindow.h"
#endif
#include "coreinterfaces.h"
#include "ascriptinterfacefactory.h"
#include "ainterfacetomultithread.h"

#include <QScriptEngine>
#include <QDebug>
#include <QScriptValueIterator>
//#include <QDialog>
#include <QElapsedTimer>

AJavaScriptManager::AJavaScriptManager(TRandom2* RandGen) :
    AScriptManager(RandGen)
{
    engine = new QScriptEngine();
    engine->setProcessEventsInterval(200);
}

AJavaScriptManager::~AJavaScriptManager()
{
    if (engine)
    {
        engine->abortEvaluation();
        engine->deleteLater();
        engine = 0;
    }
#ifdef GUI
    for (AScriptMessengerDialog* d : ThreadMessangerDialogs)
    {
        delete d;
        d = 0;
    }
    ThreadMessangerDialogs.clear();
#endif
}

QString AJavaScriptManager::Evaluate(const QString& Script)
{
    LastError.clear();
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
    EvaluationResult = engine->evaluate(Script);
    fEngineIsRunning = false;

    timerEvalTookMs = timer->elapsed();
    delete timer; timer = 0;

    QString result = EvaluationResult.toString();
    emit onFinish(result);
    return result;
}

QVariant AJavaScriptManager::EvaluateScriptInScript(const QString &script)
{
    int line = FindSyntaxError(script);
    if (line == -1)
      {
          QScriptValue val = engine->evaluate(script);
          return val.toVariant();
      }

    AbortEvaluation("Syntax error in script provided to core.evaluate()");
    return 0;
}

bool AJavaScriptManager::isUncaughtException() const
{
    return engine->hasUncaughtException();
}

int AJavaScriptManager::getUncaughtExceptionLineNumber() const
{
    return engine->uncaughtExceptionLineNumber();
}

const QString AJavaScriptManager::getUncaughtExceptionString() const
{
    return engine->uncaughtException().toString();
}

void AJavaScriptManager::collectGarbage()
{
    engine->collectGarbage();
}

QScriptValue AJavaScriptManager::getMinimalizationFunction()
{
    return engine->globalObject().property(MiniFunctionName);
}

#ifdef GUI
void AJavaScriptManager::hideAllMessengerWidgets()
{
    for (AScriptMessengerDialog* d : ThreadMessangerDialogs)
        if (d) d->HideWidget();
}

void AJavaScriptManager::showAllMessengerWidgets()
{
    for (AScriptMessengerDialog* d : ThreadMessangerDialogs)
        if (d) d->RestoreWidget();
}

void AJavaScriptManager::clearUnusedMsgDialogs()
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

void AJavaScriptManager::closeAllMsgDialogs()
{
    for (AScriptMessengerDialog* d : ThreadMessangerDialogs)
        if (d) d->Hide();
}

void AJavaScriptManager::hideMsgDialogs()
{
    AScriptManager::hideMsgDialogs();

    for (AScriptMessengerDialog* d : ThreadMessangerDialogs)
        if (d) d->HideWidget();
}

void AJavaScriptManager::restoreMsgDialogs()
{
    AScriptManager::restoreMsgDialogs();

    for (AScriptMessengerDialog* d : ThreadMessangerDialogs)
        if (d)
            if (d->IsShown()) d->RestoreWidget();
}
#endif

void AJavaScriptManager::SetInterfaceObject(QObject *interfaceObject, QString name)
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

int AJavaScriptManager::FindSyntaxError(const QString& script)
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
    //  qDebug() << "=====Copy started====="<<obj.isArray();
    QScriptEngine& engine = m_toEngine;

    if (obj.isUndefined()) return QScriptValue(QScriptValue::UndefinedValue);
    if (obj.isNull())      return QScriptValue(QScriptValue::NullValue);

    // too slow access to these values in threads without engine.evaluate
    //if (obj.isNumber() || obj.isString() || obj.isBool() || obj.isBoolean() || obj.isVariant())
    //  {
    //    return engine.newVariant(obj.toVariant());
    //  }

    if (obj.isNumber())
    {
        QVariant var = obj.toVariant();
        if (var.type() ==  QVariant::Int)
          {
            int integerVal = var.toInt();
            //  qDebug() << "     Integer:"<<integerVal;
            return QScriptValue(integerVal);
          }
        else
          {
            double doubleVal = var.toDouble();
            //  qDebug() << "     Double:"<<doubleVal;
            return QScriptValue(doubleVal);
          }
    }
    if (obj.isString())
    {
        //  qDebug() << "     String:"<< obj.toString();
        return QScriptValue(obj.toString());
    }
    if (obj.isBool())
      {
        //  qDebug() << "     Bool:" << obj.toBool();
        return QScriptValue(obj.toBool());
      }
    if (obj.isBoolean())
      {
        //  qDebug() << "     Boolean:" << obj.toBoolean();
        return QScriptValue(obj.toBoolean());
      }
    if (obj.isVariant())
    {
        //  qDebug() << "    !!!Variant - potentially slow!!! :" << obj.toVariant();
        return engine.newVariant(obj.toVariant());
    }

    // If we've already copied this object, don't copy it again.
    QScriptValue copy;
    if (obj.isObject())
    {
        if (copiedObjs.contains(obj.objectId()))
        {
            //  qDebug() << "--------------Already have this obj!";
            return copiedObjs.value(obj.objectId());
        }        
        copiedObjs.insert(obj.objectId(), copy);
        //  qDebug() << "     -->Added object to list of already defined objects";
    }

    if (obj.isQObject())
    {
        //  qDebug() << "     QObject";
        copy = engine.newQObject(copy, obj.toQObject());
        copy.setPrototype(this->copy(obj.prototype()));
    }
    else if (obj.isQMetaObject())
    {
        //  qDebug() << "     QMetaObject";
        copy = engine.newQMetaObject(obj.toQMetaObject());
    }
    else if (obj.isFunction())
    {
        //  qDebug() << "     Function";
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
    else if (obj.isArray())
    {
        //  qDebug() << "      Array";
        copy = engine.newArray();
//        QScriptValueIterator itt(copy);
//        while ( itt.hasNext() )
//        {
//            itt.next();
//            qDebug() << "     >>> Array already contain sub property:"<<itt.name();
//        }

        //copy.setPrototype(this->copy(obj.prototype())); // no - copies all infrastructure!

        QScriptValueIterator it(obj);
        while ( it.hasNext() )
        {
            it.next();
            const QString& name = it.name();
            //  qDebug() << "     -----Sub property:"<<name;
            if (copy.property(it.name()).isValid())
            {
                //  qDebug() << "     -----Already have this sub-member";  //"length" is otherwise added
                continue;
            }
            else
            {
                const QScriptValue& property = it.value();
                //  qDebug() << "     -----New sub-memeber"<<it.name()<<it.value().toString();
                copy.setProperty(name, this->copy(property));
            }
        }
    }
    else if (obj.isObject())
    {
        //  qDebug() << "      Object"<<obj.toString();
        if (obj.scriptClass())
            copy = engine.newObject(obj.scriptClass(), this->copy(obj.data()));
        else
            copy = engine.newObject();
        //copy.setPrototype(this->copy(obj.prototype()));

        QScriptValueIterator it(obj);
        while ( it.hasNext())
        {
            it.next();

            const QString& name = it.name();
            //  qDebug() << "     -----Sub property:"<<name;

            if (copy.property(it.name()).isValid())
            {
                //do not want to intrude to array standard infrastructure
                //  qDebug() << "     -----Already have this sub-member";
                continue;
            }
            else
            {
                const QScriptValue& property = it.value();
                //  qDebug() << "     -----New sub-memeber"<<it.name()<<it.value().toString();

                copy.setProperty(name, this->copy(property));
            }
        }
    }
    else
    {
        // Error handling…
        qDebug() << "-----------------problem---------------";
    }

    return copy;
}

#include "TRandom2.h"
AJavaScriptManager *AJavaScriptManager::createNewScriptManager(int threadNumber, bool bAbortIsGlobal)
{
    int seed = RandGen->Rndm()*100000;
    TRandom2* rnd = new TRandom2(seed);
    AJavaScriptManager* sm = new AJavaScriptManager(rnd);
    sm->bOwnRandomGen = true;
    sm->bShowAbortMessageInOutput = bAbortIsGlobal;

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
            AInterfaceToMinimizerJavaScript* mini = dynamic_cast<AInterfaceToMinimizerJavaScript*>(copy);
            if (mini)
            {
                //qDebug() << "--this is mini";
                mini->SetScriptManager(sm);
            }
#ifdef GUI
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
#endif
            // connecting the request for abort script
            if (bAbortIsGlobal)
            {
                AScriptInterface* base = dynamic_cast<AScriptInterface*>(copy);
                if (base) connect(base, &AScriptInterface::AbortScriptEvaluation, coreObj, &AInterfaceToCore::abort);
            }

            sm->SetInterfaceObject(copy, io->objectName());
        }
        else
        {
            qDebug() << "Unknown interface object type for unit" << io->objectName();
        }
    }



#ifdef GUI
    //connect web and msg
    AInterfaceToWebSocket* web = 0;
    AInterfaceToMessageWindow* msg = 0;
    for (QObject* io : sm->interfaces)
    {
        AInterfaceToMessageWindow* ob = dynamic_cast<AInterfaceToMessageWindow*>(io);
        if (ob) msg = ob;
        else
        {
            AInterfaceToWebSocket* ob = dynamic_cast<AInterfaceToWebSocket*>(io);
            if (ob) web = ob;
        }
    }
//    qDebug() << "-----------"<<msg << web;
    if (msg && web)
    {
        QObject::connect(web, &AInterfaceToWebSocket::showTextOnMessageWindow, msg, &AInterfaceToMessageWindow::Append);
        QObject::connect(web, &AInterfaceToWebSocket::clearTextOnMessageWindow, msg, &AInterfaceToMessageWindow::Clear);
    }
#endif

    QScriptValue global = engine->globalObject();
    ScriptCopier SC(*sm->engine);
    QScriptValueIterator it(global);
    while (it.hasNext())
    {
        it.next();
        //  qDebug() << "==> Found property " << it.name() << it.value().toString();

        if (!sm->engine->globalObject().property(it.name()).isValid()) // if resource with this name does not exist...
        {
            //do not copy QObjects - the multi-thread friendly units were already copied
            if (!it.value().isQObject())
            {
                const QScriptValue sv = SC.copy(it.value());
                sm->engine->globalObject().setProperty(it.name(), sv);
                //  qDebug() << "    Registered:"<<it.name() << "-:->" << sm->engine->globalObject().property(it.name()).toVariant();
            }
            else
            {
                //  qDebug() << "    Skipped: new property, but it is a QObject";// << it.name();
            }
        }
        else
          {
            //  qDebug() << "    Skipped: already have this property!";
          }
    }
#ifdef GUI
    //connect core.print() to the ScriptManager of the GUI thread, as queued!
    connect(sm, &AJavaScriptManager::showMessage, this, &AJavaScriptManager::showMessage, Qt::QueuedConnection);
#endif
    //  qDebug() << "  Scriptmanager created!"<<sm;
    return sm;
}

void AJavaScriptManager::abortEvaluation()
{
    qDebug() << "Abort for JavaScript";
    engine->abortEvaluation();
}

QScriptValue AJavaScriptManager::getProperty(const QString &properyName) const
{
    QScriptValue global = engine->globalObject();
    return global.property(properyName);
}

QScriptValue AJavaScriptManager::registerNewVariant(const QVariant& Variant)
{
    return engine->toScriptValue(Variant);
}
