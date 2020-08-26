#include "acameracontroldialog.h"
#include "ui_acameracontroldialog.h"

#include <QDebug>
#include <QDoubleValidator>

#include "TCanvas.h"
#include "TView3D.h"

ACameraControlDialog::ACameraControlDialog(TCanvas *Canvas, QWidget * parent) :
    QDialog(parent), Canvas(Canvas),
    ui(new Ui::ACameraControlDialog)
{
    ui->setupUi(this);

    updateGui();

    QDoubleValidator * dv = new QDoubleValidator(this);
    dv->setNotation(QDoubleValidator::ScientificNotation);
    QList<QLineEdit*> list = findChildren<QLineEdit*>();
    for (QLineEdit * w : list)
        if (w->objectName().startsWith("led")) w->setValidator(dv);
}

ACameraControlDialog::~ACameraControlDialog()
{
    delete ui;
}

void ACameraControlDialog::on_pbClose_clicked()
{
    accept();
}

void ACameraControlDialog::updateGui()
{
    if (!Canvas->HasViewer3D()) return;

    double LL[3];
    double UR[3];
    Canvas->GetView()->GetRange(LL, UR);

    ui->ledRangeLX->setText(QString::number(LL[0]));
    ui->ledRangeLY->setText(QString::number(LL[1]));
    ui->ledRangeLZ->setText(QString::number(LL[2]));

    ui->ledRangeUX->setText(QString::number(UR[0]));
    ui->ledRangeUY->setText(QString::number(UR[1]));
    ui->ledRangeUZ->setText(QString::number(UR[2]));

    ui->ledCenterX->setText(QString::number( 0.5*(UR[0] + LL[0])) );
    ui->ledCenterY->setText(QString::number( 0.5*(UR[1] + LL[1])) );
    ui->ledCenterZ->setText(QString::number( 0.5*(UR[2] + LL[2])) );
}

void ACameraControlDialog::on_pbUpdateRange_clicked()
{
    if (!Canvas->HasViewer3D()) return;

    double LL[3];
    double UR[3];

    LL[0] = ui->ledRangeLX->text().toDouble();
    LL[1] = ui->ledRangeLY->text().toDouble();
    LL[2] = ui->ledRangeLZ->text().toDouble();

    UR[0] = ui->ledRangeUX->text().toDouble();
    UR[1] = ui->ledRangeUY->text().toDouble();
    UR[2] = ui->ledRangeUZ->text().toDouble();

    for (int i=0; i<3; i++)
    {
        if (LL[i] > UR[i]) std::swap(LL[i], UR[i]);
        else if (LL[i] == UR[i]) UR[i] += 1.0;
    }

    Canvas->GetView()->SetRange(LL, UR);
    updateGui();
}

void ACameraControlDialog::on_ledCenterX_editingFinished()
{
    setCenter(0, ui->ledCenterX);
}

void ACameraControlDialog::on_ledCenterY_editingFinished()
{
    setCenter(1, ui->ledCenterY);
}

void ACameraControlDialog::on_ledCenterZ_editingFinished()
{
    setCenter(2, ui->ledCenterZ);
}

void ACameraControlDialog::setCenter(int index, QLineEdit *led)
{
    if (!Canvas->HasViewer3D()) return;

    double LL[3];
    double UR[3];
    Canvas->GetView()->GetRange(LL, UR);
    double halfRange = 0.5 * (UR[index] - LL[index]);

    double c = led->text().toDouble();
    LL[index] = c - halfRange;
    UR[index] = c + halfRange;

    Canvas->GetView()->SetRange(LL, UR);
    updateGui();
}
