#include "arootcolorselectordialog.h"
#include "ui_arootcolorselectordialog.h"
#include "amessage.h"

#include <QPainter>
#include <QPaintEvent>

#include "TROOT.h"
#include "TColor.h"

ARootColorSelectorDialog::ARootColorSelectorDialog(int &color, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ARootColorSelectorDialog),
    Color(color)
{
    ui->setupUi(this);

    ui->pbDummy->setDefault(true);
    ui->pbDummy->setVisible(false);

    BaseColors << 880 << 900 << 800 << 820 << 840 << 860 << 9;
    ui->frColorPanel->setFixedSize(SquareSize * 20, SquareSize * BaseColors.size());
    ui->frCol->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    ui->frCol->setFixedSize(30, 30);

    resize( ui->frColorPanel->width() + 2, 1 + ui->frColorPanel->height() + 1 + ui->pbClose->height() + ui->frCol->height() + 3 );
    setFixedWidth(width());

    validateColor();
    ColorOnStart = Color;
    ui->sbColor->setValue(color);
    showColor();
}

ARootColorSelectorDialog::~ARootColorSelectorDialog()
{
    delete ui;
}

void ARootColorSelectorDialog::paintEvent(QPaintEvent *)
{
    QPainter p;
    p.begin(this);

    p.setPen(Qt::NoPen);

    for (int i=0; i<BaseColors.size(); i++)
        PaintColorRow(&p, i, BaseColors.at(i));

    p.end();
}

void ARootColorSelectorDialog::mousePressEvent(QMouseEvent *e)
{
    int row = e->y() / SquareSize;
    int num = e->x() / SquareSize;

    if (row >= BaseColors.size()) return;

    Color = -9 + num + BaseColors.at(row);

    bool bOK = validateColor();

    if (bOK) accept();
    else showColor();
}

bool ARootColorSelectorDialog::validateColor()
{
    TColor *tc = gROOT->GetColor(Color);
    if (!tc)
    {
        message("Not a valid color index!", this);
        Color = ColorOnStart;
    }

    ui->sbColor->setValue(Color);
    return tc;
}

void ARootColorSelectorDialog::showColor()
{
    TColor *tc = gROOT->GetColor(Color);

    int red = 255;
    int green = 255;
    int blue = 255;

    if (tc)
    {
        red = 255*tc->GetRed();
        green = 255*tc->GetGreen();
        blue = 255*tc->GetBlue();
    }
    ui->frCol->setStyleSheet(  QString("background-color:rgb(%1,%2,%3)").arg(red).arg(green).arg(blue)  );
}

void ARootColorSelectorDialog::PaintColorRow(QPainter *p, int row, int colorBase)
{
    for (int i=0; i<20; i++)
    {
        int c = -9 +i +colorBase;
        TColor *tc = gROOT->GetColor(c);
        int red = 255*tc->GetRed();
        int green = 255*tc->GetGreen();
        int blue = 255*tc->GetBlue();
        p->setBrush(QBrush(QColor( red, green, blue )));
        p->drawRect( i*SquareSize, row*SquareSize, SquareSize,SquareSize);
    }
}

void ARootColorSelectorDialog::on_pbClose_clicked()
{
    accept();
}

void ARootColorSelectorDialog::on_sbColor_editingFinished()
{
    Color = ui->sbColor->value();
    validateColor();
    showColor();
}
