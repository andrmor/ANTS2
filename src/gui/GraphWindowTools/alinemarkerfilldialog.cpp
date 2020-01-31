#include "alinemarkerfilldialog.h"
#include "ui_alinemarkerfilldialog.h"
#include "arootcolorselectordialog.h"
#include "adrawobject.h"

#include <QDebug>

#include "TROOT.h"
#include "TColor.h"
#include "TObject.h"
#include "TAttLine.h"
#include "TAttMarker.h"
#include "TAttFill.h"

ALineMarkerFillDialog::ALineMarkerFillDialog(ADrawObject & drawObject, bool bFirstObject, QWidget * parent) :
    QDialog(parent),
    ui(new Ui::ALineMarkerFillDialog),
    DrawObject(drawObject),
    bFirstObject(bFirstObject)
{
    ui->setupUi(this);

    setWindowTitle("Draw attributes");
    connect(this, &ALineMarkerFillDialog::rejected, this, &ALineMarkerFillDialog::onReject);

    ui->pbPreview->setDefault(true);
    ui->pbUpdateObject->setVisible(false);

    ui->frFillPattern->setVisible(false);

    QStringList Lstyles = {"Straight","Short_dash","Dot","Short_dash dot","Dash dot","Dash 3_dots","Dash","Dash 2_dots","Long_dash","Long_dash dot"};
    ui->cobLineStyle->addItems(Lstyles);

    QStringList Mstyles = {"1 - small dot","2 - cross","3 - asterisk","4 - circle","5 - diagonal cross","6 - rhomb dot","7 - square dot","8 - large round dot",
              "20 - filled circle","21 - filled square","22 - filled triangle","23 - inv filled triangle","24 - circle","25 - square",
              "26 - triangle","27 - romb","28 - big cross","29 - filled star","30 - star","32 - inverted triangle","33 - filled romb","34 - filled cross"};
    ui->cobMarkerStyle->addItems(Mstyles);

    lineAtt = dynamic_cast<TAttLine*>(DrawObject.Pointer);
    if (lineAtt) CopyLineAtt = new TAttLine(*lineAtt);

    markerAtt = dynamic_cast<TAttMarker*>(DrawObject.Pointer);
    if (markerAtt) CopyMarkerAtt = new TAttMarker(*markerAtt);

    fillAtt = dynamic_cast<TAttFill*>(DrawObject.Pointer);
    if (fillAtt) CopyFillAtt = new TAttFill(*fillAtt);

    updateGui();

    if (MarkerColor == LineColor) ui->cbMarkerColorAsLine->setChecked(true);
    if (FillColor   == LineColor) ui->cbFillColorAsLine->setChecked(true);
}

ALineMarkerFillDialog::~ALineMarkerFillDialog()
{
    delete CopyMarkerAtt;
    delete CopyLineAtt;
    delete CopyFillAtt;

    delete ui;
}

void ALineMarkerFillDialog::onReject()
{
    if (CopyLineAtt)   *lineAtt   = *CopyLineAtt;
    if (CopyMarkerAtt) *markerAtt = *CopyMarkerAtt;
    if (CopyFillAtt)   *fillAtt   = *CopyFillAtt;
}

void ALineMarkerFillDialog::updateGui()
{
    ui->leOptions->setText(DrawObject.Options);
    updateLineGui();
    updateMarkerGui();
    updateFillGui();
}

void ALineMarkerFillDialog::on_pbLineColor_clicked()
{
    if (!lineAtt) return;

    ARootColorSelectorDialog D(LineColor, this);
    D.exec();

    if (ui->cbMarkerColorAsLine->isChecked()) MarkerColor = LineColor;
    if (ui->cbFillColorAsLine->isChecked())   FillColor   = LineColor;

    on_pbUpdateObject_clicked();
    updateGui();
}

void ALineMarkerFillDialog::on_pbMarkerColor_clicked()
{
    if (!markerAtt) return;

    ARootColorSelectorDialog D(MarkerColor, this);
    D.exec();

    if (MarkerColor != LineColor) ui->cbMarkerColorAsLine->setChecked(false);

    on_pbUpdateObject_clicked();
    updateGui();
}

void ALineMarkerFillDialog::on_pbFillColor_clicked()
{
    if (!fillAtt) return;

    ARootColorSelectorDialog D(FillColor, this);
    D.exec();

    if (FillColor != LineColor) ui->cbFillColorAsLine->setChecked(false);

    on_pbUpdateObject_clicked();
    updateGui();
}

void ALineMarkerFillDialog::on_pbClose_clicked()
{
    updateObject();
    accept();
}

void ALineMarkerFillDialog::updateLineGui()
{
    ui->frLine->setEnabled(lineAtt);
    if (!lineAtt) return;

    ui->cobLineStyle->setCurrentIndex(lineAtt->GetLineStyle()-1);

    ui->sbLineWidth->setValue(lineAtt->GetLineWidth());

    LineColor = lineAtt->GetLineColor();
    previewColor(LineColor, ui->frLineCol);
}

void ALineMarkerFillDialog::updateMarkerGui()
{
    ui->frMarker->setEnabled(markerAtt);
    if (!markerAtt) return;

    int Style = markerAtt->GetMarkerStyle();
    QVector<int> map;
    for (int i=1; i<9; i++) map << i;
    for (int i=9; i<20; i++) map << 8;
    for (int i=20; i<31; i++) map << 9 + i -20;
    map << 3;
    for (int i=32; i<35; i++) map << 20 + i - 32;
    int iStyle = 1;
    if (Style > 0 && Style <= map.size())
        iStyle = map[Style-1];
    ui->cobMarkerStyle->setCurrentIndex(iStyle - 1);

    ui->ledMarkerSize->setText( QString::number(markerAtt->GetMarkerSize()) );

    MarkerColor = markerAtt->GetMarkerColor();
    previewColor(MarkerColor, ui->frMarkerCol);
}

void ALineMarkerFillDialog::updateFillGui()
{
    ui->frFill->setEnabled(fillAtt);
    if (!fillAtt) return;

    int Style = fillAtt->GetFillStyle();
    int iCobIndex = 0;
    if (Style == 1001) iCobIndex = 1;
    else if (Style > 3000 && Style < 4000)
    {
        iCobIndex = 2;
        ui->sbFillPattern->setValue(Style);
    }
    ui->cobFillStyle->setCurrentIndex(iCobIndex);

    FillColor = fillAtt->GetFillColor();
    previewColor(FillColor, ui->frFillCol);
}

void ALineMarkerFillDialog::previewColor(int & color, QFrame * frame)
{
    TColor *tc = gROOT->GetColor(color);

    int red = 255;
    int green = 255;
    int blue = 255;

    if (tc)
    {
        red = 255*tc->GetRed();
        green = 255*tc->GetGreen();
        blue = 255*tc->GetBlue();
    }
    frame->setStyleSheet( QString("background-color:rgb(%1,%2,%3)").arg(red).arg(green).arg(blue) );
}

void ALineMarkerFillDialog::updateObject()
{
    QString opt = ui->leOptions->text();
    if (!bFirstObject && !opt.contains("same", Qt::CaseInsensitive))
    {
        opt += "same";
        ui->leOptions->setText(opt);
    }
    DrawObject.Options = opt;

    if (lineAtt)
    {
        lineAtt->SetLineWidth(ui->sbLineWidth->value());
        lineAtt->SetLineStyle(ui->cobLineStyle->currentIndex()+1);
        lineAtt->SetLineColor(LineColor);
    }

    if (markerAtt)
    {
        markerAtt->SetMarkerSize(ui->ledMarkerSize->text().toFloat());
        int Style = 1;
        QString st = ui->cobMarkerStyle->currentText();
        QStringList l = st.split(" - ");
        if (l.size() > 0)
        {
            QString num = l.first();
            Style = num.toInt();
        }
        markerAtt->SetMarkerStyle(Style);
        markerAtt->SetMarkerColor(MarkerColor);
    }

    if (fillAtt)
    {
        const int iCobIndex = ui->cobFillStyle->currentIndex();
        int Style = 0;
        if (iCobIndex == 1) Style = 1001;
        else if (iCobIndex == 2) Style = ui->sbFillPattern->value();
        fillAtt->SetFillStyle(Style);
        fillAtt->SetFillColor(FillColor);
    }
}

void ALineMarkerFillDialog::on_pbUpdateObject_clicked()
{
    updateObject();
}

void ALineMarkerFillDialog::on_cobFillStyle_currentIndexChanged(int index)
{
    ui->frFillPattern->setVisible(index == 2);
}


void ALineMarkerFillDialog::on_cbMarkerColorAsLine_clicked(bool checked)
{
    if (checked)
    {
        MarkerColor = LineColor;
        updateObject();
        updateGui();
    }
}

void ALineMarkerFillDialog::on_cbFillColorAsLine_clicked(bool checked)
{
    if (checked)
    {
        FillColor = LineColor;
        updateObject();
        updateGui();
    }
}

void ALineMarkerFillDialog::on_pbPreview_clicked()
{
    updateObject();
    emit requestRedraw();
}

#include <QDesktopServices>
#include <QUrl>
void ALineMarkerFillDialog::on_pbOptionsHelp_clicked()
{
    QDesktopServices::openUrl(QUrl("https://root.cern/doc/master/classTHistPainter.html"));
}
