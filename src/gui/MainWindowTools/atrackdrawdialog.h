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
    explicit ATrackDrawDialog(QWidget *parent, ATrackBuildOptions* settings);
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

private:
    ATrackBuildOptions* settings;
    Ui::ATrackDrawProperties *ui;

};

#endif // ATRACKDRAWPROPERTIES_H
