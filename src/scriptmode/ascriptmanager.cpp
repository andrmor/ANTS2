#include "ascriptmanager.h"
#include "ascriptinterface.h"
#include "ascriptinterface.h"

#ifdef GUI
#include "ainterfacetomessagewindow.h"
#endif

#include <QDebug>
#include <QElapsedTimer>
#include <QMetaMethod>

#include "TRandom2.h"

AScriptManager::AScriptManager(TRandom2 *RandGen) : RandGen(RandGen)
{
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

  delete timer;

  if (bOwnRandomGen) delete RandGen;
}

#ifdef GUI
void AScriptManager::hideMsgDialogs()
{
  for (int i=0; i<interfaces.size(); i++)
  {
      AInterfaceToMessageWindow* t = dynamic_cast<AInterfaceToMessageWindow*>(interfaces[i]);
      if (t)  t->HideWidget();
    }
}

void AScriptManager::restoreMsgDialogs()
{
  for (int i=0; i<interfaces.size(); i++)
  {
      AInterfaceToMessageWindow* t = dynamic_cast<AInterfaceToMessageWindow*>(interfaces[i]);
      if (t) t->RestorelWidget();
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
#endif

qint64 AScriptManager::getElapsedTime()
{
  if (timer) return timer->elapsed();
  return timerEvalTookMs;
}

const QString AScriptManager::getFunctionReturnType(const QString &UnitFunction)
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

void AScriptManager::ifError_AbortAndReport()
{
    if (isUncaughtException())
    {
        int lineNum = getUncaughtExceptionLineNumber();
        QString err = getUncaughtExceptionString();
        //err += "<br>Line #" + QString::number(lineNum) + ", call originating from function: " + functionName;
        requestHighlightErrorLine(lineNum);
        AbortEvaluation(err);
    }
}

void AScriptManager::AbortEvaluation(QString message)
{
  //  qDebug() << "ScriptManager: Abort requested!"<<fAborted<<fEngineIsRunning;

  //if (fAborted || !fEngineIsRunning) return;
  if (fAborted)
  {
      //  qDebug() << "...ignoring, already aborted";
      return;
  }
  fAborted = true;

  abortEvaluation();
  fEngineIsRunning = false;

  // going through registered units and requesting abort
  for (int i=0; i<interfaces.size(); i++)
    {
      AScriptInterface* bi = dynamic_cast<AScriptInterface*>(interfaces[i]);
      if (bi) bi->ForceStop();
    }

  if (!message.isEmpty() && bShowAbortMessageInOutput)
  {
      message = "<font color=\"red\">"+ message +"</font><br>";
      emit showMessage(message);
  }
  emit onAbort();
}
