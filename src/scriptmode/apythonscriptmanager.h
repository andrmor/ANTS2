#ifndef APYTHONSCRIPTMANAGER_H
#define APYTHONSCRIPTMANAGER_H

#include "PythonQtObjectPtr.h"
#include "ascriptmanager.h"

#include <QObject>

class QScriptEngine;
class TRandom2;
class QDialog;
class AInterfaceToMessageWindow;
class PythonQtObjectPtr;

class APythonScriptManager : public AScriptManager
{
    Q_OBJECT

public:
    APythonScriptManager(TRandom2 *RandGen);
    ~APythonScriptManager() {}

    //configuration
    //virtual void      SetInterfaceObject(QObject* interfaceObject, QString name) override;
    virtual void      RegisterInterfaceAsGlobal(AScriptInterface* interface) override;
    virtual void      RegisterCoreInterfaces(bool bCore = true, bool bMath = true) override;
    virtual void      RegisterInterface(AScriptInterface* interface, const QString& name) override;

    //run
    virtual QString   Evaluate(const QString &Script) override;
    virtual QVariant  EvaluateScriptInScript(const QString& script) override;

    virtual void      abortEvaluation() override;

    PythonQtObjectPtr MinimizationFunctor;
    PythonQtObjectPtr GlobalDict;

private:
    void handleError();

private slots:
    void stdOut(const QString& s);
    void stdErr(const QString& s);

};

#endif // APYTHONSCRIPTMANAGER_H
