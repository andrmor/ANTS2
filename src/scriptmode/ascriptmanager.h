#ifndef ASCRIPTMANAGER_H
#define ASCRIPTMANAGER_H
#include "ascriptmessengerdialog.h"
#include <QObject>
#include <QVector>
#include <QString>
#include <QScriptValue>

class QScriptEngine;
class TRandom2;
class QElapsedTimer;
class AInterfaceToCore;
class QDialog;
class AInterfaceToMessageWindow;

class AScriptManager : public QObject
{
    Q_OBJECT

public:
    AScriptManager(TRandom2 *RandGen);
    ~AScriptManager();    

    //configuration
    void            SetInterfaceObject(QObject* interfaceObject, QString name = "");

    //run
    int             FindSyntaxError(QString script); //returns line number of the first syntax error; -1 if no errors found
    QString         Evaluate(QString Script);
    QScriptValue    EvaluateScriptInScript(const QString& script);

    bool            isEngineRunning() const {return fEngineIsRunning;}
    bool            isEvalAborted() const {return fAborted;}
    bool            isUncaughtException() const;
    int             getUncaughtExceptionLineNumber() const;
    const QString   getUncaughtExceptionString() const;
    const QString   getLastError() const {return LastError;}

    const QString   getFunctionReturnType(const QString& UnitFunction);
    void            collectGarbage();

    QScriptValue    getMinimalizationFunction();

    void            deleteMsgDialogs();  //needed in batch mode to force close MSG window if shown
    void            hideMsgDialogs();
    void            restoreMsgDialogs();

    //for multithread-in-scripting
    AScriptManager* createNewScriptManager(int threadNumber); // *** !!!
    void            abortEvaluation();
    QScriptValue    getProperty(const QString& properyName) const;
    QScriptValue    registerNewVariant(const QVariant &Variant);

    QScriptValue    EvaluationResult;

    qint64          getElapsedTime();

public slots:
    void            AbortEvaluation(QString message = "Aborted!");

    void            hideAllMessengerWidgets();
    void            showAllMessengerWidgets();
    void            clearUnusedMsgDialogs();

public:
    //registered interfaces (units)
    QVector<QObject*> interfaces;

    AInterfaceToCore* coreObj = 0;  //core interface - to forward evaluate-script-in-script

    TRandom2*       RandGen;     //math module uses it


    //starter dirs
    QString         LibScripts, LastOpenDir, ExamplesDir;

    //for minimizer
    QString         MiniFunctionName;
    double          MiniBestResult;
    int             MiniNumVariables;

    QScriptEngine*  engine;
private:

    bool            fEngineIsRunning;
    bool            fAborted;

    QString         LastError;

    QElapsedTimer*  timer;
    qint64          timeOfStart;
    qint64          timerEvalTookMs;

    QVector<AScriptMessengerDialog*> ThreadMessangerDialogs;

signals:
    void            onStart();
    void            onAbort();
    void            onFinish(QString eval);

    void            showMessage(QString message);
    void            clearText();
};

#endif // ASCRIPTMANAGER_H
