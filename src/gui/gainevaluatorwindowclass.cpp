#include "gainevaluatorwindowclass.h"
#include "ui_gainevaluatorwindowclass.h"
#include "mainwindow.h"
#include "reconstructionwindow.h"
#include "pms.h"
#include "pmtypeclass.h"
#include "windownavigatorclass.h"
#include "guiutils.h"
#include "outputwindow.h"
#include "graphwindowclass.h"
#include "eventsdataclass.h"
#include "ageomarkerclass.h"
#include "apositionenergyrecords.h"
#include "amessage.h"
#include "acommonfunctions.h"
#include "detectorclass.h"
#include "apmgroupsmanager.h"
#include "geometrywindowclass.h"

//Qt
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QLineF>
#include <QDebug>
#include <QTimer>

//Root
#include "TVectorD.h"
#include "TMatrixD.h"
#include "TDecompSVD.h"
#include "TMath.h"
#include "TH1D.h"

//=== external vars/functions using by sorting
static int CurrentIPM; //used in Center algorithm sorting
static QVector< QVector< float> >* Events;
static QVector<AReconRecord*>* RecData;
static bool DoFiltering;
bool CentersLessThan(const int &first, const int &second)
{
   double sig1 = Events->at(first)[CurrentIPM];
   double sig2 = Events->at(second)[CurrentIPM];
   if (DoFiltering)
     {
       if (!RecData->at(first)->GoodEvent) sig1 = 1e-10;
       if (!RecData->at(second)->GoodEvent) sig2 = 1e-10;
     }
   return sig1>sig2; //inverted lessthan - list will be sorted in decreasing signal order
}
//====

GainEvaluatorWindowClass::GainEvaluatorWindowClass(QWidget *parent, MainWindow *mw, EventsDataClass *eventsDataHub) :
  QMainWindow(parent),
  ui(new Ui::GainEvaluatorWindowClass)
{  
  MW = mw;
  EventsDataHub = eventsDataHub;
  ui->setupUi(this);

  ui->fCutOffsSubSet->setEnabled(false);
  ui->fCentersSubSet->setEnabled(false);
  ui->f4->setEnabled(false);
  ui->fLogR->setEnabled(false);

  ui->pbUpdateCutOff->setVisible(false);
  ui->pbUpdateCenters->setVisible(false);
  ui->pbUpdate4->setVisible(false);
  ui->pbUpdateGraphics->setVisible(false);
  ui->pbUpdateUI->setVisible(false);
  ui->fCutOffsOption->setVisible(false);

  ui->fUniformtest->setVisible(false);
  ui->fLogPairTester->setVisible(false);

  move(50,50);

  Equations = 0;
  Variables = 0;
  tmpHistForLogR = 0;
  TMPignore = false; 
  flagShowNeighbourGroups = false;
  flagShowNonLinkedSet = false;
  flagAllPMsCovered = false;
  flagShowingLinked = false;
  CenterTopFraction = 0.01 * ui->ledCenterTopFraction->text().toDouble();
  CutOffFraction = 0.01 * ui->ledCutOffFraction->text().toDouble();
  CenterGroups.clear();
  CenterGroups << CenterGroupClass(30.0, 0.5);

  ui->pbUpdateLogR->setVisible(false);
  ui->pbUpdateUniform->setVisible(false);

  //validators
  QDoubleValidator* dv = new QDoubleValidator(this);
  dv->setNotation(QDoubleValidator::ScientificNotation);
  QList<QLineEdit*> list = this->findChildren<QLineEdit *>();
  foreach(QLineEdit *w, list) if (w->objectName().startsWith("led")) w->setValidator(dv);

  iSets4PMs.clear();
  ui->tabwid4sets->resizeColumnsToContents();

  //icons
  RedIcon = createColorCircleIcon(ui->twAlgorithms->iconSize(), Qt::red);
  GreenIcon = createColorCircleIcon(ui->twAlgorithms->iconSize(), Qt::green);
  ui->labIconWarningCutOffs->setPixmap(RedIcon.pixmap(16,16));

  //Graphics view
  GVscale = 10.0;
  scene = new QGraphicsScene(this);
  connect(scene, SIGNAL(selectionChanged()), this, SLOT(sceneSelectionChanged()));
  gvGains = new myQGraphicsView(this);
  int gvY = ui->fGV->y() + ui->fGV->height() + 3;
  gvGains->setGeometry(ui->fGV->x(), gvY, 256, 192);
  GainEvaluatorWindowClass::resizeEvent(0);
  gvGains->setScene(scene);
  gvGains->setDragMode(QGraphicsView::ScrollHandDrag); //if zoomed, can use hand to center needed area
  //gvOut->setTransformationAnchor(QGraphicsView::AnchorViewCenter);
  gvGains->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
  gvGains->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  gvGains->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  gvGains->setRenderHints(QPainter::Antialiasing);

  GainEvaluatorWindowClass::ResetViewport();
  GainEvaluatorWindowClass::UpdateEvaluator();
  ui->pbEvaluateGains->setFocus();

  GainEvaluatorWindowClass::onCurrentSensorGroupsChanged();
}

void GainEvaluatorWindowClass::onCurrentSensorGroupsChanged()
{
     //qDebug() << "Current sensor group changed";
   CurrentGroup = MW->Detector->PMgroups->getCurrentGroup();
   ui->fGroupName->setText(MW->Detector->PMgroups->getGroupName(CurrentGroup));

   GainEvaluatorWindowClass::UpdateEvaluator();
   GainEvaluatorWindowClass::UpdateGraphics();
}

GainEvaluatorWindowClass::~GainEvaluatorWindowClass()
{
    //this->hide();
    //qDebug() << "  Starting destructor of gain eval window";
    //qDebug() << scene->hasFocus();
    scene->clearSelection();
    scene->clear();
    //qDebug() << "  Scene cleared";
    //qDebug() << gvGains->baseSize();
    gvGains->clearFocus();
//    for (int i=0; i<PMicons.size(); i++)
//    {
//        qDebug() << PMicons.at(i);
//        scene->removeItem(PMicons[i]);
//        delete PMicons[i];
//        qDebug() << "   deleted!";
//    }
//    PMicons.clear();

//    gvGains->setScene(0);
//    delete scene;
//    scene = 0;
//  qDebug() << "Here-----";

  delete gvGains;

  delete ui;

  if (tmpHistForLogR) delete tmpHistForLogR;
  //qDebug() << "  Gain win destructor finished!";
}

void GainEvaluatorWindowClass::Reset()
{  
  iPMs.clear();
  iCutOffPMs.clear();
  iCentersPMs.clear();
  iSets4PMs.clear();

  ui->cbActivateCutOffs->setChecked(false);
  ui->cbCentersActivate->setChecked(false);
  ui->cb4Activate->setChecked(false);

  UpdateGraphics();
  ResetViewport();
}


void GainEvaluatorWindowClass::on_pbUpdateUI_clicked()
{
   GainEvaluatorWindowClass::UpdateEvaluator();
}

void GainEvaluatorWindowClass::UpdateEvaluator()
{  
    //qDebug() << "Updating evaluator...";

    //PMs to use - Variables
    iPMs.clear();   
    for (int i=0; i<MW->PMs->count(); i++)
       if (!MW->Detector->PMgroups->isStaticPassive(i))
       //if (MW->PMs->at(i).group == igroup)
       if (MW->Detector->PMgroups->isPmBelongsToGroup(i, CurrentGroup))
           iPMs.append(i);
    Variables = iPMs.size();
    //all methods - Equations + collecting statistics on PMs and store in dedicated sets
    Equations = 1; //the only one equation with nonzero "y" value of SVD

    //cut-offs   
    if (ui->cbActivateCutOffs->isChecked())
      {
        int pmsSelected = iCutOffPMs.size();
        Equations += 0.5 * pmsSelected * (pmsSelected -1 ); //each selected PM is linked to all other selected PMs       
      }

    //centers    
    SetNeighbour.resize(0);
    if (ui->cbCentersActivate->isChecked())
      {
        QList<int> Neighbours;
        int numGroups = CenterGroups.size();
        SetNeighbour.resize(numGroups);
        for (int igroup = 0; igroup < numGroups; igroup++) SetNeighbour[igroup].clear();

        for (int igroup = 0; igroup < numGroups; igroup++)
          {
            for (int ipmIndex=0; ipmIndex<iCentersPMs.size(); ipmIndex++)
              {
                int ipm = iCentersPMs[ipmIndex];
                GainEvaluatorWindowClass::findNeighbours(ipm, igroup, &Neighbours);
                int iNei = Neighbours.size();  //number of neighbours around this central PM
                Equations += 0.5*iNei*(iNei-1); //number of links between these neighbours

                //convering Neighbours to normal pm numbers
                for (int i=0; i<iNei; i++) Neighbours[i] = iPMs[ Neighbours[i] ];

                SetNeighbour[igroup] += QSet<int>::fromList(Neighbours);
              }
          }        
      }


    //Quartets
    if (ui->cb4Activate->isChecked())
     for (int i=0; i<iSets4PMs.size(); i++)
      {
        if (iSets4PMs[i].Symmetric) Equations += 6;
        else Equations += 2;
      }

    //FromUniform
    if (ui->cbFromUniform->isChecked())
      {        
        //cycle by first PM
        for (int i1 = 0; i1<iPMs.count(); i1++)
            {
              int ipm1 = iPMs[i1];
              //cycle by second PM
              for (int i2=i1+1; i2<iPMs.count(); i2++)
                  {
                    int ipm2 = iPMs[i2];                    
                    if (isPMDistanceCheckFail(ipm1, ipm2)) continue;
                    else Equations++;
                  }
            }
      }

    //LogR
    if (ui->cbActivateLogR->isChecked())
      {
        if (ui->cobLogMethodSelection->currentIndex() == 0)
          {
            //Pairs
              //cycle by first PM
            for (int i1 = 0; i1<iLogPMs.size(); i1++)
                {
                  int ipm1 = iLogPMs[i1];
                  //cycle by second PM
                  for (int i2=i1+1; i2<iLogPMs.size(); i2++)
                      {
                        int ipm2 = iLogPMs[i2];
                        if (isPMDistanceCheckFailLogR(ipm1, ipm2)) continue;
                        else Equations++;
                      }
                }
          }
        else
          {
            //Triads
            int numTriads = static_cast<int>(Triads.getTriads()->size());
            Equations += 2.0*numTriads;
          }
      }

    //building indexation data
    GainEvaluatorWindowClass::BuildIndexationData();

    //Full coverage check up and indication
    GainEvaluatorWindowClass::PMsCoverageCheck();

    //visualization
    ui->labVars->setText(QString::number(Variables));
    ui->labPMsinUse->setText(QString::number(Variables));
    ui->labEquations->setText(QString::number(Equations));
    if (Equations < Variables)
       ui->labEquatonsDefinedWarning->setPixmap(RedIcon.pixmap(16,16));
    else
       ui->labEquatonsDefinedWarning->setPixmap(GreenIcon.pixmap(16,16));

    //update graphics view
    //GainEvaluatorWindowClass::UpdateGraphics();
    QTimer::singleShot(50, this, SLOT(UpdateGraphics()));  //to exit first this procedure!
}

bool GainEvaluatorWindowClass::isPMDistanceCheckFail(int ipm1, int ipm2)
{
  double minDistance = ui->ledMinDistancePMs->text().toDouble();
  double maxDistance = ui->ledMaxDistancePMs->text().toDouble();

  QLineF l12(QPointF(MW->PMs->X(ipm1), MW->PMs->Y(ipm1)), QPointF(MW->PMs->X(ipm2), MW->PMs->Y(ipm2)));
  double dist = l12.length();
  return (dist<minDistance-0.01 || dist>maxDistance+0.01);
}
bool GainEvaluatorWindowClass::isPMDistanceCheckFailLogR(int ipm1, int ipm2)
{
  double minDistance = ui->ledLogR_MinDistancePMs->text().toDouble();
  double maxDistance = ui->ledLogR_MaxDistancePMs->text().toDouble();

  QLineF l12(QPointF(MW->PMs->X(ipm1), MW->PMs->Y(ipm1)), QPointF(MW->PMs->X(ipm2), MW->PMs->Y(ipm2)));
  double dist = l12.length();
  return (dist<minDistance-0.01 || dist>maxDistance+0.01);
}

void GainEvaluatorWindowClass::BuildIndexationData()
{
  //buiding indexation data
// qDebug()<<"--  --  --  --  --  building indexation data -- -- -- -- -- ";
  AllLinks.resize(MW->PMs->count());
  for (int ipm=0; ipm<MW->PMs->count(); ipm++) AllLinks[ipm].clear();

  for (int ipm=0; ipm<MW->PMs->count(); ipm++)
    {      
      if (!iPMs.contains(ipm)) continue; //not a pm from our list

      //cut-off method
      if (ui->cbActivateCutOffs->isChecked())
       if (iCutOffPMs.contains(ipm))
        {
//          qDebug()<<"cutoffs contains "<<ipm;
          QSet<int> tmp = QSet<int>::fromList(iCutOffPMs);
          AllLinks[ipm] += tmp;
        }

      //centers method
      if (ui->cbCentersActivate->isChecked())
       if (!iCentersPMs.isEmpty())
        {
          for (int icenter=0; icenter<iCentersPMs.size(); icenter++)
            for (int igroup=0; igroup<CenterGroups.size(); igroup++)
              {
                int centerPM = iCentersPMs[icenter];
                QList<int> Neighbours;
                GainEvaluatorWindowClass::findNeighbours(centerPM, igroup, &Neighbours);
                if (Neighbours.size() < 2) continue; //we use lists of at least 2 elements

                //converting list to normal ipm (it is index in iPMs originally)
                for (int i=0; i<Neighbours.size(); i++) Neighbours[i] = iPMs[Neighbours[i]];

                int pos = Neighbours.indexOf(ipm);
                if (pos < 0) continue; //not in the list

//                qDebug()<<ipm<<" is memeber of center group"<<igroup<<" with center ipm="<<centerPM;

                //adding to the Linked
                Neighbours.removeAt(pos);
                QSet<int> tmp = QSet<int>::fromList(Neighbours);
                AllLinks[ipm] += tmp;
              }
        }

      //4PMs
      if (ui->cb4Activate->isChecked())
       if (!iSets4PMs.isEmpty())
        {
//          qDebug()<<"4 PM sets !";
          for (int iset=0; iset<iSets4PMs.size(); iset++)
            {
              int i;
              for (i=0; i<4; i++)
                {
                  if (iSets4PMs[iset].PM[i] == ipm) break;
                }
//              qDebug()<<"------i="<<i;

              if (i<4)
                {
                  //found ipm is the member of the 4set
                  if (iSets4PMs[iset].Symmetric)
                    {
                      //adding all but this one
                      for (int ii=0; ii<4;ii++)
                          if (ii != ipm) AllLinks[ipm].insert(iSets4PMs[iset].PM[ii]);
                    }
                  else
                    {
                      //adding only the PM from the other corner
                           if (i==0) AllLinks[ipm].insert(iSets4PMs[iset].PM[2]);
                      else if (i==1) AllLinks[ipm].insert(iSets4PMs[iset].PM[3]);
                      else if (i==2) AllLinks[ipm].insert(iSets4PMs[iset].PM[0]);
                                else AllLinks[ipm].insert(iSets4PMs[iset].PM[1]);  //i==3
                    }
                }

            }
        }

      //flood method
      if (ui->cbFromUniform->isChecked())
        {
          AllLinks[ipm] <<ipm;

          for (int ipm2=ipm+1; ipm2<MW->PMs->count(); ipm2++)
            {
             if (!iPMs.contains(ipm2)) continue; //not a pm from our list
             if (isPMDistanceCheckFail(ipm, ipm2)) continue;

             AllLinks[ipm] << ipm2;
             AllLinks[ipm2] << ipm;
            }          
        }

      //Log method
      if (ui->cbActivateLogR->isChecked())
       if (iLogPMs.contains(ipm))
        {
          AllLinks[ipm] <<ipm;

          if (ui->cobLogMethodSelection->currentIndex() == 0)
            {
              //Pairs
              for (int ipm2=ipm+1; ipm2<MW->PMs->count(); ipm2++)
                {
                 if (!iLogPMs.contains(ipm2)) continue; //not a pm from our list
                 if (isPMDistanceCheckFailLogR(ipm, ipm2)) continue;

                 AllLinks[ipm] << ipm2;
                 AllLinks[ipm2] << ipm;
                }
            }
          else
            {
              //triads are trited after the main cycle!
            }

        }


    }

  //Triads - if activated
  if (ui->cbActivateLogR->isChecked() && ui->cobLogMethodSelection->currentIndex() == 1)
    {
      int numTriads = static_cast<int>(Triads.getTriads()->size());
      for (int iTriad=0; iTriad<numTriads; iTriad++)
        {
          int ipm0 =  iLogPMs[ (*Triads.getTriads())[iTriad][0] ];
          int ipm1 =  iLogPMs[ (*Triads.getTriads())[iTriad][1] ];
          int ipm2 =  iLogPMs[ (*Triads.getTriads())[iTriad][2] ];

          AllLinks[ipm0] << ipm1 << ipm2;
          AllLinks[ipm1] << ipm0 << ipm2;
          AllLinks[ipm2] << ipm0 << ipm1;
        }
    }


  /*
    //debug
  qDebug()<<"------------checkup data------------";
  for (int i=0; i<MW->PMs->count(); i++)
    {
      if (!iPMs.contains(i)) continue; //not a pm from our list
      else qDebug()<<i<<"> "<<AllLinks[i];
    }

  qDebug()<<"------------------------------------";
  */
}


bool GainEvaluatorWindowClass::PMsCoverageCheck()
{
   QList<int> LinkedPMs;

   if (!iPMs.isEmpty())
     {
       LinkedPMs << iPMs[0];

       for (int index = 0; index<LinkedPMs.size(); index++) //auto-growing list!
         {
           int ipm = LinkedPMs[index];

           //going through all links to this PM and adding to list all new linkd PMs
           QSet<int>::const_iterator iterator = AllLinks[ipm].constBegin();
           while (iterator != AllLinks[ipm].constEnd())
             {
               int iNewPM = *iterator;
               if (iNewPM != ipm)
                 {
                   if (!LinkedPMs.contains(iNewPM))
                     {
                       //new PM found!
                       LinkedPMs.append(iNewPM);
                     }
                 }

               ++iterator;
             }
         }
     }

//   qDebug()<<"Linked list found:"<<LinkedPMs;

   //reporting
   if (LinkedPMs.size() == iPMs.size())
     {
       //Good - full coverafe!
       //ui->twAlgorithms->tabBar()->setTabIcon(5, GreenIcon);
       firstUncoveredSet.clear();

       ui->labLinkingWarning->setPixmap(GreenIcon.pixmap(16,16));
       ui->label_40->setText("Yes");
       ui->pbShowNonLinkedSet->setEnabled(false);

       return true;
     }
   else
     {
       //Bad - there are non-linked PMs
       //ui->twAlgorithms->tabBar()->setTabIcon(5, RedIcon);
       firstUncoveredSet = QSet<int>::fromList(LinkedPMs);


       //ui->teOverview->clear();
       //ui->teOverview->append("Not enough links between PMs!");
       //ui->teOverview->append("The first set of PMs which is not linked to the rest:");
       //qSort(LinkedPMs);
       //QString str;
       //for (int i=0; i<LinkedPMs.size(); i++) str += QString::number(LinkedPMs[i])+',';
       //str.chop(1);
       //ui->teOverview->append(str);
       ui->labLinkingWarning->setPixmap(RedIcon.pixmap(16,16));
       ui->label_40->setText("No");
       ui->pbShowNonLinkedSet->setEnabled(true);

       return false;
     }

   return true; //just formal
}

void GainEvaluatorWindowClass::on_pbEvaluateGains_clicked()
{
  //____________watchdogs____________

  if (iPMs.count() == 0)
    {
      message("There are no PMs selected!", this);
      return;
    }

  if (Equations < Variables)
    {
      message("Too few equation for this number of PMs", this);
      return;
    }

  if (ui->cbActivateCutOffs->isChecked())
    {
      if (ui->cobCutOffsOptions->currentIndex() == 1) //top fraction option
        {
          if (EventsDataHub->isEmpty())
            {
              message("No events!", this);
              return;
            }

          int TopCount = EventsDataHub->Events.size() * CutOffFraction;
          if (TopCount < 1)
            {
              message("Direct algorithm: Too few events for the selected fraction!", this);
              return;
            }
        }
    }

  if (ui->cbCentersActivate->isChecked())
    {
      if (EventsDataHub->isEmpty())
        {
          message("No events!", this);
          return;
        }

      int TopCount = EventsDataHub->Events.size() * CenterTopFraction;
      if (TopCount < 1)
        {
          message("Center algorithm: Too few events for the selected fraction!", this);
          return;
        }      
    }

  if (ui->cb4Activate->isChecked())
    {
      if (EventsDataHub->isEmpty())
        {
          message("No events!", this);
          return;
        }
    }

  if (ui->cbActivateLogR->isChecked())
    {
      if (EventsDataHub->isEmpty())
        {
          message("Events are not connected!", this);
          return;
        }
    }

  //______________________________________

  //inits  
  Events = &EventsDataHub->Events; //no problem if empty, checked on start
  if (!EventsDataHub->isReconstructionReady()) DoFiltering = false; //in this case cannot check for event reconstruction status
  else
  {
      DoFiltering = true;
      RecData = &EventsDataHub->ReconstructionData[0];
  }

  TMatrixD A(Equations, Variables);
  TVectorD y(Equations);

  int iequation = 0; //current number of equation
  GeoMarkerClass* marks;
  bool DoVisualization = false;  
  if (ui->cbVisualizeEventSelection->isChecked())
    {
      DoVisualization = true;
      //MW->DotsTGeo.clear();
      MW->clearGeoMarkers();
      marks = new GeoMarkerClass();
    }

  //setting up the list usied for sorting
  QList<int> Sorter; //using QList - seems to be more efficient memory usage when sorting is involved
  Sorter.clear();
  if (!EventsDataHub->isEmpty())
    for (int i=0; i<EventsDataHub->Events.size(); i++) Sorter << i; //Sorter now contains unsorted event numbers

  // +++ initialization state completed +++


  MW->WindowNavigator->BusyOn();
//  qDebug()<<"Starting gain evaluation...";
  MW->Owindow->OutText("  preparing equations...");
  qApp->processEvents();
  //============================== CutOffs ===============================
  if (ui->cbActivateCutOffs->isChecked())
    {
      //preparation phase
      //for cut-off option, relative gains = cut off - can be taken directly.
      //for Top %% option, we have to calculate average signals for all PMs
      QVector<double> RelGains(iCutOffPMs.size(), 0); //linked to iCutOffPMs in indexing
      if (ui->cobCutOffsOptions->currentIndex() == 1) //using top fraction
         {
            for (int ipmIndex = 0; ipmIndex<iCutOffPMs.size(); ipmIndex++)  //iCutOffPms contain proper ipm indexes (ipm)
              {
                CurrentIPM = iCutOffPMs[ipmIndex];                
                qSort(Sorter.begin(), Sorter.end(), CentersLessThan);
                //qDebug()<<"sorted!";

                int TopCount = Sorter.size() * CutOffFraction;
                for (int ievIndex=0; ievIndex<TopCount; ievIndex++)
                  {
                    int iev = Sorter[ievIndex];
                    //qDebug()<<"iev:"<<iev;
                    RelGains[ipmIndex] += EventsDataHub->Events.at(iev)[CurrentIPM];

                    //vis
                    if (DoVisualization)
                      {
                         if (!EventsDataHub->isScanEmpty())
                           //MW->DotsTGeo.append(DotsTGeoStruct(EventsDataHub->SimulatedScan[iev]->Points[0].r, kBlue));
                           marks->SetNextPoint(EventsDataHub->Scan[iev]->Points[0].r[0], EventsDataHub->Scan[iev]->Points[0].r[1], EventsDataHub->Scan[iev]->Points[0].r[2]);
                         else
                           {
                             //else reconstructed positions if available
                             if (EventsDataHub->isReconstructionReady())
                               //MW->DotsTGeo.append(DotsTGeoStruct(EventsDataHub->ReconstructionData[iev]->Points[0].r, kRed));
                               marks->SetNextPoint(EventsDataHub->ReconstructionData[0][iev]->Points[0].r[0], EventsDataHub->ReconstructionData[0][iev]->Points[0].r[1], EventsDataHub->ReconstructionData[0][iev]->Points[0].r[2]);
                           }
                      }
                  }  
              }
         }

      //main phase
      for (int ipmIndex=0; ipmIndex<iCutOffPMs.size()-1; ipmIndex++) //-1 - see inside cycle - we do not want to use the same pairs several times
        {
          //each PM has to be linked to all other PMs, ratio of gains = ratio of cutOffs
          for (int ipmIndexOther=ipmIndex+1; ipmIndexOther<iCutOffPMs.size(); ipmIndexOther++)
           {
             double RelativeGainThisPM = 0;
             double RelativeGainOtherPM = 0;
             int ipm = iCutOffPMs[ipmIndex]; //this PM index
             int ipmOther = iCutOffPMs[ipmIndexOther]; //index of the other PM
             int indexInIPMs = iPMs.indexOf(ipm);
             int indexOtherInIPMs = iPMs.indexOf(ipmOther);

             if (ui->cobCutOffsOptions->currentIndex() == 0)
               {
                 //Using individual chanel maximum signal cut-offs set by the user
                 //double CutOff = MW->PMs->at(ipm).cutOffMax;
                 double CutOff = MW->Detector->PMgroups->getCutOffMax(ipm, CurrentGroup);
                 //double CutOffOther = MW->PMs->at(ipmOther).cutOffMax;
                 double CutOffOther = MW->Detector->PMgroups->getCutOffMax(ipmOther, CurrentGroup);

                 RelativeGainThisPM = CutOff;
                 RelativeGainOtherPM = CutOffOther;
               }
             else if (ui->cobCutOffsOptions->currentIndex() == 1)
               {
                 //already have relative gains calculated during preparation phase
                 RelativeGainThisPM = RelGains[ipmIndex];
                 RelativeGainOtherPM = RelGains[ipmIndexOther];
               }

             //normalization to unity - so all methods have the same weight
             double g1 = fabs(RelativeGainThisPM);
             double g2 = fabs(RelativeGainOtherPM);
             double norm = ( g1 > g2 ) ? g1 : g2;
             if (norm > 1e-20)
               {
                 RelativeGainThisPM /= norm;
                 RelativeGainOtherPM /= norm;
               }

//             qDebug()<<ipm<<RelativeGainThisPM<<" : "<<ipmOther<<RelativeGainOtherPM;

             /*
             //filling the matrix line
             if (iequation>Equations)
               {
                 message("Error filling SVD matrix!", this);
                 MW->WindowNavigator->BusyOff();
                 return;
               }
             */

             for (int i=0; i<Variables; i++)
               {
                 if (i == indexInIPMs) A(iequation, i) = RelativeGainOtherPM;
                 else if (i == indexOtherInIPMs) A(iequation, i) = -RelativeGainThisPM;
                 else A(iequation, i) = 0;

                 y[iequation] = 0;
               }
             iequation++;
           }
        }
    }

  //============================== Centers =================================
  if (ui->cbCentersActivate->isChecked())
    {
      QList<int> Neighbours;

      //starting cycle by grops
      for (int igroup = 0; igroup < CenterGroups.size(); igroup++)
        {
          //starting cycle by Center PMs
          for (int ipmIndex=0; ipmIndex<iCentersPMs.size(); ipmIndex++)
            {
              int ipm = iCentersPMs[ipmIndex];
              //for each of center PMs find the list of neighbours
              GainEvaluatorWindowClass::findNeighbours(ipm, igroup, &Neighbours); //WARNING!!! Neighbours contains indeces of PM in iPMs
              if (Neighbours.size() < 2) continue; //need at least 2 PMs

              //sorting events in decreasing order in respect to the signal of the central PM (index in iPMs = ipmIndex)
              CurrentIPM = ipm;              
              qSort(Sorter.begin(), Sorter.end(), CentersLessThan);
//              qDebug()<<Sorter;

              //extracting relative gains of the neighbours - they are proportional to the average signal of these PMs in the selected top fraction of the sorted list
              int TopCount = Sorter.size() * CenterTopFraction;
              QVector<double> RelativeGains(Neighbours.size(), 0); //indexed according to Neighbours!!!
              for (int ievIndex=0; ievIndex<TopCount; ievIndex++)
                {
                  int iev = Sorter[ievIndex];

                  for (int iN = 0; iN<Neighbours.size(); iN++ )
                    {
                      int ipm = iPMs[ Neighbours[iN] ];                      
                      RelativeGains[iN] += EventsDataHub->Events.at(iev)[ipm];
                    }

                  //vis
                  if (DoVisualization)
                    {
                       if (!EventsDataHub->isScanEmpty())
                         //MW->DotsTGeo.append(DotsTGeoStruct(EventsDataHub->SimulatedScan[iev]->Points[0].r, kBlue));
                         marks->SetNextPoint(EventsDataHub->Scan[iev]->Points[0].r[0], EventsDataHub->Scan[iev]->Points[0].r[1], EventsDataHub->Scan[iev]->Points[0].r[2]);
                       else
                         {
                           //else reconstructed positions if available
                           if (!EventsDataHub->ReconstructionData.isEmpty())
                             //MW->DotsTGeo.append(DotsTGeoStruct(EventsDataHub->ReconstructionData[iev]->Points[0].r, kRed));
                             marks->SetNextPoint(EventsDataHub->ReconstructionData[0][iev]->Points[0].r[0], EventsDataHub->ReconstructionData[0][iev]->Points[0].r[1], EventsDataHub->ReconstructionData[0][iev]->Points[0].r[2]);
                         }
                    }
                }

              //now can fill equations
              //relate all neighbour PMs in pairs
              for (int iN = 0; iN<Neighbours.size()-1; iN++ )  //-1: see inside cycle - we do not want to include the same pairs several times
                {
                  int indexThisPM = Neighbours[iN];
                  for (int iNN = iN+1; iNN<Neighbours.size(); iNN++ )
                    {
                      int indexOtherPM = Neighbours[iNN];

                      for (int i=0; i<Variables; i++)
                        {
                          //normalization to unity - so all methods have the same weight
                          double g1 = fabs(RelativeGains[iN]);
                          double g2 = fabs(RelativeGains[iNN]);
                          double norm = ( g1 > g2 ) ? g1 : g2;
                          double RelativeGainThisPM = RelativeGains[iN];
                          double RelativeGainOtherPM = RelativeGains[iNN];
                          if (norm > 1e-20)
                            {
                              RelativeGainThisPM /= norm;
                              RelativeGainOtherPM /= norm;
                            }

                          if (i == indexThisPM) A(iequation, i) = RelativeGainOtherPM;
                          else if (i == indexOtherPM) A(iequation, i) = -RelativeGainThisPM;
                          else A(iequation, i) = 0;

                          y[iequation] = 0;
                        }
                      iequation++;
                    }
                }
            }//end cycle by Center PM
        }//end cycle by groups
    }

  //============================== Quartets =================================
  if (ui->cb4Activate->isChecked())
    {
      QList<int> CenterEventsList;
      for (int iset = 0; iset<iSets4PMs.size(); iset++)
        {
          //starting cycle by available sets of 4 PMs
          FindEventsInMiddle(iSets4PMs[iset].PM[0], iSets4PMs[iset].PM[1], iSets4PMs[iset].PM[2], iSets4PMs[iset].PM[3], &Sorter, &CenterEventsList);
          //now CenterEventsList contains indeces of events which are supposedly in the center of the 4pm set

          //vis
          if (DoVisualization)
            {
               if (!EventsDataHub->isScanEmpty())
                 {
                   //then adding actual positions
                   for (int i=0; i<CenterEventsList.size(); i++)
                     {
                       int iev = CenterEventsList[i];
                       //MW->DotsTGeo.append(DotsTGeoStruct(EventsDataHub->SimulatedScan[iev]->Points[0].r, kBlue));
                       marks->SetNextPoint(EventsDataHub->Scan[iev]->Points[0].r[0], EventsDataHub->Scan[iev]->Points[0].r[1], EventsDataHub->Scan[iev]->Points[0].r[2]);
                     }
                 }
               else
                 {
                   //else reconstructed positions if available
                   if (EventsDataHub->isReconstructionReady())
                     {
                       for (int i=0; i<CenterEventsList.size(); i++)
                         {
                           int iev = CenterEventsList[i];
                           //MW->DotsTGeo.append(DotsTGeoStruct(EventsDataHub->ReconstructionData[iev]->Points[0].r, kRed));
                           marks->SetNextPoint(EventsDataHub->ReconstructionData[0][iev]->Points[0].r[0], EventsDataHub->ReconstructionData[0][iev]->Points[0].r[1], EventsDataHub->ReconstructionData[0][iev]->Points[0].r[2]);
                         }
                     }
                 }
            }

          //calculating relative gains
          QVector<double> RelativeGains(4, 0);
          for (int ievIndex=0; ievIndex<CenterEventsList.size(); ievIndex++)  //ievIndex - index of the event stored in CenterEventsList
            {
              for (int iN = 0; iN<4; iN++ ) //four pms of the set
                {
                  int ipm = iSets4PMs[iset].PM[iN];
                  int iev = CenterEventsList[ievIndex];
                  RelativeGains[iN] += EventsDataHub->Events.at(iev)[ipm];
                }
            }

          //Filling the equations
          if (iSets4PMs[iset].Symmetric)
            {
              //symmetric - link all PMs - 6 equations
              for (int iFirstPM = 0; iFirstPM<3; iFirstPM++)
                for (int iSecondPM = 1; iSecondPM<4; iSecondPM++)
                  {
                    //linking PMs
                    for (int i=0; i<Variables; i++)
                      {
                        //normalization to unity - so all methods have the same weight
                        double g1 = fabs(RelativeGains[iFirstPM]);
                        double g2 = fabs(RelativeGains[iSecondPM]);
                        double norm = ( g1 > g2 ) ? g1 : g2;
                        double RelativeGainThisPM = RelativeGains[iFirstPM];
                        double RelativeGainOtherPM = RelativeGains[iSecondPM];
                        if (norm > 1e-20)
                          {
                            RelativeGainThisPM /= norm;
                            RelativeGainOtherPM /= norm;
                          }

                        if (i == iSets4PMs[iset].PM[iFirstPM]) A(iequation, i) = RelativeGainOtherPM;
                        else if (i == iSets4PMs[iset].PM[iSecondPM]) A(iequation, i) = -RelativeGainThisPM;
                        else A(iequation, i) = 0;

                        y[iequation] = 0;
                      }
                    iequation++;
                  }
            }
          else
            {
              //non-symmetric, linking only opposite PMs - 2 equations
                //PM0 vs PM2
              for (int i=0; i<Variables; i++)
                {
                  //normalization to unity - so all methods have the same weight
                  double g1 = fabs(RelativeGains[0]);
                  double g2 = fabs(RelativeGains[2]);
                  double norm = ( g1 > g2 ) ? g1 : g2;
                  double RelativeGainThisPM = RelativeGains[0];
                  double RelativeGainOtherPM = RelativeGains[2];
                  if (norm > 1e-20)
                    {
                      RelativeGainThisPM /= norm;
                      RelativeGainOtherPM /= norm;
                    }

                  if (i == iSets4PMs[iset].PM[0]) A(iequation, i) = RelativeGainOtherPM;
                  else if (i == iSets4PMs[iset].PM[2]) A(iequation, i) = -RelativeGainThisPM;
                  else A(iequation, i) = 0;

                  y[iequation] = 0;
                }
              iequation++;
                //PM1 vs PM3
              for (int i=0; i<Variables; i++)
                {
                  //normalization to unity - so all methods have the same weight
                  double g1 = fabs(RelativeGains[1]);
                  double g2 = fabs(RelativeGains[3]);
                  double norm = ( g1 > g2 ) ? g1 : g2;
                  double RelativeGainThisPM = RelativeGains[1];
                  double RelativeGainOtherPM = RelativeGains[3];
                  if (norm > 1e-20)
                    {
                      RelativeGainThisPM /= norm;
                      RelativeGainOtherPM /= norm;
                    }

                  if (i == iSets4PMs[iset].PM[1]) A(iequation, i) = RelativeGainOtherPM;
                  else if (i == iSets4PMs[iset].PM[3]) A(iequation, i) = -RelativeGainThisPM;
                  else A(iequation, i) = 0;

                  y[iequation] = 0;
                }
              iequation++;
            }

        }//cycle by 4 PMs ends

    }

  //============================== Uniform =================================
  if (ui->cbFromUniform->isChecked())
    {
      //cycle by first PM
      for (int i1 = 0; i1<iPMs.count(); i1++)
        {
            int ipm1 = iPMs[i1];

            //cycle by second PM
            for (int i2=i1+1; i2<iPMs.count(); i2++)
                {
                  int ipm2 = iPMs[i2];
                  if (isPMDistanceCheckFail(ipm1, ipm2)) continue; //check distance between PM is withing the user range

                  //getting relative gains
                  double g2 = GainEvaluatorWindowClass::AdjustGainPair(ipm1, ipm2); //g2 is relative to g1 = 1.0
                  if (g2 == 0)
                    {
                      //error detected!
                      message("Method failed: one of the PM pairs reported zero overlap");
                      MW->WindowNavigator->BusyOff();
                      return;
                    }

                  double g1;
                  if (g2 < 1.0) g1 = 1.0;
                  else
                    {
                      g1 = 1.0/g2;
                      g2 = 1.0;
                    }

                  //filling this equation
                  for (int i=0; i<iPMs.count(); i++)
                    {
                      int ipm = iPMs[i];
                      if (ipm == ipm1) A(iequation, i) = g2;
                      else if (ipm == ipm2) A(iequation, i) = -g1;
                      else A(iequation, i) = 0;

                      y[iequation] = 0;
                    }
                  iequation++;
                }
        }
    }

  //============================== Log =================================
  if (ui->cbActivateLogR->isChecked())
    {
      qDebug()<<"----------!!!--------------";
      if (ui->cobLogMethodSelection->currentIndex() == 0)
        {          
          //Log pairs
            //cycle by first PM
          for (int i1 = 0; i1<iLogPMs.size(); i1++)
            {
                int ipm1 = iLogPMs[i1];

                //cycle by second PM
                for (int i2=i1+1; i2<iLogPMs.size(); i2++)
                    {
                      int ipm2 = iLogPMs[i2];
                      qDebug()<<"PMs:"<<ipm1<<ipm2;
                      if (isPMDistanceCheckFailLogR(ipm1, ipm2)) continue; //check distance between PM is withing the user range

                      //getting relative gains
                      double g2 = GainEvaluatorWindowClass::AdjustGainPairLogR(ipm1, ipm2); //g2 is relative to g1 = 1.0
                      double g1;
                      if (g2 < 1.0) g1 = 1.0;
                      else
                        {
                          g1 = 1.0/g2;
                          g2 = 1.0;
                        }
                      qDebug()<<"   -> relative gains:"<<g1<<g2;

                      //filling this equation
                      for (int i=0; i<iPMs.count(); i++)
                        {
                          int ipm = iPMs[i];
                          if (ipm == ipm1) A(iequation, i) = g2;
                          else if (ipm == ipm2) A(iequation, i) = -g1;
                          else A(iequation, i) = 0;

                          y[iequation] = 0;
                        }
                      iequation++;
                    }
            }
        }
      else
        {
          // Triads
          qDebug()<<"----------!!!--------------";
          int numTriads = static_cast<int>( Triads.getTriads()->size() );
          for (int iTriad=0; iTriad<numTriads; iTriad++)
            {
              double gainRat10, gainRat20;
              Triads.findRelativeGains(iTriad, &gainRat10, &gainRat20);

              int iPM0 =  iLogPMs[ (*Triads.getTriads())[iTriad][0] ];
              int iPM1 =  iLogPMs[ (*Triads.getTriads())[iTriad][1] ];
              int iPM2 =  iLogPMs[ (*Triads.getTriads())[iTriad][2] ];
              qDebug()<<"Triad:"<<iTriad<<" ipms:"<<iPM0<<iPM1<<iPM2<<" Relative gains10 and 20:"<<gainRat10<<gainRat20;

              int ipm1[2], ipm2[2];
              ipm1[0] = iPM0; ipm2[0] = iPM1;
              ipm1[1] = iPM0; ipm2[1] = iPM2;
              double g1[2], g2[2];
              if (gainRat10<=1.0 || fabs(gainRat10)<1.0e-10)
                {
                  g1[0] = 1.0;
                  g2[0] = gainRat10;
                }
              else
                {
                  g1[0] = 1.0/gainRat10;
                  g2[0] = 1.0;
                }

              if (gainRat20<=1.0 || fabs(gainRat20)<1.0e-10)
                {
                  g1[1] = 1.0;
                  g2[1] = gainRat20;
                }
              else
                {
                  g1[1] = 1.0/gainRat20;
                  g2[1] = 1.0;
                }

              //filling these 2 equations
              for (int ieq=0; ieq<2; ieq++)
                {
                  for (int i=0; i<iPMs.count(); i++)
                    {
                      int ipm = iPMs[i];
                      if (ipm == ipm1[ieq]) A(iequation, i) = g2[ieq];
                      else if (ipm == ipm2[ieq]) A(iequation, i) = -g1[ieq];
                      else A(iequation, i) = 0;

                      y[iequation] = 0;
                    }
                  iequation++;
                }
            }
        }
    }


  //=============------============= normalization ===============---------============
  A(iequation, 0) = 1.0; //can set to unity - all equations already scaled to 1
  y[iequation] = 1.0;
  iequation++; //since it starts from 0 - for later indication

  if (iequation<iPMs.count())
    {
      message("Cannot perform gain evaluation: Number of equations is less than number of variables!", this);
      MW->WindowNavigator->BusyOff();
      return;
    }


  //=============------============= SVD ===============---------============
  MW->Owindow->OutText("  done!");
  MW->Owindow->OutText("  doing SVD...");
  qApp->processEvents();


  //debug
  qDebug()<<"-------------------";
  qDebug()<<"Variables (PMs):"<<iPMs.count();
  qDebug()<<"Actual equations: "<<iequation;  //its +1 but we started from 0
  qDebug()<<"-------------------";
  qDebug()<<"y ("<<y.GetNoElements()<<"):";
  for (int i=0; i<Equations; i++) qDebug()<<y[i];
  QString str;  
  qDebug()<<"-------------------";
  qDebug()<<"matrix ("<<A.GetNrows()<<","<<A.GetNcols()<<"):";
  for (int ix=0; ix<Equations; ix++)
    {
      str="";
      for (int iy=0; iy<Variables; iy++) str += QString::number(A(ix,iy))+" ";
      qDebug()<<str;
    }
  qDebug()<<"-------------------";


  //solving SVD
  TDecompSVD svd(A);
  bool success;
  const TVectorD c_svd = svd.Solve(y, success);
  if (!success)
    {
       message("SVD fail!", this);
       MW->WindowNavigator->BusyOff();
       return;
    }

/*
  //reporting results
  for (int i=0; i<iPMs.count(); i++)
    qDebug()<<"PM#:"<<iPMs[i]<<" result="<<c_svd(i);
*/
  //normalizing and assigning gains
  int imax = 0;
  for (int i=1; i<iPMs.count(); i++) if (c_svd(i) > c_svd(imax)) imax = i;

  double max;
  if (c_svd(imax) == 0) max = 1.0;
  else max = c_svd(imax);

  for (int i=0; i<iPMs.count(); i++)
      //MW->PMs->at(iPMs[i]).relGain = c_svd(i)/max;
      MW->Detector->PMgroups->setGain(iPMs[i], CurrentGroup, c_svd(i)/max);

  //reporting gains:
  qDebug()<<"-------------------";
  for (int i=0; i<iPMs.count(); i++)
    //qDebug()<<"PM#:"<<iPMs[i]<<" gain="<<MW->PMs->at(iPMs[i]).relGain;
    qDebug()<<"PM#:"<<iPMs[i]<<" gain="<<MW->Detector->PMgroups->getGain(iPMs[i], CurrentGroup);

  GainEvaluatorWindowClass::UpdateGraphics();

  MW->Rwindow->UpdateReconConfig(); //Config->JSON update
  MW->Rwindow->onUpdateGainsIndication();

  if (DoVisualization)
    {      
      if (!EventsDataHub->isScanEmpty()) marks->SetMarkerColor(kBlue);
      else marks->SetMarkerColor(kRed);
      marks->SetMarkerSize(2);
      marks->SetMarkerStyle(2);
      MW->GeoMarkers.append(marks);     
      MW->GeometryWindow->ShowGeometry();
      //MW->ShowGeoMarkers();
    }

  MW->Owindow->OutText("  done!");
  MW->WindowNavigator->BusyOff();
}

void GainEvaluatorWindowClass::FindEventsInMiddle(int ipm1, int ipm2, int ipm3, int ipm4, QList<int>* Sorter, QList<int>* Result)
{
  // 1 2
  // 4 3

  QList<int> List13;
  FindEventsInCone(ipm1, ipm3, Sorter, &List13);
  //qDebug()<<"----------------------------Events in List13:"<<List13.size();

  QList<int> List24;
  FindEventsInCone(ipm2, ipm4, Sorter, &List24);
  //qDebug()<<"----------------------------Events in List24:"<<List24.size();

  QList<int> List31;
  FindEventsInCone(ipm3, ipm1, Sorter, &List31);
  //qDebug()<<"----------------------------Events in List31:"<<List31.size();

  QList<int> List42;
  FindEventsInCone(ipm4, ipm2, Sorter, &List42);
  //qDebug()<<"----------------------------Events in List42:"<<List42.size();

  Result->clear();
  int minOverlaps = Overlap4PMs.toInt();

  //looking for overlaps
  QSet<int> CombinedSet = List13.toSet() + List24.toSet() + List31.toSet() + List42.toSet(); //set takes care of dublication

  QSet<int>::const_iterator iterator = CombinedSet.constBegin();
  while (iterator != CombinedSet.constEnd())
    {
      int iev = *iterator;

      int overlaps = 0;
      if (List13.contains(iev)) overlaps++;
      if (List24.contains(iev)) overlaps++;
      if (List31.contains(iev)) overlaps++;
      if (List42.contains(iev)) overlaps++;
      if (overlaps >= minOverlaps) Result->append(iev);
//      if (overlaps > 1) qDebug()<<"----overlaps:"<<overlaps;

      ++iterator;
    }
//  qDebug()<<"==============================Events in Result:"<<Result->size();
}

//function gives list of events situated close to the line (in cone) from ipmFrom to ipmTo
void GainEvaluatorWindowClass::FindEventsInCone(int ipmFrom, int ipmTo, QList<int>* Sorter, QList<int>* Result)
{
  Result->clear();

  CurrentIPM = ipmFrom;
  //Events = &EventsDataHub->Events;
  //RecData = &EventsDataHub->ReconstructionData[0];
  qSort(Sorter->begin(), Sorter->end(), CentersLessThan);
  //Sorter list is sorted by the signal in ipmFrom (decreasing)

  //binning the list
  double MaxSignal = Events->at(Sorter->first())[ipmFrom];
  double MinSignal = Events->at(Sorter->last())[ipmFrom];
  double BinSize = ( MaxSignal - MinSignal ) / Bins4PMs;

  QList<int> tmp; //tmp list

  for (int ibin = 0; ibin<Bins4PMs; ibin++)
    {
      //for each bin copy proper event in tmp list
      tmp.clear();
      for (int ievIndex = 0; ievIndex<Events->size(); ievIndex++)  //ievIndex - index of event in Sorter!
        {
          //Sorter contains event sorted in decreasing order!
          int iev = Sorter->at(ievIndex); //actual index of this event
          double signal = Events->at(iev)[ipmFrom];
          double min = ibin*BinSize;
          double max = min + BinSize;
          if (signal < min) break; //since sorted!
          if (signal > max) continue; //have not reached proper region yet
          tmp << iev;
        }

      //tmp list cotains events within this bin
      //sorting it in respect of the signal of ipmTo
      CurrentIPM = ipmTo;
      bool tmpDoFiltering = DoFiltering; DoFiltering = false; //not needed - already filtered
      qSort(tmp.begin(), tmp.end(), CentersLessThan);
      DoFiltering = tmpDoFiltering;
      //tmp list is sorted by the signal in ipmTo (decreasing)

      //now copying the top fraction of the list to the Results list
      int length = tmp.size();
      int selection = length * Fraction4PMs;
      for (int i=0; i<selection; i++) Result->append(tmp[i]);
    }
}


void GainEvaluatorWindowClass::on_pbUpdateCutOff_clicked()
{
  //qDebug()<<"---CutOffPMs: "<<iCutOffPMs;
  bool enabled = ui->cbActivateCutOffs->isChecked();

  ui->fCutOffsSubSet->setEnabled( enabled );
  ui->fCutOffsOption->setVisible( (bool)ui->cobCutOffsOptions->currentIndex() );
  CutOffFraction = 0.01 * ui->ledCutOffFraction->text().toDouble();

  ui->labPMsCutOff->setText(QString::number(iCutOffPMs.size()));

  bool CutOffsNotDefined = false;
  if (ui->cobCutOffsOptions->currentIndex() == 0)
    {
      //checking are cut-offs defined or not
      for (int i=0; i<iCutOffPMs.size(); i++)
        {
          int ipm = iCutOffPMs[i];
          //if (MW->PMs->at(ipm).cutOffMax == 1e10)
          if (MW->Detector->PMgroups->getCutOffMax(ipm, CurrentGroup) == 1.0e10)
            {
              CutOffsNotDefined = true;
              break;
            }
        }

      ui->fCutOffsUndefined->setVisible(CutOffsNotDefined);
    }
  else ui->fCutOffsUndefined->setVisible(false);

  if (!enabled) ui->twAlgorithms->tabBar()->setTabIcon(0, QIcon());
  else
    {
      if (iCutOffPMs.size()<2 || CutOffsNotDefined) ui->twAlgorithms->tabBar()->setTabIcon(0, RedIcon);
      else ui->twAlgorithms->tabBar()->setTabIcon(0, GreenIcon);
    }

  GainEvaluatorWindowClass::on_pbUpdateUI_clicked();
}

void GainEvaluatorWindowClass::on_pbCutOffsAddAll_clicked()
{
  iCutOffPMs = iPMs;
  GainEvaluatorWindowClass::on_pbUpdateCutOff_clicked();
}

void GainEvaluatorWindowClass::on_pbCutOffsRemoveAll_clicked()
{
  iCutOffPMs.clear();
  GainEvaluatorWindowClass::on_pbUpdateCutOff_clicked();
}

void GainEvaluatorWindowClass::on_pbCutOffsAdd_clicked()
{
  QString input = ui->leCutOffsSet->text();
  GainEvaluatorWindowClass::AddRemovePMs('+', input, &iCutOffPMs);
  GainEvaluatorWindowClass::on_pbUpdateCutOff_clicked();
}

void GainEvaluatorWindowClass::on_pbCutOffsRemove_clicked()
{
  QString input = ui->leCutOffsSet->text();
  GainEvaluatorWindowClass::AddRemovePMs('-', input, &iCutOffPMs);
  GainEvaluatorWindowClass::on_pbUpdateCutOff_clicked();
}

void GainEvaluatorWindowClass::AddRemovePMs(QChar option, QString input, QList<int>* data)
{
  QList<int> tmp;
  bool ok = ExtractNumbersFromQString(input, &tmp);
  if (!ok)
    {
      TMPignore = true;
      message("Input error!", this);
      TMPignore = false;
      return;
    }

  for (int i=0; i<tmp.size(); i++)
    if (i<0 || i>MW->PMs->count())
      {
        TMPignore = true;
        message("PM number out of range!", this);
        TMPignore = false;
        return;
      }

  if (option == '-')
    {
      //remove
      for (int i=0; i<tmp.size(); i++)
        {
          int irez = data->lastIndexOf(tmp[i]);
          if (irez != -1) data->removeAt(irez);
        }
    }
  else if (option == '+')
    {
      //add
      for (int i=0; i<tmp.size(); i++)
        {
          int ipm = tmp[i];
          if (!iPMs.contains(ipm)) continue; //ignore it - not in the list of allowed PMs
          if (data->contains(ipm)) continue; //already there

          int j;
          for (j=0; j<data->size(); j++)
            if (data->at(j) > ipm) break;
          //now j contains the index of the first PM which is larger than ipm
          data->insert(j, ipm);
        }
    }
}

void GainEvaluatorWindowClass::on_leCutOffsSet_editingFinished()
{
  if (TMPignore) return;
  QString input = ui->leCutOffsSet->text();

  QList<int> tmp;
  bool ok = ExtractNumbersFromQString(input, &tmp);
  if (!ok)
    {
      TMPignore = true;
      message("Input error!", this);
      TMPignore = false;
      return;
    }

  for (int i=0; i<tmp.size(); i++)
    if (i<0 || i>MW->PMs->count())
      {
        TMPignore = true;
        message("PM number out of range!", this);
        TMPignore = false;
        return;
      }
}

void GainEvaluatorWindowClass::on_pbCutOffsAddCenterPMs_clicked()
{
  //add CentersPMs to Cutoffs PMs
  for (int i=0; i<iCentersPMs.size(); i++)
    {
      int ipm = iCentersPMs[i];
      if (!iPMs.contains(ipm)) continue; //ignore it - not in the list of allowed PMs
      if (iCutOffPMs.contains(ipm)) continue; //already there

      int j;
      for (j=0; j<iCutOffPMs.size(); j++)
        if (iCutOffPMs.at(j) > ipm) break;
      //now j contains the index of the first PM which is larger than ipm
      iCutOffPMs.insert(j, ipm);
    }

   GainEvaluatorWindowClass::on_pbUpdateCutOff_clicked();
}

void GainEvaluatorWindowClass::on_pbCentersAddAll_clicked()
{
  iCentersPMs = iPMs;
  GainEvaluatorWindowClass::on_pbUpdateCenters_clicked();
}

void GainEvaluatorWindowClass::on_pbCentersRemoveAll_clicked()
{
  iCentersPMs.clear();
  GainEvaluatorWindowClass::on_pbUpdateCenters_clicked();
}

void GainEvaluatorWindowClass::on_leCentersSet_editingFinished()
{
  if (TMPignore) return;
  QString input = ui->leCentersSet->text();

  QList<int> tmp;
  bool ok = ExtractNumbersFromQString(input, &tmp);
  if (!ok)
    {
      TMPignore = true;
      message("Input error!", this);
      TMPignore = false;
      return;
    }

  for (int i=0; i<tmp.size(); i++)
    if (i<0 || i>MW->PMs->count())
      {
        TMPignore = true;
        message("PM number out of range!", this);
        TMPignore = false;
        return;
      }
}

void GainEvaluatorWindowClass::on_pbCentersAdd_clicked()
{
  QString input = ui->leCentersSet->text();
  GainEvaluatorWindowClass::AddRemovePMs('+', input, &iCentersPMs);
  GainEvaluatorWindowClass::on_pbUpdateCenters_clicked();
}

void GainEvaluatorWindowClass::on_pbCentersRemove_clicked()
{
  QString input = ui->leCentersSet->text();
  GainEvaluatorWindowClass::AddRemovePMs('-', input, &iCentersPMs);
  GainEvaluatorWindowClass::on_pbUpdateCenters_clicked();
}

void GainEvaluatorWindowClass::on_ledCentersDistance_editingFinished()
{
  int igroup = ui->sbCentersGroupNumber->value();
  CenterGroups[igroup].setDistance(ui->ledCentersDistance->text().toDouble());

  GainEvaluatorWindowClass::on_pbUpdateCenters_clicked();
}

void GainEvaluatorWindowClass::on_ledCentersTolerance_returnPressed()
{
  int igroup = ui->sbCentersGroupNumber->value();
  CenterGroups[igroup].setTolerance(ui->ledCentersTolerance->text().toDouble());

  GainEvaluatorWindowClass::on_pbUpdateCenters_clicked();
}

void GainEvaluatorWindowClass::on_pbUpdateCenters_clicked()
{
  qDebug()<<"CentersPMs: "<<iCentersPMs;
  bool enabled = ui->cbCentersActivate->isChecked();
  ui->labPMsCenters->setText(QString::number(iCentersPMs.size()));

  ui->labCentersNumberOfGroups->setText( QString::number( CenterGroups.size() ) );
  int igroup = ui->sbCentersGroupNumber->value();
  ui->ledCentersDistance->setText( QString::number( CenterGroups[igroup].getDistance(), 'g', 4) );
  ui->ledCentersTolerance->setText( QString::number( CenterGroups[igroup].getTolerance(), 'g', 4) );

  ui->fCentersSubSet->setEnabled( enabled );
  ui->pbCutOffsAddCenterPMs->setEnabled( enabled );

  //neighbours for this group?
  int counter = 0;
  QList<int> tmp;
  for (int i=0; i<iCentersPMs.size(); i++)
      {
        int ipm = iCentersPMs[i];
        GainEvaluatorWindowClass::findNeighbours(ipm, igroup, &tmp);
        counter += tmp.size();
      }
  ui->labTotalNeighbours->setText(QString::number(counter));

  if (!enabled) ui->twAlgorithms->tabBar()->setTabIcon(1, QIcon());
  else
    {
      if (counter == 0) ui->twAlgorithms->tabBar()->setTabIcon(1, RedIcon);
      else ui->twAlgorithms->tabBar()->setTabIcon(1, GreenIcon);
    }

  GainEvaluatorWindowClass::on_pbUpdateUI_clicked();
}

//WARNING!!! this function returns arrays which contain indexes of PMs in iPM array - need it for filling SVD
void GainEvaluatorWindowClass::findNeighbours(int ipm, int igroup, QList<int>* NeighboursiPMindexes )
{
  NeighboursiPMindexes->clear(); 
  double NeighbourMinDist2 = CenterGroups[igroup].getMinDist2();
  double NeighbourMaxDist2 = CenterGroups[igroup].getMaxDist2();

  for (int i=0; i<iPMs.size(); i++)
    {
      int iother = iPMs[i];
      if (iother == ipm) continue;

      double dx = MW->PMs->X(ipm) - MW->PMs->X(iother);
      double dy = MW->PMs->Y(ipm) - MW->PMs->Y(iother);
      double dist2 = dx*dx + dy*dy;

      if (dist2>NeighbourMinDist2 && dist2<NeighbourMaxDist2) NeighboursiPMindexes->append(i);  //i, not iother!!!
    }
}

void GainEvaluatorWindowClass::on_ledCenterTopFraction_editingFinished()
{
  if (TMPignore) return;

  double val = ui->ledCenterTopFraction->text().toDouble();

  if (val<0 || val >100)
    {
      TMPignore = true;
      message("Input error!", this);
      ui->ledCenterTopFraction->focusWidget();
      ui->ledCenterTopFraction->selectAll();
      TMPignore = false;
    }

  CenterTopFraction = 0.01*val;
}

void GainEvaluatorWindowClass::on_sbCentersGroupNumber_valueChanged(int arg1)
{
  if ( arg1>CenterGroups.size() )
    {
      ui->sbCentersGroupNumber->setValue(0);
      return; //update on_change
    }

  GainEvaluatorWindowClass::on_pbUpdateCenters_clicked();
}

void GainEvaluatorWindowClass::on_pbCentersAddGroup_clicked()
{
  CenterGroups << CenterGroupClass(30.0, 0.5);
  ui->sbCentersGroupNumber->setValue( CenterGroups.size()-1 );

  GainEvaluatorWindowClass::on_pbUpdateCenters_clicked();
}

void GainEvaluatorWindowClass::on_pbCentersRemoveGroup_clicked()
{
  if (CenterGroups.size() < 2) return;

  int igroup = ui->sbCentersGroupNumber->value();
  CenterGroups.remove(igroup);

  if (igroup>0) ui->sbCentersGroupNumber->setValue(igroup-1);

  GainEvaluatorWindowClass::on_pbUpdateCenters_clicked();
}

void GainEvaluatorWindowClass::on_ledCutOffFraction_editingFinished()
{
  if (TMPignore) return;

  double val = ui->ledCutOffFraction->text().toDouble();

  if (val<0 || val >100)
    {
      TMPignore = true;
      message("Input error!", this);
      ui->ledCutOffFraction->focusWidget();
      ui->ledCutOffFraction->selectAll();
      TMPignore = false;
    }

   CutOffFraction = 0.01 * ui->ledCutOffFraction->text().toDouble();
}

void GainEvaluatorWindowClass::on_pbUpdate4_clicked()
{
  qDebug()<<"4Sets: "<<iSets4PMs.size();
  bool enabled = ui->cb4Activate->isChecked();

  ui->f4->setEnabled( enabled );

  Tolerance4PMs = ui->led4tolerance->text().toDouble();
  Bins4PMs = ui->sb4bins->value();
  Fraction4PMs = 0.01 * ui->led4overlap->text().toDouble();
  Overlap4PMs = ui->cob4segments->currentText();

  //updating the table
  ui->tabwid4sets->clearContents();
  ui->tabwid4sets->setShowGrid(false);
  int rows = iSets4PMs.size();

  ui->tabwid4sets->setRowCount(rows);
  ui->tabwid4sets->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->tabwid4sets->setSelectionMode(QAbstractItemView::SingleSelection);

  for (int j=0; j<ui->tabwid4sets->rowCount(); j++)
    {
      QString str;
      for (int ic = 0; ic<4; ic++)
        {
          str.setNum(iSets4PMs[j].PM[ic]);
          ui->tabwid4sets->setItem(j, ic, new QTableWidgetItem(str));
        }

      if (iSets4PMs[j].Symmetric) str = "+"; else str = "-";
      ui->tabwid4sets->setItem(j, 4, new QTableWidgetItem(str));
    }

  ui->tabwid4sets->resizeColumnsToContents();
  ui->tabwid4sets->resizeRowsToContents();
  //for (int i=0; i<ui->tabwid4sets->columnCount(); i++) ui->tabwid4sets->setColumnWidth(i, 30);

  if (!enabled) ui->twAlgorithms->tabBar()->setTabIcon(2, QIcon());
  else
    {
      if (iSets4PMs.isEmpty()) ui->twAlgorithms->tabBar()->setTabIcon(2, RedIcon);
      else ui->twAlgorithms->tabBar()->setTabIcon(2, GreenIcon);
    }

  GainEvaluatorWindowClass::on_pbUpdateUI_clicked();
}

void GainEvaluatorWindowClass::on_le4newQuartet_editingFinished()
{
  if (TMPignore) return;

  if (ui->le4newQuartet->text() == "") return; //way to escape

  GainEvaluatorWindowClass::validate4PMsInput();
}

bool GainEvaluatorWindowClass::validate4PMsInput()
{
  QList<int> tmp;
  bool ok = ExtractNumbersFromQString(ui->le4newQuartet->text(), &tmp);
  if (!ok)
    {
      TMPignore = true;
      message("Input error!", this);
      TMPignore = false;
      return false;
    }

  if (tmp.size() != 4)
    {
      TMPignore = true;
      message("List should contain 4 PMs!", this);
      TMPignore = false;
      return false;
    }

  for (int i=0; i<tmp.size(); i++)
    if (!iPMs.contains(i))
      {
        TMPignore = true;
        message("All PMs should belong to the selected list!", this);
        TMPignore = false;
        return false;
      }

  //checking quartet - should be center symmetric (0-1 vs 2-3)
  double x02 = 0.5 * (MW->PMs->X(tmp[0]) + MW->PMs->X(tmp[2]));
  double y02 = 0.5 * (MW->PMs->Y(tmp[0]) + MW->PMs->Y(tmp[2]));
  double x13 = 0.5 * (MW->PMs->X(tmp[1]) + MW->PMs->X(tmp[3]));
  double y13 = 0.5 * (MW->PMs->Y(tmp[1]) + MW->PMs->Y(tmp[3]));
    //distance between 02 and 13 should be smaller than the tolerance
  double dist2 = (x02 - x13)*(x02 - x13) + (y02 - y13)*(y02 - y13);
  if (dist2 > Tolerance4PMs*Tolerance4PMs )
    {
      TMPignore = true;
      message("PM set should be center-symmetric withing the configured tolerance!", this);
      TMPignore = false;
      return false;
    }

  return true;
}

void GainEvaluatorWindowClass::on_pb4add_clicked()
{
   bool ok = GainEvaluatorWindowClass::validate4PMsInput();
   if (!ok) return;

   QList<int> tmp;
   ExtractNumbersFromQString(ui->le4newQuartet->text(), &tmp);
   //checking symmetry
   bool sym;
   double dx02 = (MW->PMs->X(tmp[0]) - MW->PMs->X(tmp[2]));
   double dy02 = (MW->PMs->Y(tmp[0]) - MW->PMs->Y(tmp[2]));
   double dx13 = (MW->PMs->X(tmp[1]) - MW->PMs->X(tmp[3]));
   double dy13 = (MW->PMs->Y(tmp[1]) - MW->PMs->Y(tmp[3]));
   double d02 = dx02*dx02 + dy02*dy02;
   double d13 = dx13*dx13 + dy13*dy13;
   if (fabs(d02-d13) < Tolerance4PMs*Tolerance4PMs ) sym = true;
   else sym = false;

   iSets4PMs << FourPMs(tmp, sym);
   GainEvaluatorWindowClass::on_pbUpdate4_clicked();
}

void GainEvaluatorWindowClass::on_pb4remove_clicked()
{
  int i = ui->tabwid4sets->currentRow();
  if (i < 0 || i > iSets4PMs.size()-1) return;

  iSets4PMs.remove(i);
  GainEvaluatorWindowClass::on_pbUpdate4_clicked();
}

void GainEvaluatorWindowClass::on_sb4bins_valueChanged(int arg1)
{
  Bins4PMs = arg1;
}

void GainEvaluatorWindowClass::on_led4overlap_editingFinished()
{
  if (TMPignore) return;

  double val = ui->led4overlap->text().toDouble();

  if (val<0 || val >100)
    {
      TMPignore = true;
      message("Input error!", this);
      ui->led4overlap->focusWidget();
      ui->led4overlap->selectAll();
      TMPignore = false;
    }

   Fraction4PMs = 0.01 * ui->led4overlap->text().toDouble();
}

void GainEvaluatorWindowClass::on_cob4segments_currentIndexChanged(int)
{
   Overlap4PMs = ui->cob4segments->currentText();
}

void GainEvaluatorWindowClass::on_led4tolerance_editingFinished()
{
   Tolerance4PMs = ui->led4tolerance->text().toDouble();
}

void GainEvaluatorWindowClass::on_pbUpdateGraphics_clicked()
{
  GainEvaluatorWindowClass::UpdateGraphics();
}

void GainEvaluatorWindowClass::UpdateGraphics()
{   
    scene->clear(); //deletes all PMicons
    PMicons.clear();

    //============================ drawing PMs ===================================
    for (int ipm=0; ipm<MW->PMs->count(); ipm++)
      {


        const APm &PM = MW->PMs->at(ipm);
        //if (PM.isStaticPassive()) continue;
        //if (igroup != -1) if ( PM.group != igroup ) continue; //wrong group

        //PM object pen
        QPen pen(Qt::black);
        int size = 6.0 * MW->PMs->SizeX(ipm) / 30.0;
        pen.setWidth(size);

        //PM object brush
        QBrush brush(Qt::white); //default color

        bool fVisible;
        //if (PM.isStaticPassive() || (igroup != -1) && ( PM.group != igroup ) )
        if (MW->Detector->PMgroups->isStaticPassive(ipm) || !MW->Detector->PMgroups->isPmBelongsToGroup(ipm, CurrentGroup) )
          {
             fVisible = false;

             brush.setColor(Qt::white);
             pen.setColor(Qt::white);             
          }
        else
          {
            fVisible = true;

            if (ui->twAlgorithms->currentIndex() == 0 && ui->cbActivateCutOffs->isChecked())
              {
                //cutoffs tab
                if (iCutOffPMs.contains(ipm)) brush.setColor(QColor(118, 191, 235));  //rgb
              }
            else if (ui->twAlgorithms->currentIndex() == 1 && ui->cbCentersActivate->isChecked())
              {
                //centers tab
                if (iCentersPMs.contains(ipm)) brush.setColor(QColor(118, 191, 235));

                bool neighbour = false;
                for (int igroup = 0; igroup<CenterGroups.size(); igroup++)
                  {
                    if ( SetNeighbour[igroup].contains(ipm) )
                      {
                        neighbour = true;
                        break;
                      }
                  }
                if (neighbour) pen.setColor(Qt::red);
              }
            else if (ui->twAlgorithms->currentIndex() == 4 && ui->cbActivateLogR->isChecked())
              {
                //Log
                if (iLogPMs.contains(ipm)) brush.setColor(QColor(118, 191, 235));
              }

            //color override
            if (flagShowNonLinkedSet)
              {
                //user pressed "Show first non-linked set"
                if (firstUncoveredSet.contains(ipm)) brush.setColor(Qt::red);
              }            
            if (flagShowingLinked)
              {
               //NOT IMPLEMENTED YET - menu item?

               // brush.setColor(Qt::white);
               // int mainipm = ui->sbPMforLinked->value();

               // if (ipm == mainipm) brush.setColor(Qt::blue);
               // else if (AllLinks[mainipm].contains(ipm)) brush.setColor(Qt::green);
              }
          }


        QGraphicsItem* tmp;
        if (MW->PMs->getType(PM.type)->Shape == 0)
          {
            double sizex = MW->PMs->getType(PM.type)->SizeX*GVscale;
            double sizey = MW->PMs->getType(PM.type)->SizeY*GVscale;
            tmp = scene->addRect(-0.5*sizex, -0.5*sizey, sizex, sizey, pen, brush);
          }
        else if (MW->PMs->getType(PM.type)->Shape == 1)
          {
            double diameter = MW->PMs->getType(PM.type)->SizeX*GVscale;
            tmp = scene->addEllipse( -0.5*diameter, -0.5*diameter, diameter, diameter, pen, brush);
          }
        else
          {
            double radius = 0.5*MW->PMs->getType(PM.type)->SizeX*GVscale;
            QPolygon polygon;
            for (int j=0; j<7; j++)
              {
                double angle = 3.1415926535/3.0 * j + 3.1415926535/2.0;
                double x = radius*cos(angle);
                double y = radius*sin(angle);
                polygon << QPoint(x, y);
              }
            tmp = scene->addPolygon(polygon, pen, brush);
          }

        tmp->setZValue(PM.z);

        tmp->setRotation(-PM.psi);
        tmp->setTransform(QTransform().translate(PM.x*GVscale, -PM.y*GVscale)); //minus!!!!

        //if (PM.isStaticPassive() || (igroup != -1) && ( PM.group != igroup ) ) ;//unclickable!
        if (MW->Detector->PMgroups->isStaticPassive(ipm) || !MW->Detector->PMgroups->isPmBelongsToGroup(ipm, CurrentGroup) ) ;//unclickable!
        else tmp->setFlag(QGraphicsItem::ItemIsSelectable);

        tmp->setCursor(Qt::PointingHandCursor);

        tmp->setVisible(fVisible);

        //tmp->setOpacity(0.75);
        PMicons.append(tmp);

      }
    //qDebug()<<"PM objects set, number="<<PMicons.size();
    //for (int i=0; i<PMicons.size(); i++) qDebug() << PMicons.at(i);

    //============================ drawing 4-PM sets =================================
    if (ui->twAlgorithms->currentIndex() == 2 && ui->cb4Activate->isChecked())
      {
        QPen PolyPen(Qt::blue);
        QBrush PolyBrush(Qt::transparent);

        for (int iset = 0; iset<iSets4PMs.size(); iset++)
          {
            //cycle by the groups of 4 pms
            QVector<QPointF> points;
            for (int i=0; i<4; i++)
              {
                int ipm = iSets4PMs[iset].PM[i];
                double x = MW->PMs->X(ipm)*GVscale;
                double y = -MW->PMs->Y(ipm)*GVscale;
                QPointF p(x, y);
                points << p;
              }

            QPolygonF poly(points);
            int ipm = iSets4PMs[iset].PM[0]; //use properties of 0th PM in the set

            int size = 6.0 * MW->PMs->SizeX(ipm) / 30.0;
            PolyPen.setWidth(size);
            QGraphicsItem* frame = scene->addPolygon(poly, PolyPen, PolyBrush);
            frame->setZValue(MW->PMs->Z(ipm)+0.001);
          }

        if (SelectedPMsFor4Set.size() > 0)
          {
            //incomplete set for the new 4PM set
            QVector<QPointF> points;
            for (int i=0; i<SelectedPMsFor4Set.size(); i++)
              {
                int ipm = SelectedPMsFor4Set[i];
                double x = MW->PMs->X(ipm)*GVscale;
                double y = -MW->PMs->Y(ipm)*GVscale;
                QPointF p(x, y);
                points << p;
              }

            int ipm = SelectedPMsFor4Set[0]; //use properties of 0th PM in the set
            QPolygonF poly(points);
            PolyPen.setColor(Qt::red);
            int size = 6.0 * MW->PMs->SizeX(ipm) / 30.0;
            PolyPen.setWidth(size);
            QGraphicsItem* frame = scene->addPolygon(poly, PolyPen, PolyBrush);
            frame->setZValue(MW->PMs->Z(ipm)+0.002);
          }
      }

    //======================= PM signal text ===========================
    if (ui->cbGVShow->isChecked())
      {
         for (int ipm=0; ipm<MW->PMs->count(); ipm++)
           {
             if (MW->Detector->PMgroups->isStaticPassive(ipm)) continue; //no static passives!
             //if ( MW->PMs->at(ipm).group != igroup ) continue; //wrong group
             if ( !MW->Detector->PMgroups->isPmBelongsToGroup(ipm, CurrentGroup) ) continue; //wrong group

             QGraphicsTextItem * io = new QGraphicsTextItem();
             double size = 0.5*MW->PMs->getType( MW->PMs->at(ipm).type )->SizeX;
             io->setTextWidth(40);
             io->setScale(0.4*size);

             //preparing text to show             
             QString text;
             if (flagShowNeighbourGroups)
               {
                 //showing groups
                 for (int igroup = 0; igroup < CenterGroups.size(); igroup++)
                   if (SetNeighbour[igroup].contains(ipm)) text += QString::number(igroup) + ",";
                 if (text.length()>0) text.chop(1);
               }
             else if (ui->cobGVshowWhat->currentIndex() == 0)
               {
                 //showing PM #
                 text = QString::number(ipm);
               }
             else if (ui->cobGVshowWhat->currentIndex() == 1)
               {
                 //showing gain
                 double gain = MW->Detector->PMgroups->getGain(ipm, CurrentGroup);
                 text = QString::number(gain, 'g', 3);
               }
             text = "<CENTER>" + text + "</CENTER>";

             io->setHtml(text);
             double x = ( MW->PMs->X(ipm) - 0.75*size  )*GVscale;
             double y = (-MW->PMs->Y(ipm) - 0.5*size  ) *GVscale; //minus y to invert the scale!!!
             io->setPos(x, y);

             io->setZValue(MW->PMs->Z(ipm)+0.01);

             scene->addItem(io);
           }
      }

  //  qDebug()<<"ready to update Gr View";
  //  gvGains->update();

    //scene->blockSignals(false);
    //    qDebug()<<" update of graphics done";
}

void GainEvaluatorWindowClass::resizeEvent(QResizeEvent *)
{
  if (!this->isVisible()) return;
  int Height = this->height();
  int Width = this->width();

  int gvWidth = Width - gvGains->x() - 3;
  int gvHeight = Height - gvGains->y() - 3;
  gvGains->resize(gvWidth, gvHeight);

  ui->fGV->resize(gvWidth, ui->fGV->height());
  GainEvaluatorWindowClass::ResetViewport();
}

bool GainEvaluatorWindowClass::event(QEvent *event)
{   
  if (event->type() == QEvent::WindowActivate)
    {
      //qDebug()<<"window activated";
      GainEvaluatorWindowClass::on_pbUpdateCutOff_clicked();
      GainEvaluatorWindowClass::on_pbUpdateLogR_clicked();
      GainEvaluatorWindowClass::UpdateEvaluator();
    }

  if (MW->WindowNavigator)
    {
      if (event->type() == QEvent::Hide) MW->WindowNavigator->HideWindowTriggered("gain");
      if (event->type() == QEvent::Show) MW->WindowNavigator->ShowWindowTriggered("gain");
    }

  return QMainWindow::event(event);
}

void GainEvaluatorWindowClass::sceneSelectionChanged()
{ 
  if (!scene)   return;
  //qDebug()<<"Scene selection changed!";
  int selectedItems = scene->selectedItems().size();
  //qDebug()<<"  --number of selected items:"<<selectedItems;
  if (selectedItems == 0)
    {
//      qDebug()<<"  --empty - ignoring event";
      return;
    }

  QGraphicsItem* pointer = scene->selectedItems().first();
  if (!pointer)
    {
      //qDebug()<<"  --zero pinter, ignoring event!";
      return;
    }

  int ipm = 0;
  ipm = PMicons.indexOf(pointer);
  if (ipm == -1)
    {
      qDebug()<<" --ipm not found!";
      return;
    }
  //qDebug()<<" -- ipm = "<<ipm;

  //scene->blockSignals(true);
  //pointer->setSelected(false); //unselecting this item to avoid drawing rectangle around it
  //scene->blockSignals(false);

  //using the proper handler
  int index = ui->twAlgorithms->currentIndex();
  if (index == 0 && ui->cbActivateCutOffs->isChecked()) GainEvaluatorWindowClass::cutOffsPMselected(ipm);
  else if (index == 1 && ui->cbCentersActivate->isChecked()) GainEvaluatorWindowClass::centersPMselected(ipm);
  else if (index == 2 && ui->cb4Activate->isChecked()) GainEvaluatorWindowClass::set4PMselected(ipm);
  else if (index == 4 && ui->cbActivateLogR->isChecked()) GainEvaluatorWindowClass::logPMselected(ipm);

  qApp->processEvents();
  //qDebug()<<"scene selection processing complete";
}

void GainEvaluatorWindowClass::cutOffsPMselected(int ipm)
{
  //qDebug()<<"++++cutoffs, ipm selected:"<<ipm;
  //qDebug()<<"old iCutOffPMs:"<<iCutOffPMs;
  bool alreadyThere = iCutOffPMs.contains(ipm);
  //qDebug()<<"++++++ already there?"<<alreadyThere;

  if (alreadyThere)
    {
      //already there, removing
      GainEvaluatorWindowClass::AddRemovePMs('-', QString::number(ipm), &iCutOffPMs);      
    }
  else
    {
      //not in the list
      GainEvaluatorWindowClass::AddRemovePMs('+', QString::number(ipm), &iCutOffPMs);
    }

  //qDebug()<<"new iCutOffPMs:"<<iCutOffPMs;
  scene->blockSignals(true);  
  GainEvaluatorWindowClass::on_pbUpdateCutOff_clicked();
  scene->blockSignals(false);
}

void GainEvaluatorWindowClass::centersPMselected(int ipm)
{
//  qDebug()<<"++++centers"<<ipm;
  bool alreadyThere = iCentersPMs.contains(ipm);

  if (alreadyThere)
    {
      //already there, removing
      GainEvaluatorWindowClass::AddRemovePMs('-', QString::number(ipm), &iCentersPMs);
    }
  else
    {
      //not in the list
      GainEvaluatorWindowClass::AddRemovePMs('+', QString::number(ipm), &iCentersPMs);
    }

  GainEvaluatorWindowClass::on_pbUpdateCenters_clicked();
}

void GainEvaluatorWindowClass::logPMselected(int ipm)
{
//  qDebug()<<"++++log"<<ipm;
  bool alreadyThere = iLogPMs.contains(ipm);

  if (alreadyThere)
    {
      //already there, removing
      GainEvaluatorWindowClass::AddRemovePMs('-', QString::number(ipm), &iLogPMs);
    }
  else
    {
      //not in the list
      GainEvaluatorWindowClass::AddRemovePMs('+', QString::number(ipm), &iLogPMs);
    }

  GainEvaluatorWindowClass::on_pbUpdateLogR_clicked();
}

void GainEvaluatorWindowClass::set4PMselected(int ipm)
{
//  qDebug()<<"++++4PMs"<<ipm;
  SelectedPMsFor4Set << ipm;

  int size = SelectedPMsFor4Set.size();
//  qDebug()<<"tmp set size:"<<size;

  if (size>3)
    {
      //set complete
      QString str = QString::number(SelectedPMsFor4Set[0])+","+QString::number(SelectedPMsFor4Set[1])+","+QString::number(SelectedPMsFor4Set[2])+","+QString::number(SelectedPMsFor4Set[3]);
      ui->le4newQuartet->setText(str);
      GainEvaluatorWindowClass::on_pb4add_clicked();
      SelectedPMsFor4Set.clear();
    }

//  qDebug()<<SelectedPMsFor4Set;
  GainEvaluatorWindowClass::on_pbUpdate4_clicked();
}

void GainEvaluatorWindowClass::ResetViewport()
{
  if (MW->PMs->count() == 0) return;

  //calculating viewing area
  double Xmin =  1e10;
  double Xmax = -1e10;
  double Ymin =  1e10;
  double Ymax = -1e10;
  for (int ipm=0; ipm<MW->PMs->count(); ipm++)
    {
      double x = MW->PMs->at(ipm).x;
      double y = MW->PMs->at(ipm).y;
      int type = MW->PMs->at(ipm).type;
      double size = MW->PMs->getType(type)->SizeX;

      if (x-size < Xmin) Xmin = x-size;
      if (x+size > Xmax) Xmax = x+size;
      if (y-size < Ymin) Ymin = y-size;
      if (y+size > Ymax) Ymax = y+size;
    }

  double Xdelta = Xmax-Xmin;
  double Ydelta = Ymax-Ymin;

  scene->setSceneRect((Xmin - 0.5*Xdelta)*GVscale, (Ymin - 0.5*Ydelta)*GVscale, (Xmax-Xmin + 1.0*Xdelta)*GVscale, (Ymax-Ymin + 1.0*Ydelta)*GVscale);
  gvGains->fitInView( (Xmin - 0.01*Xdelta)*GVscale, (Ymin - 0.01*Ydelta)*GVscale, (Xmax-Xmin + 0.02*Xdelta)*GVscale, (Ymax-Ymin + 0.02*Ydelta)*GVscale, Qt::KeepAspectRatio);
}

void GainEvaluatorWindowClass::on_twAlgorithms_currentChanged(int index)
{
  SelectedPMsFor4Set.clear(); //clear input for new 4Pm set

  if (index == 1) ui->pbtShowNeighbourGroups->setEnabled(true);
  else ui->pbtShowNeighbourGroups->setEnabled(false);
}

void GainEvaluatorWindowClass::on_pbtShowNeighbourGroups_pressed()
{
  flagShowNeighbourGroups = true;
  GainEvaluatorWindowClass::UpdateGraphics();
}

void GainEvaluatorWindowClass::on_pbtShowNeighbourGroups_released()
{
  flagShowNeighbourGroups = false;
  GainEvaluatorWindowClass::UpdateGraphics();
}

void GainEvaluatorWindowClass::on_pb4removeAll_clicked()
{
  iSets4PMs.clear();
  GainEvaluatorWindowClass::on_pbUpdate4_clicked();
}

void GainEvaluatorWindowClass::on_pbEvaluateRatio_clicked()
{
  if (EventsDataHub->isEmpty()) return;

  int ipm1 = ui->sbLogRatioPM1->value();
  int ipm2 = ui->sbLogRatioPM2->value();
  if (ipm1>MW->PMs->count()-1) return;
  if (ipm2>MW->PMs->count()-1) return;

  auto hist1D = new TH1D("GainRationEst","LogStat",ui->sbLogRatioBins->value(),0,0);

  for (int iev = 0; iev<EventsDataHub->Events.size(); iev++)
    {
      double sig1 = EventsDataHub->Events[iev][ipm1];
      double sig2 = EventsDataHub->Events[iev][ipm2];
      if (sig1<=0 || sig2<=0) continue;
      double ratio = TMath::Log( sig1/sig2 );
      //qDebug()<<ratio;

      hist1D->Fill(ratio);
    }

  MW->GraphWindow->Draw(hist1D);
}

void GainEvaluatorWindowClass::on_pbCalcAverage_clicked()
{  
  if (EventsDataHub->isEmpty()) return;

  int ipm1 = ui->sbUniformPM1->value();
  int ipm2 = ui->sbUniformPM2->value();
  if (ipm1>MW->PMs->count()-1) return;
  if (ipm2>MW->PMs->count()-1) return;

  if (isPMDistanceCheckFail(ipm1, ipm2))
    {
      qDebug()<<"warning: distance check fail!";
    }

  GainEvaluatorWindowClass::AdjustGainPair(ipm1, ipm2);
}

double GainEvaluatorWindowClass::AdjustGainPair(int ipm1, int ipm2) //----automatic gain determination for pair----
{  
  int numEvents = EventsDataHub->Events.size();

  //define the ratio of areas cut by the line between the two PMs inside the PM_above_noise circles
  double A1OverA2 = CalculateAreaFraction(ipm1, ipm2);
  if (A1OverA2 <= 0) return 0; //error detected

  //find gains properly reproducing the fractions
  double g1 = 1.0;
  double g2 = 1.0;
  bool fDone = false;
  bool fStart = true;
  double shift = 0.02;
  double sign;
  do
    {
      int larger1 = 0;
      int larger2 = 0;
      for (int iev = 0; iev<numEvents; iev++)
        if (EventsDataHub->ReconstructionData[0][iev]->GoodEvent)
        {          
          double sig1 = EventsDataHub->Events[iev][ipm1] / g1;
          double sig2 = EventsDataHub->Events[iev][ipm2] / g2;

          if (ui->cbUniformPoisson->isChecked())
            {
              if (  fabs(sig1-sig2) < 2.0*(TMath::Sqrt(fabs(sig1)) + TMath::Sqrt(fabs(sig2))) ) continue;
            }


          if (sig1 > sig2) larger1++; else larger2++;
        }
      double ratio;
      if (larger2>0) ratio = 1.0*larger1/larger2; else ratio = 1.0;
///      qDebug()<<"-------";
///      qDebug()<<"larger1, larger2:"<<larger1<<larger2<<"ratio:"<<ratio;


      //choosing direction on start
      double delta = ratio - A1OverA2;
//      qDebug()<<"delta:"<<delta;
      if (fStart)
        {
          if (delta > 0) sign = -1.0; //will reduce gain of second PM
          else sign = 1.0;  //will increase gain of the second PM
          fStart = false;
//          qDebug()<<"     selected shift:"<<sign*shift;
        }

      if (fabs(delta)<0.02 ) break; //good precision already
      if (sign == -1 && delta < 0) break; //passed optimum
      if (sign == 1 && delta > 0) break; //passed optimum

      //going next iteration
      g2 += sign*shift;
///      qDebug()<<"new g2:"<<g2;
    }
  while ( !fDone );

  qDebug()<<"ipms:"<<ipm1<<ipm2<<"Area fraction:"<<A1OverA2<<"Relative gains: g1=1, g2="<<g2;
  return g2;
}

double GainEvaluatorWindowClass::CalculateAreaFraction(int ipm1, int ipm2) //returns area1/area2
{
  const double illuminationRadius = 0.5*ui->ledIlluminatedDiameter->text().toDouble();  
  QLineF lineBetweenPMs(MW->PMs->X(ipm1), MW->PMs->Y(ipm1), MW->PMs->X(ipm2), MW->PMs->Y(ipm2));
  QPointF middlePoint = lineBetweenPMs.pointAt(0.5);
///qDebug()<<"-------"<<middlePoint.x()<<middlePoint.y();

  QLineF tmp(middlePoint, lineBetweenPMs.p2());
  QLineF splitLineVector1 = tmp.unitVector().normalVector();
  do splitLineVector1.setLength( splitLineVector1.length() +0.1 );
  while (splitLineVector1.p2().x()*splitLineVector1.p2().x() + splitLineVector1.p2().y()*splitLineVector1.p2().y() < illuminationRadius*illuminationRadius);
  QPointF FirstPointOnCircle = splitLineVector1.p2();

  QLineF splitLineVector2 = tmp.unitVector().normalVector();
  splitLineVector2.setP2( QPointF(splitLineVector2.x1() - splitLineVector2.dx(),  splitLineVector2.y1() - splitLineVector2.dy()) );
  do splitLineVector2.setLength( splitLineVector2.length() +0.1 );
  while (splitLineVector2.p2().x()*splitLineVector2.p2().x() + splitLineVector2.p2().y()*splitLineVector2.p2().y() < illuminationRadius*illuminationRadius);
  QPointF SecondPointOnCircle = splitLineVector2.p2();

///  MW->AddLineToGeometry(FirstPointOnCircle, SecondPointOnCircle, 1, 3);

  //creating polygons - circles around PMs 1 and 2
  double PMaboveNoiseRadius = ui->ledPMaboveNoiseRadius->text().toDouble();
  QPolygonF circ1, circ2;
  const int numSectors = 24; //polygon sectors
  for (int i=0; i<numSectors; i++)
    {
      double angle = 2.0*3.1415926/numSectors * i;
      double rcos = cos(angle)*PMaboveNoiseRadius;
      double rsin = sin(angle)*PMaboveNoiseRadius;
      QPointF p1(MW->PMs->X(ipm1) + rcos, MW->PMs->Y(ipm1) + rsin );
      QPointF p2(MW->PMs->X(ipm2) + rcos, MW->PMs->Y(ipm2) + rsin );
      circ1 << p1;
      circ2 << p2;
    }
  circ1<< circ1.first(); //making closed
  circ2<< circ2.first();
///  qDebug()<<"\n\nCircles around ipm1 and ipm2:"<<circ1<<"\n\n"<<circ2;
///  MW->AddPolygonfToGeometry(circ1, 3, 2);
///  MW->AddPolygonfToGeometry(circ2, 4, 2);

  //creating polygons for illuminated area split by the middle line
  QPolygonF illum1, illum2; //on start dont know which one belongs to which pm!
    //starting/finishing angles?
  QLineF radius1(QPointF(0,0), FirstPointOnCircle);
  double startAngle = radius1.angle(); //in degrees!
  QLineF radius2(QPointF(0,0), SecondPointOnCircle);
  double endAngle = radius2.angle(); //in degrees!
  if (endAngle<startAngle) std::swap(startAngle, endAngle);
///  qDebug()<<"\n\nStart and end angles:"<<startAngle<<endAngle;
///  MW->AddLineToGeometry(QPointF(0,0), FirstPointOnCircle);
///  MW->AddLineToGeometry(QPointF(0,0), SecondPointOnCircle);
    //filling polygons
     //finding angular step per sector
  startAngle *= 3.1415926/180.0;
  endAngle   *= 3.1415926/180.0;
  double illum1Step = (endAngle-startAngle) / numSectors;
  double illum2Step = (2.0*3.1415926 - (endAngle-startAngle)) / numSectors;
     //doing numSectors steps
  for (int i=0; i<numSectors+1; i++) //+1 here!
    {
      double angle1 = startAngle + illum1Step*i;     
      illum1 << QPointF(illuminationRadius*cos(-angle1), illuminationRadius*sin(-angle1));
      double angle2 = endAngle + illum2Step*i;
      illum2 << QPointF(illuminationRadius*cos(-angle2), illuminationRadius*sin(-angle2));
    }
  illum1<< illum1.first(); //making closed
  illum2<< illum2.first();
     //checking and swapping illum1<->illum2 if needed
  if (illum2.containsPoint( QPointF(MW->PMs->X(ipm1), MW->PMs->Y(ipm1) ), Qt::OddEvenFill))
    {
        //have to swap!
///      qDebug()<<"swapping illum1 and illum2!";
      illum1.swap(illum2);
    }
///  qDebug()<<"\n\nIllum1 and illum2:"<<illum1<<"\n\n"<<illum2;
///  MW->AddPolygonfToGeometry(illum1, 3, 2);
///  MW->AddPolygonfToGeometry(illum2, 4, 2);

    //overlaping polygons
  circ1 = circ1.intersected(illum1);
  circ2 = circ2.intersected(illum2);
///  qDebug()<<"\n\nIntersected results for ipm1 and ipm2:"<<circ1<<"\n\n"<<circ2;
    //calculating areas
  double area1 = GainEvaluatorWindowClass::getPolygonArea(circ1);
  double area2 = GainEvaluatorWindowClass::getPolygonArea(circ2);
///  qDebug()<<"\n------\nArea1 and Area2:"<<area1<<area2;

  if (area1 == 0 || area2 == 0)
    {
      qCritical()<<"Error in area calculation: zero area reported!";
///      qDebug()<<"ipms:"<<ipm1<<ipm2;
      return 0;
    }

///  MW->ShowGeometry();
///  MW->GeoManager->DrawTracks();
///  MW->GeometryWindow->UpdateRootCanvas();
///  MW->GeometryWindow->raise();

  return area1/area2;
}

double GainEvaluatorWindowClass::getPolygonArea(QPolygonF &p)
{
  if (p.size()<3) return 0;

  double area = 0;
  for (int i=0; i<p.size()-1; i++)
    area += p[i].x()*p[i+1].y() - p[i+1].x()*p[i].y();

  return 0.5*area;
}

double GainEvaluatorWindowClass::AdjustGainPairLogR(int ipm1, int ipm2)
{
  if (tmpHistForLogR) delete tmpHistForLogR;
  tmpHistForLogR = new TH1D("gr","",ui->sbLogRatioBins->value(),0,0);

  for (int iev = 0; iev<EventsDataHub->Events.size(); iev++)
    {
      double sig1 = EventsDataHub->Events[iev][ipm1];
      double sig2 = EventsDataHub->Events[iev][ipm2];
      if (sig1<=0 || sig2<=0) continue;
      double ratio = TMath::Log( sig2/sig1 );

      tmpHistForLogR->Fill(ratio);
    }

  double av = tmpHistForLogR->GetMean();
  qDebug()<<"av:"<<av;

  return TMath::Exp(av);
}

void GainEvaluatorWindowClass::on_pbUpdateUniform_clicked()
{
   ui->fFromUniform->setEnabled(ui->cbFromUniform->isChecked());
   GainEvaluatorWindowClass::UpdateEvaluator();

   if (!ui->cbFromUniform->isChecked()) ui->twAlgorithms->tabBar()->setTabIcon(3, QIcon());
   else
     {
       //if (iSets4PMs.isEmpty()) ui->twAlgorithms->tabBar()->setTabIcon(2, RedIcon); else
       ui->twAlgorithms->tabBar()->setTabIcon(3, GreenIcon);
     }
}

void GainEvaluatorWindowClass::on_pbUpdateLogR_clicked()
{
  ui->fLogR->setEnabled(ui->cbActivateLogR->isChecked());

  if (ui->cobLogMethodSelection->currentIndex() == 1)
    {
      //updating triads
      GainEvaluatorWindowClass::UpdateTriads();
      Triads.find_triads();
//      qDebug() << "Found " << Triads.getTriads()->size() << " triads";
//      unsigned int triadsFound = Triads.getTriads()->size();
//      for (unsigned int i=0; i<triadsFound; i++)
//          qDebug() << i << "> " << (*Triads.getTriads())[i][0] << ", " << (*Triads.getTriads())[i][1] << ", " << (*Triads.getTriads())[i][2];
    }
  GainEvaluatorWindowClass::UpdateEvaluator();

  if (!ui->cbActivateLogR->isChecked()) ui->twAlgorithms->tabBar()->setTabIcon(4, QIcon());
  else ui->twAlgorithms->tabBar()->setTabIcon(4, GreenIcon);
  ui->labPMsLog->setText(QString::number(iLogPMs.size()));
}

void GainEvaluatorWindowClass::UpdateTriads()
{
  std::vector <double> xx, yy;
  std::vector <int> list;

  for (int i=0; i<iLogPMs.size(); i++)
    {
      int ipm = iLogPMs[i];
      float x = MW->PMs->X(ipm);
      float y = MW->PMs->Y(ipm);

      xx.push_back(x);
      yy.push_back(y);
      list.push_back(ipm);
    }

  double Rmax = ui->ledLogRmax->text().toDouble();
  Triads.Init(xx, yy, Rmax, 0);

  Triads.SetEvents(list, &EventsDataHub->Events);
}

void GainEvaluatorWindowClass::SetWindowFont(int ptsize)
{
    QFont font = this->font();
    font.setPointSize(ptsize);
    this->setFont(font);
}

void GainEvaluatorWindowClass::on_pbShowNonLinkedSet_clicked()
{
   flagShowNonLinkedSet = true;
   GainEvaluatorWindowClass::UpdateGraphics();
   flagShowNonLinkedSet = false;
}

void GainEvaluatorWindowClass::on_pbLogAddAll_clicked()
{
  iLogPMs = iPMs;
  GainEvaluatorWindowClass::on_pbUpdateLogR_clicked();
}

void GainEvaluatorWindowClass::on_pbLogAdd_clicked()
{
  QString input = ui->leLogSet->text();
  GainEvaluatorWindowClass::AddRemovePMs('+', input, &iLogPMs);
  GainEvaluatorWindowClass::on_pbUpdateLogR_clicked();
}

void GainEvaluatorWindowClass::on_pbLogRemoveAll_clicked()
{
  iLogPMs.clear();
  GainEvaluatorWindowClass::on_pbUpdateLogR_clicked();
}

void GainEvaluatorWindowClass::on_pbLogRemove_clicked()
{
  QString input = ui->leLogSet->text();
  GainEvaluatorWindowClass::AddRemovePMs('-', input, &iLogPMs);
  GainEvaluatorWindowClass::on_pbUpdateLogR_clicked();
}
