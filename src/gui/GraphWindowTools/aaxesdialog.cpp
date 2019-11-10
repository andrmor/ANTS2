#include "aaxesdialog.h"
#include "ui_aaxesdialog.h"
#include "amessage.h"

#include <QDebug>

#include "TAxis.h"
#include "TString.h"

AAxesDialog::AAxesDialog(QVector<TAxis *> & axes, int axisIndex, QWidget * parent) :
    QDialog(parent), ui(new Ui::AAxesDialog),
    Axes(axes), AxisIndex(axisIndex)
{
    ui->setupUi(this);

    if (axisIndex >=0 && axisIndex < Axes.size())
        Axis = Axes[axisIndex];

    if (Axis) updateGui();

    /*
    QString oldTitle;
    oldTitle = axis->GetTitle();
    bool ok;
    QString newTitle = QInputDialog::getText(&GraphWindow, "Change axis title", QString("New %1 axis title:").arg(X0Y1 == 0 ? "X" : "Y"), QLineEdit::Normal, oldTitle, &ok);
    if (ok) axis->SetTitle(newTitle.toLatin1().data());
    */
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
    qDebug() << "dummy";
    emit requestRedraw();
}

void AAxesDialog::updateGui()
{
    ui->leTitle->setText(Axis->GetTitle());

    ui->ledOffset->setText( QString::number(Axis->GetTitleOffset()) );

    QString tickOptStr = Axis->GetTicks();
    qDebug() << tickOptStr;
    bool bIn  = tickOptStr.contains('+');
    bool bOut = tickOptStr.contains('-');
    int tickOpt;
    if      (bOut && bIn) tickOpt = 2;
    else if (bIn)         tickOpt = 0;
    else                  tickOpt = 1;
    ui->cobTickInsideOutside->setCurrentIndex(tickOpt);
}

#include "aroottextconfigurator.h"
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
}

void AAxesDialog::on_ledOffset_editingFinished()
{
    Axis->SetTitleOffset( ui->ledOffset->text().toDouble() );
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
