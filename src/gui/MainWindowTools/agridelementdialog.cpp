#include "agridelementdialog.h"
#include "ui_agridelementdialog.h"

AGridElementDialog::AGridElementDialog(const QStringList& MaterialList, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AGridElementDialog)
{
    ui->setupUi(this);

    ui->cobBulkMaterial->insertItems(0, MaterialList);
    ui->cobWireMaterial->insertItems(0, MaterialList);

    QDoubleValidator* dv = new QDoubleValidator(this);
    dv->setNotation(QDoubleValidator::ScientificNotation);
    QList<QLineEdit*> list = this->findChildren<QLineEdit *>();
    foreach(QLineEdit *w, list) if (w->objectName().startsWith("led")) w->setValidator(dv);
}

AGridElementDialog::~AGridElementDialog()
{
    delete ui;
}

void AGridElementDialog::setValues(int shape, double p0, double p1, double p2)
{
    ui->cobGridType->setCurrentIndex(shape);
    switch (shape)
    {
    case 0:
        ui->ledPitch->setText(QString::number(2.0*p0));
        ui->ledMaxLength->setText(QString::number(2.0*p1));
        ui->ledWireDiameter->setText(QString::number(2.0*p2));
        break;
    case 1:
        ui->ledPitchX->setText(QString::number(2.0*p0));
        ui->ledPitchY->setText(QString::number(2.0*p1));
        ui->ledWireDiameter->setText(QString::number(2.0*p2));
        break;
    case 2:
        ui->ledOut->setText(QString::number(2.0*p0));
        ui->ledIn->setText(QString::number(2.0*p1));
        ui->ledHeight->setText(QString::number(2.0*p2));
        break;
    }
}

void AGridElementDialog::setBulkMaterial(int iMat)
{
    if (iMat < ui->cobBulkMaterial->count())
        ui->cobBulkMaterial->setCurrentIndex(iMat);
}

void AGridElementDialog::setWireMaterial(int iMat)
{
    if (iMat < ui->cobWireMaterial->count())
        ui->cobWireMaterial->setCurrentIndex(iMat);
}

int AGridElementDialog::shape()
{
    return ui->cobGridType->currentIndex();
}

double AGridElementDialog::pitch()
{
    return ui->ledPitch->text().toDouble();
}

double AGridElementDialog::length()
{
    return ui->ledMaxLength->text().toDouble();
}

double AGridElementDialog::pitchX()
{
    return ui->ledPitchX->text().toDouble();
}

double AGridElementDialog::pitchY()
{
    return ui->ledPitchY->text().toDouble();
}

double AGridElementDialog::diameter()
{
    return ui->ledWireDiameter->text().toDouble();
}

double AGridElementDialog::inner()
{
    return ui->ledIn->text().toDouble();
}

double AGridElementDialog::outer()
{
    return ui->ledOut->text().toDouble();
}

double AGridElementDialog::height()
{
    return ui->ledHeight->text().toDouble();
}

int AGridElementDialog::bulkMaterial()
{
    return ui->cobBulkMaterial->currentIndex();
}

int AGridElementDialog::wireMaterial()
{
    return ui->cobWireMaterial->currentIndex();
}

void AGridElementDialog::on_cobGridType_currentIndexChanged(int index)
{
    ui->frDiameter->setVisible(index != 2);
}

void AGridElementDialog::on_pbAccept_clicked()
{
    accept();
}

void AGridElementDialog::on_pbCancel_clicked()
{
    reject();
}
