#ifndef ALOGCONFIGDIALOG_H
#define ALOGCONFIGDIALOG_H

#include <QObject>
#include <QDialog>

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

private slots:
    void on_pbAccept_clicked();

private:
    Ui::ALogConfigDialog *ui;
    ALogsAndStatisticsOptions & LogStatOpt;
};

#endif // ALOGCONFIGDIALOG_H
