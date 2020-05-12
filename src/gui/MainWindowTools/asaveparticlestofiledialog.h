#ifndef ASAVEPARTICLESTOFILEDIALOG_H
#define ASAVEPARTICLESTOFILEDIALOG_H

#include <QDialog>

namespace Ui {
class ASaveParticlesToFileDialog;
}

class ASaveParticlesToFileSettings;

class ASaveParticlesToFileDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ASaveParticlesToFileDialog(ASaveParticlesToFileSettings & settings, QWidget *parent = 0);
    ~ASaveParticlesToFileDialog();

private slots:
    void on_pbChooseDir_clicked();
    void on_pbAccept_clicked();
    void on_pbCancel_clicked();

private:
    ASaveParticlesToFileSettings   & settings;
    Ui::ASaveParticlesToFileDialog * ui;
};

#endif // ASAVEPARTICLESTOFILEDIALOG_H
