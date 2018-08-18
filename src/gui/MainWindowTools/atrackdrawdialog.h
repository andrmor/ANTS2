#ifndef ATRACKDRAWPROPERTIES_H
#define ATRACKDRAWPROPERTIES_H

#include <QDialog>

class ATrackBuildOptions;

namespace Ui {
class ATrackDrawProperties;
}

class ATrackDrawDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ATrackDrawDialog(QWidget *parent, ATrackBuildOptions* settings, const QStringList& ParticleNames);
    ~ATrackDrawDialog();

private slots:
    void on_pbPhotons_norm_clicked();

    void on_pbPhotons_PM_clicked();

    void on_pbPhotons_sec_clicked();

    void on_cbSkipPhotonsMissingPMs_clicked(bool checked);

    void on_pbClose_clicked();

    void on_cbSpecialRule_hitPM_clicked(bool checked);

    void on_cbSpecialRule_secScint_clicked(bool checked);

    void on_cbBuildPhotonTracks_clicked(bool checked);

    void on_cbBuildParticleTracks_clicked(bool checked);

    void on_cbSkipPrimaries_clicked(bool checked);

    void on_cbSkipSecondaries_clicked(bool checked);

    void on_pbDefaultParticleAtt_clicked();

    void on_sbParticle_valueChanged(int arg1);

    void on_pbEditCustom_clicked();

    void on_pbCustomDelete_clicked();

    void on_pbSave_clicked();

    void on_pbLoad_clicked();

private:
    ATrackBuildOptions* settings;
    const QStringList& ParticleNames;
    Ui::ATrackDrawProperties *ui;

    void updateParticleAttributes();
};

#endif // ATRACKDRAWPROPERTIES_H
