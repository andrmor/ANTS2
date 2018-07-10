#include "agridrunner.h"
#include "awebsocketsession.h"
#include "aremoteserverrecord.h"
#include "ajsontools.h"

#include <QThread>
#include <QCoreApplication>
#include <QTimer>

void AGridRunner::CheckStatus(QVector<ARemoteServerRecord*>& Servers)
{
    QVector<AWebSocketWorker_Base*> workers;

    for (int i = 0; i < Servers.size(); i++)
        workers << startCheckStatusOfServer(i, Servers[i]);

    waitForWorkersToFinish(workers);
}

void AGridRunner::Simulate(QVector<ARemoteServerRecord *> &Servers, const QJsonObject* config)
{
    QVector<AWebSocketWorker_Base*> workers;

    for (int i = 0; i < Servers.size(); i++)
        workers << startSimulationOnServer(i, Servers[i], config);

    waitForWorkersToFinish(workers);
}

void AGridRunner::waitForWorkersToFinish(QVector<AWebSocketWorker_Base*>& workers)
{
    qDebug() << "Waiting...";

    bool bStillWorking;
    do
    {
        bStillWorking = false;
        for (AWebSocketWorker_Base* w : workers)
            if (w->isRunning()) bStillWorking = true;
        emit requestGuiUpdate();
        QCoreApplication::processEvents();
        QThread::usleep(100);
    }
    while (bStillWorking);

    qDebug() << "All threads finished!";

    for (AWebSocketWorker_Base* w : workers)
        delete w;
}

void AGridRunner::onRequestTextLog(int index, const QString message)
{
    //qDebug() << index << "--->" << message;
    emit requestTextLog(index, message);
}

AWebSocketWorker_Base* AGridRunner::startCheckStatusOfServer(int index, ARemoteServerRecord* server)
{
    qDebug() << "Starting checking for record#" << index << server->IP << server->Port << TimeOut;
    AWebSocketWorker_Base* worker = new AWebSocketWorker_Check(index, server, TimeOut);

    startInNewThread(worker);
    return worker;
}

AWebSocketWorker_Base* AGridRunner::startSimulationOnServer(int index, ARemoteServerRecord *server, const QJsonObject* config)
{
    qDebug() << "Starting simulation for server#" << index << server->IP;
    AWebSocketWorker_Base* worker = new AWebSocketWorker_Sim(index, server, TimeOut, config);

    startInNewThread(worker);
    return worker;
}

void AGridRunner::startInNewThread(AWebSocketWorker_Base* worker)
{
    QObject::connect(worker, &AWebSocketWorker_Base::requestTextLog, this, &AGridRunner::onRequestTextLog, Qt::QueuedConnection);

    QThread* t = new QThread();
    QObject::connect(t, &QThread::started, worker, &AWebSocketWorker_Base::run);
    QObject::connect(worker, &AWebSocketWorker_Base::finished, t, &QThread::quit);
    QObject::connect(t, &QThread::finished, t, &QThread::deleteLater);
    worker->moveToThread(t);

    worker->setStarted(); //otherwise problems on start - in first check it will be still false
    t->start();
}

// ----------------------- Workers ----------------------------

AWebSocketWorker_Base::AWebSocketWorker_Base(int index, ARemoteServerRecord *rec, int timeOut) :
    index(index), rec(rec), TimeOut(timeOut) {}

AWebSocketWorker_Check::AWebSocketWorker_Check(int index, ARemoteServerRecord *rec, int timeOut) :
    AWebSocketWorker_Base(index, rec, timeOut) {}

void AWebSocketWorker_Check::onTimer()
{
    timeElapsed += timerInterval;
    rec->Progress = 100.0 * timeElapsed / TimeOut;
    //qDebug() << "Timer!"<<timeElapsed << "progress:" << rec->Progress;
}

void AWebSocketWorker_Check::run()
{
    bRunning = true;

    const QString url = "ws://" + rec->IP + ':' + QString::number(rec->Port);
    qDebug() << url;
    emit requestTextLog(index, QString("Connecting to dispatcher ") + url);

    AWebSocketSession* socket = new AWebSocketSession();
    socket->SetTimeout(TimeOut);
    rec->NumThreads = 0;
    qDebug() << "Connecting...";

    QTimer timer;
    timer.setInterval(timerInterval);
    timeElapsed = 0;
    QObject::connect(&timer, &QTimer::timeout, this, &AWebSocketWorker_Check::onTimer);
    rec->Progress = 0;
    timer.start();

    rec->Status = ARemoteServerRecord::Connecting;

    bool bOK = socket->Connect(url, true);
    if (!bOK)
    {
        rec->Status = ARemoteServerRecord::Dead;
        rec->Error = socket->GetError();
        emit requestTextLog(index, QString("Connection failed: ") + socket->GetError());
    }
    else
    {
        QString reply = socket->GetTextReply();
        qDebug() << "On connect reply:"<<reply;
        QJsonObject json = strToObject(reply);
        if (!json.contains("result") || !json["result"].toBool())
        {
            rec->Status = ARemoteServerRecord::Dead;
            rec->Error = socket->GetError();
            emit requestTextLog(index, QString("Connection failed: ") + socket->GetError());
        }
        else
        {
            emit requestTextLog(index, "Requesting status");
            bOK = socket->SendText( "{ \"command\" : \"report\" }" );
            if (!bOK)
            {
                rec->Status = ARemoteServerRecord::Dead;
                rec->Error = socket->GetError();
                emit requestTextLog(index, "Failed to receive status!");
            }
            else
            {
                reply = socket->GetTextReply();
                qDebug() << "On request status reply:"<< reply;
                emit requestTextLog(index, QString("Dispatcher reply: ") + reply);

                socket->Disconnect();

                json = strToObject(reply);
                parseJson(json, "threads", rec->NumThreads);
                qDebug() << "Available threads:"<<rec->NumThreads;
                rec->Status = ARemoteServerRecord::Alive;
                emit requestTextLog(index, QString("Available threads: ") + QString::number(rec->NumThreads));
            }
        }
    }

    timer.stop();
    socket->deleteLater();
    bRunning = false;
    emit finished();
}

AWebSocketWorker_Sim::AWebSocketWorker_Sim(int index, ARemoteServerRecord *rec, int timeOut, const QJsonObject *config) :
    AWebSocketWorker_Base(index, rec, timeOut), config(config) {}

void AWebSocketWorker_Sim::run()
{

}
