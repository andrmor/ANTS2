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

AGridRunner::AGridRunner(QVector<ARemoteServerRecord *> & ServerRecords, EventsDataClass & EventsDataHub, const APmHub & PMs) :
    ServerRecords(ServerRecords), EventsDataHub(EventsDataHub), PMs(PMs) {}

const QString AGridRunner::CheckStatus()
{
    if (ServerRecords.isEmpty())
        return "Configure at least one dispatcher record!";

    onStart();

    bool bEmpty = true;
    for (ARemoteServerRecord* r : ServerRecords)
        if (r->bEnabled)
        {
            bEmpty = false;
            break;
        }
    if (bEmpty)
        return "All dispatcher records are disabled!";

    emit requestStatusLog("Checking status of servers...");
    QVector<AWebSocketWorker_Base*> workers;

    for (int i = 0; i < ServerRecords.size(); i++)
        workers << startCheckStatusOfServer(i, ServerRecords[i]);

    waitForWorkersToFinish(workers);

    for (AWebSocketWorker_Base* w : workers) delete w;
    workers.clear();

    int ser = 0;
    int th = 0;
    for (const ARemoteServerRecord* r : ServerRecords)
        if (r->NumThreads_Allocated > 0)
        {
            ser++;
            th += r->NumThreads_Allocated;
        }
    QString s = (th > 0 ? QString("Found %1 servers with %2 available threads").arg(ser).arg(th) : "There are no servers with available threads" );
    emit requestStatusLog(s);

    return "";
}

const QString AGridRunner::Simulate(const QJsonObject* config)
{
    onStart();

    if (!config->contains("SimulationConfig") || !config->contains("DetectorConfig"))
        return "Configuration does not contain simulation or detector settings";

    QJsonObject jSimulationConfig = (*config)["SimulationConfig"].toObject();

    bool pPointSourceSim = (jSimulationConfig["Mode"].toString() == "PointSim");
    int  numEvents = 0;
    int  PointSourceSimType = 0;
    QJsonArray nodesAr;

    if (pPointSourceSim)
    {
        QJsonObject jPointSourcesConfig = jSimulationConfig["PointSourcesConfig"].toObject();
            QJsonObject jControlOptions = jPointSourcesConfig["ControlOptions"].toObject();

        PointSourceSimType = jControlOptions["Single_Scan_Flood"].toInt();

        switch (PointSourceSimType)
        {
        case 0:
            if ( !jControlOptions["MultipleRuns"].toBool() )
                return "Point source / single position simulation:\nDistributed simulation is only possible when multirun is activated";
            numEvents = jControlOptions["MultipleRunsNumber"].toInt();
            break;
        case 1:
        {
            QJsonObject jRegularScanOptions = jPointSourcesConfig["RegularScanOptions"].toObject();
            regularToCustomNodes(jRegularScanOptions, nodesAr);
            numEvents = nodesAr.size();
            break;
        }
        case 2:
        {
            QJsonObject jFloodOptions =  jPointSourcesConfig["FloodOptions"].toObject();
            numEvents = jFloodOptions["Nodes"].toInt();
            break;
        }
        case 3:
        {
            QJsonObject jCustomNodesOptions = jPointSourcesConfig["CustomNodesOptions"].toObject();
            nodesAr = jCustomNodesOptions["Nodes"].toArray();
            numEvents = nodesAr.size();
            break;
        }
        default:;
        }
    }
    else
    {
        QJsonObject jParticleSourcesConfig = jSimulationConfig["ParticleSourcesConfig"].toObject();
            QJsonObject jSourceControlOptions = jParticleSourcesConfig["SourceControlOptions"].toObject();

        numEvents = jSourceControlOptions["EventsToDo"].toInt();
    }

    emit requestStatusLog("Starting remote simulation...");
    QVector<AWebSocketWorker_Base*> workers;

    for (int i = 0; i < ServerRecords.size(); i++)
        workers << startSim(i, ServerRecords[i], config);

    waitForWorkersToPauseOrFinish(workers);

    //init distributor of load
    double sumSpeedfactor = 0;
    int numThr = 0;
    for (int i = 0; i < workers.size(); i++)
        if (ServerRecords.at(i)->NumThreads_Allocated > 0)
        {
            numThr++;
            sumSpeedfactor += ServerRecords.at(i)->SpeedFactor;
        }
    if (numThr == 0)
    {
        emit requestStatusLog("Cannot simulate: there are no active servers");
        return "Cannot simulate: there are no active servers";
    }

    //distributing load over servers and resuming workers
    int counter = 0;
    for (int i = 0; i < workers.size(); i++)
    {
        if (ServerRecords.at(i)->NumThreads_Allocated > 0)
        {
            QString modScript;

            double perUnitSpeed = 1.0 * numEvents / sumSpeedfactor;
            int numEventsToDo = std::ceil(perUnitSpeed * ServerRecords.at(i)->SpeedFactor);
            if (counter + numEventsToDo > numEvents) numEventsToDo = numEvents - counter;

            if (pPointSourceSim)
            {
                qDebug() << "-------- Photon sources ----------";
                switch (PointSourceSimType)
                {
                case 0:
                    qDebug() << "--- single ---";
                    qDebug() << "Server #"<<i << "will simulate"<<numEventsToDo<<"runs at single position";
                    modScript = QString("config.Replace(\"SimulationConfig.PointSourcesConfig.ControlOptions.MultipleRunsNumber\", %1)").arg(numEventsToDo);
                    break;
                case 2:
                    qDebug() << "--- flood ---";
                    qDebug() << "Server #"<<i << "will simulate"<<numEventsToDo<<"flood events";
                    modScript = QString("config.Replace(\"SimulationConfig.PointSourcesConfig.FloodOptions.Nodes\", %1)").arg(numEventsToDo);
                    break;
                case 1:
                case 3:
                    {
                        qDebug() << "--- custom (and regular->custom) nodes ---";
                        qDebug() << "Server #"<<i << "will simulate"<<numEventsToDo<<"custom nodes";
                        QJsonArray ar;
                        for (int i = counter; i < counter + numEventsToDo; i++)
                            ar.append( nodesAr.at(i) );
                        QJsonObject json;
                        json["Nodes"] = ar;
                        modScript  = "var obj = ";
                        modScript += jsonToString(json);
                        modScript += "; config.Replace(\"SimulationConfig.PointSourcesConfig.CustomNodesOptions\", obj)";
                    }
                    break;
                default: qWarning() << "Not implemented point source sim type "<< PointSourceSimType;
                }

                if (PointSourceSimType == 1)
                {
                    qDebug() << "Fix for regular nodes: change to custom mode";
                    modScript += "; config.Replace(\"SimulationConfig.PointSourcesConfig.ControlOptions.Single_Scan_Flood\", 3)";
                }
            }
            else
            {
                qDebug() << "-------- Particle sources ----------";
                qDebug() << "Server #"<<i << "will simulate"<<numEventsToDo<<"particle source events";
                modScript = QString("config.Replace(\"SimulationConfig.ParticleSourcesConfig.SourceControlOptions.EventsToDo\", %1)").arg(numEventsToDo);
            }

            workers[i]->setExtraScript(modScript);
            counter += numEventsToDo;
        }

        workers[i]->setPaused(false); //resume worker
    }

    waitForWorkersToFinish(workers);

    for (AWebSocketWorker_Base* w : workers) delete w;
    workers.clear();

    //loading simulated events
    bool bError = false;
    EventsDataHub.clear();
    for (int i=0; i<ServerRecords.size(); i++)
    {
        if ( !ServerRecords.at(i)->FileName.isEmpty() )
        {
            int numEv = EventsDataHub.loadSimulatedEventsFromTree( ServerRecords.at(i)->FileName, PMs );
            QString s;
            if (EventsDataHub.ErrorString.isEmpty())
                s = QString("%1 events were registered").arg(numEv);
            else
            {
                s = QString("Error loading events from the TTree file sent by the remote host:\n") + EventsDataHub.ErrorString;
                ServerRecords[i]->Error = s;
                bError = true;
            }
            requestTextLog(i, s);
        }
    else
        {
            if ( !ServerRecords.at(i)->Error.isEmpty() )
                bError = true;
        }
    }

    if (bError)
    {
        emit requestStatusLog("Finished. There were errors!");
    }
    else
    {
        emit requestStatusLog("Done!");
    }
    requestDelegateGuiUpdate();

    return "";
}

const QString AGridRunner::Reconstruct(const QJsonObject *config)
{
    if (EventsDataHub.isEmpty())
        return "There are no events to reconstruct!";

    onStart();

    emit requestStatusLog("Starting remote reconstruction...");
    QVector<AWebSocketWorker_Base*> workers;

    for (int i = 0; i < ServerRecords.size(); i++)
        workers << startRec(i, ServerRecords[i], config);

    waitForWorkersToPauseOrFinish(workers);

    //init distributor of load
    double sumSpeedfactor = 0;
    int numThr = 0;
    for (int i = 0; i < workers.size(); i++)
        if (ServerRecords.at(i)->NumThreads_Allocated > 0)
        {
            numThr++;
            sumSpeedfactor += ServerRecords.at(i)->SpeedFactor;
        }
    if (numThr == 0)
    {
        emit requestStatusLog("Cannot reconstruct: there are no active servers");
        return "Cannot reconstruct: there are no active servers";
    }
    int numEvents = EventsDataHub.countEvents();
    double perUnitSpeed = 1.0 * numEvents / sumSpeedfactor;

    //distributing load and resuming workers
    int from = 0;
    for (int i = 0; i < workers.size(); i++)
    {
        ServerRecords[i]->ByteArrayReceived.clear();
        ServerRecords[i]->EventsFrom = 0;
        ServerRecords[i]->EventsTo = 0;

        if (ServerRecords.at(i)->NumThreads_Allocated > 0)
        {
            ServerRecords[i]->EventsFrom = from;
            int toDo = std::ceil(perUnitSpeed * ServerRecords.at(i)->SpeedFactor);
            if (from + toDo > numEvents) toDo = numEvents - from;

            ServerRecords[i]->EventsTo = from + toDo;
            from += toDo;

            EventsDataHub.packEventsToByteArray(ServerRecords[i]->EventsFrom, ServerRecords[i]->EventsTo, ServerRecords[i]->ByteArrayToSend);
            qDebug() << "Server #"<<i << "will reconstruct events from "<<ServerRecords[i]->EventsFrom <<"to"<<ServerRecords[i]->EventsTo;
        }

        workers[i]->setPaused(false); //resume worker
    }

    waitForWorkersToFinish(workers);

    for (AWebSocketWorker_Base* w : workers) delete w;
    workers.clear();

    bool bRecFailed = false;
    for (int i = 0; i < ServerRecords.size(); i++)
    {
        ServerRecords[i]->ByteArrayToSend.clear();
        ServerRecords[i]->ByteArrayToSend.squeeze();

        if (ServerRecords.at(i)->EventsTo > 0)
        {
            if (!ServerRecords.at(i)->Error.isEmpty()) bRecFailed = true;
            else
            {
                bool bOK = EventsDataHub.unpackReconstructedFromByteArray(ServerRecords.at(i)->EventsFrom, ServerRecords.at(i)->EventsTo, ServerRecords.at(i)->ByteArrayReceived);
                if (!bOK)
                {
                    ServerRecords[i]->Error = "Failed to set reconstruction data using the obtained data";
                    emit requestTextLog(i, ServerRecords[i]->Error);
                    bRecFailed = true;
                }
            }
        }
    }

    if (bRecFailed) emit requestStatusLog("Reconstruction failed!");
    else
    {
        EventsDataHub.fReconstructionDataReady = true;
        emit EventsDataHub.requestFilterEvents();
        emit requestStatusLog("Done!");
    }

    return "";
}

const QString AGridRunner::RateServers(const QJsonObject *config)
{
    if (ServerRecords.isEmpty())
        return "Configure at least one dispatcher record!";

    onStart();

    bool bEmpty = true;
    for (ARemoteServerRecord* r : ServerRecords)
        if (r->bEnabled)
        {
            bEmpty = false;
            break;
        }
    if (bEmpty)
        return "All dispatcher records are disabled!";

    emit requestStatusLog("Rating servers...");
    QVector<AWebSocketWorker_Base*> workers;

    for (int i = 0; i < ServerRecords.size(); i++)
        workers << startSim(i, ServerRecords[i], config);

    waitForWorkersToPauseOrFinish(workers);

    for (int i = 0; i < workers.size(); i++)
        workers[i]->setPaused(false);

    waitForWorkersToFinish(workers);

    for (AWebSocketWorker_Base* w : workers) delete w;
    workers.clear();

    for (int i=0; i<ServerRecords.size(); i++)
        if ( ServerRecords.at(i)->TimeElapsed > 0 )
        {
            ServerRecords[i]->SpeedFactor = 7000.0 / ServerRecords.at(i)->TimeElapsed; //factor of 1 corresponds to 7s sim
            requestTextLog(i, QString("Elapsed time %1 ms").arg(ServerRecords.at(i)->TimeElapsed));
            requestTextLog(i, QString("Attributed performance factor of %1").arg(ServerRecords.at(i)->SpeedFactor));
        }

    emit requestStatusLog("Done!");

    return "";
}

void AGridRunner::Abort()
{
    bAbortRequested = true;
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

        if (bAbortRequested) doAbort(workers);
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
        QThread::usleep(250);

        if (bAbortRequested) doAbort(workers);
    }
    while (bStillWorking);

    qDebug() << "All threads paused or finished!";
}

void AGridRunner::regularToCustomNodes(const QJsonObject &RegularScanOptions, QJsonArray &toArray)
{
    // "SimulationConfig.PointSourcesConfig.RegularScanOptions"

    double RegGridOrigin[3]; //grid origin
    RegGridOrigin[0] = RegularScanOptions["ScanX0"].toDouble();
    RegGridOrigin[1] = RegularScanOptions["ScanY0"].toDouble();
    RegGridOrigin[2] = RegularScanOptions["ScanZ0"].toDouble();
    //
    double RegGridStep[3][3]; //vector [axis] [step]
    int RegGridNodes[3]; //number of nodes along the 3 axes
    bool RegGridFlagPositive[3]; //Axes option
    //
    QJsonArray rsdataArr = RegularScanOptions["AxesData"].toArray();
    for (int ic=0; ic<3; ic++)
    {
        if(ic < rsdataArr.count())
        {
            QJsonObject adjson = rsdataArr[ic].toObject();
            RegGridStep[ic][0] = adjson["dX"].toDouble();
            RegGridStep[ic][1] = adjson["dY"].toDouble();
            RegGridStep[ic][2] = adjson["dZ"].toDouble();
            RegGridNodes[ic] = adjson["Nodes"].toInt(); //1 is disabled axis
            RegGridFlagPositive[ic] = adjson["Option"].toInt();
        }
        else
        {
            RegGridStep[ic][0] = 0;
            RegGridStep[ic][1] = 0;
            RegGridStep[ic][2] = 0;
            RegGridNodes[ic] = 1; //1 is disabled axis
            RegGridFlagPositive[ic] = true;
        }
    }

    int iAxis[3];
    for (iAxis[0]=0; iAxis[0]<RegGridNodes[0]; iAxis[0]++)
        for (iAxis[1]=0; iAxis[1]<RegGridNodes[1]; iAxis[1]++)
            for (iAxis[2]=0; iAxis[2]<RegGridNodes[2]; iAxis[2]++)  //iAxis - counters along the axes!!!
            {
                double r[3];
                for (int i=0; i<3; i++)
                    r[i] = RegGridOrigin[i];
                //shift from the origin
                for (int axis=0; axis<3; axis++)
                { //going axis by axis
                    double ioffset = 0;
                    if (!RegGridFlagPositive[axis])
                        ioffset = -0.5*( RegGridNodes[axis] - 1 );
                    for (int i=0; i<3; i++)
                        r[i] += (ioffset + iAxis[axis]) * RegGridStep[axis][i];
                }

                QJsonArray ar;
                for (int i=0; i<3; i++) ar << r[i];
                toArray.append(ar);
            }
}

void AGridRunner::doAbort(QVector<AWebSocketWorker_Base *> &workers)
{
    for (AWebSocketWorker_Base* w : workers)
    {
        w->RequestAbort();
        emit w->finished();
    }
}

void AGridRunner::onStart()
{
    bAbortRequested = false;
    for (ARemoteServerRecord* r : ServerRecords)
    {
        r->Progress = 0;
        r->Error.clear();
    }
    emit requestDelegateGuiUpdate();
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

void AWebSocketWorker_Base::RequestAbort()
{
    if (ants2socket) ants2socket->ExternalAbort();
    bExternalAbort = true;
}

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
    rec->AntsServerTicket.clear();
    rec->NumThreads_Allocated = 0;
    bool bOK = false;

    emit requestTextLog(index, "Connecting to dispatcher");
    AWebSocketSession* socket = connectToServer(rec->Port);
    if (socket)
    {
        emit requestTextLog(index, "Requesting ANTS2 server...");
        QJsonObject cn;
        cn["command"] = "new";
        cn["threads"] = rec->NumThreads_Possible;
        QString strCn = jsonToString(cn);

        bOK = socket->SendText( strCn );
        if (!bOK)
        {
            emit requestTextLog(index, "Failed to send request");
            rec->Status = ARemoteServerRecord::Dead;
        }
        else
        {
            QString reply = socket->GetTextReply();
            emit requestTextLog(index, QString("Dispatcher reply: ") + reply );
            QJsonObject ro = strToObject(reply);
            if ( !ro.contains("result") || !ro["result"].toBool())
            {
                bOK = false;
                emit requestTextLog(index, "Dispatcher refused to provide ANTS2 server!");
                rec->Status = ARemoteServerRecord::Busy;
            }
            else
            {
                rec->Status = ARemoteServerRecord::Alive;
                rec->AntsServerPort = ro["port"].toInt();
                rec->AntsServerTicket = ro["ticket"].toString();
                rec->NumThreads_Allocated = rec->NumThreads_Possible;
                emit requestTextLog(index, QString("Dispatcher allocated port ") + QString::number(rec->AntsServerPort) + "  and ticket " + rec->AntsServerTicket);
            }
        }

        socket->Disconnect();
        delete socket;
    }
    else
        rec->Status = ARemoteServerRecord::Dead;

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
            rec->Status = ARemoteServerRecord::Dead;
            rec->NumThreads_Allocated = 0;
        }
    }

    return socket;
}

bool AWebSocketWorker_Base::establishSession()
{
    ants2socket = 0;

    if (rec->bEnabled)
    {
        bool bOK = allocateAntsServer();
        if (bOK)
            ants2socket = connectToAntsServer();
    }
    else rec->NumThreads_Allocated = 0;

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
    if (rec->bEnabled)
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
        rec->NumThreads_Allocated = 0;
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
                parseJson(json, "threads", rec->NumThreads_Allocated);
                qDebug() << "Available threads:"<<rec->NumThreads_Allocated;
                if (rec->NumThreads_Allocated > 0)
                {
                    rec->Status = ARemoteServerRecord::Alive;
                    rec->NumThreads_Possible = rec->NumThreads_Allocated;
                }
                else rec->Status = ARemoteServerRecord::Busy;
                emit requestTextLog(index, QString("Available threads: ") + QString::number(rec->NumThreads_Possible));
            }
        }

        timer.stop();
        rec->Progress = 0;
        if (socket) delete socket;
    }
    else rec->NumThreads_Allocated = 0;

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
            runSimulation();
            if (!rec->Error.isEmpty())
            {
                rec->Progress = 0;
                emit requestTextLog(index, rec->Error);
            }
            else rec->Progress = 100;
            ants2socket->Disconnect();
        }
    }

    bRunning = false;
    emit finished();
}

#include <QElapsedTimer>
void AWebSocketWorker_Sim::runSimulation()
{
    rec->Error.clear();

    if (!ants2socket || !ants2socket->ConfirmSendPossible())
    {
        rec->Error = "There is no connection to ANTS2 server";
        return;
    }

    if (!config || !config->contains("SimulationConfig") || !config->contains("DetectorConfig"))
    {
        rec->Error = "Config does not contain detector or simulation settings";
        return;
    }

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

    if (!extraScript.isEmpty())
    {
        emit requestTextLog(index, "Sending script to modify configuration...");
        bOK = ants2socket->SendText(extraScript);
        reply = ants2socket->GetTextReply();
        ro = strToObject(reply);
        if (!bOK || !ro.contains("result") || !ro["result"].toBool())
        {
            rec->Error = "Failed to modify config on the ants2 server";
            return;
        }
    }

    QJsonObject jsSimSet = (*config)["SimulationConfig"].toObject();
    QString modeSetup = jsSimSet["Mode"].toString();
    bool bPhotonSource = (modeSetup == "PointSim"); //Photon simulator
    const QString RemoteSimTreeFileName = QString("SimTree-") + QString::number(index) + ".root";

    Script.clear();
    Script += "server.SetAcceptExternalProgressReport(true);";
    if (bPhotonSource)
        Script += "sim.RunPhotonSources(" + QString::number(rec->NumThreads_Allocated) + ");";
    else
        Script += "sim.RunParticleSources(" + QString::number(rec->NumThreads_Allocated) + ");";
    Script += "var fileName = \"" + RemoteSimTreeFileName + "\";";
    Script += "var ok = sim.SaveAsTree(fileName);";
    Script += "if (!ok) core.abort(\"Failed to save simulation data\");";
    Script += "server.SetAcceptExternalProgressReport(false);";
    Script += "server.SendFile(fileName);";
    //  qDebug() << Script;

    rec->TimeElapsed = 0;
    QElapsedTimer timer;
    timer.start();

    //execute the script on remote server
    emit requestTextLog(index, "Sending simulation script...");
    bOK = ants2socket->SendText(Script);
    if (!bOK)
    {
        rec->Error = "Failed to send script to ANTS2 server";
        return;
    }

    reply = ants2socket->GetTextReply();
    if (reply.isEmpty())
    {
        rec->Error = "Got no reply after sending a script to ANTS2 server";
        return;
    }
    emit requestTextLog(index, reply);
    QJsonObject obj = strToObject(reply);
    if (obj.contains("error"))
    {
        rec->Error = obj["error"].toString();
        return;
    }

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

    if (rec->Error.isEmpty())
    {
        rec->TimeElapsed = timer.elapsed();
        rec->Progress = 100;
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
    else rec->Progress = 0;
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
            if (!rec->Error.isEmpty())
            {
                rec->Progress = 0;
                emit requestTextLog(index, rec->Error);
            }
            else rec->Progress = 100;
            ants2socket->Disconnect();
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
    Script = "server.SetAcceptExternalProgressReport(true);"; //even if not showing to the user, still want to send reports to see that the server is alive
    Script += "rec.ReconstructEvents(" + QString::number(rec->NumThreads_Allocated) + ", false);";
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
