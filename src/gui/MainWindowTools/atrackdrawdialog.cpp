#include "atrackdrawdialog.h"
#include "ui_atrackdrawdialog.h"
#include "atrackbuildoptions.h"
#include "arootlineconfigurator.h"

ATrackDrawDialog::ATrackDrawDialog(QWidget *parent, ATrackBuildOptions *settings) :
    QDialog(parent), settings(settings),
    ui(new Ui::ATrackDrawProperties)
{
    setWindowTitle("Track build options");
    ui->setupUi(this);
    ui->pbClose->setDefault(true);

    ui->cbBuildPhotonTracks->setChecked(settings->bBuildPhotonTracks);
    ui->cbBuildParticleTracks->setChecked(settings->bBuildParticleTracks);

    ui->cbSkipPhotonsMissingPMs->setChecked(settings->bSkipPhotonsMissingPMs);

    ui->cbSpecialRule_hitPM->setChecked(settings->bPhotonSpecialRule_HittingPMs);
    ui->cbSpecialRule_secScint->setChecked(settings->bPhotonSpecialRule_SecScint);
}

ATrackDrawDialog::~ATrackDrawDialog()
{
    delete ui;
}

void ATrackDrawDialog::on_pbPhotons_norm_clicked()
{
    ARootLineConfigurator* rlc = new ARootLineConfigurator(&settings->TA_Photons.color,
                                                           &settings->TA_Photons.width,
                                                           &settings->TA_Photons.style, this);
    rlc->exec();
}

void ATrackDrawDialog::on_pbPhotons_PM_clicked()
{
    ARootLineConfigurator* rlc = new ARootLineConfigurator(&settings->TA_PhotonsHittingPMs.color,
                                                           &settings->TA_PhotonsHittingPMs.width,
                                                           &settings->TA_PhotonsHittingPMs.style, this);
    rlc->exec();
}

void ATrackDrawDialog::on_pbPhotons_sec_clicked()
{
    ARootLineConfigurator* rlc = new ARootLineConfigurator(&settings->TA_PhotonsSecScint.color,
                                                           &settings->TA_PhotonsSecScint.width,
                                                           &settings->TA_PhotonsSecScint.style, this);
    rlc->exec();
}

void ATrackDrawDialog::on_cbSkipPhotonsMissingPMs_clicked(bool checked)
{
    settings->bSkipPhotonsMissingPMs = checked;
}

void ATrackDrawDialog::on_cbSpecialRule_hitPM_clicked(bool checked)
{
    settings->bPhotonSpecialRule_HittingPMs = checked;
}

void ATrackDrawDialog::on_cbSpecialRule_secScint_clicked(bool checked)
{
    settings->bPhotonSpecialRule_SecScint = checked;
}

void ATrackDrawDialog::on_pbClose_clicked()
{
    accept();
}

void ATrackDrawDialog::on_cbBuildPhotonTracks_clicked(bool checked)
{
    settings->bBuildPhotonTracks = checked;
}

void ATrackDrawDialog::on_cbBuildParticleTracks_clicked(bool checked)
{
    settings->bBuildParticleTracks = checked;
}
