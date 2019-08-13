#ifndef AJAVASCRIPTMANAGER_H
#define AJAVASCRIPTMANAGER_H

#include "ascriptmanager.h"
#ifdef GUI
#include "ascriptmessengerdialog.h"
#endif
#include <QObject>
#include <QVector>
#include <QString>
#include <QScriptValue>

class QScriptEngine;
class TRandom2;
class QDialog;
#ifdef GUI
class AMsg_SI;
#endif

class AJavaScriptManager : public AScriptManager
{
    Q_OBJECT

public:
    AJavaScriptManager(TRandom2 *RandGen);
    ~AJavaScriptManager();

    //configuration
    //virtual void    SetInterfaceObject(QObject* interfaceObject, QString name = "") override;
    virtual void    RegisterInterfaceAsGlobal(AScriptInterface* interface) override;
    virtual void    RegisterCoreInterfaces(bool bCore = true, bool bMath = true) override;
    virtual void    RegisterInterface(AScriptInterface* interface, const QString& name) override;

    //run
    virtual int     FindSyntaxError(const QString &script) override; //returns line number of the first syntax error; -1 if no errors found
    virtual QString Evaluate(const QString &Script) override;
    virtual QVariant EvaluateScriptInScript(const QString& script) override;

    virtual void    abortEvaluation() override;
    virtual void    collectGarbage() override;

    virtual bool    isUncaughtException() const override;
    virtual int     getUncaughtExceptionLineNumber() const override;
    virtual const QString getUncaughtExceptionString() const override;
#ifdef GUI
    virtual void    hideMsgDialogs() override;
    virtual void    restoreMsgDialogs() override;
#endif
    QScriptValue    getMinimalizationFunction();

    //for multithread-in-scripting
    AJavaScriptManager* createNewScriptManager(int threadNumber, bool bAbortIsGlobal); // *** !!!
    QScriptValue    getProperty(const QString& properyName) const;
    QScriptValue    registerNewVariant(const QVariant &Variant);
    QScriptValue    EvaluationResult;
    ACore_SI* coreObj = 0;  //core interface - to forward evaluate-script-in-script

public slots:
#ifdef GUI
    void            hideAllMessengerWidgets();
    void            showAllMessengerWidgets();
    void            clearUnusedMsgDialogs();
    void            closeAllMsgDialogs();
#endif
private:
    QScriptEngine*  engine;
#ifdef GUI
    QVector<AScriptMessengerDialog*> ThreadMessangerDialogs;
#endif

private:
    void doRegister(AScriptInterface *interface, const QString &name);
    void addQVariantToString(const QVariant &var, QString &string);
};

#endif // AJAVASCRIPTMANAGER_H
