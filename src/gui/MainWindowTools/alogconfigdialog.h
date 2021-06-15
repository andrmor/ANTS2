#ifndef ALOGCONFIGDIALOG_H
#define ALOGCONFIGDIALOG_H

#include <QObject>
#include <QDialog>
#include <QString>

namespace Ui {
class ALogConfigDialog;
}

class ALogsAndStatisticsOptions;

class ALogConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ALogConfigDialog(ALogsAndStatisticsOptions & LogStatOpt, QWidget *parent = 0);
    ~ALogConfigDialog();

    bool    bRequestShowSettings = false;
    QString sRequestOpenFile;

private slots:
    void on_pbAccept_clicked();
    void on_pbDirSettings_clicked();
    void on_pbLoadLog_clicked();

private:
    Ui::ALogConfigDialog *ui;
    ALogsAndStatisticsOptions & LogStatOpt;

};

#endif // ALOGCONFIGDIALOG_H
