#ifndef AEXTERNALPROCESSHANDLER_H
#define AEXTERNALPROCESSHANDLER_H

#include <QObject>
#include <QString>
#include <QStringList>

class QProcess;

class AExternalProcessHandler : public QObject
{
    Q_OBJECT

public:
    AExternalProcessHandler(const QString & program, const QStringList & args);
    ~AExternalProcessHandler();

    bool startAndWait();
    void abort();

    void setVerbose() {bVerbose = true;}
    void setProgressVariable(int * progressPercent) {ExeProgress = progressPercent;}
    const QString & getOutput() {return output;}

public:
    QString ErrorString;

private slots:
    void onReadReady();

private:
    QString Program;
    QStringList Args;
    QProcess * Process = nullptr;
    QString output;
    bool bVerbose = false;
    int * ExeProgress;
};

#endif // AEXTERNALPROCESSHANDLER_H
