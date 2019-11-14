#include "aaxesdialog.h"
#include "ui_aaxesdialog.h"
#include "amessage.h"
#include "aroottextconfigurator.h"

#include <QDebug>
#include <QDoubleValidator>

#include "TAxis.h"
#include "TAttAxis.h"
#include "TString.h"

AAxesDialog::AAxesDialog(QVector<TAxis *> & axes, int axisIndex, QWidget * parent) :
    QDialog(parent), ui(new Ui::AAxesDialog),
    Axes(axes), AxisIndex(axisIndex)
{
    ui->setupUi(this);
    setWindowTitle("Axis property editor");

    ui->pbDummy->setDefault(true);
    ui->pbDummy->setVisible(false);
    ui->pbConfigureDivisions->setVisible(false);

    QDoubleValidator* dv = new QDoubleValidator(this);
    dv->setNotation(QDoubleValidator::ScientificNotation);
    QList<QLineEdit*> list = this->findChildren<QLineEdit *>();
    foreach(QLineEdit *w, list) if (w->objectName().startsWith("led")) w->setValidator(dv);

    if (axisIndex >=0 && axisIndex < Axes.size())
        Axis = Axes[axisIndex];
    if (Axis) updateGui();
}

AAxesDialog::~AAxesDialog()
{
    delete ui;
}

int AAxesDialog::exec()
{
    if (!Axis)
    {
        message("Axis does not exist!", this);
        return QDialog::Rejected;
    }
    return QDialog::exec();
}

void AAxesDialog::on_pbDummy_clicked()
{
    emit requestRedraw();
}

void AAxesDialog::updateGui()
{
    ui->leTitle->setText(Axis->GetTitle());

    ui->ledOffset->setText( QString::number(Axis->GetTitleOffset()) );

    QString tickOptStr = Axis->GetTicks();
    bool bIn  = tickOptStr.contains('+');
    bool bOut = tickOptStr.contains('-');
    int tickOpt;
    if      (bOut && bIn) tickOpt = 2;
    else if (bIn)         tickOpt = 0;
    else                  tickOpt = 1;
    ui->cobTickInsideOutside->setCurrentIndex(tickOpt);

    ui->ledTickLength->setText( QString::number(Axis->GetTickLength()) );

    ui->ledLabelOffset->setText( QString::number(Axis->GetLabelOffset()) );

    int ndiv = Axis->GetNdivisions();
    ui->cbDivAllowOptimize->setChecked(ndiv >= 0);
    ndiv = abs(ndiv);
    int n3 = ndiv / 10000;
    int n2 = ndiv / 100 - n3 * 100;
    int n1 = ndiv       - n2 * 100;
    ui->sbNdiv->setValue(n1);
    ui->sbNdivSub->setValue(n2);
    ui->sbNdivSubSub->setValue(n3);
}

void AAxesDialog::on_pbTitleProperties_clicked()
{
    int   color = Axis->GetTitleColor();
    int   align = ( Axis->GetCenterTitle() ? 0 : 1 );
    int   font = Axis->GetTitleFont();
    float size = Axis->GetTitleSize();

    ARootAxisTitleTextConfigurator D(color, align, font, size, this);
    int res = D.exec();
    if (res == QDialog::Accepted)
    {
        for (int i=0; i<3; i++)
        {
            TAxis * axis = Axes[i];
            if (!axis) continue;
            if (axis == Axis || ui->cbApplyTitleTextPropToAllAxes->isChecked())
            {
                axis->SetTitleColor(color);
                axis->SetTitleFont(font);
                axis->CenterTitle( align == 1 );
                axis->SetTitleSize(size);
            }
        }
        emit requestRedraw();
    }
}

void AAxesDialog::on_pbAccept_clicked()
{
    accept();
}

void AAxesDialog::on_leTitle_editingFinished()
{
    Axis->SetTitle(ui->leTitle->text().toLatin1().data());
    emit requestRedraw();
}

void AAxesDialog::on_ledOffset_editingFinished()
{
    Axis->SetTitleOffset( ui->ledOffset->text().toDouble() );
    emit requestRedraw();
}

void AAxesDialog::on_pbLabelProperties_clicked()
{
    int   color = Axis->GetLabelColor();
    int   align;
    int   font = Axis->GetLabelFont();
    float size = Axis->GetLabelSize();

    ARootAxisLabelTextConfigurator D(color, align, font, size, this);
    int res = D.exec();
    if (res == QDialog::Accepted)
    {
        for (int i=0; i<3; i++)
        {
            TAxis * axis = Axes[i];
            if (!axis) continue;
            if (axis == Axis || ui->cbApplyLableTextPropToAllAxes->isChecked())
            {
                axis->SetLabelColor(color);
                axis->SetLabelFont(font);
                axis->SetLabelSize(size);
            }
        }
        emit requestRedraw();
    }
}

void AAxesDialog::on_cobTickInsideOutside_activated(int index)
{
    QString tickOptStr;
    if      (index == 0) tickOptStr = '+';
    else if (index == 1) tickOptStr = '-';
    else                 tickOptStr = "+-";

    Axis->SetTicks(tickOptStr.toLatin1().data());
    emit requestRedraw();
}

void AAxesDialog::on_ledTickLength_editingFinished()
{
    Axis->SetTickLength(ui->ledTickLength->text().toFloat());
    emit requestRedraw();
}

void AAxesDialog::on_pbConfigureDivisions_clicked()
{
    int ndiv = ui->sbNdiv->value() + ui->sbNdivSub->value()*100 + ui->sbNdivSubSub->value()*10000;
    Axis->SetNdivisions(ndiv, ui->cbDivAllowOptimize->isChecked());
    emit requestRedraw();
}

void AAxesDialog::on_ledLabelOffset_editingFinished()
{
    Axis->SetLabelOffset( ui->ledLabelOffset->text().toFloat());
    emit requestRedraw();
}
