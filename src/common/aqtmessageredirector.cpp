#include "aqtmessageredirector.h"

#include <QtWidgets/QApplication>
#include <QtDebug>
#include <QFile>
#include <QTextStream>
#include <QDateTime>

static QString filename;

void myMessageHandler(QtMsgType type, const QMessageLogContext &, const QString & str)
{
    QString txt;
    switch (type)
    {
    case QtDebugMsg:
        //txt = QString("Debug: %1").arg(str);
        txt = str;
        break;
    case QtWarningMsg:
        txt = QString("Warning: %1").arg(str);
        break;
    case QtCriticalMsg:
        txt = QString("Critical: %1").arg(str);
        break;
    case QtFatalMsg:
        txt = QString("Fatal: %1").arg(str);
        abort();
    }

    txt.replace("\\\\", "\\");

    QFile outFile(filename);
    outFile.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream ts(&outFile);
    ts << txt << endl;
}

bool AQtMessageRedirector::activate(QString fileName)
{
    filename = fileName;

    QFile outFile(filename);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Append))
    {
        qDebug() << "Failed to open log file:"<<filename;
        return false;
    }
    QTextStream ts(&outFile);
    ts << "\n"<< "Run started on: " <<QDateTime::currentDateTime().toString("d/M/yyyy H:m:s") << "\n";
    outFile.close();

    qInstallMessageHandler(myMessageHandler);
    return true;
}
