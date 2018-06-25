#ifndef AINTERFACETOMULTITHREAD_H
#define AINTERFACETOMULTITHREAD_H

#include "ascriptinterface.h"

#include <QString>
#include <QObject>
#include <QVector>
#include <QScriptValue>
#include <QVariant>
#include <QVariantList>

class AJavaScriptManager;
class QThread;

class AScriptThreadBase;

class AInterfaceToMultiThread : public AScriptInterface
{
    Q_OBJECT

friend class AScriptThreadBase;

public:
    AInterfaceToMultiThread(AJavaScriptManager *MasterScriptManager);

    virtual void  ForceStop() override;

public slots:

    void          evaluateScript(const QString script);
    void          evaluateFunction(const QVariant function, const QVariant arguments = QVariant());

    void          waitForAll();
    void          waitForOne(int IndexOfWorker);

    void          abortAll();
    void          abortOne(int IndexOfWorker);

    int           countAll();
    int           countNotFinished();

    QVariant      getResult(int IndexOfWorker);
    bool          isAborted(int IndexOfWorker);

    void          deleteAll();
    bool          deleteOne(int IndexOfWorker);

    void          setAbortInThreadIsGlobal(bool flag) {bAbortIsGlobal = flag;}

private:
    AJavaScriptManager *MasterScriptManager;
    QVector<AScriptThreadBase*> workers;

    bool          bAbortIsGlobal = true;

    void          startEvaluation(AJavaScriptManager *sm, AScriptThreadBase* worker);

private slots:
    void          onErrorInTread(AScriptThreadBase *workerWithError);
};

class AScriptThreadBase : public QObject
{
    Q_OBJECT

public:
    AScriptThreadBase(AJavaScriptManager* ScriptManager = 0);
    virtual         ~AScriptThreadBase();

    bool            isRunning() {return bRunning;}
    void            abort();
    QVariant        getResult() {return Result;}
    bool            isAborted() const;

public slots:
    virtual void    Run() = 0;

signals:
    void            errorFound(AScriptThreadBase*);

public://protected:
    AJavaScriptManager* ScriptManager = 0;
    bool            bRunning = false;
    QVariant        Result = QString("Evaluation was not yet performed");

    const QVariant  resultToQVariant(const QScriptValue& result) const;
};

class AScriptThreadScr : public AScriptThreadBase
{
    Q_OBJECT

public:
    AScriptThreadScr(){}
    AScriptThreadScr(AJavaScriptManager* ScriptManager, const QString& Script);

public slots:
    virtual void    Run() override;

private:
    const QString   Script;
};

class AScriptThreadFun : public AScriptThreadBase
{
    Q_OBJECT

public:
    AScriptThreadFun(){}
    AScriptThreadFun(AJavaScriptManager* ScriptManager, const QString& Function, const QVariant& Arguments);

public slots:
    virtual void       Run() override;

private:
    const QString      Function;
    const QVariant     Arguments;
};

#endif // AINTERFACETOMULTITHREAD_H
