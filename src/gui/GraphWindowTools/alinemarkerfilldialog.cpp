#include "alinemarkerfilldialog.h"
#include "ui_alinemarkerfilldialog.h"
#include "arootcolorselectordialog.h"

#include <QDebug>

#include "TROOT.h"
#include "TColor.h"
#include "TObject.h"
#include "TAttLine.h"
#include "TAttMarker.h"

#include "arootmarkerconfigurator.h"

ALineMarkerFillDialog::ALineMarkerFillDialog(TObject * tobj, QWidget * parent) :
    QDialog(parent),
    ui(new Ui::ALineMarkerFillDialog),
    tobj(tobj)
{
    ui->setupUi(this);

    ui->pbUpdateObject->setDefault(true);
    ui->pbUpdateObject->setVisible(false);

    QStringList Lstyles = {"Straight","Short_dash","Dot","Short_dash dot","Dash dot","Dash 3_dots","Dash","Dash 2_dots","Long_dash","Long_dash dot"};
    ui->cobLineStyle->addItems(Lstyles);

    QStringList Mstyles = {"1 - small dot","2 - cross","3 - asterisk","4 - circle","5 - diagonal cross","6 - rhomb dot","7 - square dot","8 - large round dot",
              "20 - filled circle","21 - filled square","22 - filled triangle","23 - rotated filled triangle","24 - circle","25 - square",
              "26 - triangle","27 - romb","28 - big cross","29 - filled star","30 - star","32 - inverted triangle","33 - filled romb","34 - filled cross"};
    ui->cobMarkerStyle->addItems(Mstyles);

    lineAtt = dynamic_cast<TAttLine*>(tobj);
    ui->frLine->setEnabled(lineAtt);
    if (lineAtt)
    {
        updateLineGui();
        CopyLineAtt = new TAttLine(*lineAtt);
    }

    markerAtt = dynamic_cast<TAttMarker*>(tobj);
    ui->frMarker->setEnabled(markerAtt);
    if (markerAtt)
    {
        updateMarkerGui();
        CopyMarkerAtt = new TAttMarker(*markerAtt);
    }

    connect(this, &ALineMarkerFillDialog::rejected, this, &ALineMarkerFillDialog::onReject);
}

ALineMarkerFillDialog::~ALineMarkerFillDialog()
{
    delete CopyMarkerAtt;
    delete CopyLineAtt;

    delete ui;
}

void ALineMarkerFillDialog::onReject()
{
    if (CopyLineAtt)   *lineAtt   = *CopyLineAtt;
    if (CopyMarkerAtt) *markerAtt = *CopyMarkerAtt;
}

void ALineMarkerFillDialog::on_pbLineColor_clicked()
{
    if (!lineAtt) return;

    ARootColorSelectorDialog D(LineColor, this);
    D.exec();

    on_pbUpdateObject_clicked();
    updateLineGui();
}

void ALineMarkerFillDialog::on_pbMarkerColor_clicked()
{
    if (!markerAtt) return;

    ARootColorSelectorDialog D(MarkerColor, this);
    D.exec();

    on_pbUpdateObject_clicked();
    updateMarkerGui();
}

void ALineMarkerFillDialog::on_pbClose_clicked()
{
    updateObject();
    accept();
}

void ALineMarkerFillDialog::updateLineGui()
{
    if (!lineAtt) return;

    ui->cobLineStyle->setCurrentIndex(lineAtt->GetLineStyle()-1);
    ui->sbLineWidth->setValue(lineAtt->GetLineWidth());
    LineColor = lineAtt->GetLineColor();

    previewColor(LineColor, ui->frLineCol);
}

void ALineMarkerFillDialog::updateMarkerGui()
{
    if (!lineAtt) return;

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
}

void ALineMarkerFillDialog::on_pbUpdateObject_clicked()
{
    updateObject();
}
