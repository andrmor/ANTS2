#include "agridrunner.h"
#include "awebsocketsession.h"
#include "aremoteserverrecord.h"
#include "eventsdataclass.h"
#include "apmhub.h"

#include "ajsontools.h"

#include <QThread>
#include <QCoreApplication>
#include <QTimer>
#include <QFile>

AGridRunner::AGridRunner(EventsDataClass *EventsDataHub, APmHub *PMs) :
    EventsDataHub(EventsDataHub), PMs(PMs) {}

void AGridRunner::CheckStatus(QVector<ARemoteServerRecord*>& Servers)
{
    emit requestStatusLog("Checking status of servers...");
    QVector<AWebSocketWorker_Base*> workers;

    for (int i = 0; i < Servers.size(); i++)
        workers << startCheckStatusOfServer(i, Servers[i]);

    waitForWorkersToFinish(workers);

    for (AWebSocketWorker_Base* w : workers) delete w;
    workers.clear();

    int ser = 0;
    int th = 0;
    for (const ARemoteServerRecord* r : Servers)
        if (r->NumThreads > 0)
        {
            ser++;
            th += r->NumThreads;
        }
    QString s = (th > 0 ? QString("Found %1 servers with %2 available threads").arg(ser).arg(th) : "There are no servers with available threads" );
    emit requestStatusLog(s);
}

void AGridRunner::Simulate(QVector<ARemoteServerRecord *> &Servers, const QJsonObject* config)
{
    emit requestStatusLog("Starting remote simulation...");
    QVector<AWebSocketWorker_Base*> workers;

    for (int i = 0; i < Servers.size(); i++)
        workers << startSim(i, Servers[i], config);

    waitForWorkersToPauseOrFinish(workers);

    for (int i = 0; i < workers.size(); i++)
        workers[i]->setPaused(false);

    waitForWorkersToFinish(workers);

    for (AWebSocketWorker_Base* w : workers) delete w;
    workers.clear();

    //loading simulated events
    EventsDataHub->clear();
    for (int i=0; i<Servers.size(); i++)
        if ( !Servers.at(i)->FileName.isEmpty() )
        {
            int numEv = EventsDataHub->loadSimulatedEventsFromTree( Servers.at(i)->FileName, PMs );
            QString s;
            if (EventsDataHub->ErrorString.isEmpty())
                s = QString("%1 events were registered").arg(numEv);
            else
                s = QString("Error loading events from the TTree file sent by the remote host:\n") + EventsDataHub->ErrorString;
            requestTextLog(i, s);
        }

    emit requestStatusLog("Done!");
}

void AGridRunner::Reconstruct(QVector<ARemoteServerRecord *> &Servers, const QJsonObject *config)
{
    emit requestStatusLog("Starting remote reconstruction...");
    QVector<AWebSocketWorker_Base*> workers;

    for (int i = 0; i < Servers.size(); i++)
        workers << startRec(i, Servers[i], config);

    waitForWorkersToPauseOrFinish(workers);

    int numSer = 0;
    int numThr = 0;
    for (int i = 0; i < workers.size(); i++)
    {
        if (Servers.at(i)->NumThreads > 0)
        {
            numSer++;
            numThr += Servers.at(i)->NumThreads;
        }
    }
    qDebug() << "Found"<<numSer<<"servers. Thread pool:"<<numThr;
    if (numThr == 0)
    {
        emit requestStatusLog("Cannot reconstruct: there are no available servers");
        return;
    }
    int numEvents = EventsDataHub->countEvents();
    int eventsPerOneThread = numEvents / numThr;
    if (eventsPerOneThread * numThr < numEvents) eventsPerOneThread++;

    int from = 0;
    for (int i = 0; i < workers.size(); i++)
    {
        Servers[i]->ByteArrayReceived.clear();
        Servers[i]->EventsFrom = 0;
        Servers[i]->EventsTo = 0;

        if (Servers.at(i)->NumThreads > 0)
        {
            Servers[i]->EventsFrom = from;
            int toDo = Servers.at(i)->NumThreads * eventsPerOneThread;
            if (from + toDo > numEvents) toDo = numEvents - from;
            Servers[i]->EventsTo = from + toDo;
            from += toDo;

            EventsDataHub->packEventsToByteArray(Servers[i]->EventsFrom, Servers[i]->EventsTo, Servers[i]->ByteArrayToSend);
        }
        workers[i]->setPaused(false);
    }

    waitForWorkersToFinish(workers);

    for (AWebSocketWorker_Base* w : workers) delete w;
    workers.clear();

    bool bRecFailed = false;
    for (int i = 0; i < Servers.size(); i++)
    {
        Servers[i]->ByteArrayToSend.clear();
        Servers[i]->ByteArrayToSend.squeeze();

        if (Servers.at(i)->EventsTo > 0)
        {
            if (!Servers.at(i)->Error.isEmpty()) bRecFailed = true;
            else
            {
                bool bOK = EventsDataHub->unpackReconstructedFromByteArray(Servers.at(i)->EventsFrom, Servers.at(i)->EventsTo, Servers.at(i)->ByteArrayReceived);
                if (!bOK)
                {
                    Servers[i]->Error = "Failed to set reconstruction data using the obtained data";
                    emit requestTextLog(i, Servers[i]->Error);
                    bRecFailed = true;
                }
            }
        }
    }

    if (bRecFailed) emit requestStatusLog("Reconstruction failed!");
    else
    {
        EventsDataHub->fReconstructionDataReady = true;
        emit requestStatusLog("Done!");
    }


    /*
    //loading simulated events
    EventsDataHub->clear();
    for (int i=0; i<Servers.size(); i++)
        if ( !Servers.at(i)->FileName.isEmpty() )
        {
            int numEv = EventsDataHub->loadSimulatedEventsFromTree( Servers.at(i)->FileName, PMs );
            QString s;
            if (EventsDataHub->ErrorString.isEmpty())
                s = QString("%1 events were registered").arg(numEv);
            else
                s = QString("Error loading events from the TTree file sent by the remote host:\n") + EventsDataHub->ErrorString;
            requestTextLog(i, s);
        }
    */

    emit requestStatusLog("Done!");
}

void AGridRunner::waitForWorkersToFinish(QVector<AWebSocketWorker_Base*>& workers)
{
    qDebug() << "Waiting for all threads to finish...";

    bool bStillWorking;
    do
    {
        bStillWorking = false;
        for (AWebSocketWorker_Base* w : workers)
            if (w->isRunning()) bStillWorking = true;
        emit requestDelegateGuiUpdate();
        QCoreApplication::processEvents();
        QThread::usleep(250);
    }
    while (bStillWorking);

    qDebug() << "All threads finished!";
}

void AGridRunner::waitForWorkersToPauseOrFinish(QVector<AWebSocketWorker_Base *> &workers)
{
    qDebug() << "Waiting for all threads to finish...";

    bool bStillWorking;
    do
    {
        bStillWorking = false;
        for (AWebSocketWorker_Base* w : workers)
            if (!w->isPausedOrFinished()) bStillWorking = true;
        emit requestDelegateGuiUpdate();
        QCoreApplication::processEvents();
        QThread::usleep(100);
    }
    while (bStillWorking);

    qDebug() << "All threads paused or finished!";
}

void AGridRunner::onRequestTextLog(int index, const QString message)
{
    //qDebug() << index << "--->" << message;
    emit requestTextLog(index, message);
}

AWebSocketWorker_Base* AGridRunner::startCheckStatusOfServer(int index, ARemoteServerRecord* serverRecord)
{
    qDebug() << "Starting checking for record#" << index << serverRecord->IP << serverRecord->Port << TimeOut;
    AWebSocketWorker_Base* worker = new AWebSocketWorker_Check(index, serverRecord, TimeOut);

    startInNewThread(worker);
    return worker;
}

AWebSocketWorker_Base *AGridRunner::startSim(int index, ARemoteServerRecord *serverRecord, const QJsonObject* config)
{
    AWebSocketWorker_Base* worker = new AWebSocketWorker_Sim(index, serverRecord, TimeOut, config);

    startInNewThread(worker);
    return worker;
}

AWebSocketWorker_Base *AGridRunner::startRec(int index, ARemoteServerRecord *serverrecord, const QJsonObject *config)
{
    AWebSocketWorker_Base* worker = new AWebSocketWorker_Rec(index, serverrecord, TimeOut, config);

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

AWebSocketWorker_Base::AWebSocketWorker_Base(int index, ARemoteServerRecord *rec, int timeOut, const QJsonObject *config) :
    index(index), rec(rec), TimeOut(timeOut), config(config) {}

AWebSocketSession* AWebSocketWorker_Base::connectToServer(int port)
{
    bRunning = true;

    const QString url = "ws://" + rec->IP + ':' + QString::number(port);
    emit requestTextLog(index, QString("Trying to connect to ") + url + "...");

    AWebSocketSession* socket = new AWebSocketSession();
    //  qDebug() << "Timeout set to:"<<TimeOut;
    socket->SetTimeout(TimeOut);

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
        delete socket;
    }

    return bOK;
}

AWebSocketSession* AWebSocketWorker_Base::connectToAntsServer()
{
    emit requestTextLog(index, "Connecting to ants2 server");

    bool bOK = false;
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
            emit requestTextLog(index, "Failed to send tiket!");
        else
        {
            QString reply = socket->GetTextReply();
            QJsonObject ro = strToObject(reply);
            if ( !ro.contains("result") || !ro["result"].toBool() )
            {
                bOK = false;
                emit requestTextLog(index, "Server rejected the ticket!");
            }
            emit requestTextLog(index, "Ants2 server is ready");
        }

        if (!bOK)
        {
            socket->Disconnect();
            delete socket;
            socket = 0;
        }
    }

    return socket;
}

bool AWebSocketWorker_Base::establishSession()
{
    ants2socket = 0;
    bool bOK = allocateAntsServer();
    if (bOK)
        ants2socket = connectToAntsServer();

    return ants2socket;
}

AWebSocketWorker_Check::AWebSocketWorker_Check(int index, ARemoteServerRecord *rec, int timeOut) :
    AWebSocketWorker_Base(index, rec, timeOut) {}

void AWebSocketWorker_Check::onTimer()
{
    timeElapsed += timerInterval;
    rec->Progress = 100.0 * timeElapsed / TimeOut;
    //  qDebug() << "Timer!"<<timeElapsed << "progress:" << rec->Progress;
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
    rec->NumThreads = 0;
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
    rec->Progress = 0;
    if (socket) delete socket;
    bRunning = false;
    emit finished();
}

AWebSocketWorker_Sim::AWebSocketWorker_Sim(int index, ARemoteServerRecord *rec, int timeOut, const QJsonObject *config) :
    AWebSocketWorker_Base(index, rec, timeOut, config) {}

void AWebSocketWorker_Sim::run()
{
    bRunning = true;

    rec->FileName.clear();

    bool bOK = establishSession();
    if (!bOK)
    {
        qDebug() << "failed to get server, aborting the thread and updating status";
        rec->Status = ARemoteServerRecord::Dead;
        rec->Progress = 0;
    }
    else
    {
        runSimulation();
    }

    bRunning = false;
    emit finished();
}

void AWebSocketWorker_Sim::runSimulation()
{
    rec->Error.clear();

    if (!ants2socket || !ants2socket->ConfirmSendPossible())
        rec->Error = "There is no connection to ANTS2 server";
    else
    {
        if (!config || !config->contains("SimulationConfig") || !config->contains("DetectorConfig"))
            rec->Error = "Config does not contain detector or simulation settings";
        else
        {
            qDebug() << "Sending config...";
            emit requestTextLog(index, "Sending config file...");
            bool bOK = ants2socket->SendJson(*config);
            QString reply = ants2socket->GetTextReply();
            QJsonObject ro = strToObject(reply);
            if (!bOK || !ro.contains("result") || !ro["result"].toBool())
                rec->Error = "Failed to send config";
            else
            {
                emit requestTextLog(index, "Sending script to setup configuration...");
                QString Script = "var c = server.GetBufferAsObject();"
                                 "var ok = config.SetConfig(c);"
                                 "if (!ok) core.abort(\"Failed to set config\");";
                bOK = ants2socket->SendText(Script);
                reply = ants2socket->GetTextReply();
                ro = strToObject(reply);
                if (!bOK || !ro.contains("result") || !ro["result"].toBool())
                    rec->Error = "failed to setup config on the ants2 server";
                else
                {
                    QJsonObject jsSimSet = (*config)["SimulationConfig"].toObject();
                    QString modeSetup = jsSimSet["Mode"].toString();
                    bool bPhotonSource = (modeSetup == "PointSim"); //Photon simulator
                    const QString RemoteSimTreeFileName = QString("SimTree-") + QString::number(index) + ".root";

                    Script.clear();
                    Script += "server.SetAcceptExternalProgressReport(true);";
                    //rec->NumThreads = 4;
                    if (bPhotonSource)
                        Script += "sim.RunPhotonSources(" + QString::number(rec->NumThreads) + ");";
                    else
                        Script += "sim.RunParticleSources(" + QString::number(rec->NumThreads) + ");";
                    Script += "var fileName = \"" + RemoteSimTreeFileName + "\";";
                    Script += "var ok = sim.SaveAsTree(fileName);";
                    Script += "if (!ok) core.abort(\"Failed to save simulation data\");";
                    Script += "server.SetAcceptExternalProgressReport(false);";
                    Script += "server.SendFile(fileName);";
                    //  qDebug() << Script;

                    //execute the script on remote server
                    emit requestTextLog(index, "Sending simulation script...");
                    bool bOK = ants2socket->SendText(Script);
                    if (!bOK)
                        rec->Error = "Failed to send script to ANTS2 server";
                    else
                    {
                        QString reply = ants2socket->GetTextReply();
                        if (reply.isEmpty())
                            rec->Error = "Got no reply after sending a script to ANTS2 server";
                        else
                        {
                            emit requestTextLog(index, reply);
                            QJsonObject obj = strToObject(reply);
                            if (obj.contains("error"))
                                rec->Error = obj["error"].toString();
                            else
                            {
                                rec->Status = ARemoteServerRecord::Progressing;
                                while ( !obj.contains("binary") ) //after server send back the file with sim, the reply is "{ \"binary\" : \"file\" }"
                                {
                                    emit requestTextLog(index, reply);
                                    bool bOK = ants2socket->ResumeWaitForAnswer();
                                    reply = ants2socket->GetTextReply();
                                    if (!bOK || reply.isEmpty())
                                    {
                                        rec->Error = ants2socket->GetError();
                                        break;
                                    }
                                    obj = strToObject(reply);
                                    if (obj.contains("progress"))
                                        rec->Progress = obj["progress"].toInt();
                                }
                                rec->Status = ARemoteServerRecord::Alive;
                                rec->Progress = 100;

                                if (rec->Error.isEmpty())
                                {
                                    emit requestTextLog(index, "Server has finished simulation");

                                    const QByteArray& ba = ants2socket->GetBinaryReply();
                                    qDebug() << "ByteArray to save size:"<<ba.size();

                                    QString LocalSimTreeFileName = RemoteSimTreeFileName; // !!!!!!!!!!!!!!
                                    QFile saveFile(LocalSimTreeFileName);
                                    if ( !saveFile.open(QIODevice::WriteOnly) )
                                        rec->Error = QString("Failed to save sim data to file: ") + LocalSimTreeFileName;
                                    else
                                    {
                                        saveFile.write(ba);
                                        saveFile.close();
                                        rec->FileName = LocalSimTreeFileName;
                                        emit requestTextLog(index, QString("Sim results saved in ") + LocalSimTreeFileName);
                                    }
                                }
                            }
                        }
                    }
                }


            }
        }
    }

    if (!rec->Error.isEmpty())
        emit requestTextLog(index, rec->Error);
}

AWebSocketWorker_Rec::AWebSocketWorker_Rec(int index, ARemoteServerRecord *rec, int timeOut, const QJsonObject *config) :
    AWebSocketWorker_Base(index, rec, timeOut, config) {}

void AWebSocketWorker_Rec::run()
{
    bRunning = true;

    bool bOK = establishSession();
    if (!bOK)
    {
        qDebug() << "failed to get server, aborting the thread and updating status";
        rec->Status = ARemoteServerRecord::Dead;
        rec->Progress = 0;
    }
    else
    {
        bPaused = true;
        //waiting while main thread will set events to reconstruct
        while (bPaused && !bExternalAbort)
        {
            QThread::usleep(100);
            QCoreApplication::processEvents();
        }
        if (!bExternalAbort)
        {
            runReconstruction();
            if (!rec->Error.isEmpty()) emit requestTextLog(index, rec->Error);
        }
    }

    bRunning = false;
    emit finished();
}

void AWebSocketWorker_Rec::runReconstruction()
{
    emit requestTextLog(index, QString("Reconstructing events from %1 to %2").arg(rec->EventsFrom).arg(rec->EventsTo));

    rec->Error.clear();

    if (!ants2socket || !ants2socket->ConfirmSendPossible())
    {
        rec->Error = "There is no connection to ANTS2 server";
        return;
    }

    if (!config || !config->contains("ReconstructionConfig") || !config->contains("DetectorConfig"))
    {
        rec->Error = "Config does not contain detector or reconstruction settings";
        return;
    }

    qDebug() << "Sending config...";
    emit requestTextLog(index, "Sending config file...");
    bool bOK = ants2socket->SendJson(*config);
    QString reply = ants2socket->GetTextReply();
    QJsonObject ro = strToObject(reply);
    if (!bOK || !ro.contains("result") || !ro["result"].toBool())
    {
        rec->Error = "Failed to send config";
        return;
    }
    emit requestTextLog(index, "Sending script to setup configuration...");
    QString Script = "var c = server.GetBufferAsObject();"
                     "var ok = config.SetConfig(c);"
                     "if (!ok) core.abort(\"Failed to set config\");";
    bOK = ants2socket->SendText(Script);
    reply = ants2socket->GetTextReply();
    ro = strToObject(reply);
    if (!bOK || !ro.contains("result") || !ro["result"].toBool())
    {
        rec->Error = "failed to setup config on the ants2 server";
        return;
    }

    //sending events
    emit requestTextLog(index, "Sending events to the server...");
    bOK = ants2socket->SendQByteArray(rec->ByteArrayToSend);
    reply = ants2socket->GetTextReply();
    ro = strToObject(reply);
    if (!bOK || !ro.contains("result") || !ro["result"].toBool())
    {
        rec->Error = "Failed to send events to remote server";
        return;
    }

    emit requestTextLog(index, "Setting event signals at the remote server...");
    Script = "server.GetBufferAsEvents()";
    bOK = ants2socket->SendText(Script);
    reply = ants2socket->GetTextReply();
    ro = strToObject(reply);
    if ( !bOK || !ro.contains("result") || !ro["result"].toBool() )
    {
        rec->Error = "Failed to set events on server.";
        return;
    }

    emit requestTextLog(index, "Starting reconstruction...");
    rec->Status = ARemoteServerRecord::Progressing;
    Script = "server.SetAcceptExternalProgressReport(true);"; //even if not showing to the user, still want to send reports to see that the server is alive
    Script += "rec.ReconstructEvents(" + QString::number(rec->NumThreads) + ", false);";
    Script += "server.SendReconstructionData()";
    bOK = ants2socket->SendText(Script);
    reply = ants2socket->GetTextReply();
    ro = strToObject(reply);
    if ( !bOK || ro.contains("error") )
    {
        const QString err = ro["error"].toString();
        rec->Error = "Server reported error:<br>" + err;
        return;
    }

    while ( !ro.contains("binary") ) //after sending the file, the reply is "{ \"binary\" : \"qbytearray\" }"
    {
        emit requestTextLog(index, reply);
        bOK = ants2socket->ResumeWaitForAnswer();
        reply = ants2socket->GetTextReply();
        ro = strToObject(reply);
        if (!bOK)
        {
            emit requestTextLog(index, "Reconstruction failed!");
            emit requestTextLog(index, ants2socket->GetError());
            return;
        }
        if (ro.contains("progress"))
            rec->Progress = ro["progress"].toInt();
    }

    rec->ByteArrayReceived = ants2socket->GetBinaryReply();
    emit requestTextLog(index, "Remote reconstruction finished");
}
