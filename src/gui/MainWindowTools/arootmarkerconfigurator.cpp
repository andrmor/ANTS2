#include "arootmarkerconfigurator.h"

#include <QPainter>
#include <QMouseEvent>
#include <QDebug>
#include <QHBoxLayout>
#include <QSpinBox>
#include <QLabel>
#include <QComboBox>
#include <QFrame>
#include <QPushButton>

#include "TROOT.h"
#include "TColor.h"

ARootMarkerConfigurator::ARootMarkerConfigurator(int* Color, int* Size, int* Style, QWidget *parent)
  : QDialog(parent)
{
  ReturnColor = Color;
  ReturnStyle = Style;
  ReturnWidth = Size;
  setMouseTracking(true);
  this->setWindowTitle("Select ROOT line properties");

  SquareSize = 30;

  //  kWhite =0, kBlack =1, kGray =920, kRed =632,
  //  kGreen =416, kBlue =600, kYellow =400, kMagenta =616,
  //  kCyan =432, kOrange =800, kSpring =820, kTeal =840,
  //  kAzure =860, kViolet =880, kPink =900
  BaseColors << 880 << 900 << 800 << 820 << 840 << 860 << 9;
  //AlternativeBaseColors << 600 << 616 << 632 << 400 << 416 << 432;

  QFrame* frMain = new QFrame(this);
  frMain->setFrameShape(QFrame::Box);
  frMain->setGeometry(3, BaseColors.size()*SquareSize + 3, SquareSize*20-6, 50);
  QHBoxLayout* lay = new QHBoxLayout();
  lay->setContentsMargins(50,0,50,0);
    QLabel* labW = new QLabel();
    labW->setText("Size:");
    labW->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    lay->addWidget(labW);

    sbSize = new QSpinBox();
    sbSize->setMinimum(0);
    sbSize->setValue(*Size);
    lay->addWidget(sbSize);

    QLabel* labS = new QLabel();
    labS->setText("Style:");
    labS->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    lay->addWidget(labS);

    comStyle = new QComboBox();
    comStyle->addItem("1 - small dot");
    comStyle->addItem("2 - cross");
    comStyle->addItem("3 - asterisk");
    comStyle->addItem("4 - circle");
    comStyle->addItem("5 - diagonal cross");
    comStyle->addItem("6 - rhomb dot");
    comStyle->addItem("7 - square dot");
    comStyle->addItem("8 - large round dot");
    comStyle->addItem("20 - filled circle");
    comStyle->addItem("21 - filled square");
    comStyle->addItem("22 - filled triangle");
    comStyle->addItem("23 - rotated filled triangle");
    comStyle->addItem("24 - circle");
    comStyle->addItem("25 - square");
    comStyle->addItem("26 - triangle");
    comStyle->addItem("27 - romb");
    comStyle->addItem("28 - big cross");
    comStyle->addItem("29 - filled star");
    comStyle->addItem("30 - star");
    comStyle->addItem("32 - inverted triangle");
    comStyle->addItem("33 - filled romb");
    comStyle->addItem("34 - filled cross");
    comStyle->setMinimumHeight(20);
    comStyle->setMinimumWidth(150);

    QVector<int> map;
    for (int i=1; i<9; i++) map << i;
    for (int i=9; i<20; i++) map << 8;
    for (int i=20; i<31; i++) map << 9 + i -20;
    map << 3;
    for (int i=32; i<35; i++) map << 20 + i - 32;

      //for (int i=0; i<map.size(); i++) qDebug() << i+1 << map[i];

    int starter = 1;
    if (*Style>0 && *Style<map.size()+1)
        starter = map[*Style-1];
      //qDebug() << *Style << starter;
    comStyle->setCurrentIndex(starter-1);
    lay->addWidget(comStyle);

    QLabel* labC = new QLabel();
    labC->setText("Color:");
    labC->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    lay->addWidget(labC);

    sbColor = new QSpinBox();
    sbColor->setMaximum(999);
    sbColor->setValue(*Color);
    lay->addWidget(sbColor);
  frMain->setLayout(lay);

  QPushButton* pbDone = new QPushButton(this);
  pbDone->setText("Accept new values");
  pbDone->setGeometry(20*SquareSize/2-75, frMain->y()+frMain->height()+5, 150, 30);
  connect(pbDone, SIGNAL(pressed()), this, SLOT(onAccept()));

  resize( SquareSize*20, pbDone->y()+pbDone->height()+5);
}

void ARootMarkerConfigurator::paintEvent(QPaintEvent *)
{
  QPainter p;
  p.begin(this);

  p.setPen(Qt::NoPen);

  for (int i=0; i<BaseColors.size(); i++)
    PaintColorRow(&p, i, BaseColors.at(i));

  p.end();
}

void ARootMarkerConfigurator::PaintColorRow(QPainter *p, int row, int colorBase)
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

void ARootMarkerConfigurator::onAccept()
{
  *ReturnWidth = sbSize->value();

  int style = 1;
  QString st = comStyle->currentText();
  QStringList l = st.split(" - ");
  if (l.size()>0)
  {
      QString num = l.first();
      style = num.toInt();
  }
  *ReturnStyle = style;
    //qDebug() << style;

  *ReturnColor = sbColor->value();
  done(1);
}

void ARootMarkerConfigurator::mousePressEvent(QMouseEvent *e)
{
  //qDebug() << "Selected at:"<<e->x() << e->y();

  int row = e->y() / SquareSize;
  int num = e->x() / SquareSize;

  if (row > BaseColors.size()-1) return;

  int color = -9 + num + BaseColors.at(row);
  sbColor->setValue(color);
  //qDebug() << color << "at row/num:"<<row<<num;
  sbColor->setValue(color);
}
