#include "athreads_si.h"
#include "ajavascriptmanager.h"

#include <QThread>
#include <QDebug>
#include <QtWidgets/QApplication>
#include <QScriptEngine>

AThreads_SI::AThreads_SI(AJavaScriptManager *ScriptManager) :
  MasterScriptManager(ScriptManager)
{
  Description = "Allows to evaluate script or function in a new thread.\n"
                "Copies all variables defined in the main script (but does not return back changes).\n"
                "Only those script units are supported which have \"Multithread-capable\" text in the unit description.\n"
                "Script or function can return any type of data, inclusing multi-level arrays and objects.";
}

void AThreads_SI::ForceStop()
{
    qDebug() << ">Multithread module:  External abort received, aborting all threads";
    abortAll();
}

void AThreads_SI::evaluateScript(const QString script)
{
    AJavaScriptManager* sm = MasterScriptManager->createNewScriptManager(workers.size(), bAbortIsGlobal);
    //  qDebug() << "Cloned SM. master:"<<MasterScriptManager<<"clone:"<<sm;

    AScriptThreadScr* worker = new AScriptThreadScr(sm, script);
    startEvaluation(sm, worker);
}

void AThreads_SI::evaluateFunction(const QVariant function, const QVariant arguments)
{
    QString functionName;

    QString typeArr = function.typeName();
    if (typeArr == "QString") functionName = function.toString();
    else if (typeArr == "QVariantMap")
    {
        QVariantMap vm = function.toMap();
        functionName = vm["name"].toString();
    }

    if (functionName.isEmpty())
    {
        abort("Evaluate function requires function or its name as the first argument!");
        return;
    }

    AJavaScriptManager* sm = MasterScriptManager->createNewScriptManager(workers.size(), bAbortIsGlobal);
    //  qDebug() << "Cloned SM. master:"<<MasterScriptManager<<"clone:"<<sm;
    //  qDebug() << "Master engine:"<<MasterScriptManager->engine<< "clone:"<<sm->engine;

    AScriptThreadFun* worker = new AScriptThreadFun(sm, functionName, arguments);
    startEvaluation(sm, worker);
}

void AThreads_SI::startEvaluation(AJavaScriptManager* sm, AScriptThreadBase *worker)
{
    workers << worker;

    QThread* t = new QThread();
    QObject::connect(t,  &QThread::started, worker, &AScriptThreadBase::Run);
    QObject::connect(sm, &AJavaScriptManager::onFinish, t, &QThread::quit);
    QObject::connect(worker, &AScriptThreadBase::errorFound, this, &AThreads_SI::onErrorInTread);
    QObject::connect(t, &QThread::finished, t, &QThread::deleteLater);
    worker->moveToThread(t);
    t->start();

    //  qDebug() << "Started new thread!";
}

void AThreads_SI::onErrorInTread(AScriptThreadBase *workerWithError)
{
    qDebug() << "Error in thread:"<<workerWithError;

    int workerIndex = workers.indexOf(workerWithError);
    QString errorMessage = workerWithError->Result.toString();

    QString msg = "Error in thread #" + QString::number(workerIndex) + ": " + errorMessage;
    if (bAbortIsGlobal) abort(msg);
}

void AThreads_SI::waitForAll()
{
    while (countNotFinished() > 0)
      {
        QThread::usleep(100);
        qApp->processEvents();
      }
}

void AThreads_SI::waitForOne(int IndexOfWorker)
{
  if (IndexOfWorker < 0 || IndexOfWorker >= workers.size()) return;
  if (!workers.at(IndexOfWorker)->isRunning()) return;

  while (workers.at(IndexOfWorker)->isRunning())
    {
      QThread::usleep(100);
      qApp->processEvents();
    }
}

void AThreads_SI::abortAll()
{
  for (AScriptThreadBase* w : workers)
     if (w->isRunning()) w->abort();
}

void AThreads_SI::abortOne(int IndexOfWorker)
{
  if (IndexOfWorker < 0 || IndexOfWorker >= workers.size()) return;
  if (!workers.at(IndexOfWorker)->isRunning()) return;

  workers.at(IndexOfWorker)->abort();
}

int AThreads_SI::countAll()
{
  return workers.size();
}

int AThreads_SI::countNotFinished()
{
  //  qDebug() << "Total number of workers:"<< workers.count();
  int counter = 0;
  for (AScriptThreadBase* w : workers)
    {
        //  qDebug() << w << w->ScriptManager << w->ScriptManager->fEngineIsRunning;
        if (w->isRunning()) counter++;
    }
  return counter;
}

QVariant AThreads_SI::getResult(int IndexOfWorker)
{
  if (IndexOfWorker < 0 || IndexOfWorker >= workers.size()) return QString("Wrong worker index");
  if (workers.at(IndexOfWorker)->isRunning()) return QString("Still running");

  return workers.at(IndexOfWorker)->getResult();
}

bool AThreads_SI::isAborted(int IndexOfWorker)
{
    if (IndexOfWorker < 0 || IndexOfWorker >= workers.size()) return false;
    if (workers.at(IndexOfWorker)->isRunning()) return false;

    return workers.at(IndexOfWorker)->isAborted();
}

void AThreads_SI::deleteAll()
{
    for (AScriptThreadBase* w : workers)
      if (w->isRunning())
      {
          abort("Cannot delete all - not all threads finished");
          return;
      }

    for (AScriptThreadBase* w : workers) w->deleteLater();
    workers.clear();
}

bool AThreads_SI::deleteOne(int IndexOfWorker)
{
   if (IndexOfWorker < 0 || IndexOfWorker >= workers.size())
   {
      qDebug() << "Wrong worker index";
      return false;
   }
   if (workers.at(IndexOfWorker)->isRunning())
   {
       qDebug() << "Still running - cannot delete";
       return false;
   }

   delete workers[IndexOfWorker];
   workers.removeAt(IndexOfWorker);

   return true;
}

AScriptThreadBase::AScriptThreadBase(AJavaScriptManager *ScriptManager) : ScriptManager(ScriptManager) {}

AScriptThreadBase::~AScriptThreadBase()
{
    delete ScriptManager;
}

void AScriptThreadBase::abort()
{
    if (ScriptManager) ScriptManager->abortEvaluation();
}

bool AScriptThreadBase::isAborted() const
{
    if (!ScriptManager) return false;
    return ScriptManager->isEvalAborted();
}

const QVariant AScriptThreadBase::resultToQVariant(const QScriptValue &result) const
{
    if (result.isString()) return result.toString();
    if (result.isNumber()) return result.toNumber();
    return result.toVariant();
}

AScriptThreadScr::AScriptThreadScr(AJavaScriptManager *ScriptManager, const QString &Script) :
    AScriptThreadBase(ScriptManager), Script(Script) {bRunning = true;}

void AScriptThreadScr::Run()
{
    bRunning = true;
    ScriptManager->Evaluate(Script);

    qDebug() << "Worker finished eval, result:"<<ScriptManager->EvaluationResult.toString();

    QScriptValue res = ScriptManager->EvaluationResult;
    if (res.isError())
    {
        //qDebug() << "ERROR";
        Result = res.toString();
        emit errorFound(this);
    }
    else
        Result = resultToQVariant(res);

    //  qDebug() << Result;
    bRunning = false;
}

AScriptThreadFun::AScriptThreadFun(AJavaScriptManager *ScriptManager, const QString &Function, const QVariant &Arguments) :
    AScriptThreadBase(ScriptManager), Function(Function), Arguments(Arguments) {bRunning = true;}

void AScriptThreadFun::Run()
{
    bRunning = true;
    //QScriptValue global = ScriptManager->engine->globalObject();
    //QScriptValue func = global.property(Function);
    QScriptValue func = ScriptManager->getProperty(Function);
    if (!func.isValid())
    {
        Result = "Cannot evaluate: Function " + Function + " not found";
        emit errorFound(this);
    }
    else if (!func.isFunction())
    {
        Result = "Cannot evaluate: " + Function + " is not a function";
        emit errorFound(this);
    }
    else
    {
        //  qDebug() << Arguments;
        QVariantList ArgsVL;
        QString typeArr = Arguments.typeName();
        if (typeArr == "QVariantList") ArgsVL = Arguments.toList();
        else ArgsVL << Arguments;

        QScriptValueList args;
        for (const QVariant& v : ArgsVL)
            args << ScriptManager->registerNewVariant(v);

        QScriptValue res = func.call(QScriptValue(), args);

        if (res.isError())
        {
            Result = res.toString();
            emit errorFound(this);
        }
        else
            Result = resultToQVariant(res);

    }
    bRunning = false;
}
