#include "aexternalprocesshandler.h"

#include <QProcess>
#include <QDebug>

AExternalProcessHandler::AExternalProcessHandler(const QString & program, const QStringList & args) :
    QObject(), Program(program), Args(args) {}

AExternalProcessHandler::~AExternalProcessHandler()
{
    abort();
}

bool AExternalProcessHandler::startAndWait()
{
    if (Process)
    {
        ErrorString = "Already started";
        return false;
    }

    Process = new QProcess();
    Process->setProcessChannelMode(QProcess::MergedChannels);

    //QObject::connect(G4antsProcess, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), [&isRunning](){isRunning = false; qDebug() << "----FINISHED!-----";});//this, &MainWindow::on_cameraControlExit);
    //connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [=](int exitCode, QProcess::ExitStatus exitStatus){ /* ... */ });
    //QObject::connect(G4antsProcess,&QProcess::readyReadStandardOutput, this, &AParticleSourceSimulator::onG4ProcessSendMessage);
    QObject::connect(Process, SIGNAL(readyReadStandardOutput()), this, SLOT(onReadReady()));

    Process->start(Program, Args);
    bool bStartedOK = Process->waitForStarted(1000);
    if (bStartedOK) Process->waitForFinished(-1);

    QProcess::ExitStatus exitStat = Process->exitStatus();
    //qDebug() << "g4---->"<<err<<(int)exitStat;

    if (output.contains("No such file or directory"))
    {
        ErrorString = "Cannot find G4ants executable: " + Program;
        return false;
    }
    if (exitStat == QProcess::CrashExit)
    {
        ErrorString = "G4ants executable crashed!";
        return false;
    }

    return true;
}

void AExternalProcessHandler::abort()
{
    if (Process)
    {
        Process->closeWriteChannel();
        Process->closeReadChannel(QProcess::StandardOutput);
        Process->closeReadChannel(QProcess::StandardError);
        Process->kill();
    }
    delete Process; Process = nullptr;
}

void AExternalProcessHandler::onReadReady()
{
    QString in = Process->readAllStandardOutput();
    if (bVerbose) qDebug() << in;

    if (in.startsWith("$$progress>") && in.endsWith("<$$\n"))
    {
        in.remove("$$progress>");
        in.remove("<$$\n");
        bool bOK;
        int pr = in.toInt(&bOK);
        if (bOK)
            *ExeProgress = pr;
    }

    output += QString(in);
}
