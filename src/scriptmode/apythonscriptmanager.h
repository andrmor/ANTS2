#ifndef APYTHONSCRIPTMANAGER_H
#define APYTHONSCRIPTMANAGER_H

#include "PythonQtObjectPtr.h"
#include "ascriptmanager.h"

#include <QObject>

class QScriptEngine;
class TRandom2;
class QDialog;
class AMsg_SI;
class PythonQtObjectPtr;

class APythonScriptManager : public AScriptManager
{
    Q_OBJECT

public:
    APythonScriptManager(TRandom2 *RandGen);
    ~APythonScriptManager() {}

    //configuration
    void      RegisterInterfaceAsGlobal(AScriptInterface * interface) override;
    void      RegisterCoreInterfaces(bool bCore = true, bool bMath = true) override;
    void      RegisterInterface(AScriptInterface * interface, const QString & name) override;

    //run
    QString   Evaluate(const QString & Script) override;
    QVariant  EvaluateScriptInScript(const QString & script) override;

    void      abortEvaluation() override;

    PythonQtObjectPtr MinimizationFunctor;
    PythonQtObjectPtr GlobalDict;

private:
    void handleError();

private slots:
    void stdOut(const QString& s);
    void stdErr(const QString& s);

};

#endif // APYTHONSCRIPTMANAGER_H
