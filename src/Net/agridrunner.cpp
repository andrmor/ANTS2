#include "agridrunner.h"
#include "awebsocketsession.h"
#include "aremoteserverrecord.h"
#include "ajsontools.h"

#include <QThread>
#include <QCoreApplication>
#include <QTimer>

void AGridRunner::CheckStatus(QVector<ARemoteServerRecord*>& Servers)
{
    QVector<AWebSocketWorker*> workers;

    for (int i = 0; i < Servers.size(); i++)
        workers << startCheckStatusOfServer(i, Servers[i]);

    qDebug() << "Waiting...";

    bool bStillWorking;
    do
    {
        bStillWorking = false;
        for (AWebSocketWorker* w : workers)
            if (w->isRunning()) bStillWorking = true;
        emit requestGuiUpdate();
        QCoreApplication::processEvents();
        QThread::usleep(100);
    }
    while (bStillWorking);

    qDebug() << "All threads finished!";

    for (AWebSocketWorker* w : workers) delete w;
}

void AGridRunner::onRequestTextLog(int index, const QString message)
{
    qDebug() << index << "--->" << message;
    emit requestTextLog(index, message);
}

AWebSocketWorker* AGridRunner::startCheckStatusOfServer(int index, ARemoteServerRecord* Server)
{
    qDebug() << "Starting checking for record#" << index << Server->IP << Server->Port << TimeOut;
    AWebSocketWorker* worker = new AWebSocketWorker(index, Server, TimeOut);
    QObject::connect(worker, &AWebSocketWorker::requestTextLog, this, &AGridRunner::onRequestTextLog, Qt::QueuedConnection);

    qDebug() << "Worker made";

    //worker->checkStatus();
    QThread* t = new QThread();
    QObject::connect(t, &QThread::started, worker, &AWebSocketWorker::checkStatus);
    QObject::connect(worker, &AWebSocketWorker::finished, t, &QThread::quit);
    QObject::connect(t, &QThread::finished, t, &QThread::deleteLater);
    worker->moveToThread(t);

    worker->setStarted(); //otherwise problems on start - on check it will be still false
    t->start();

    return worker;
}

AWebSocketWorker::AWebSocketWorker(int index, ARemoteServerRecord *rec, int timeOut) :
    index(index), rec(rec), TimeOut(timeOut) {}

void AWebSocketWorker::onTimer()
{
    timeElapsed += timerInterval;
    rec->Progress = 100.0 * timeElapsed / TimeOut;
    //qDebug() << "Timer!"<<timeElapsed << "progress:" << rec->Progress;
}

void AWebSocketWorker::checkStatus()
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
    QObject::connect(&timer, &QTimer::timeout, this, &AWebSocketWorker::onTimer);
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

