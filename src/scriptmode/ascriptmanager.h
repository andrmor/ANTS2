#ifndef ASCRIPTMANAGER_H
#define ASCRIPTMANAGER_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QVariant>

class TRandom2;
class AInterfaceToCore;
class QElapsedTimer;

class AScriptManager : public QObject
{
  Q_OBJECT

public:
  AScriptManager(TRandom2 *RandGen);
  virtual ~AScriptManager();

  //configuration
  virtual void      SetInterfaceObject(QObject* interfaceObject, QString name = "") = 0;

  //run
  virtual int       FindSyntaxError(const QString & /*script*/ ) {return -1;} //returns line number of the first syntax error; -1 if no errors found
  virtual QString   Evaluate(const QString &Script) = 0;
  virtual QVariant  EvaluateScriptInScript(const QString& script) = 0;

  virtual bool      isUncaughtException() const {return false;}
  virtual int       getUncaughtExceptionLineNumber() const {return -1;}
  virtual const QString getUncaughtExceptionString() const {return "";}

  virtual void      collectGarbage(){}
  virtual void      abortEvaluation() = 0;

  virtual void      hideMsgDialogs();
  virtual void      restoreMsgDialogs();


  bool              isEngineRunning() const {return fEngineIsRunning;}
  bool              isEvalAborted() const {return fAborted;}

  const QString&    getLastError() const {return LastError;}
  qint64            getElapsedTime();
  const QString     getFunctionReturnType(const QString& UnitFunction);

  void              deleteMsgDialogs();

  void              ifError_AbortAndReport();

public slots:
  virtual void      AbortEvaluation(QString message = "Aborted!");

public:
  QVector<QObject*> interfaces;  //registered interfaces (units)
  TRandom2*         RandGen;     //math module uses it

  //starter dirs
  QString           LibScripts, LastOpenDir, ExamplesDir;

  //for minimizer
  QString           MiniFunctionName;
  double            MiniBestResult;
  int               MiniNumVariables;

protected:
  bool              fEngineIsRunning;
  bool              fAborted;

  QString           LastError;

  QElapsedTimer*    timer;
  qint64            timeOfStart;
  qint64            timerEvalTookMs;

signals:
    void            onStart();
    void            onAbort();
    void            onFinish(QString eval);

    void            showMessage(QString message);
    void            clearText();
    void            requestHighlightErrorLine(int lineNumber);

    void            reportProgress(int percent);
};

#endif // ASCRIPTMANAGER_H
