#ifndef SCRIPTMINIMIZER_H
#define SCRIPTMINIMIZER_H

#include "localscriptinterfaces.h"

#include <QObject>
#include <QVector>

class MainWindow;
class AScriptManager;

class InterfaceToMinimizerScript : public AScriptInterface
{
  Q_OBJECT

public:
  InterfaceToMinimizerScript(AScriptManager* ScriptManager);
  ~InterfaceToMinimizerScript() {}

  virtual void ForceStop();

public slots:  
  void Clear();
  void SetFunctorName(QString name);
  void AddVariable(QString name, double start, double step, double min, double max);
  void ModifyVariable(int varNumber, double start, double step, double min, double max);
  QString Run();

private:
  AScriptManager* ScriptManager;
  QVector<QString> Name;
  QVector<double> Start, Step, Min, Max;

};

#endif // SCRIPTMINIMIZER_H
