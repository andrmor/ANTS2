#include "viewer2darrayobject.h"
#include "apmhub.h"
#include "apmtype.h"

#include <QGraphicsItem>
#include <QDebug>
#include <math.h>

Viewer2DarrayObject::Viewer2DarrayObject(myQGraphicsView *GV, APmHub* PM_module)
//  QObject()
{
  gv = GV;
  PMs = PM_module;
  GVscale = 1.0;
  CursorMode = 0;
  scene = new QGraphicsScene(this);
  connect(scene, SIGNAL(selectionChanged()), this, SLOT(sceneSelectionChanged()));

  gv->setScene(scene);
  gv->setDragMode(QGraphicsView::ScrollHandDrag); //if zoomed, can use hand to center needed area
  //gvOut->setTransformationAnchor(QGraphicsView::AnchorViewCenter);
  gv->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
  gv->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  gv->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  gv->setRenderHints(QPainter::Antialiasing);

  Viewer2DarrayObject::ResetViewport();
}

Viewer2DarrayObject::~Viewer2DarrayObject()
{
  disconnect(scene, SIGNAL(selectionChanged()), this, SLOT(sceneSelectionChanged()));

  if (PMicons.size() > 0)
     for (int i=0; i<PMicons.size(); i++) scene->removeItem(PMicons[i]);
  PMicons.resize(0);
  scene->clear();
  delete scene;
  scene = 0;
}

void Viewer2DarrayObject::DrawAll()
{
      if (CursorMode == 1) gv->setCursor(Qt::CrossCursor);
      int numPMs = PMs->count();
//      qDebug()<<"Starting update of graphics";
      //scene->blockSignals(true);

      if (PMicons.size() > 0)
         for (int i=0; i<PMicons.size(); i++) scene->removeItem(PMicons[i]);
      PMicons.resize(0);
      scene->clear();
//      qDebug()<<"Scene cleared";

      PMprops.resize(numPMs);
      //asserted size of PM properies vector

      //============================ drawing PMs ===================================
      for (int ipm=0; ipm<numPMs; ipm++)
        {
          const APm &PM = PMs->at(ipm);

          //PM object pen
          QPen pen(PMprops[ipm].pen);
          int size = 6.0 * PMs->SizeX(ipm) / 300.0;
          pen.setWidth(size);

          //PM object brush
          QBrush brush(PMprops[ipm].brush);

          QGraphicsItem* tmp;
          if (PMs->getType(PM.type)->Shape == 0)
            {
              double sizex = PMs->getType(PM.type)->SizeX*GVscale;
              double sizey = PMs->getType(PM.type)->SizeY*GVscale;
              tmp = scene->addRect(-0.5*sizex, -0.5*sizey, sizex, sizey, pen, brush);
            }
          else if (PMs->getType(PM.type)->Shape == 1)
            {
              double diameter = PMs->getType(PM.type)->SizeX*GVscale;
              tmp = scene->addEllipse( -0.5*diameter, -0.5*diameter, diameter, diameter, pen, brush);
            }
          else
            {
              double radius = 0.5*PMs->getType(PM.type)->SizeX*GVscale;
              QPolygon polygon;
              for (int j=0; j<7; j++)
                {
                  double angle = 3.1415926535/3.0 * j + 3.1415926535/2.0;
                  double x = radius * cos(angle);
                  double y = radius * sin(angle);
                  polygon << QPoint(x, y);
                }
              tmp = scene->addPolygon(polygon, pen, brush);
            }

          tmp->setZValue(PM.z);
          tmp->setVisible(PMprops.at(ipm).visible);

          tmp->setRotation(-PM.psi);
      //    tmp->setTransform(QTransform().translate(PM.x*GVscale, -PM.y*GVscale)); //minus!!!!
          tmp->setPos(PM.x*GVscale, -PM.y*GVscale); //minus!!!!

          tmp->setFlag(QGraphicsItem::ItemIsSelectable);
          if      (CursorMode == 0) tmp->setCursor(Qt::PointingHandCursor);
          else if (CursorMode == 1) tmp->setCursor(Qt::CrossCursor);

          PMicons.append(tmp);
        }
//      qDebug()<<"PM objects set, number="<<PMicons.size();


      //======================= PM signal text ===========================
      if (true)
        {
           for (int ipm=0; ipm<numPMs; ipm++)
             {
               QGraphicsTextItem * io = new QGraphicsTextItem();
               double size = 0.5*PMs->getType( PMs->at(ipm).type )->SizeX;
               io->setTextWidth(40);
               io->setScale(0.04*size);

               //preparing text to show
               QString text = PMprops[ipm].text;

               text = "<CENTER>" + text + "</CENTER>";
               io->setDefaultTextColor(PMprops[ipm].textColor);
               io->setHtml(text);
               double x = ( PMs->X(ipm) - 0.75*size  )*GVscale;
               double y = (-PMs->Y(ipm) - 0.5*size  ) *GVscale; //minus y to invert the scale!!!
               io->setPos(x, y);

               io->setZValue(PMs->Z(ipm)+0.01);
               io->setVisible(PMprops.at(ipm).visible);

               scene->addItem(io);
             }
        }

//      qDebug()<<" update of graphics done";
}

void Viewer2DarrayObject::ResetViewport()
{
   if (PMs->count() == 0) return;

   //calculating viewing area
   double Xmin =  1e10;
   double Xmax = -1e10;
   double Ymin =  1e10;
   double Ymax = -1e10;
   for (int ipm=0; ipm<PMs->count(); ipm++)
      {
        double x = PMs->at(ipm).x;
        double y = PMs->at(ipm).y;
        int type = PMs->at(ipm).type;
        double size = PMs->getType(type)->SizeX;

        if (x-size < Xmin) Xmin = x-size;
        if (x+size > Xmax) Xmax = x+size;
        if (y-size < Ymin) Ymin = y-size;
        if (y+size > Ymax) Ymax = y+size;
      }

    double Xdelta = Xmax-Xmin;
    double Ydelta = Ymax-Ymin;

    scene->setSceneRect((Xmin - 0.1*Xdelta)*GVscale, (Ymin - 0.1*Ydelta)*GVscale, (Xmax-Xmin + 0.2*Xdelta)*GVscale, (Ymax-Ymin + 0.2*Ydelta)*GVscale);
    gv->fitInView( (Xmin - 0.01*Xdelta)*GVscale, (Ymin - 0.01*Ydelta)*GVscale, (Xmax-Xmin + 0.02*Xdelta)*GVscale, (Ymax-Ymin + 0.02*Ydelta)*GVscale, Qt::KeepAspectRatio);
}

void Viewer2DarrayObject::ClearColors()
{
  PMprops.resize(0);
  PMprops.resize(PMs->count());
}

void Viewer2DarrayObject::SetPenColor(int ipm, QColor color)
{
  if (ipm<0 || ipm>=PMs->count()) return;

  if (PMprops.size() < PMs->count()) PMprops.resize(PMs->count());
  PMprops[ipm].pen = color;
}

void Viewer2DarrayObject::SetBrushColor(int ipm, QColor color)
{
   if (ipm<0 || ipm>=PMs->count()) return;

   if (PMprops.size() < PMs->count()) PMprops.resize(PMs->count());
   PMprops[ipm].brush = color;
}

void Viewer2DarrayObject::SetText(int ipm, QString text)
{
   if (ipm<0 || ipm>=PMs->count()) return;

   if (PMprops.size() < PMs->count()) PMprops.resize(PMs->count());
   PMprops[ipm].text = text;
}

void Viewer2DarrayObject::SetTextColor(int ipm, QColor color)
{
   if (ipm<0 || ipm>=PMs->count()) return;

   if (PMprops.size() < PMs->count()) PMprops.resize(PMs->count());
   PMprops[ipm].textColor = color;
}

void Viewer2DarrayObject::SetVisible(int ipm, bool fFlag)
{
  if (ipm<0 || ipm>=PMs->count()) return;

  if (PMprops.size() < PMs->count()) PMprops.resize(PMs->count());
  PMprops[ipm].visible = fFlag;
}

void Viewer2DarrayObject::forceResize()
{
  qDebug()<<"resize!";
  Viewer2DarrayObject::ResetViewport();
}

void Viewer2DarrayObject::sceneSelectionChanged()
{
//  qDebug()<<"Scene selection changed!";
  int selectedItems = scene->selectedItems().size();
//  qDebug()<<"  --number of selected items:"<<selectedItems;
  if (selectedItems == 0)
    {
//      qDebug()<<"  --empty - ignoring event";
      return;
    }

  QGraphicsItem* pointer = scene->selectedItems().first();
  if (pointer == 0)
    {
//      qDebug()<<"  --zero pinter, ignoring event!";
      return;
    }

  int ipm = 0;
  ipm = PMicons.indexOf(pointer);
  if (ipm == -1)
    {
//      qDebug()<<" --ipm not found!";
      return;
    }
//  qDebug()<<" -- ipm = "<<ipm;

  QVector<int> result;
  for (int i=0; i<scene->selectedItems().size(); i++)
    {
      QGraphicsItem* pointer = scene->selectedItems().at(i);
      if (pointer == 0) continue;
      int ipm = 0;
      ipm = PMicons.indexOf(pointer);
      if (ipm == -1) continue;
      result.append(ipm);
    }
  emit PMselectionChanged(result);

//  qDebug()<<"scene selection processing complete";
}


