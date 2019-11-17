#include "alogconfigdialog.h"
#include "ui_alogconfigdialog.h"
#include "alogsandstatisticsoptions.h"

#include <QCheckBox>

ALogConfigDialog::ALogConfigDialog(ALogsAndStatisticsOptions & LogStatOpt, QWidget *parent) :
    QDialog(parent), ui(new Ui::ALogConfigDialog),
    LogStatOpt(LogStatOpt)
{
    ui->setupUi(this);

    ui->cbParticleTransport->setChecked(LogStatOpt.bParticleTransportLog);
    ui->cbSaveParticleLog->setChecked(LogStatOpt.bSaveParticleLog);
    ui->cbSaveDepositionLog->setChecked(LogStatOpt.bSaveDepositionLog);

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
    LogStatOpt.bSaveParticleLog = ui->cbSaveParticleLog->isChecked();
    LogStatOpt.bSaveDepositionLog = ui->cbSaveDepositionLog->isChecked();

    LogStatOpt.bPhotonDetectionStat  = ui->cbDetectionStatistics->isChecked();
    LogStatOpt.bPhotonGenerationLog  = ui->cbPhotonGeneration->isChecked();

    accept();
}

void ALogConfigDialog::on_pbDirSettings_clicked()
{
    bRequestShowSettings = true;
    accept();
}
