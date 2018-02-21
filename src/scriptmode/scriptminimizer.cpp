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

//static double bestResult = 1e30;
//static bool fAbort = false;
//static int numVariables = 0;
//static QString FunctName = "";

double ScriptFunctor(const double *p)
//last parameter contains the pointer to MainWindow object
{
  void *thisvalue;
  memcpy(&thisvalue, &p[0], sizeof(void *));
  AScriptManager* ScriptManager = (AScriptManager*)thisvalue;

  if (ScriptManager->isEvalAborted()) return 1e30;

  //QString str;
  //for (int i=0; i<ScriptManager->numVariables; i++)
  //  str += QString::number(p[i+1])+"  ";
  //qDebug() << "Functor call with parameters:"<<str;

  QScriptValueList input;
  for (int i=0; i<ScriptManager->MiniNumVariables; i++)
    input << p[i+1];

  QScriptValue sv = ScriptManager->getMinimalizationFunction();
  double result = sv.call(QScriptValue(), input).toNumber();

  //qDebug() << "Minimization parameter value obtained:"<<result;
  return result;
}

AInterfaceToMinimizerScript::AInterfaceToMinimizerScript(AScriptManager *ScriptManager) :
  ScriptManager(ScriptManager) {}

AInterfaceToMinimizerScript::AInterfaceToMinimizerScript(const AInterfaceToMinimizerScript& other)
  : AScriptInterface(other)
{
    ScriptManager = 0; // need to be set on copy!
    Clear();
}

void AInterfaceToMinimizerScript::ForceStop()
{    
    //qDebug() << "Abort requested for minimization procedure";
    //qDebug() << "aborted:"<<ScriptManager->fAborted;
}

void AInterfaceToMinimizerScript::Clear()
{
  Name.clear();
  Start.clear();
  Step.clear();
  Min.clear();
  Max.clear();
}

void AInterfaceToMinimizerScript::SetFunctorName(QString name)
{
  //FunctorName = FunctName = name;
    ScriptManager->MiniFunctionName = name;
}

void AInterfaceToMinimizerScript::AddVariable(QString name, double start, double step, double min, double max)
{
  Name << name;
  Start << start;
  Step << step;
  Min << min;
  Max << max;
}

void AInterfaceToMinimizerScript::ModifyVariable(int varNumber, double start, double step, double min, double max)
{
  int size = Name.size();
  if (varNumber > size-1)
    {
      abort("Wrong variable number: " + QString::number(varNumber) + "  There are "+ QString::number(size) +" variables defined");
      return;
    }
  Start.replace(varNumber, start);
  Step.replace(varNumber, step);
  Min.replace(varNumber, min);
  Max.replace(varNumber, max);
}

QString AInterfaceToMinimizerScript::Run()
{
  if (!ScriptManager)
    {
      abort("ScriptManager is not set!");
      return "";
    }

  //qDebug() << "Optimization run called";
  ScriptManager->MiniNumVariables = Name.size();
  if (ScriptManager->MiniNumVariables == 0)
    {
      qDebug() << "No variables defined!";
      abort("Variables are not defined!");
      return "";
    }

  QScriptValue sv = ScriptManager->getMinimalizationFunction();
  if (!sv.isFunction())
    {
      qDebug() << "Minimization function not defined!";
      abort("Minimization function is not defined!");
      return "";
    }

  //Creating ROOT minimizer
  ROOT::Minuit2::Minuit2Minimizer *RootMinimizer = new ROOT::Minuit2::Minuit2Minimizer(ROOT::Minuit2::kSimplex);//(ROOT::Minuit2::kMigrad);
  RootMinimizer->SetMaxFunctionCalls(5000);
  RootMinimizer->SetMaxIterations(5000);
  RootMinimizer->SetTolerance(0.001);
  RootMinimizer->SetPrintLevel(1);

  // 1 standard
  // 2 try to improve minimum (slower)
  RootMinimizer->SetStrategy(2);

  ROOT::Math::Functor *Funct = new ROOT::Math::Functor(&ScriptFunctor, ScriptManager->MiniNumVariables+1);
  RootMinimizer->SetFunction(*Funct);
  //prepare to transfer pointer to ScriptManager - it will the first variable
  double dPoint;
  void *thisvalue = ScriptManager;
  memcpy(&dPoint, &thisvalue, sizeof(void *));
  //We need to fix for the possibility that double isn't enough to store void*
  RootMinimizer->SetFixedVariable(0, "p", dPoint);
  //setting up variables   -  start step min max
  for (int i=0; i<ScriptManager->MiniNumVariables; i++)
      RootMinimizer->SetLimitedVariable(i+1, Name[i].toLatin1().data(),  Start[i], Step[i], Min[i], Max[i]);

  //qDebug() << "Minimizer created and configured";

  // do the minimization
  //fAbort = false;
  bool fOK = RootMinimizer->Minimize();
  fOK = fOK && !ScriptManager->isEvalAborted();

  //report results
  qDebug()<<"Minimization success? "<<fOK;

  //loading back settings of det and sim
  //MW->fSimDataNotSaved = false;
  //MW->readDetectorFromJson(json);
  //MW->readSimSettingsFromJson(json);

  delete Funct;
  delete RootMinimizer;

  return "Done!";
}
