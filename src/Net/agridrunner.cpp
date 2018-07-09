#include "agridrunner.h"
#include "awebsocketsession.h"
#include "ajsontools.h"

#include <QThread>
#include <QCoreApplication>

void AGridRunner::CheckStatus(QVector<ARemoteServerRecord>& Servers)
{
    for (int i = 0; i < Servers.size(); i++)
        startCheckStatusOfServer(i, Servers[i]);
}

void AGridRunner::onRequestTextLog(int index, const QString message)
{
    qDebug() << index << "--->" << message;
    emit requestTextLog(index, message);
}

void AGridRunner::startCheckStatusOfServer(int index, ARemoteServerRecord& Server)
{
    qDebug() << index << Server.IP << Server.Port << TimeOut;
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

    qDebug() << "Waiting...";
    while (worker->isRunning())
    {
        QCoreApplication::processEvents();
        QThread::usleep(200);
    }
    qDebug() << "Returned";
}

AWebSocketWorker::AWebSocketWorker(int index, ARemoteServerRecord& rec, int timeOut) :
    index(index), rec(rec), TimeOut(timeOut)
{
    qDebug() << "constr:"<<rec.IP;
}

void AWebSocketWorker::checkStatus()
{
    bRunning = true;

    const QString url = "ws://" + rec.IP + ':' + QString::number(rec.Port);
    qDebug() << url;
    emit requestTextLog(index, QString("Connecting to dispatcher ") + url);

    AWebSocketSession* socket = new AWebSocketSession();
    socket->SetTimeout(TimeOut);
    rec.NumThreads = 0;
    qDebug() << "Connecting...";

    bool bOK = socket->Connect(url, true);
    if (!bOK)
    {
        rec.error = socket->GetError();
        emit requestTextLog(index, QString("Connection failed: ") + socket->GetError());
    }
    else
    {
        QString reply = socket->GetTextReply();
        qDebug() << "On connect reply:"<<reply;
        QJsonObject json = strToObject(reply);
        if (!json.contains("result") || !json["result"].toBool())
        {
            rec.error = socket->GetError();
            emit requestTextLog(index, QString("Connection failed: ") + socket->GetError());
        }
        else
        {
            emit requestTextLog(index, "Requesting status");
            bOK = socket->SendText( "{ \"command\" : \"report\" }" );
            if (!bOK)
            {
                rec.error = socket->GetError();
                emit requestTextLog(index, "Failed to receive status!");
            }
            else
            {
                reply = socket->GetTextReply();
                qDebug() << "On request status reply:"<< reply;
                emit requestTextLog(index, QString("Dispatcher reply: ") + reply);

                socket->Disconnect();

                json = strToObject(reply);
                parseJson(json, "threads", rec.NumThreads);
                qDebug() << "Available threads:"<<rec.NumThreads;
                emit requestTextLog(index, QString("Available threads: ") + QString::number(rec.NumThreads));
            }
        }
    }

    socket->deleteLater();
    bRunning = false;

    emit finished();
}

