#ifndef SCRIPTMINIMIZER_H
#define SCRIPTMINIMIZER_H

#include "localscriptinterfaces.h"

#include <QObject>
#include <QVector>
#include <QVariant>

#include <string>

class MainWindow;
class AScriptManager;

class AInterfaceToMinimizerScript : public AScriptInterface
{
  Q_OBJECT

  struct AVarRecord
  {
      AVarRecord(QString Name, double Start, double Step, double Min, double Max) :
          Name(Name.toLatin1().data()), Start(Start), Step(Step), Min(Min), Max(Max), bFixed(false) {}
      AVarRecord(QString Name, double Val) :
          Name(Name.toLatin1().data()), Value(Val), bFixed(true) {}
      AVarRecord(){}

      std::string Name;
      double Start, Step, Min, Max;

      bool bFixed;
      double Value;
  };

public:
  AInterfaceToMinimizerScript(AScriptManager* ScriptManager);
  AInterfaceToMinimizerScript(const AInterfaceToMinimizerScript& other);
  ~AInterfaceToMinimizerScript() {}

  bool IsMultithreadCapable() const override {return true;}

  void ForceStop() override;

  void SetScriptManager(AScriptManager* NewScriptManager) {ScriptManager = NewScriptManager;}

public slots:

  void           SetHighPrecision(bool flag) {bHighPrecision = flag;}
  void           SetVerbosity(int level) {PrintVerbosity = level;}

  void           Clear();
  void           SetFunctorName(QString name);
  void           AddVariable(QString name, double start, double step, double min, double max);
  void           AddFixedVariable(QString name, double value);

  bool           Run();

  const QVariant GetResults() const {return Results;}

private:
  AScriptManager* ScriptManager;
  QVector<AVarRecord> Variables;
  QVariantList    Results;

  bool bHighPrecision = false;
  int  PrintVerbosity = -1;

};

#endif // SCRIPTMINIMIZER_H
