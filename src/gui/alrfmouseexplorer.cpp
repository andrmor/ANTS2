#include "alrfmouseexplorer.h"
#include "viewer2darrayobject.h"
#include "guiutils.h"
#include "sensorlrfs.h"
#include "apmhub.h"
#include "apmgroupsmanager.h"

#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPointF>
#include <QDebug>

ALrfMouseExplorer::ALrfMouseExplorer(SensorLRFs *LRFs, APmHub* PMs, APmGroupsManager* PMgroups, double SuggestedZ, QWidget* parent) :
  QDialog(parent), LRFs(LRFs), PMs(PMs), PMgroups(PMgroups)
{  
  setModal(true);
  setWindowTitle("LRF viewer");

  QVBoxLayout *mainLayout = new QVBoxLayout;
    //tools
    QHBoxLayout *hbox = new QHBoxLayout;
      QLabel *l1 = new QLabel("PM selection:");
      hbox->addWidget(l1);

      cobSG = new QComboBox();
      for (int igr=0; igr<PMgroups->countPMgroups(); igr++)
        cobSG->addItem(PMgroups->getGroupName(igr));
      cobSG->addItem("All PMs");
      cobSG->setCurrentIndex(cobSG->count()-1);
      connect(cobSG, SIGNAL(activated(int)), this, SLOT(onCobActivated(int)));
      hbox->addWidget(cobSG);

      QLabel *l2 = new QLabel("Z:");
      hbox->addWidget(l2);

      ledZ = new QLineEdit(QString::number(SuggestedZ));
      QDoubleValidator* dv = new QDoubleValidator(this);
      dv->setNotation(QDoubleValidator::ScientificNotation);
      ledZ->setValidator(dv);
      hbox->addWidget(ledZ);
    mainLayout->addLayout(hbox);

    //graphics
    gv = new myQGraphicsView(this);
      connect(gv, SIGNAL(MouseMovedSignal(QPointF*)), this, SLOT(paintLRFonDialog(QPointF*)));
    mainLayout->addWidget(gv);

    //close button
    QPushButton *okButton = new QPushButton("Close");
      connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
    mainLayout->addWidget(okButton);
  setLayout(mainLayout);

  LRFviewObj = new Viewer2DarrayObject(gv, PMs);
  LRFviewObj->SetCursorMode(1);

  okButton->setAutoDefault(false);
}

ALrfMouseExplorer::~ALrfMouseExplorer()
{
  delete gv;
  delete LRFviewObj;
}

void ALrfMouseExplorer::Start()
{
    resize(800,800);
    show();
    gv->show();
    LRFviewObj->DrawAll();
    LRFviewObj->ResetViewport();

    QPointF tmpp(0,0);
    paintLRFonDialog(&tmpp);

    exec();
}

void ALrfMouseExplorer::paintLRFonDialog(QPointF *pos)
{
  double r[3];
  r[0] = pos->x();
  r[1] = -pos->y(); //inverted!
  r[2] = ledZ->text().toDouble();
  //qDebug() << r[0] << r[1] << r[2];
  QString title = QString::number(r[0], 'f', 1) +" , "+ QString::number(r[1], 'f', 1) +" , "+ QString::number(r[2], 'f', 1);
  setWindowTitle(title);

  int numPMs = PMs->count();
  QVector<double> lrfs;
  lrfs.resize(numPMs);

  int iGroup = cobSG->currentIndex();
  bool fAll = ( iGroup==cobSG->count()-1 );

  double max = -100;
  for (int ipm=0; ipm<numPMs; ipm++)
    if (fAll || PMgroups->isPmBelongsToGroupFast(ipm, iGroup))
       {
         lrfs[ipm] = LRFs->getLRF(ipm, r);
         if (lrfs[ipm]>max) max = lrfs[ipm];
       }

  for (int ipm=0; ipm<numPMs; ipm++)
    {
      if (fAll || PMgroups->isPmBelongsToGroupFast(ipm, iGroup))
         {
          LRFviewObj->SetVisible(ipm, true);

          int g = 255 - lrfs[ipm]/max*254.0;
          LRFviewObj->SetText(ipm, QString::number(lrfs[ipm], 'f', 1));

          if (lrfs[ipm] <= 0)  /// to update to isInRange() check
            {
              LRFviewObj->SetBrushColor(ipm, Qt::red);
              LRFviewObj->SetTextColor(ipm, Qt::black);
            }
          else
            {
              LRFviewObj->SetBrushColor(ipm, QColor(g,g,g));
              if (g>128) LRFviewObj->SetTextColor(ipm, Qt::black);
              else LRFviewObj->SetTextColor(ipm, Qt::white);
            }
         }
      else LRFviewObj->SetVisible(ipm, false);
    }
  LRFviewObj->DrawAll();
}

void ALrfMouseExplorer::onCobActivated(int)
{
  QPointF tmpp(0,0);
  paintLRFonDialog(&tmpp);
}

