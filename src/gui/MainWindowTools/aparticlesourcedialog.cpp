#include "aparticlesourcedialog.h"
#include "ui_aparticlesourcedialog.h"
#include "mainwindow.h"
#include "aparticlesourcerecord.h"
#include "amaterialparticlecolection.h"
#include "amessage.h"
#include "geometrywindowclass.h"
#include "graphwindowclass.h"
#include "aglobalsettings.h"
#include "ajsontools.h"
#include "asourceparticlegenerator.h"
#include "detectorclass.h"

#include <QDebug>
#include <QDoubleValidator>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QApplication>

#include "TGeoManager.h"
#include "TH1D.h"

AParticleSourceDialog::AParticleSourceDialog(MainWindow & MW, const AParticleSourceRecord * Rec) :
    QDialog(&MW),
    MW(MW), Rec(Rec->clone()), OriginalRec(Rec),
    ui(new Ui::AParticleSourceDialog)
{
    ui->setupUi(this);

    setWindowModality(Qt::WindowModal);
    setWindowTitle("Particle source configurator");

    QDoubleValidator* dv = new QDoubleValidator(this);
    dv->setNotation(QDoubleValidator::ScientificNotation);
    QList<QLineEdit*> list = this->findChildren<QLineEdit *>();
    foreach(QLineEdit *w, list) if (w->objectName().startsWith("led")) w->setValidator(dv);

    ui->pbUpdateRecord->setDefault(true);
    ui->pbUpdateRecord->setVisible(false);

    ui->cobGunParticle->addItems(MW.MpCollection->getListOfParticleNames());

    ui->leSourceName->setText(Rec->name);
    ui->cobGunSourceType->setCurrentIndex(Rec->shape);

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
    updateColorLimitingMat();
    if ( !Rec->GunParticles.isEmpty() )
    {
        ui->lwGunParticles->setCurrentRow(0);
        UpdateParticleInfo();
    }

    on_cobGunSourceType_currentIndexChanged(ui->cobGunSourceType->currentIndex());

//    QMenuBar* mb = new QMenuBar(this);
//    QMenu* fileMenu = mb->addMenu("&File");
//    fileMenu->addAction("Load source", this, &AParticleSourceDialog::loadSource);
//    fileMenu->addAction("Save source", this, &AParticleSourceDialog::saveSource);
//    layout()->setMenuBar(mb);
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

void AParticleSourceDialog::closeEvent(QCloseEvent *e)
{
    on_pbUpdateRecord_clicked();

    QJsonObject jo, jn;
    Rec->writeToJson(jo, *MW.MpCollection);
    OriginalRec->writeToJson(jn, *MW.MpCollection);
    if (jo != jn)
    {
        QMessageBox msgBox(this);
        msgBox.setText("The source has been modified.");
        msgBox.setInformativeText("Discard all the changes?");
        msgBox.setStandardButtons(QMessageBox::Discard | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Discard);
        int ret = msgBox.exec();
        if (ret == QMessageBox::No)
        {
            e->ignore();
            return;
        }
    }
    QDialog::closeEvent(e);
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
    ASourceParticleGenerator ps(MW.Detector, MW.Detector->RandGen);
    ps.append(Rec);

    MW.GeometryWindow->ShowAndFocus();
    MW.ShowSource(Rec, true);
    MW.TestParticleGun(&ps, ui->sbGunTestEvents->value());

    ps.forget(Rec); //so Rec is not deleted
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
      case 2: s <<"X:"<<"Y:"<<"";
        break;
      case 3: s <<"Diameter:"<<""<<"";
        break;
      case 4: s <<"X:"<<"Y:"<<"Z:";
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

    bool bPoint = (index == 0);
    ui->labDimensions->setVisible(!bPoint);

    bool b1 = !s[0].isEmpty();
    ui->lGun1DSize->setVisible(b1);
    ui->ledGun1DSize->setVisible(b1);

    bool b2 = !s[1].isEmpty();
    ui->lGun2DSize->setVisible(b2);
    ui->ledGun2DSize->setVisible(b2);

    bool b3 = !s[2].isEmpty();
    ui->lGun3DSize->setVisible(b3);
    ui->ledGun3DSize->setVisible(b3);

    ui->ledGunPhi->setEnabled(index != 0);
    ui->ledGunTheta->setEnabled(index != 0);
    ui->ledGunPsi->setEnabled(index > 1);

    if (index == 1)
    {
        ui->labGunPhi->setText("Phi:");
        ui->labGunTheta->setText("Theta:");
    }
    else
    {
        ui->labGunPhi->setText("Around X:");
        ui->labGunTheta->setText("Around Y:");
    }
}

void AParticleSourceDialog::on_pbGunAddNew_clicked()
{
    GunParticleStruct* tmp = new GunParticleStruct();
    tmp->ParticleId = ui->cobGunParticle->currentIndex();
    tmp->StatWeight = ui->ledGunParticleWeight->text().toDouble();
    tmp->energy = ui->ledGunEnergy->text().toDouble();
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
    bUpdateInProgress = true; // >>>---

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
        if (gps->bUseFixedEnergy)
             str += QString(" E=%1").arg(gps->energy);
        else str += " E=spec";

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

    bUpdateInProgress = false; // <<<---
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
        ui->sbLinkedTo->setValue(gRec->LinkedTo);
        str.setNum(gRec->LinkingProbability);
        ui->ledLinkingProbability->setText(str);
        ui->cbLinkingOpposite->setChecked(gRec->LinkingOppositeDir);

        ui->pbGunShowSpectrum->setEnabled(gRec->spectrum);
        bool bFix = gRec->bUseFixedEnergy;
        ui->cobEnergy->setCurrentIndex( bFix ? 0 : 1 );
        ui->pbGunShowSpectrum->setVisible(!bFix);
        ui->pbGunLoadSpectrum->setVisible(!bFix);
        ui->pbDeleteSpectrum->setVisible(!bFix);
        ui->ledGunEnergy->setVisible(bFix);
        ui->cobUnits->setVisible(bFix);
    }
    else ui->fGunParticle->setEnabled(false);
}

void AParticleSourceDialog::on_lwGunParticles_currentRowChanged(int)
{
    if (bUpdateInProgress) return;

    UpdateParticleInfo();
}

void AParticleSourceDialog::on_cobUnits_activated(int)
{
    int iPart = ui->lwGunParticles->currentRow();
    if (iPart == -1) return;
    Rec->GunParticles[iPart]->PreferredUnits = ui->cobUnits->currentText();
    UpdateParticleInfo();
}

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
    Rec->name = ui->leSourceName->text();
    Rec->shape = ui->cobGunSourceType->currentIndex();

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
        p->bUseFixedEnergy = ( ui->cobEnergy->currentIndex() == 0);
        p->PreferredUnits = ui->cobUnits->currentText();
        double energy = ui->ledGunEnergy->text().toDouble();
        if      (p->PreferredUnits == "MeV") energy *= 1.0e3;
        else if (p->PreferredUnits == "keV") ;
        else if (p->PreferredUnits == "eV") energy *= 1.0e-3;
        else if (p->PreferredUnits == "meV") energy *= 1.0e-6;
        p->energy = energy;
        p->Individual = !ui->cbLinkedParticle->isChecked();
        p->LinkedTo = ui->sbLinkedTo->value();
        p->LinkingProbability = ui->ledLinkingProbability->text().toDouble();
        p->LinkingOppositeDir = ui->cbLinkingOpposite->isChecked();
    }

    int curRow = ui->lwGunParticles->currentRow();
    UpdateListWidget();
    if ( curRow < 0 && curRow >= ui->lwGunParticles->count() )
        curRow = 0;
    ui->lwGunParticles->setCurrentRow(curRow);
    UpdateParticleInfo();
    updateColorLimitingMat();
    if (ui->pbShowSource->isChecked()) MW.ShowSource(Rec, true);
}

void AParticleSourceDialog::on_cbLinkedParticle_clicked(bool checked)
{
    if (checked)
    {
        if (ui->lwGunParticles->currentRow() == 0)
        {
            ui->cbLinkedParticle->setChecked(false);
            message("First particle cannot be linked", this);
            return;
        }
    }
    on_pbUpdateRecord_clicked();
}

void AParticleSourceDialog::on_sbLinkedTo_editingFinished()
{
    int index = ui->sbLinkedTo->value();
    if (index >= ui->lwGunParticles->currentRow())
    {
        ui->sbLinkedTo->setValue(0);
        message("Bad index of the particle linked to! Setting to 0", this);
    }
    on_pbUpdateRecord_clicked();
}

void AParticleSourceDialog::on_ledLinkingProbability_editingFinished()
{
    double val = ui->ledLinkingProbability->text().toDouble();
    if (val < 0 || val > 1.0)
    {
        ui->ledLinkingProbability->setText(0);
        message("Linking probability has to be within [0, 1] range. Setting to 0", this);
    }
    on_pbUpdateRecord_clicked();
}

void AParticleSourceDialog::on_pbGunShowSpectrum_clicked()
{
    int particle = ui->lwGunParticles->currentRow();
    TH1D* h = new TH1D(*Rec->GunParticles[particle]->spectrum);

    h->GetXaxis()->SetTitle("Energy, keV");
    MW.GraphWindow->Draw(h); //registering!
    MW.GraphWindow->ShowAndFocus();
}

void AParticleSourceDialog::on_pbGunLoadSpectrum_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Load energy spectrum", MW.GlobSet.LastOpenDir, "Data files (*.dat *.txt);;All files (*)");
    if (fileName.isEmpty()) return;
    MW.GlobSet.LastOpenDir = QFileInfo(fileName).absolutePath();

    int iPart = ui->lwGunParticles->currentRow();
    bool bOK = Rec->GunParticles[iPart]->loadSpectrum(fileName);
    if (!bOK) message("Load file failed!", this);

    UpdateParticleInfo();
}

void AParticleSourceDialog::on_pbDeleteSpectrum_clicked()
{
    int iPart = ui->lwGunParticles->currentRow();
    delete Rec->GunParticles[iPart]->spectrum; Rec->GunParticles[iPart]->spectrum = 0;

    UpdateParticleInfo();
}

void AParticleSourceDialog::updateColorLimitingMat()
{
    if (!ui->cbSourceLimitmat->isChecked()) return;

    const QString name = ui->leSourceLimitMaterial->text();
    bool fFound = false;
    for (int iMat=0; iMat < MW.MpCollection->countMaterials(); iMat++)
        if (name == (*MW.MpCollection)[iMat]->name)
        {
            fFound = true;
            break;
        }

    QPalette palette = ui->leSourceLimitMaterial->palette();
    palette.setColor(QPalette::Text, (fFound ? Qt::black : Qt::red) );
    ui->leSourceLimitMaterial->setPalette(palette);
}
