#include "alogconfigdialog.h"
#include "ui_alogconfigdialog.h"
#include "alogsandstatisticsoptions.h"

#include <QCheckBox>

ALogConfigDialog::ALogConfigDialog(ALogsAndStatisticsOptions & LogStatOpt, QWidget *parent) :
    LogStatOpt(LogStatOpt), QDialog(parent),
    ui(new Ui::ALogConfigDialog)
{
    ui->setupUi(this);

    ui->cbParticleTransport->setChecked(LogStatOpt.bParticleTransportLog);
    ui->cbDetectionStatistics->setChecked(LogStatOpt.bPhotonDetectionStat);
    ui->cbPhotonGeneration->setChecked(LogStatOpt.bPhotonGenerationLog);
}

ALogConfigDialog::~ALogConfigDialog()
{
    delete ui;
}

void ALogConfigDialog::on_pbAccept_clicked()
{
    LogStatOpt.bParticleTransportLog = ui->cbParticleTransport->isChecked();
    LogStatOpt.bPhotonDetectionStat  = ui->cbDetectionStatistics->isChecked();
    LogStatOpt.bPhotonGenerationLog  = ui->cbPhotonGeneration->isChecked();

    accept();
}
