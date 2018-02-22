#ifndef SCRIPTMINIMIZER_H
#define SCRIPTMINIMIZER_H

#include "localscriptinterfaces.h"

#include <QObject>
#include <QVector>

class MainWindow;
class AScriptManager;

class AInterfaceToMinimizerScript : public AScriptInterface
{
  Q_OBJECT

public:
  AInterfaceToMinimizerScript(AScriptManager* ScriptManager);
  AInterfaceToMinimizerScript(const AInterfaceToMinimizerScript& other);
  ~AInterfaceToMinimizerScript() {}

  bool IsMultithreadCapable() const override {return true;}

  void ForceStop() override;

  void SetScriptManager(AScriptManager* NewScriptManager) {ScriptManager = NewScriptManager;}

public slots:

  void SetHighPrecision(bool flag) {bHighPrecision = flag;}
  void SetVerbosity(int level) {PrintVerbosity = level;}

  void Clear();
  void SetFunctorName(QString name);
  void AddVariable(QString name, double start, double step, double min, double max);
  void ModifyVariable(int varNumber, double start, double step, double min, double max);
  const QString Run();

  void Test();

private:
  AScriptManager* ScriptManager;
  QVector<QString> Name;
  QVector<double> Start, Step, Min, Max;

  bool bHighPrecision = false;
  int  PrintVerbosity = -1;

};

#endif // SCRIPTMINIMIZER_H
