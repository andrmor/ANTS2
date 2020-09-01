#include "acameracontroldialog.h"
#include "ui_acameracontroldialog.h"
#include "rasterwindowbaseclass.h"
#include "detectorclass.h"
#include "asandwich.h"
#include "ageoobject.h"
#include "apmhub.h"

#include <QDebug>
#include <QDoubleValidator>

#include "TCanvas.h"
#include "TView3D.h"

ACameraControlDialog::ACameraControlDialog(RasterWindowBaseClass * RasterWin, const DetectorClass & Detector, QWidget * parent) :
    QDialog(parent), RW(RasterWin), Detector(Detector),
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
    if (xPos != 0 || yPos != 0) move(xPos, yPos);
    show();
    updateGui();
}

void ACameraControlDialog::setView(bool bSkipReadRange)
{
    if (!RW->fCanvas->HasViewer3D()) return;

    AGeoViewParameters & p = RW->ViewParameters;

    // Range
    if (!bSkipReadRange)
    {
        p.RangeLL[0] = ui->ledRangeLX->text().toDouble();
        p.RangeLL[1] = ui->ledRangeLY->text().toDouble();
        p.RangeLL[2] = ui->ledRangeLZ->text().toDouble();

        p.RangeUR[0] = ui->ledRangeUX->text().toDouble();
        p.RangeUR[1] = ui->ledRangeUY->text().toDouble();
        p.RangeUR[2] = ui->ledRangeUZ->text().toDouble();
    }

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

    setView(true);
}

void ACameraControlDialog::on_pbSetView_clicked()
{
    setView();
}

void ACameraControlDialog::on_pbStepXM_clicked()
{
    makeStep(0, -1.0 * ui->ledStepX->text().toDouble());
}

void ACameraControlDialog::on_pbStepXP_clicked()
{
    makeStep(0, ui->ledStepX->text().toDouble());
}

void ACameraControlDialog::on_pbStepYM_clicked()
{
    makeStep(1, -1.0 * ui->ledStepY->text().toDouble());
}

void ACameraControlDialog::on_pbStepYP_clicked()
{
    makeStep(1, ui->ledStepY->text().toDouble());
}

void ACameraControlDialog::on_pbStepZM_clicked()
{
    makeStep(2, -1.0 * ui->ledStepZ->text().toDouble());
}

void ACameraControlDialog::on_pbStepZP_clicked()
{
    makeStep(2, ui->ledStepZ->text().toDouble());
}

void ACameraControlDialog::makeStep(int index, double step)
{
    if (!RW->fCanvas->HasViewer3D()) return;

    AGeoViewParameters & p = RW->ViewParameters;
    p.read(RW->fCanvas);
    p.RangeLL[index] += step;
    p.RangeUR[index] += step;
    p.apply(RW->fCanvas);

    updateGui();
}

#include "amessage.h"
#include "ageoshape.h"
void ACameraControlDialog::on_pbSetFocus_clicked()
{
    QString name = ui->leFocusVolume->text();
    if (name.isEmpty())
    {
        message("Provide the name for the volume to focus", this);
        return;
    }

    QString err = setFocus(name);
    if (!err.isEmpty()) message(err, this);
}

QString ACameraControlDialog::setFocus(const QString & name)
{
    if (!RW->fCanvas->HasViewer3D()) return "There is no 3D view!";
    if (name.isEmpty()) return "Provide the name for the volume to focus";

    double worldPos[3];
    double size = 100;
    bool ok = true;

    AGeoObject * obj = Detector.Sandwich->World->findObjectByName(name);
    if (!obj)
    {
        QString numStr = name;
        if (numStr.startsWith("PM")) // it could be a PM
        {
            numStr.remove("PM");
            int iPM = numStr.toInt(&ok);
            if (ok && iPM > -1 && iPM < Detector.PMs->count())
            {
                worldPos[0] = Detector.PMs->at(iPM).x;
                worldPos[1] = Detector.PMs->at(iPM).y;
                worldPos[2] = Detector.PMs->at(iPM).z;
                size = std::max(Detector.PMs->getTypeForPM(iPM)->SizeX, Detector.PMs->getTypeForPM(iPM)->SizeY);
            }
            else return "Volume/PM " + name + " not found!";
        }
        else if (numStr.startsWith("dPM")) // or dummy PM
        {
            numStr.remove("dPM");
            int iDpm = numStr.toInt(&ok);
            if (ok && iDpm > -1 && iDpm < Detector.PMdummies.size())
            {
                for (int i = 0; i < 3; i++)
                    worldPos[i] = Detector.PMdummies.at(iDpm).r[i];
                int iType = Detector.PMdummies.at(iDpm).PMtype;
                if (iType > -1 && iType < Detector.PMs->countPMtypes())
                {
                    const APmType * PMtype = Detector.PMs->getType(iType);
                    size = std::max(PMtype->SizeX, PMtype->SizeY);
                }
            }
            else return "Volume/dummyPM " + name + " not found!";
        }
        else return "Volume " + name + " not found!";
    }


    if (obj) // else already have the position
    {
        ok = obj->getPositionInWorld(worldPos);
        if (!ok) return "Failed to compute global position of this volume!";

        size = obj->getMaxSize();
    }

    AGeoViewParameters & p = RW->ViewParameters;
    p.read(RW->fCanvas);

    for (int i=0; i<3; i++)
    {
        p.RangeLL[i] = worldPos[i] - size;
        p.RangeUR[i] = worldPos[i] + size;
    }
    p.WinX = 0;
    p.WinY = 0;

    RW->setVisible(false);
        p.apply(RW->fCanvas);
    RW->setVisible(true);
    TView3D * v = static_cast<TView3D*>(RW->fCanvas->GetView());
    v->Zoom();

    updateGui();
    return "";
}
