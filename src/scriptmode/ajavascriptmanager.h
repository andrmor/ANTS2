#ifndef AJAVASCRIPTMANAGER_H
#define AJAVASCRIPTMANAGER_H

#include "ascriptmanager.h"

#include <QObject>
#include <QVector>
#include <QString>
#include <QScriptValue>

#ifdef GUI
#include "ascriptmessengerdialog.h"
class AMsg_SI;
#endif

class QScriptEngine;
class TRandom2;
class QDialog;

class AJavaScriptManager : public AScriptManager
{
    Q_OBJECT

public:
    AJavaScriptManager(TRandom2 *RandGen);
    ~AJavaScriptManager();

    void            RegisterInterfaceAsGlobal(AScriptInterface * interface) override;
    void            RegisterCoreInterfaces(bool bCore = true, bool bMath = true) override;
    void            RegisterInterface(AScriptInterface * interface, const QString & name) override;

    int             FindSyntaxError(const QString & script) override; //returns line number of the first syntax error; -1 if no errors found
    QString         Evaluate(const QString & Script) override;
    QVariant        EvaluateScriptInScript(const QString & script) override;

    void            abortEvaluation() override;
    void            collectGarbage() override;

    bool            isUncaughtException() const override;
    int             getUncaughtExceptionLineNumber() const override;
    QString         getUncaughtExceptionString() const override;

#ifdef GUI
    void            hideMsgDialogs() override;
    void            restoreMsgDialogs() override;
public slots:
    void            hideAllMessengerWidgets();
    void            showAllMessengerWidgets();
    void            clearUnusedMsgDialogs();
    void            closeAllMsgDialogs();
#endif

public:
    QScriptValue    getMinimalizationFunction();
    AJavaScriptManager * createNewScriptManager(int threadNumber, bool bAbortIsGlobal); // *** !!!
    QScriptValue    getProperty(const QString & properyName) const;
    QScriptValue    registerNewVariant(const QVariant & Variant);

protected:
    void            updateBlockCommentStatus(const QString & Line, bool & bInsideBlockComments) const override;

public:
    //for multithread-in-scripting
    QScriptValue    EvaluationResult;

private:
    QScriptEngine * engine = nullptr;
#ifdef GUI
    QVector<AScriptMessengerDialog*> ThreadMessangerDialogs;
#endif
    ACore_SI *      coreObj = nullptr;  //core interface - to forward evaluate-script-in-script

private:
    void            doRegister(AScriptInterface * interface, const QString & name);
    void            addQVariantToString(const QVariant & var, QString & string);
};

#endif // AJAVASCRIPTMANAGER_H
