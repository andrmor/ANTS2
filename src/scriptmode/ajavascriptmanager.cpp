#include "ajavascriptmanager.h"

#ifdef GUI
#include "amsg_si.h"
#endif

#include "ascriptinterface.h"
#include "acore_si.h"
#include "amath_si.h"
#include "ascriptinterfacefactory.h"
#include "athreads_si.h"

#include <QScriptEngine>
#include <QDebug>
#include <QScriptValueIterator>
#include <QElapsedTimer>
#include <QFile>

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

void AJavaScriptManager::addQVariantToString(const QVariant & var, QString & string)
{
    switch (var.type())
    {
    case QVariant::Map:
      {
        string += '{';
        const QMap<QString, QVariant> map = var.toMap();
        for (const QString & k : map.keys())
        {
            string += QString("\"%1\":").arg(k);
            addQVariantToString(map.value(k), string);
            string += ", ";
        }
        if (string.endsWith(", ")) string.chop(2);
        string += '}';
        break;
      }
    case QVariant::List:
        string += '[';
        for (const QVariant & v : var.toList())
        {
            addQVariantToString(v, string);
            string += ", ";
        }
        if (string.endsWith(", ")) string.chop(2);
        string += ']';
        break;
    default:
        string += var.toString();// implicit convertion to string
    }
}

QString AJavaScriptManager::Evaluate(const QString & Script)
{
    LastError.clear();
    LastErrorLineNumber = -1;
    fAborted = false;

    bScriptExpanded = false;
    QString ModScript = expandScript(Script);

    if (ModScript == sIncludeInfiniteLoop)
    {
        LastError = ModScript;
        return LastError;
    }
    if (ModScript == sIncludeFileError)
    {
        LastError = ModScript;
        return LastError;
    }

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
    EvaluationResult = engine->evaluate(ModScript);
    fEngineIsRunning = false;

    timerEvalTookMs = timer->elapsed();
    delete timer; timer = nullptr;

    QString result;
    if (EvaluationResult.isArray() || EvaluationResult.isObject())
    {
        QVariant resVar = EvaluationResult.toVariant();
        addQVariantToString(resVar, result);
    }
    else result = EvaluationResult.toString();

    emit onFinish(result);
    return result;
}

QString AJavaScriptManager::expandScript(const QString & OriginalScript)
{
    QString Script = OriginalScript;

    const int OriginalSize = OriginalScript.count('\n') + 1;
    LineNumberMapper.resize(OriginalSize);
    for (int i = 0; i < OriginalSize; i++)
        LineNumberMapper[i] = i + 1; // line numbers start from 1

    bool bWasExpanded;
    bool bInsideBlockComments;
    int iCycleCounter = 0;
    do
    {
        if ( !Script.contains("#include") ) break;

        bWasExpanded = false;
        bInsideBlockComments = false;

        QString WorkScript;

        const QStringList SL = Script.split('\n', QString::KeepEmptyParts);
        for (int iLine = 0; iLine < SL.size(); iLine++)
        {
            const QString Line = SL.at(iLine);
            //qDebug() << Line;

            if ( bInsideBlockComments || !Line.simplified().startsWith("#include") )
            {
                WorkScript += Line + '\n';

                const int len = Line.length();
                if (len > 1)
                {
                    for (int iChar = 0; iChar < len - 1; iChar++)
                    {
                        if (Line.at(iChar) == '/' && Line.at(iChar+1) == '/') break; // line comment started

                        if      (Line.at(iChar) == '/' && Line.at(iChar+1) == '*')
                        {
                            bInsideBlockComments = true;
                            iChar++;
                        }
                        else if (Line.at(iChar) == '*' && Line.at(iChar+1) == '/')
                        {
                            bInsideBlockComments = false;
                            iChar++;
                        }
                    }
                }
            }
            else
            {
                const QStringList tmp = Line.split('\"', QString::KeepEmptyParts);
                if (tmp.size() < 3)
                {
                    qWarning() << "Error in processing #include arguments";
                    LastErrorLineNumber = LineNumberMapper[iLine];
                    return sIncludeFileError;
                }
                const QString FileName = tmp.at(1);

                QFile file(FileName);
                if (!file.exists() || !file.open(QIODevice::ReadOnly | QFile::Text))
                {
                    qWarning() << "Error in expanding #include: failed to read the file";
                    LastErrorLineNumber = LineNumberMapper[iLine];
                    return sIncludeFileError;
                }

                QTextStream in(&file);

                int LineCounter = 0;
                while(!in.atEnd())
                {
                    in.readLine();
                    LineCounter++;
                }
                in.seek(0);
                QString IncludedScript = in.readAll();
                file.close();
                if (IncludedScript.isEmpty()) IncludedScript = "\n";
                WorkScript += IncludedScript + "\n";

                int old = LineNumberMapper[iLine];
                LineNumberMapper.insert(iLine, LineCounter-1, old);

                bWasExpanded = true;
                bScriptExpanded = true;
            }
        }
        Script = WorkScript;

        iCycleCounter++;
        //qDebug() << iCycleCounter;
        if (iCycleCounter > 1000)
            return sIncludeInfiniteLoop;
    }
    while (bWasExpanded);

    /*
    const QStringList SL = Script.split('\n', QString::KeepEmptyParts);
    for (int iLine = 0; iLine < SL.size(); iLine++)
        qDebug() << iLine + 1 << " "<< SL.at(iLine);
    qDebug() << "-----\n"<< LineNumberMapper;
    */

    return Script;
}

void AJavaScriptManager::correctLineNumber(int & iLineNumber) const
{
    if (!bScriptExpanded) return;

    //iLineNumber for the first line is 1
    if (iLineNumber < 1 || iLineNumber > LineNumberMapper.size()) return;

    iLineNumber = LineNumberMapper[iLineNumber - 1];
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
    int iLineNumber = engine->uncaughtExceptionLineNumber();
    //qDebug() << "->-"<<iLineNumber;
    correctLineNumber(iLineNumber);
    //qDebug() << "-<-"<<iLineNumber;
    return iLineNumber;
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
        AThreads_SI* t = dynamic_cast<AThreads_SI*>(interfaces[i]);
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

/*
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
        QObject* mathObj = new AMathScriptInterface(RandGen);
        QScriptValue mathVal = engine->newQObject(mathObj, QScriptEngine::QtOwnership);
        QString mathName = "math";
        mathObj->setObjectName(mathName);
        engine->globalObject().setProperty(mathName, mathVal);
        interfaces.append(mathObj);
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
*/

void AJavaScriptManager::RegisterInterfaceAsGlobal(AScriptInterface *interface)
{
    QScriptValue obj = engine->newQObject(interface, QScriptEngine::QtOwnership);
    engine->setGlobalObject(obj);
    doRegister(interface, "");
}

void AJavaScriptManager::RegisterCoreInterfaces(bool bCore, bool bMath)
{
    if (bCore)
    {
        coreObj = new ACore_SI(this);
        QScriptValue coreVal = engine->newQObject(coreObj, QScriptEngine::QtOwnership);
        engine->globalObject().setProperty("core", coreVal);
        doRegister(coreObj, "core");
    }

    if (bMath)
    {
        AMath_SI* mathObj = new AMath_SI(RandGen);
        QScriptValue mathVal = engine->newQObject(mathObj, QScriptEngine::QtOwnership);
        engine->globalObject().setProperty("math", mathVal);
        doRegister(mathObj, "math");
    }
}

void AJavaScriptManager::RegisterInterface(AScriptInterface *interface, const QString &name)
{
    QScriptValue obj = engine->newQObject(interface, QScriptEngine::QtOwnership);
    engine->globalObject().setProperty(name, obj);
    doRegister(interface, name);
}

void AJavaScriptManager::doRegister(AScriptInterface *interface, const QString &name)
{
    interface->setObjectName(name);
    interfaces.append(interface);
    QObject::connect(interface, &AScriptInterface::AbortScriptEvaluation, this, &AJavaScriptManager::AbortEvaluation);
}

int AJavaScriptManager::FindSyntaxError(const QString & script)
{
    QString txt = script;
    txt.replace("#include", "//#include");

    QScriptSyntaxCheckResult check = QScriptEngine::checkSyntax(txt);
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

    for (AScriptInterface* si : interfaces)
    {
        if (!si->IsMultithreadCapable()) continue;

        AScriptInterface* copy = AScriptInterfaceFactory::makeCopy(si); //cloning script interfaces
        if (copy)
        {
            //  qDebug() << "Making available for multi-thread use: "<<io->objectName();

            //special for core unit
            ACore_SI* core = dynamic_cast<ACore_SI*>(copy);
            if (core)
            {
                //qDebug() << "--this is core";
                core->SetScriptManager(sm);
            }
            AMini_JavaScript_SI* mini = dynamic_cast<AMini_JavaScript_SI*>(copy);
            if (mini)
            {
                //qDebug() << "--this is mini";
                mini->SetScriptManager(sm);
            }
#ifdef GUI
            AMsg_SI* msg = dynamic_cast<AMsg_SI*>(copy);
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
                if (base) connect(base, &AScriptInterface::AbortScriptEvaluation, coreObj, &ACore_SI::abort);
            }

            sm->RegisterInterface(copy, si->objectName());
        }
        else
        {
            qDebug() << "Unknown interface object type for unit" << si->objectName();
        }
    }



#ifdef GUI
    //connect web and msg
    AWeb_SI* web = 0;
    AMsg_SI* msg = 0;
    for (QObject* io : sm->interfaces)
    {
        AMsg_SI* ob = dynamic_cast<AMsg_SI*>(io);
        if (ob) msg = ob;
        else
        {
            AWeb_SI* ob = dynamic_cast<AWeb_SI*>(io);
            if (ob) web = ob;
        }
    }
//    qDebug() << "-----------"<<msg << web;
    if (msg && web)
    {
        QObject::connect(web, &AWeb_SI::showTextOnMessageWindow, msg, &AMsg_SI::Append);
        QObject::connect(web, &AWeb_SI::clearTextOnMessageWindow, msg, &AMsg_SI::Clear);
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
