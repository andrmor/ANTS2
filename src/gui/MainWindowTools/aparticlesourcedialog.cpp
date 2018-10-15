#include "aparticlesourcedialog.h"
#include "ui_aparticlesourcedialog.h"
#include "mainwindow.h"
#include "amaterialparticlecolection.h"
#include "amessage.h"

#include <QDebug>

AParticleSourceDialog::AParticleSourceDialog(MainWindow & MW, const AParticleSourceRecord * Rec) :
    QDialog(&MW),
    MW(MW), Rec(Rec->clone()),
    ui(new Ui::AParticleSourceDialog)
{
    ui->setupUi(this);

    ui->pbUpdateRecord->setDefault(true);
    ui->pbUpdateRecord->setVisible(false);

    ui->cobGunParticle->addItems(MW.MpCollection->getListOfParticleNames());

    ui->leSourceName->setText(Rec->name);
    ui->cobGunSourceType->setCurrentIndex(Rec->index);

    ui->ledGun1DSize->setText(QString::number(2.0 * Rec->size1));
    ui->ledGun2DSize->setText(QString::number(2.0 * Rec->size2));
    ui->ledGun3DSize->setText(QString::number(2.0 * Rec->size3));

    ui->ledGunOriginX->setText(QString::number(Rec->X0));
    ui->ledGunOriginY->setText(QString::number(Rec->Y0));
    ui->ledGunOriginZ->setText(QString::number(Rec->Z0));

    ui->ledGunPhi->setText(QString::number(Rec->Phi));
    ui->ledGunTheta->setText(QString::number(Rec->Theta));
    ui->ledGunPsi->setText(QString::number(Rec->Psi));

    ui->ledGunCollPhi->setText(QString::number(Rec->CollPhi));
    ui->ledGunCollTheta->setText(QString::number(Rec->CollTheta));
    ui->ledGunSpread->setText(QString::number(Rec->Spread));

    ui->cbSourceLimitmat->setChecked(Rec->DoMaterialLimited);
    ui->leSourceLimitMaterial->setText(Rec->LimtedToMatName);

    ui->frSecondary->setVisible(false);
    UpdateListWidget();
    if ( !Rec->GunParticles.isEmpty() )
    {
        ui->lwGunParticles->setCurrentRow(0);
        UpdateParticleInfo();
    }
}

AParticleSourceDialog::~AParticleSourceDialog()
{
    delete ui;
    delete Rec;
}

AParticleSourceRecord *AParticleSourceDialog::getResult()
{
    AParticleSourceRecord* tmp = Rec;
    Rec = 0;
    return tmp;
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
        s <<""<<""<<"";
    }
    ui->lGun1DSize->setText(s[0]);
    ui->lGun2DSize->setText(s[1]);
    ui->lGun3DSize->setText(s[2]);

    bool b1 = !s[0].isEmpty();
    ui->lGun1DSize->setVisible(b1);
    ui->ledGun1DSize->setVisible(b1);
    ui->lGun1DSize_mm->setVisible(b1);

    bool b2 = !s[1].isEmpty();
    ui->lGun2DSize->setVisible(b2);
    ui->ledGun2DSize->setVisible(b2);
    ui->lGun2DSize_mm->setVisible(b2);

    bool b3 = !s[2].isEmpty();
    ui->lGun3DSize->setVisible(b3);
    ui->ledGun3DSize->setVisible(b3);
    ui->lGun3DSize_mm->setVisible(b3);

    ui->ledGunPhi->setEnabled(index != 0);
    ui->ledGunTheta->setEnabled(index != 0);
    ui->ledGunPsi->setEnabled(index > 1);

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

void AParticleSourceDialog::on_pbGunAddNew_clicked()
{
    GunParticleStruct* tmp = new GunParticleStruct();
    tmp->ParticleId = ui->cobGunParticle->currentIndex();
    tmp->StatWeight = ui->ledGunParticleWeight->text().toDouble();
    tmp->energy = ui->ledGunEnergy->text().toDouble();
    tmp->spectrum = 0;
    Rec->GunParticles << tmp;

    UpdateListWidget();
    ui->lwGunParticles->setCurrentRow( Rec->GunParticles.size()-1 );
    UpdateParticleInfo();
}

void AParticleSourceDialog::on_pbGunRemove_clicked()
{
    if (Rec->GunParticles.size() < 2)
    {
        message("There should be at least one particle!", this);
        return;
    }

    int iparticle = ui->lwGunParticles->currentRow();
    if (iparticle == -1)
      {
        message("Select a particle to remove!", this);
        return;
      }

    delete Rec->GunParticles[iparticle];
    Rec->GunParticles.remove(iparticle);

    UpdateListWidget();
    ui->lwGunParticles->setCurrentRow( Rec->GunParticles.size()-1 );
    UpdateParticleInfo();
}

void AParticleSourceDialog::UpdateListWidget()
{
    ui->lwGunParticles->clear();
    int counter = 0;
    for (const GunParticleStruct* gps : Rec->GunParticles)
    {
        bool Individual = gps->Individual;

        QString str, str1;
        if (Individual) str = "";
        else str = ">";
        str1.setNum(counter++);
        str += str1 + "> ";
        str += MW.MpCollection->getParticleName(gps->ParticleId);
        if (gps->spectrum) str += " E=spec";
        else
        {
            str1.setNum(gps->energy);
            str += " E=" + str1;
        }

        if (Individual)
        {
            str += " W=";
            str1.setNum(gps->StatWeight);
            str += str1;
        }
        else
        {
            str += " Link:";
            str += QString::number(gps->LinkedTo);
            str += " P=";
            str += QString::number(gps->LinkingProbability);
            if (gps->LinkingOppositeDir)
                str += " Opposite";
        }
        ui->lwGunParticles->addItem(str);
    }
}

void AParticleSourceDialog::UpdateParticleInfo()
{
    int row = ui->lwGunParticles->currentRow();

    int DefinedSourceParticles = Rec->GunParticles.size();
    if (DefinedSourceParticles > 0 && row>-1)
    {
        ui->fGunParticle->setEnabled(true);
        int part = Rec->GunParticles.at(row)->ParticleId;
        ui->cobGunParticle->setCurrentIndex(part);

        const GunParticleStruct* gRec = Rec->GunParticles.at(row);

        QString str;
        str.setNum(gRec->StatWeight);
        ui->ledGunParticleWeight->setText(str);

        int iPrefUnits = ui->cobUnits->findText(gRec->PreferredUnits);
        double energy = gRec->energy;
        if (iPrefUnits > -1)
        {
            ui->cobUnits->setCurrentIndex(iPrefUnits);
            if      (gRec->PreferredUnits == "MeV") energy *= 1.0e-3;
            else if (gRec->PreferredUnits == "keV") ;
            else if (gRec->PreferredUnits == "eV") energy *= 1.0e3;
            else if (gRec->PreferredUnits == "meV") energy *= 1.0e6;
        }
        else ui->cobUnits->setCurrentText("keV");
        ui->ledGunEnergy->setText( QString::number(energy) );

        ui->cbLinkedParticle->setChecked(!gRec->Individual);
        ui->frSecondary->setVisible(!gRec->Individual);
        ui->leiParticleLinkedTo->setText( QString::number(gRec->LinkedTo) );
        str.setNum(gRec->LinkingProbability);
        ui->ledLinkingProbability->setText(str);
        ui->cbLinkingOpposite->setChecked(gRec->LinkingOppositeDir);
        if (gRec->spectrum == 0)
        {
            ui->pbGunShowSpectrum->setEnabled(false);
            ui->ledGunEnergy->setEnabled(true);
            ui->rbFixed->setChecked(true);
        }
        else
        {
            ui->pbGunShowSpectrum->setEnabled(true);
            ui->ledGunEnergy->setEnabled(false);
            ui->rbSpectrum->setChecked(false);
        }
    }
    else ui->fGunParticle->setEnabled(false);
}

void AParticleSourceDialog::on_lwGunParticles_currentRowChanged(int)
{
    UpdateParticleInfo();
}

void AParticleSourceDialog::on_cobUnits_activated(int)
{
    int iPart = ui->lwGunParticles->currentRow();
    if (iPart == -1) return;
    Rec->GunParticles[iPart]->PreferredUnits = ui->cobUnits->currentText();
    UpdateParticleInfo();
}

#include "geometrywindowclass.h"
#include "TGeoManager.h"
void AParticleSourceDialog::on_pbShowSource_toggled(bool checked)
{
    if (checked)
      {
        MW.GeometryWindow->ShowAndFocus();
        MW.ShowSource(Rec, true);
      }
    else
      {
        gGeoManager->ClearTracks();
        MW.GeometryWindow->ShowGeometry();
      }
}

void AParticleSourceDialog::on_cbLinkedParticle_toggled(bool checked)
{
    ui->fLinkedParticle->setVisible(checked);
}

void AParticleSourceDialog::on_pbUpdateRecord_clicked()
{
    Rec->index = ui->cobGunSourceType->currentIndex();

    Rec->size1 = 0.5 * ui->ledGun1DSize->text().toDouble();
    Rec->size2 = 0.5 * ui->ledGun2DSize->text().toDouble();
    Rec->size3 = 0.5 * ui->ledGun3DSize->text().toDouble();

    Rec->DoMaterialLimited = ui->cbSourceLimitmat->isChecked();
    Rec->LimtedToMatName = ui->leSourceLimitMaterial->text();
    //ParticleSources->checkLimitedToMaterial(Rec);

    Rec->X0 = ui->ledGunOriginX->text().toDouble();
    Rec->Y0 = ui->ledGunOriginY->text().toDouble();
    Rec->Z0 = ui->ledGunOriginZ->text().toDouble();

    Rec->Phi = ui->ledGunPhi->text().toDouble();
    Rec->Theta = ui->ledGunTheta->text().toDouble();
    Rec->Psi = ui->ledGunPsi->text().toDouble();

    Rec->CollPhi = ui->ledGunCollPhi->text().toDouble();
    Rec->CollTheta = ui->ledGunCollTheta->text().toDouble();
    Rec->Spread = ui->ledGunSpread->text().toDouble();

    int iPart = ui->lwGunParticles->currentRow();
    if (iPart >= 0)
    {
        GunParticleStruct* p = Rec->GunParticles[iPart];

        p->ParticleId = ui->cobGunParticle->currentIndex();
        p->StatWeight = ui->ledGunParticleWeight->text().toDouble();
        p->energy = ui->ledGunEnergy->text().toDouble();
        p->Individual = !ui->cbLinkedParticle->isChecked();
        p->LinkingProbability = ui->ledLinkingProbability->text().toDouble();
        p->LinkingOppositeDir = ui->cbLinkingOpposite->isChecked();
    }

    UpdateParticleInfo();
}
