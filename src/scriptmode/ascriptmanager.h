#ifndef ASCRIPTMANAGER_H
#define ASCRIPTMANAGER_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QVariant>

class AScriptInterface;
class TRandom2;
class ACoreScriptInterface;
class QElapsedTimer;

class AScriptManager : public QObject
{
  Q_OBJECT

public:
  AScriptManager(TRandom2 *RandGen);
  virtual ~AScriptManager();

  //configuration
  virtual void      RegisterInterfaceAsGlobal(AScriptInterface* interface) = 0;
  virtual void      RegisterCoreInterfaces(bool bCore = true, bool bMath = true) = 0;
  virtual void      RegisterInterface(AScriptInterface* interface, const QString& name) = 0;

  //run
  virtual int       FindSyntaxError(const QString & /*script*/ ) {return -1;} //returns line number of the first syntax error; -1 if no errors found
  virtual QString   Evaluate(const QString &Script) = 0;
  virtual QVariant  EvaluateScriptInScript(const QString& script) = 0;

  virtual bool      isUncaughtException() const {return false;}
  virtual int       getUncaughtExceptionLineNumber() const {return -1;}
  virtual const QString getUncaughtExceptionString() const {return "";}

  virtual void      collectGarbage(){}
  virtual void      abortEvaluation() = 0;

#ifdef GUI
  virtual void      hideMsgDialogs();
  virtual void      restoreMsgDialogs();
  void              deleteMsgDialogs();
#endif
  bool              isEngineRunning() const {return fEngineIsRunning;}
  bool              isEvalAborted() const {return fAborted;}

  const QString&    getLastError() const {return LastError;}
  qint64            getElapsedTime();
  const QString     getFunctionReturnType(const QString& UnitFunction);



  void              ifError_AbortAndReport();

public slots:
  virtual void      AbortEvaluation(QString message = "Aborted!");

public:
  QVector<AScriptInterface*> interfaces;  //registered interfaces (units)
  TRandom2*         RandGen;     //math module uses it

  //pointers to starter dirs
  QString*          LibScripts = 0;
  QString*          LastOpenDir = 0;
  QString*          ExamplesDir = 0;

  //for minimizer
  QString           MiniFunctionName;
  double            MiniBestResult;
  int               MiniNumVariables;

protected:
  bool              bOwnRandomGen = false;  //the main manager (GUI thread) does not own RandGen, but multithread managers do

  bool              fEngineIsRunning;
  bool              fAborted;

  QString           LastError;
  bool              bShowAbortMessageInOutput = true; //can be false -> in multithread if abort is local to this thread

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
