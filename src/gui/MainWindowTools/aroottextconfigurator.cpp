#include "aroottextconfigurator.h"
#include "ui_aroottextconfigurator.h"
#include "amessage.h"

#include <QDebug>
#include <QPushButton>
#include <QPainter>
#include <QMouseEvent>
#include <QDoubleValidator>

#include "TAttText.h"
#include "TColor.h"
#include "TROOT.h"

ARootTextConfigurator::ARootTextConfigurator(int & color, int & align, int & font, float &size, QWidget * parent) :
    QDialog(parent),
    ui(new Ui::ARootTextConfigurator),
    color(color), align(align), font(font), size(size)
{
    ui->setupUi(this);

    setMouseTracking(true);
    setWindowTitle("ROOT text properties");

    QDoubleValidator* dv = new QDoubleValidator(this);
    dv->setNotation(QDoubleValidator::ScientificNotation);
    QList<QLineEdit*> list = this->findChildren<QLineEdit *>();
    foreach(QLineEdit *w, list) if (w->objectName().startsWith("led")) w->setValidator(dv);

    QPushButton* dummy = new QPushButton(this); //intercepts eneter key hits
    dummy->setDefault(true);
    dummy->setVisible(false);

    BaseColors << 880 << 900 << 800 << 820 << 840 << 860 << 9;
    ui->frColorPanel->setFixedSize(SquareSize * 20, SquareSize * BaseColors.size());
    ui->frCol->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    ui->frCol->setFixedSize(30, 30);

    setFixedWidth(width());

    ui->sbColor->setValue(color);
    setupAlignmentControls();
    int fontIndex = font / 10;
    int precision = font % 10;
    ui->sbFont->setValue(fontIndex);
    ui->sbPrecision->setValue(precision);
    ui->ledSize->setText( QString::number(size) );

    updateColorFrame();
}

ARootTextConfigurator::~ARootTextConfigurator()
{
    delete ui;
}

void ARootTextConfigurator::paintEvent(QPaintEvent *)
{
    QPainter p;
    p.begin(this);

    p.setPen(Qt::NoPen);

    for (int i=0; i<BaseColors.size(); i++)
      PaintColorRow(&p, i, BaseColors.at(i));

    p.end();
}

void ARootTextConfigurator::mousePressEvent(QMouseEvent *e)
{
    //qDebug() << "Selected at:"<<e->x() << e->y();

    int row = e->y() / SquareSize;
    int num = e->x() / SquareSize;

    if (row > BaseColors.size()-1) return;

    int color = -9 + num + BaseColors.at(row);
    ui->sbColor->setValue(color);
    //qDebug() << color << "at row/num:"<<row<<num;
    ui->sbColor->setValue(color);
    updateColorFrame();
}

void ARootTextConfigurator::setupAlignmentControls()
{
    int vert = align % 10;
    int hor  = align / 10.0;
    qDebug() << align << hor << vert;
    ui->cobHorizontalAlignment->setCurrentIndex(hor-1);
    ui->cobVerticalAlignment->setCurrentIndex(vert-1);
}

void ARootTextConfigurator::readAlignment()
{
    align = 10 * (1 + ui->cobHorizontalAlignment->currentIndex()) + (1 + ui->cobVerticalAlignment->currentIndex());
}

void ARootTextConfigurator::PaintColorRow(QPainter *p, int row, int colorBase)
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

void ARootTextConfigurator::updateColorFrame()
{
    TColor *tc = gROOT->GetColor(ui->sbColor->value());

    previewColor();

    if (!tc)
    {
        message("Not a valid color index!", this);
        ui->sbColor->setValue(1);
    }
}

void ARootTextConfigurator::previewColor()
{
    TColor *tc = gROOT->GetColor(ui->sbColor->value());

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

void ARootTextConfigurator::on_pbAccept_clicked()
{
    color = ui->sbColor->value();
    font = ui->sbFont->value() * 10 + ui->sbPrecision->value();
    size = ui->ledSize->text().toFloat();
    readAlignment();
    accept();
}

void ARootTextConfigurator::on_pbCancel_clicked()
{
    reject();
}

ARootAxisTitleTextConfigurator::ARootAxisTitleTextConfigurator(int &color, int &align, int &font, float &size, QWidget *parent) :
    ARootTextConfigurator(color, align, font, size, parent)
{
    setWindowTitle("Axis title text properties");
    setupAlignmentControls();
}

void ARootAxisTitleTextConfigurator::setupAlignmentControls()
{
    ui->cobVerticalAlignment->setVisible(false);
    ui->cobHorizontalAlignment->clear();
    ui->cobHorizontalAlignment->addItems({"Right", "Center"});

    ui->cobHorizontalAlignment->setCurrentIndex(align == 0);
}

void ARootAxisTitleTextConfigurator::readAlignment()
{
    align = ui->cobHorizontalAlignment->currentIndex();
}

ARootAxisLabelTextConfigurator::ARootAxisLabelTextConfigurator(int &color, int &align, int &font, float &size, QWidget *parent) :
    ARootTextConfigurator(color, align, font, size, parent)
{
    setWindowTitle("Axis title text properties");
    setupAlignmentControls();
}

void ARootAxisLabelTextConfigurator::setupAlignmentControls()
{
    ui->cobVerticalAlignment->setVisible(false);
    ui->cobHorizontalAlignment->setVisible(false);
    ui->labAlign->setVisible(false);
    ui->lineAlign->setVisible(false);
}

void ARootAxisLabelTextConfigurator::readAlignment()
{
    // nothing to do
}
