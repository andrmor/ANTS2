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

AWebSocketSession* AWebSocketWorker_Base::connectToServer(int port)
{
    bRunning = true;

    const QString url = "ws://" + rec->IP + ':' + QString::number(port);
    emit requestTextLog(index, QString("Trying to connect to ") + url + "...");

    AWebSocketSession* socket = new AWebSocketSession();

    socket->SetTimeout(TimeOut);
    rec->NumThreads = 0;
    qDebug() << "Connecting to"<<rec->IP << "on port"<<port;

    bool bOK = socket->Connect(url, true);
    if (bOK)
    {
        QString reply = socket->GetTextReply();
        qDebug() << "On connect reply:"<<reply;
        QJsonObject json = strToObject(reply);
        if (!json.contains("result") || !json["result"].toBool())
            bOK = false;
    }

    if (!bOK)
    {
        rec->Status = ARemoteServerRecord::Dead;
        rec->Error = socket->GetError();
        emit requestTextLog(index, QString("Connection failed: ") + socket->GetError());
        delete socket;
        socket = 0;
    }

    return socket;
}

bool AWebSocketWorker_Base::allocateAntsServer()
{
    rec->AntsServerAllocated = false;
    bool bOK = false;

    emit requestTextLog(index, "Connecting to dispatcher");
    AWebSocketSession* socket = connectToServer(rec->Port);
    if (socket)
    {
        emit requestTextLog(index, "Requesting ANTS2 server...");
        QJsonObject cn;
        cn["command"] = "new";
        cn["threads"] = rec->NumThreads;
        QString strCn = jsonToString(cn);

        bOK = socket->SendText( strCn );
        if (!bOK)
            emit requestTextLog(index, "Failed to send request");
        else
        {
            QString reply = socket->GetTextReply();
            emit requestTextLog(index, QString("Dispatcher reply: ") + reply );
            QJsonObject ro = strToObject(reply);
            if ( !ro.contains("result") || !ro["result"].toBool())
            {
                bOK = false;
                emit requestTextLog(index, "Dispatcher refused to provide ANTS2 server!");
            }
            else
            {
                rec->AntsServerPort = ro["port"].toInt();
                rec->AntsServerTicket = ro["ticket"].toString();
                rec->AntsServerAllocated = true;
                emit requestTextLog(index, QString("Dispatcher allocated port ") + QString::number(rec->AntsServerPort) + "  and ticket " + rec->AntsServerTicket);
            }
        }

        socket->Disconnect();
        socket->deleteLater();
    }

    return bOK;
}

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

    QTimer timer;
    timer.setInterval(timerInterval);
    timeElapsed = 0;
    QObject::connect(&timer, &QTimer::timeout, this, &AWebSocketWorker_Check::onTimer);
    rec->Progress = 0;
    timer.start();

    emit requestTextLog(index, "Connecting to dispatcher");
    rec->Status = ARemoteServerRecord::Connecting;
    AWebSocketSession* socket = connectToServer(rec->Port);
    if (socket)
    {
        emit requestTextLog(index, "Requesting status...");
        bool bOK = socket->SendText( "{ \"command\" : \"report\" }" );
        if (!bOK)
        {
            rec->Status = ARemoteServerRecord::Dead;
            rec->Error = socket->GetError();
            emit requestTextLog(index, "Failed to acquire status!");
        }
        else
        {
            QString reply = socket->GetTextReply();
            qDebug() << "On request status reply:"<< reply;
            emit requestTextLog(index, QString("Dispatcher reply: ") + reply);

            socket->Disconnect();

            QJsonObject json = strToObject(reply);
            parseJson(json, "threads", rec->NumThreads);
            qDebug() << "Available threads:"<<rec->NumThreads;
            rec->Status = ARemoteServerRecord::Alive;
            emit requestTextLog(index, QString("Available threads: ") + QString::number(rec->NumThreads));
        }
    }

    timer.stop();
    if (socket) socket->deleteLater();
    bRunning = false;
    emit finished();
}

AWebSocketWorker_Sim::AWebSocketWorker_Sim(int index, ARemoteServerRecord *rec, int timeOut, const QJsonObject *config) :
    AWebSocketWorker_Base(index, rec, timeOut), config(config) {}

void AWebSocketWorker_Sim::run()
{
    bRunning = true;

    bool bOK = allocateAntsServer();
    if (bOK)
    {
        emit requestTextLog(index, "Connecting to ants2 server");
        AWebSocketSession* socket = connectToServer(rec->AntsServerPort);
        if (socket)
        {
            emit requestTextLog(index, QString("Sending ticket ") + rec->AntsServerTicket);

            QString m = "__";
            m += "{\"ticket\" : \"";
            m += rec->AntsServerTicket;
            m += "\"}";

            bOK = socket->SendText(m);
            if (!bOK)
            {
                emit requestTextLog(index, "Failed to send tiket!");
            }
            else
            {
                QString reply = socket->GetTextReply();
                QJsonObject ro = strToObject(reply);
                if ( !ro.contains("result") || !ro["result"].toBool() )
                {
                    emit requestTextLog(index, "Server rejected the ticket!");
                }

                emit requestTextLog(index, "Ants2 server is ready");
            }
        }
    }

    bRunning = false;
    emit finished();
}
