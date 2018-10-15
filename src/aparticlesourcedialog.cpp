#include "aparticlesourcedialog.h"
#include "ui_aparticlesourcedialog.h"
#include "mainwindow.h"

#include <QDebug>

AParticleSourceDialog::AParticleSourceDialog(MainWindow &MW, const AParticleSourceRecord &Rec) :
    QDialog(*MW), ui(new Ui::AParticleSourceDialog),
    MW(MW), Rec(Rec.clone())
{
    ui->setupUi(this);
}

AParticleSourceDialog::~AParticleSourceDialog()
{
    delete ui;
    delete Rec;
}

void AParticleSourceDialog::on_pbAccept_clicked()
{
    accept();
}

void AParticleSourceDialog::on_pbReject_clicked()
{
    reject();
}

void AParticleSourceDialog::on_pbGunTest_clicked()
{

}

void AParticleSourceDialog::on_cobGunSourceType_currentIndexChanged(int index)
{
    QVector<QString> s;
    switch (index)
      {
      default:
      case 0: s <<""<<""<<"";
        break;
      case 1: s <<"Length:"<<""<<"";
        break;
      case 2: s <<"Size X:"<<"Size Y:"<<"";
        break;
      case 3: s <<"Diameter:"<<""<<"";
        break;
      case 4: s <<"Size X:"<<"Size Y:"<<"Size Z:";
        break;
      case 5: s <<"Diameter:"<<""<<"Height:";
        break;
      default:
        qWarning() << "Unknown source type!";
      }
    ui->lGun1DSize->setText(s[0]);
    ui->lGun2DSize->setText(s[1]);
    ui->lGun3DSize->setText(s[2]);
    ui->fGun1D->setVisible(!s[0].isEmpty());
    ui->fGun2D->setVisible(!s[1].isEmpty());
    ui->fGun3D->setVisible(!s[2].isEmpty());

    ui->fGunPhi->setEnabled(index != 0);
    ui->fGunTheta->setEnabled(index != 0);
    ui->fGunPsi->setEnabled(index > 1);

    if (index == 1)
    {
        ui->labGunPhi->setText("Phi");
        ui->labGunTheta->setText("Theta");
    }
    else
    {
        ui->labGunPhi->setText("around X");
        ui->labGunTheta->setText("around Y");
    }
}
