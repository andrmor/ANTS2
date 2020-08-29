#include "acameracontroldialog.h"
#include "ui_acameracontroldialog.h"

#include <QDebug>
#include <QDoubleValidator>

#include "rasterwindowbaseclass.h"
#include "TCanvas.h"
#include "TView3D.h"

ACameraControlDialog::ACameraControlDialog(RasterWindowBaseClass *RasterWin, QWidget * parent) :
    QDialog(parent), RW(RasterWin),
    ui(new Ui::ACameraControlDialog)
{
    ui->setupUi(this);
    ui->pbDummy->setDefault(true);
    for (QPushButton * pb : {ui->pbDummy, ui->pbSetView}) pb->setVisible(false);

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

void ACameraControlDialog::showAndUpdate()
{
    show();
    if (xPos != 0 && yPos != 0) move(xPos, yPos);
    updateGui();
}

void ACameraControlDialog::setView()
{
    if (!RW->fCanvas->HasViewer3D()) return;

    AGeoViewParameters & p = RW->ViewParameters;

    // Range
    p.RangeLL[0] = ui->ledRangeLX->text().toDouble();
    p.RangeLL[1] = ui->ledRangeLY->text().toDouble();
    p.RangeLL[2] = ui->ledRangeLZ->text().toDouble();

    p.RangeUR[0] = ui->ledRangeUX->text().toDouble();
    p.RangeUR[1] = ui->ledRangeUY->text().toDouble();
    p.RangeUR[2] = ui->ledRangeUZ->text().toDouble();

    for (int i = 0; i < 3; i++)
    {
        if (p.RangeLL[i] > p.RangeUR[i]) std::swap(p.RangeLL[i], p.RangeUR[i]);
        else if (p.RangeLL[i] == p.RangeUR[i]) p.RangeUR[i] += 1.0;
    }

    RW->fCanvas->GetView()->SetRange(p.RangeLL, p.RangeUR);

    // Window
    p.WinX = ui->ledWx->text().toDouble();
    p.WinY = ui->ledWy->text().toDouble();
    p.WinW = ui->ledWw->text().toDouble();
    p.WinH = ui->ledWh->text().toDouble();

    RW->fCanvas->GetView()->SetWindow(p.WinX, p.WinY, p.WinW, p.WinH);

    // Rotation
    p.Long = ui->ledLong->text().toDouble();
    p.Lat  = ui->ledLat->text().toDouble();
    p.Psi  = ui->ledPsi->text().toDouble();

    int err;
    RW->fCanvas->GetView()->SetView(p.Long, p.Lat, p.Psi, err);

    // Update canvas
    RW->fCanvas->Modified();
    RW->fCanvas->Update();

    // Read back
    updateGui();
}

void ACameraControlDialog::on_pbClose_clicked()
{
    close();
}

void ACameraControlDialog::updateGui()
{
    if (!RW->fCanvas->HasViewer3D()) return;

    AGeoViewParameters & p = RW->ViewParameters;
    p.read(RW->fCanvas);

    ui->ledRangeLX->setText(QString::number(p.RangeLL[0]));
    ui->ledRangeLY->setText(QString::number(p.RangeLL[1]));
    ui->ledRangeLZ->setText(QString::number(p.RangeLL[2]));
    ui->ledRangeUX->setText(QString::number(p.RangeUR[0]));
    ui->ledRangeUY->setText(QString::number(p.RangeUR[1]));
    ui->ledRangeUZ->setText(QString::number(p.RangeUR[2]));
    ui->ledCenterX->setText(QString::number(p.RotCenter[0], 'g', 4));
    ui->ledCenterY->setText(QString::number(p.RotCenter[1], 'g', 4));
    ui->ledCenterZ->setText(QString::number(p.RotCenter[2], 'g', 4));

    ui->ledWx->setText(QString::number(p.WinX));
    ui->ledWy->setText(QString::number(p.WinY));
    ui->ledWw->setText(QString::number(p.WinW));
    ui->ledWh->setText(QString::number(p.WinH));

    ui->ledLat-> setText( QString::number(p.Lat) );
    ui->ledLong->setText( QString::number(p.Long) );
    ui->ledPsi-> setText( QString::number(p.Psi) );
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

void ACameraControlDialog::closeEvent(QCloseEvent *)
{
    xPos = x();
    yPos = y();
}

void ACameraControlDialog::setCenter(int index, QLineEdit *led)
{
    if (!RW->fCanvas->HasViewer3D()) return;

    AGeoViewParameters & p = RW->ViewParameters;

    p.read(RW->fCanvas);
    double halfRange = 0.5 * (p.RangeUR[index] - p.RangeLL[index]);

    double c = led->text().toDouble();
    p.RangeLL[index] = c - halfRange;
    p.RangeUR[index] = c + halfRange;

    setView();
}

void ACameraControlDialog::on_pbSetView_clicked()
{
    setView();
}
