#ifndef APARTICLESOURCEDIALOG_H
#define APARTICLESOURCEDIALOG_H

#include "particlesourcesclass.h"

#include <QDialog>

namespace Ui {
class AParticleSourceDialog;
}

class MainWindow;

class AParticleSourceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AParticleSourceDialog(MainWindow& MW, const AParticleSourceRecord& Rec);
    ~AParticleSourceDialog();

    const AParticleSourceRecord& getResultToClone() const {return Rec;} //clone the result!

private slots:
    void on_pbAccept_clicked();
    void on_pbReject_clicked();
    void on_pbGunTest_clicked();

    void on_cobGunSourceType_currentIndexChanged(int index);

private:
    Ui::AParticleSourceDialog *ui;
    MainWindow& MW;
    AParticleSourceRecord Rec;
};

#endif // APARTICLESOURCEDIALOG_H
