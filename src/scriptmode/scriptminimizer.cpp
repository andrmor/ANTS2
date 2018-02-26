#include "scriptminimizer.h"
#include "ascriptmanager.h"

#ifdef GUI
#include "mainwindow.h"
#include "genericscriptwindowclass.h"
#endif

#include <QScriptEngine>
#include <QDebug>

#include "TMath.h"
#include "Math/Functor.h"
#include "Minuit2/Minuit2Minimizer.h"

double ScriptFunctor(const double *p) //last parameter contains the pointer to MainWindow object
{
  void *thisvalue;
  memcpy(&thisvalue, &p[0], sizeof(void *));
  AScriptManager* ScriptManager = (AScriptManager*)thisvalue;

  if (ScriptManager->isEvalAborted()) return 1e30;

  /*
    QString str;
    for (int i=0; i<ScriptManager->MiniNumVariables; i++)
        str += QString::number(p[i+1])+"  ";
    qDebug() << "Functor call with parameters:"<<str<<ScriptManager;
  */


  QScriptValueList input;
  for (int i=0; i<ScriptManager->MiniNumVariables; i++) input << p[i+1];

  QScriptValue sv = ScriptManager->getMinimalizationFunction();
  double result = sv.call(QScriptValue(), input).toNumber();

  //    qDebug() << "SM"<<ScriptManager<<"engine"<<ScriptManager->engine<<"sv reports engine:"<<sv.engine();
  //    qDebug() << "Minimization parameter value obtained:"<<result;

  return result;
}

AInterfaceToMinimizerScript::AInterfaceToMinimizerScript(AScriptManager *ScriptManager) :
  ScriptManager(ScriptManager) {}

AInterfaceToMinimizerScript::AInterfaceToMinimizerScript(const AInterfaceToMinimizerScript& other)
  : AScriptInterface(other)
{
    ScriptManager = 0; // need to be set on copy!
}

AInterfaceToMinimizerScript::~AInterfaceToMinimizerScript()
{
  Clear();
}

void AInterfaceToMinimizerScript::ForceStop()
{    
  //qDebug() << "Abort requested for minimization procedure";
    //qDebug() << "aborted:"<<ScriptManager->fAborted;
}

void AInterfaceToMinimizerScript::Clear()
{
  for (AVarRecordBase* r : Variables)
    delete r;
  Variables.clear();
}

void AInterfaceToMinimizerScript::SetFunctorName(QString name)
{
    ScriptManager->MiniFunctionName = name;
}

void AInterfaceToMinimizerScript::AddVariable(QString name, double start, double step, double min, double max)
{
  Variables << new AVarRecordLimited(name, start, step, min, max);
}

void AInterfaceToMinimizerScript::AddVariable(QString name, double start, double step)
{
  Variables << new AVarRecordNormal(name, start, step);
}

void AInterfaceToMinimizerScript::AddFixedVariable(QString name, double value)
{
  Variables << new AVarRecordFixed(name, value);
}

void AInterfaceToMinimizerScript::AddLowerLimitedVariable(QString name, double value, double step, double lowerBound)
{
  Variables << new AVarRecordLowerLimited(name, value, step, lowerBound);
}

void AInterfaceToMinimizerScript::AddUpperLimitedVariable(QString name, double value, double step, double upperBound)
{
  Variables << new AVarRecordUpperLimited(name, value, step, upperBound);
}

bool AInterfaceToMinimizerScript::Run()
{
  if (!ScriptManager)
    {
      abort("ScriptManager is not set!");
      return false;
    }

  //qDebug() << "Optimization run called";
  ScriptManager->MiniNumVariables = Variables.size();
  if (ScriptManager->MiniNumVariables == 0)
    {
      abort("Variables are not defined!");
      return false;
    }

  QScriptValue sv = ScriptManager->getMinimalizationFunction();
  if (!sv.isFunction())
    {
      abort("Minimization function is not defined!");
      return false;
    }

  //Creating ROOT minimizer
  //ROOT::Minuit2::Minuit2Minimizer *RootMinimizer = new ROOT::Minuit2::Minuit2Minimizer(ROOT::Minuit2::kSimplex);//(ROOT::Minuit2::kMigrad);
  ROOT::Minuit2::Minuit2Minimizer *RootMinimizer = new ROOT::Minuit2::Minuit2Minimizer(ROOT::Minuit2::kMigrad);
  RootMinimizer->SetMaxFunctionCalls(500);
  RootMinimizer->SetMaxIterations(1000);
  RootMinimizer->SetTolerance(0.001);
  RootMinimizer->SetPrintLevel(PrintVerbosity);

  // 1 standard
  // 2 try to improve minimum (slower)
  RootMinimizer->SetStrategy( bHighPrecision ? 2 : 1);

  ROOT::Math::Functor *Funct = new ROOT::Math::Functor(&ScriptFunctor, ScriptManager->MiniNumVariables+1);
  RootMinimizer->SetFunction(*Funct);
  //prepare to transfer pointer to ScriptManager - it will the first variable
  double dPoint;
  void *thisvalue = ScriptManager;
  memcpy(&dPoint, &thisvalue, sizeof(void *));
  //We need to fix for the possibility that double isn't enough to store void*
  RootMinimizer->SetFixedVariable(0, "p", dPoint);
  //setting up variables   -  start step min max etc
  for (int i=0; i<ScriptManager->MiniNumVariables; i++)
      Variables[i]->AddToMinimizer(i+1, RootMinimizer);

  //    qDebug() << "Minimizer created and configured";

  // do the minimization
  bool fOK = RootMinimizer->Minimize();
  fOK = fOK && !ScriptManager->isEvalAborted();
  //    qDebug()<<"Minimization success? "<<fOK;

  //results
  Results.clear();
  if (fOK)
  {
      const double *VarVals = RootMinimizer->X();
      for (int i=0; i<Variables.size(); i++)
      {
          //    qDebug() << i << "-->--"<<VarVals[i+1];
          Results << VarVals[i+1];
      }
  }

  delete Funct;
  delete RootMinimizer;

  return fOK;
}

AInterfaceToMinimizerScript::AVarRecordNormal::AVarRecordNormal(QString name, double start, double step)
{
  Name = name.toLatin1().data();
  Value = start;
  Step = step;
}

void AInterfaceToMinimizerScript::AVarRecordNormal::AddToMinimizer(int varIndex, ROOT::Minuit2::Minuit2Minimizer *minimizer)
{
  minimizer->SetVariable(varIndex, Name, Value, Step);
}

AInterfaceToMinimizerScript::AVarRecordFixed::AVarRecordFixed(QString name, double value)
{
  Name = name.toLatin1().data();
  Value = value;
}

void AInterfaceToMinimizerScript::AVarRecordFixed::AddToMinimizer(int varIndex, ROOT::Minuit2::Minuit2Minimizer *minimizer)
{
  minimizer->SetFixedVariable(varIndex, Name, Value);
}

AInterfaceToMinimizerScript::AVarRecordLimited::AVarRecordLimited(QString name, double start, double step, double min, double max)
{
  Name = name.toLatin1().data();
  Value = start;
  Step = step;
  Min = min;
  Max = max;
}

void AInterfaceToMinimizerScript::AVarRecordLimited::AddToMinimizer(int varIndex, ROOT::Minuit2::Minuit2Minimizer *minimizer)
{
  minimizer->SetLimitedVariable(varIndex, Name, Value, Step, Min, Max);
}

AInterfaceToMinimizerScript::AVarRecordLowerLimited::AVarRecordLowerLimited(QString name, double start, double step, double min)
{
  Name = name.toLatin1().data();
  Value = start;
  Step = step;
  Min = min;
}

void AInterfaceToMinimizerScript::AVarRecordLowerLimited::AddToMinimizer(int varIndex, ROOT::Minuit2::Minuit2Minimizer *minimizer)
{
  minimizer->SetLowerLimitedVariable(varIndex, Name, Value, Step, Min);
}

AInterfaceToMinimizerScript::AVarRecordUpperLimited::AVarRecordUpperLimited(QString name, double start, double step, double max)
{
  Name = name.toLatin1().data();
  Value = start;
  Step = step;
  Max = max;
}

void AInterfaceToMinimizerScript::AVarRecordUpperLimited::AddToMinimizer(int varIndex, ROOT::Minuit2::Minuit2Minimizer *minimizer)
{
  minimizer->SetUpperLimitedVariable(varIndex, Name, Value, Step, Max);
}
