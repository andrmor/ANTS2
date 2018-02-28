#ifndef SCRIPTMINIMIZER_H
#define SCRIPTMINIMIZER_H

#include "localscriptinterfaces.h"

#include <QObject>
#include <QVector>
#include <QVariant>

#include <string>

class MainWindow;
class AScriptManager;
namespace ROOT { namespace Minuit2 { class Minuit2Minimizer; } }

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
  AInterfaceToMinimizerScript(const AInterfaceToMinimizerScript& other);
  ~AInterfaceToMinimizerScript();

  bool           IsMultithreadCapable() const override {return true;}
  void           ForceStop() override;

  void           SetScriptManager(AScriptManager* NewScriptManager) {ScriptManager = NewScriptManager;}

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

  bool           Run();

  const QVariant GetResults() const {return Results;}

private:
  AScriptManager* ScriptManager;
  QVector<AVarRecordBase*> Variables;
  QVariantList    Results;

  bool            bHighPrecision = false;
  int             PrintVerbosity = -1;
  int             Method = 0; // 0-Migrad, 1-Simplex

};

#endif // SCRIPTMINIMIZER_H
