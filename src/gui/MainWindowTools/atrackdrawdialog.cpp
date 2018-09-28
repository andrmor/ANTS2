#include "atrackdrawdialog.h"
#include "ui_atrackdrawdialog.h"
#include "atrackbuildoptions.h"
#include "arootlineconfigurator.h"

#include "TColor.h"
#include "TROOT.h"

ATrackDrawDialog::ATrackDrawDialog(QWidget *parent, ATrackBuildOptions *settings, const QStringList& ParticleNames) :
    QDialog(parent), settings(settings), ParticleNames(ParticleNames),
    ui(new Ui::ATrackDrawProperties)
{
    setWindowTitle("Track build options");
    ui->setupUi(this);
    ui->pbClose->setDefault(true);

    ui->cbBuildPhotonTracks->setChecked(settings->bBuildPhotonTracks);
    ui->cbBuildParticleTracks->setChecked(settings->bBuildParticleTracks);

    ui->sbMaxPhotonTracks->setValue(settings->MaxPhotonTracks);
    ui->sbMaxParticleTracks->setValue(settings->MaxParticleTracks);

    ui->cbSkipPhotonsMissingPMs->setChecked(settings->bSkipPhotonsMissingPMs);

    ui->cbSpecialRule_hitPM->setChecked(settings->bPhotonSpecialRule_HittingPMs);
    ui->cbSpecialRule_secScint->setChecked(settings->bPhotonSpecialRule_SecScint);

    ui->cbSkipPrimaries->setChecked(settings->bSkipPrimaries);
    ui->cbSkipPrimariesNoInteraction->setChecked(settings->bSkipPrimariesNoInteraction);
    ui->cbSkipSecondaries->setChecked(settings->bSkipSecondaries);

    updateParticleAttributes();
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

void ATrackDrawDialog::on_cbSkipPrimaries_clicked(bool checked)
{
    settings->bSkipPrimaries = checked;
}

void ATrackDrawDialog::on_cbSkipPrimariesNoInteraction_clicked(bool checked)
{
    settings->bSkipPrimariesNoInteraction = checked;
}

void ATrackDrawDialog::on_cbSkipSecondaries_clicked(bool checked)
{
    settings->bSkipSecondaries = checked;
}

void ATrackDrawDialog::on_pbDefaultParticleAtt_clicked()
{
    ARootLineConfigurator* rlc = new ARootLineConfigurator(&settings->TA_DefaultParticle.color,
                                                           &settings->TA_DefaultParticle.width,
                                                           &settings->TA_DefaultParticle.style, this);
    rlc->exec();
}

void ATrackDrawDialog::on_sbParticle_valueChanged(int /*arg1*/)
{
    updateParticleAttributes();
}

void ATrackDrawDialog::updateParticleAttributes()
{
    int iParticle = ui->sbParticle->value();

    ui->labParticleName->setText( iParticle < ParticleNames.size() ? ParticleNames.at(iParticle) : "");

    if ( iParticle  < settings->CustomParticle_Attributes.size() )
        if ( settings->CustomParticle_Attributes.at(iParticle) )
        {
            ui->swCustomParticle->setCurrentIndex(1);
            const ATrackAttributes* s = settings->CustomParticle_Attributes.at(iParticle);
            ui->frCustom->setVisible(true);
            TColor *tc = gROOT->GetColor(s->color);
            int red = 255;
            int green = 255;
            int blue = 255;
            if (tc)
            {
                red = 255*tc->GetRed();
                green = 255*tc->GetGreen();
                blue = 255*tc->GetBlue();
            }
            ui->frCustom->setStyleSheet(  QString("background-color:rgb(%1,%2,%3)").arg(red).arg(green).arg(blue)  );

            return;
        }

    //else undefined
    ui->frCustom->setVisible(false);
    ui->swCustomParticle->setCurrentIndex(0);
}

void ATrackDrawDialog::on_pbEditCustom_clicked()
{
    int iParticle = ui->sbParticle->value();

    ATrackAttributes* s;
    if ( iParticle  < settings->CustomParticle_Attributes.size() &&
         settings->CustomParticle_Attributes.at(iParticle) )
            s = settings->CustomParticle_Attributes[iParticle];
    else
    {
        s = new ATrackAttributes();
        *s = settings->TA_DefaultParticle;
        while (iParticle >= settings->CustomParticle_Attributes.size())
            settings->CustomParticle_Attributes << 0;
        settings->CustomParticle_Attributes[iParticle] = s;
    }

    ARootLineConfigurator* rlc = new ARootLineConfigurator(&s->color,
                                                           &s->width,
                                                           &s->style, this);
    rlc->exec();

    updateParticleAttributes();
}

void ATrackDrawDialog::on_pbCustomDelete_clicked()
{
    int iParticle = ui->sbParticle->value();
    if ( iParticle  < settings->CustomParticle_Attributes.size() )
        if ( settings->CustomParticle_Attributes.at(iParticle) )
        {
            delete settings->CustomParticle_Attributes.at(iParticle);
            settings->CustomParticle_Attributes[iParticle] = 0;
        }
    updateParticleAttributes();
}

#include <QFileDialog>
#include "ajsontools.h"
void ATrackDrawDialog::on_pbSave_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Save track drawing settings", "", "Json files (*.json);;All files (*.*)");
    if (fileName.isEmpty()) return;
    QJsonObject json;
    settings->writeToJson(json);
    SaveJsonToFile(json, fileName);
}

void ATrackDrawDialog::on_pbLoad_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Load track drawing settings", "", "Json files (*.json);;All files (*.*)");
    if (fileName.isEmpty()) return;
    QJsonObject json;
    LoadJsonFromFile(json, fileName);
    settings->readFromJson(json);
}

void ATrackDrawDialog::on_sbMaxPhotonTracks_editingFinished()
{
    settings->MaxPhotonTracks = ui->sbMaxPhotonTracks->value();
}

void ATrackDrawDialog::on_sbMaxParticleTracks_editingFinished()
{
    settings->MaxParticleTracks = ui->sbMaxParticleTracks->value();
}
