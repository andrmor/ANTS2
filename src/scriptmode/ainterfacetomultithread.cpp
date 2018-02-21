#include "ainterfacetomultithread.h"
#include "ascriptmanager.h"

#include <QThread>
#include <QDebug>
#include <QApplication>
#include <QScriptEngine>

AInterfaceToMultiThread::AInterfaceToMultiThread(AScriptManager *ScriptManager) :
  MasterScriptManager(ScriptManager)
{
  Description = "Allows to evaluate script or function in a new thread.\n"
                "Copies all variables defined in the main script (but does not return back changes).\n"
                "Only those script units are supported which have \"Multithread-capable\" text in the unit description.\n"
                "Script or function can return any type of data, inclusing multi-level arrays and objects.";
}

void AInterfaceToMultiThread::ForceStop()
{
    qDebug() << ">Multithread module:  External abort received, aborting all threads";
    abortAll();
}

void AInterfaceToMultiThread::evaluateScript(const QString script)
{
    AScriptManager* sm = MasterScriptManager->createNewScriptManager();
    //  qDebug() << "Cloned SM. master:"<<MasterScriptManager<<"clone:"<<sm;

    AScriptThreadScr* worker = new AScriptThreadScr(sm, script);
    startEvaluation(sm, worker);
}

void AInterfaceToMultiThread::evaluateFunction(const QVariant function, const QVariant arguments)
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

    AScriptManager* sm = MasterScriptManager->createNewScriptManager();
    //  qDebug() << "Cloned SM. master:"<<MasterScriptManager<<"clone:"<<sm;

    AScriptThreadFun* worker = new AScriptThreadFun(sm, functionName, arguments);
    startEvaluation(sm, worker);
}

void AInterfaceToMultiThread::startEvaluation(AScriptManager* sm, AScriptThreadBase *worker)
{
    workers << worker;

    connect(sm, &AScriptManager::showMessage, MasterScriptManager, &AScriptManager::showMessage, Qt::QueuedConnection);

    QThread* t = new QThread();
    QObject::connect(t,  &QThread::started, worker, &AScriptThreadBase::Run);
    QObject::connect(sm, &AScriptManager::onFinish, t, &QThread::quit);
    QObject::connect(worker, &AScriptThreadBase::errorFound, this, &AInterfaceToMultiThread::onErrorInTread);
    QObject::connect(t, &QThread::finished, t, &QThread::deleteLater);
    worker->moveToThread(t);
    t->start();

    //  qDebug() << "Started new thread!";
}

void AInterfaceToMultiThread::onErrorInTread(AScriptThreadBase *workerWithError)
{
    qDebug() << "Error in thread:"<<workerWithError;

    int workerIndex = workers.indexOf(workerWithError);
    QString errorMessage = workerWithError->Result.toString();

    QString msg = "Error in thread #" + QString::number(workerIndex) + ": " + errorMessage;
    abort(msg);
}

void AInterfaceToMultiThread::waitForAll()
{
    while (countNotFinished() > 0)
      {
        qApp->processEvents();
      }
}

void AInterfaceToMultiThread::waitForOne(int IndexOfWorker)
{
  if (IndexOfWorker < 0 || IndexOfWorker >= workers.size()) return;
  if (!workers.at(IndexOfWorker)->isRunning()) return;

  while (workers.at(IndexOfWorker)->isRunning())
    {
      qApp->processEvents();
    }
}

void AInterfaceToMultiThread::abortAll()
{
  for (AScriptThreadBase* w : workers)
     if (w->isRunning()) w->abort();
}

void AInterfaceToMultiThread::abortOne(int IndexOfWorker)
{
  if (IndexOfWorker < 0 || IndexOfWorker >= workers.size()) return;
  if (!workers.at(IndexOfWorker)->isRunning()) return;

  workers.at(IndexOfWorker)->abort();
}

int AInterfaceToMultiThread::countAll()
{
  return workers.size();
}

int AInterfaceToMultiThread::countNotFinished()
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

QVariant AInterfaceToMultiThread::getResult(int IndexOfWorker)
{
  if (IndexOfWorker < 0 || IndexOfWorker >= workers.size()) return QString("Wrong worker index");
  if (workers.at(IndexOfWorker)->isRunning()) return QString("Still running");

  return workers.at(IndexOfWorker)->getResult();
}

void AInterfaceToMultiThread::deleteAll()
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

bool AInterfaceToMultiThread::deleteOne(int IndexOfWorker)
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

AScriptThreadBase::AScriptThreadBase(AScriptManager *ScriptManager) : ScriptManager(ScriptManager) {}

AScriptThreadBase::~AScriptThreadBase()
{
    delete ScriptManager;
}

void AScriptThreadBase::abort()
{
    if (ScriptManager) ScriptManager->abortEvaluation();
}

const QVariant AScriptThreadBase::resultToQVariant(const QScriptValue &result) const
{
    if (result.isString()) return result.toString();
    if (result.isNumber()) return result.toNumber();
    return result.toVariant();
}

AScriptThreadScr::AScriptThreadScr(AScriptManager *ScriptManager, const QString &Script) :
    AScriptThreadBase(ScriptManager), Script(Script) {bRunning = true;}

void AScriptThreadScr::Run()
{
    bRunning = true;
    ScriptManager->Evaluate(Script);

    QScriptValue res = ScriptManager->EvaluationResult;
    if (res.isError())
    {
        qDebug() << "EEEEEEEEEEEROR";
        Result = res.toString();
        emit errorFound(this);
    }
    else
        Result = resultToQVariant(res);

    //  qDebug() << Result;
    bRunning = false;
}

AScriptThreadFun::AScriptThreadFun(AScriptManager *ScriptManager, const QString &Function, const QVariant &Arguments) :
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
