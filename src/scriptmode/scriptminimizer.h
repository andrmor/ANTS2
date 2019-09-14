#ifndef SCRIPTMINIMIZER_H
#define SCRIPTMINIMIZER_H

#include "localscriptinterfaces.h"

#include <QObject>
#include <QVector>
#include <QVariant>

#include <string>

class MainWindow;
class AScriptManager;
class AJavaScriptManager;
class AFunctorBase;

namespace ROOT { namespace Minuit2 { class Minuit2Minimizer; } }
namespace ROOT { namespace Math { class Functor; } }

class AInterfaceToMinimizerScript : public AScriptInterface
{
  Q_OBJECT

  class AVarRecordBase
  {
  public:
      virtual ~AVarRecordBase() {}

      virtual void AddToMinimizer(int varIndex, ROOT::Minuit2::Minuit2Minimizer *minimizer) = 0;
      virtual void Debug() const = 0;

  protected:
      std::string Name;
      double      Value;
      double      Step;
      double      Min;
      double      Max;
  };

  class AVarRecordNormal : public AVarRecordBase
  {
    public:
      AVarRecordNormal(QString name, double start, double step);
      void AddToMinimizer(int varIndex, ROOT::Minuit2::Minuit2Minimizer *minimizer) override;
      void Debug() const override;
  };

  class AVarRecordFixed : public AVarRecordBase
  {
    public:
      AVarRecordFixed(QString name, double value);
      void AddToMinimizer(int varIndex, ROOT::Minuit2::Minuit2Minimizer *minimizer) override;
      void Debug() const override;
  };

  class AVarRecordLimited : public AVarRecordBase
  {
    public:
      AVarRecordLimited(QString name, double start, double step, double min, double max);
      void AddToMinimizer(int varIndex, ROOT::Minuit2::Minuit2Minimizer *minimizer) override;
      void Debug() const override;
  };

  class AVarRecordLowerLimited : public AVarRecordBase
  {
    public:
      AVarRecordLowerLimited(QString name, double start, double step, double min);
      void AddToMinimizer(int varIndex, ROOT::Minuit2::Minuit2Minimizer *minimizer) override;
      void Debug() const override;
  };

  class AVarRecordUpperLimited : public AVarRecordBase
  {
    public:
      AVarRecordUpperLimited(QString name, double start, double step, double max);
      void AddToMinimizer(int varIndex, ROOT::Minuit2::Minuit2Minimizer *minimizer) override;
      void Debug() const override;
  };

public:
  AInterfaceToMinimizerScript(AScriptManager* ScriptManager);
  virtual ~AInterfaceToMinimizerScript();

  bool           IsMultithreadCapable() const override {return true;}
  void           ForceStop() override;

public slots:

  void           SetHighPrecision(bool flag) {bHighPrecision = flag;}
  void           SetVerbosity(int level) {PrintVerbosity = level;}

  void           Clear();
  void           SetFunctorName(QString name);
  void           AddVariable(QString name, double start, double step, double min, double max);
  void           AddVariable(QString name, double start, double step);
  void           AddFixedVariable(QString name, double value);
  void           AddLowerLimitedVariable(QString name, double value, double step, double lowerLimit);
  void           AddUpperLimitedVariable(QString name, double value, double step, double upperLimit);

  void           AddAllVariables(QVariant array);

  void           SetSimplex();
  void           SetMigrad();

  virtual bool   Run();

  const QVariant GetResults() const {return Results;}

protected:
  AScriptManager* ScriptManager = nullptr;
  QVector<AVarRecordBase*> Variables;
  QVariantList    Results;

  bool            bHighPrecision = false;
  int             PrintVerbosity = -1;
  int             Method = 0; // 0-Migrad, 1-Simplex

  virtual ROOT::Math::Functor * configureFunctor() = 0;
};

class AMini_JavaScript_SI : public AInterfaceToMinimizerScript
{
    Q_OBJECT

public:
    AMini_JavaScript_SI(AJavaScriptManager * ScriptManager);
    AMini_JavaScript_SI(const AMini_JavaScript_SI & other);
    ~AMini_JavaScript_SI();

    void SetScriptManager(AJavaScriptManager * NewScriptManager);

    ROOT::Math::Functor * configureFunctor() override;

private:
    AFunctorBase * baseFunctor = nullptr;
};

#ifdef __USE_ANTS_PYTHON__
class APythonScriptManager;

class AMini_Python_SI : public AInterfaceToMinimizerScript
{
    Q_OBJECT

public:
    AMini_Python_SI(APythonScriptManager * ScriptManager);
    ~AMini_Python_SI();

    ROOT::Math::Functor * configureFunctor() override;

private:
    AFunctorBase * baseFunctor = nullptr;
};
#endif

#endif // SCRIPTMINIMIZER_H
