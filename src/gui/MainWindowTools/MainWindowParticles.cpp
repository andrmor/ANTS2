#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "amaterialparticlecolection.h"
#include "materialinspectorwindow.h"
#include "outputwindow.h"
#include "detectorclass.h"
#include "asourceparticlegenerator.h"
#include "amessage.h"
#include "aconfiguration.h"
#include "atrackrecords.h"
#include "asimulationmanager.h"
#include "aglobalsettings.h"
#include "aisotopeabundancehandler.h"

#include <QMessageBox>
#include <QDebug>

#include "TColor.h"
#include "TROOT.h"

void MainWindow::ListActiveParticles()
{
    ui->lwParticles->clear();
    int NumParticles = Detector->MpCollection->countParticles();
    QString tmpStr, tmpStr2;
    for (int i=0; i<NumParticles; i++)
      {
        tmpStr2.setNum(i);
        tmpStr =tmpStr2+">  " + Detector->MpCollection->getParticleName(i);
        QListWidgetItem* pItem =new QListWidgetItem(tmpStr);
        ui->lwParticles->addItem(pItem);
      }
    MainWindow::on_pbRefreshParticles_clicked();
}

void MainWindow::on_pbShowColorCoding_pressed()
{
  ui->lwParticles->clearSelection();
  for (int i=0; i<ui->lwParticles->count(); i++)
    {
      //TColor* rc = gROOT->GetColor(i +1 +ui->sbParticleTrackColorIndexAdd->value());
      TrackHolderClass th;
      SimulationManager->TrackBuildOptions.applyToParticleTrack(&th, i);
      TColor* rc = gROOT->GetColor(th.Color);

      if (!rc) continue;
      QColor qc = QColor(255*rc->GetRed(), 255*rc->GetGreen(), 255*rc->GetBlue(), 255*rc->GetAlpha());

      QListWidgetItem* pItem = ui->lwParticles->item(i);
      pItem->setTextColor(qc);
    }
}

void MainWindow::on_pbShowColorCoding_released()
{
  for (int i=0; i<ui->lwParticles->count(); i++)
    {
      QListWidgetItem* pItem = ui->lwParticles->item(i);
      pItem->setTextColor(Qt::black);
    }
}

void updateParticleCOB(QComboBox* cob, DetectorClass* Detector)
{
  int old = cob->currentIndex();

  cob->clear();
  for (int i=0; i<Detector->MpCollection->countParticles(); i++)
      cob->addItem(Detector->MpCollection->getParticleName(i));

  if (cob->count() != 0)
  {
     if (old>-1 && old<cob->count()) cob->setCurrentIndex(old);
     else cob->setCurrentIndex(0);
  }
}

void MainWindow::on_pbRefreshParticles_clicked()
{
  QList< QComboBox* > cobs;
  cobs << ui->cobParticleToStack;
  foreach (QComboBox* cob, cobs)
    updateParticleCOB(cob, Detector);

  Owindow->UpdateParticles();
}

void MainWindow::on_lwParticles_currentRowChanged(int currentRow)
{
    if (currentRow < 0) return;

    if (currentRow < Detector->MpCollection->countParticles())
    {
        const AParticle* p = Detector->MpCollection->getParticle(currentRow);
        ui->leParticleName->setText(p->ParticleName);
        ui->leiIonZ->setText(QString::number(p->ionZ));
        ui->leiMass->setText(QString::number(p->ionA));

        bool bIon = (p->ionZ != -1);
        ui->frIon->setVisible(bIon);

        bool bCanRename = !bIon &&
                          p->type != AParticle::_gamma_ &&
                          p->type != AParticle::_neutron_ &&
                          p->ParticleName != "e-" &&
                          p->ParticleName != "e+";
        ui->leParticleName->setReadOnly(!bCanRename);
        ui->pbRenameParticle->setEnabled(bCanRename);
    }
}

void MainWindow::on_pbRemoveParticle_clicked()
{
    int Id = ui->lwParticles->currentRow();
    if (Id == -1)
    {
        message("Select one of the defined particles", this);
        return;
    }

  switch( QMessageBox::information( this, "", "Remove particle " + MpCollection->getParticleName(Id) + " ?",
                                        "Yes", "Cancel", 0, 1 ) )
    {
      case 0: break;
      default: return;
    }

  QString err = Config->RemoveParticle(Id); //all updates are automatic, including GUI
  if (!err.isEmpty()) message(err, this);
}

void MainWindow::on_pbRenameParticle_clicked()
{
    int Id = ui->lwParticles->currentRow();
    if (Id == -1)
    {
        message("Select one of the defined particles", this);
        return;
    }

    AParticle* p = Detector->MpCollection->getParticle(Id);
    QString newName = ui->leParticleName->text();
    if (newName == p->ParticleName)
    {
        message("Particle name is the same", this);
        return;
    }

    p->ParticleName = newName;
    onRequestDetectorGuiUpdate();
    ui->lwParticles->setCurrentRow(Id);
}

void MainWindow::on_pbAddNewParticle_clicked()
{
    QString name = ui->cobAddNewParticle->currentText();
    if (name.isEmpty())
    {
        message("Select first a particle to add", this);
        return;
    }

    int Id = MpCollection->getParticleId(name);
    if (Id != -1)
    {
        message( QString("Particle %1 already exists").arg(name), this);
        return;
    }

    AParticle p(name, AParticle::_charged_);
    //MainWindowInits.cpp Line approx. 300:  "gamma" << "neutron" << "e-" << "e+" << "proton" << "custom_particle";
    if (name == "gamma")
        p.type = AParticle::_gamma_;
    else if (name == "neutron")
        p.type = AParticle::_neutron_;

    MpCollection->findOrAddParticle(p);
    onRequestDetectorGuiUpdate();
    ui->lwParticles->setCurrentRow(ui->lwParticles->count()-1);
}

void MainWindow::on_pbAddIon_clicked()
{
    QString name = ui->cobAddIon->currentText();
    if (name.isEmpty())
    {
        message("Select symbol", this);
        return;
    }
    QString massStr = ui->leiAddIonMass->text();
    int mass = massStr.toInt();
    if (massStr.isEmpty() || mass == 0)
    {
        message("Enter mass of the ion", this);
        return;
    }

    int Z = GlobSet.getIsotopeAbundanceHandler().getZ(name);
    if (Z == 0)
    {
        message(QString("Symbol '%1' is not a valid element").arg(name), this);
        return;
    }

    name += massStr;
    int Id = MpCollection->getParticleId(name);
    if (Id != -1)
    {
        message( QString("Particle name %1 already defined").arg(name), this);
        return;
    }

    AParticle p(name, AParticle::_charged_, Z, mass);
    MpCollection->findOrAddParticle(p);
    onRequestDetectorGuiUpdate();
    ui->lwParticles->setCurrentRow(ui->lwParticles->count()-1);
}

