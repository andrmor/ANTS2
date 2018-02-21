#ifndef ASCRIPTMANAGER_H
#define ASCRIPTMANAGER_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QScriptValue>

class QScriptEngine;
class TRandom2;

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

    const QString   getFunctionReturnType(const QString& UnitFunction);
    void            collectGarbage();

    QScriptValue    getMinimalizationFunction();

    void            deleteMsgDialog();  //needed in batch mode to force close MSG window if shown
    void            hideMsgDialog();
    void            restoreMsgDialog();

public slots:
    void            AbortEvaluation(QString message = "Aborted!");

public:
    //registered interfaces (units)
    QVector<QObject*> interfaces;

    TRandom2*       RandGen;     //math module uses it
    QString         LastError;

    //starter dirs
    QString         LibScripts, LastOpenDir, ExamplesDir;

    //for minimizer
    QString         MiniFunctionName;
    double          MiniBestResult;
    int             MiniNumVariables;

private:
    QScriptEngine*  engine;
    bool            fEngineIsRunning;
    bool            fAborted;

signals:
    void            onStart();
    void            onAbort();
    void            success(QString eval);
    void            showMessage(QString message);
    void            clearText();
};

#endif // ASCRIPTMANAGER_H
