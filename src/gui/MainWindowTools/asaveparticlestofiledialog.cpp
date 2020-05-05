#include "asaveparticlestofiledialog.h"
#include "ui_asaveparticlestofiledialog.h"
#include "asaveparticlestofilesettings.h"
#include "aglobalsettings.h"
#include "generalsimsettings.h"

#include <QFileDialog>
#include <QDoubleValidator>

ASaveParticlesToFileDialog::ASaveParticlesToFileDialog(ASaveParticlesToFileSettings & settings, QWidget *parent) :
    QDialog(parent),
    settings(settings),
    ui(new Ui::ASaveParticlesToFileDialog)
{
    ui->setupUi(this);
    ui->pbAccept->setDefault(true);

    QDoubleValidator* dv = new QDoubleValidator(this);
    dv->setNotation(QDoubleValidator::ScientificNotation);
    QList<QLineEdit*> list = this->findChildren<QLineEdit *>();
    foreach(QLineEdit *w, list) if (w->objectName().startsWith("led")) w->setValidator(dv);

    ui->cbSaveParticles->setChecked(settings.SaveParticles);
    ui->leExitVolumeName->setText(settings.VolumeName);
    ui->leParticlesFileName->setText(settings.FileName);
    ui->cbBinary->setChecked(settings.UseBinary);
    ui->cbUseTimeWindow->setChecked(settings.UseTimeWindow);
    ui->ledTimeFrom->setText( QString::number(settings.TimeFrom) );
    ui->ledTimeTo->setText( QString::number(settings.TimeTo) );
    ui->cbStopTrackExiting->setChecked(settings.StopTrack);
}

ASaveParticlesToFileDialog::~ASaveParticlesToFileDialog()
{
    delete ui;
}

void ASaveParticlesToFileDialog::on_pbChooseDir_clicked()
{
    QString s = QFileDialog::getOpenFileName(this, "Choose the file directory", AGlobalSettings::getInstance().LastOpenDir);
    if ( !s.isEmpty() ) ui->leParticlesFileName->setText(s);
}

void ASaveParticlesToFileDialog::on_pbAccept_clicked()
{
    settings.SaveParticles  = ui->cbSaveParticles->isChecked();
    settings.VolumeName     = ui->leExitVolumeName->text();
    settings.FileName       = ui->leParticlesFileName->text();
    settings.UseBinary      = ui->cbBinary->isChecked();
    settings.UseTimeWindow  = ui->cbUseTimeWindow->isChecked();
    settings.TimeFrom       = ui->ledTimeFrom->text().toDouble();
    settings.TimeTo         = ui->ledTimeTo->text().toDouble();
    settings.StopTrack      = ui->cbStopTrackExiting->isChecked();

    accept();
}

void ASaveParticlesToFileDialog::on_pbCancel_clicked()
{
    reject();
}
