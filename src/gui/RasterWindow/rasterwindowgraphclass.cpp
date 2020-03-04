#include "TCanvas.h"

#include "rasterwindowgraphclass.h"

#include <QDebug>
#include <QMouseEvent>
#include <QMainWindow>

#include "TMath.h"
#include "TView.h"
#include "TLine.h"
#include "TEllipse.h"
#include "TBox.h"
#include "TPolyLine.h"

RasterWindowGraphClass::RasterWindowGraphClass(QMainWindow *MasterWindow) :
    RasterWindowBaseClass(MasterWindow) {}

RasterWindowGraphClass::~RasterWindowGraphClass()
{ 
  //qDebug()<< "     <--Starting cleanup for graph raster window...";

  if (VertLine1) delete VertLine1;
  if (Line2D) delete Line2D;
  if (Ellipse) delete Ellipse;
  if (Box2D) delete Box2D;
  if (Polygon) delete Polygon;

  //qDebug()<<"    ...delegating delete to base class";
}

/*
void RasterWindowGraphClass::setShowCursorPosition(bool flag)
{
  ShowCursorPosition = flag;
  if (!flag) MasterWindow->setWindowTitle("Graph");
}
*/

bool RasterWindowGraphClass::event(QEvent *event)
{
  switch (event->type())
    {
    case QEvent::Close :
        //qDebug() <<  "       --Close Root window signal detected: hiding the window";


        ExtractionOfXStarted = false;
        ExtractionComplete = true;
     //   LastDistributionShown = "";
        this->hide();
        event->ignore();
        return true;

/*         ***!!!
    case QEvent::UpdateRequest :
   //     qDebug()<<"canvas update request received";
        if (isExposed())
          if (fCanvas) fCanvas->Update();
        return true;
*/

   /*
    case QEvent::WindowStateChange :
        break;
   */
    default:
        break;
    }
  return QWidget::event(event);
}

void RasterWindowGraphClass::mousePressEvent(QMouseEvent *event)
{
  //    qDebug()<<"graph raster -> Mouse press event!";
  //    qDebug()<<"X from event"<<event->x()<<"   Xreported"<<fCanvas->AbsPixeltoX(event->x());
  //    qDebug()<<"Y from event"<<event->y()<<"   Yreported"<<fCanvas->AbsPixeltoY(event->y());

  if (!fCanvas) return;
  fCanvas->cd();

  double x = fCanvas->AbsPixeltoX(event->x());
  double y = fCanvas->AbsPixeltoY(event->y());
  if (fCanvas->GetLogx()) x = TMath::Power(10.0, x);
  if (fCanvas->GetLogy()) y = TMath::Power(10.0, y);

  if (event->button() == Qt::LeftButton)
    {
      //LEFT button pressed
      if (ExtractionOfXPending)
        {
           ExtractionOfXPending = false;
           ExtractionOfXStarted = true;
           extractedX = x;
           RasterWindowGraphClass::DrawVerticalLine();
           return;
        }

      if (ExtractionOf2DLinePending)
        {
           ExtractionOf2DLinePending = false;
           ExtractionOf2DLineStarted = true;
           Line2DstartX = x;
           Line2DstopX  = Line2DstartX;
           Line2DstartY = y;
           Line2DstopY  = Line2DstartY;
           RasterWindowGraphClass::Draw2DLine();
           return;
        }

      if (ExtractionOf2DBoxPending)
        {
           ExtractionOf2DBoxPending = false;
           ExtractionOf2DBoxStarted = true;
           extractedX1 = x;
           extractedX2  = extractedX1;
           extractedY1 = y;
           extractedY2  = extractedY1;
           RasterWindowGraphClass::Draw2DBox();
           return;
        }

      if (ExtractionOf2DEllipsePending)
        {
           //first will do a line to extract the angle
           ExtractionOf2DEllipsePending = false;

           Line2DstartX = x;
           Line2DstopX  = Line2DstartX;
           Line2DstartY = y;
           Line2DstopY  = Line2DstartY;
           RasterWindowGraphClass::Draw2DLine();

           extracted2DEllipseX = Line2DstartX;
           extracted2DEllipseY = Line2DstartY;

           ExtractionOf2DEllipsePhase = 1; // center is selected, dragging line to show angle
           return;
        }

      if (ExtractionOf2DEllipsePhase == 2)
        {
           //Waiting for release to stop Ellipse extraction
           ExtractionOf2DEllipsePhase = 3; //waiting for release
           return;
        }

      if (ExtractionOf2DPolygonPending)
        {
           ExtractionOf2DPolygonPending = false;
           ExtractionOf2DPolygonStarted = true;
           extractedPolygon.clear();
           extractedPolygon << x;  //start
           extractedPolygon << y;
           extractedPolygon << x;  //current point we move by shifting mouse
           extractedPolygon << y;
           extractedPolygon << x;  //end - repeats start
           extractedPolygon << y;

           RasterWindowGraphClass::Draw2DPolygon(); //empty one

           for (int i=0; i<extractedPolygon.size()/2; i++)
              Polygon->SetPoint(i, extractedPolygon[i*2], extractedPolygon[i*2+1]);

           Polygon->Draw();
           fCanvas->Update();
           return;
        }

      if (ExtractionOf2DPolygonStarted)
        {
          //adding new node to the polygon
          int size = extractedPolygon.size(); //end point: size-2, size-1
          extractedPolygon.insert(size-4, x);
          extractedPolygon.insert(size-3, y);

          for (int i=0; i<extractedPolygon.size()/2; i++)
             Polygon->SetPoint(i, extractedPolygon[i*2], extractedPolygon[i*2+1]);

          Polygon->Paint();
          fCanvas->Update();
          return;
        }
    }


  if (event->button() == Qt::RightButton)
    {
      //RIGHT button pressed
      if (ExtractionOf2DPolygonStarted)
        {
          //settingthe last point of the polygon
          ExtractionOf2DPolygonStarted = false;
          int size = extractedPolygon.size(); //end point: size-2, size-1
          extractedPolygon[size-4] = x;
          extractedPolygon[size-3] = y;

          for (int i=0; i<(size-2)/2; i++)
             Polygon->SetPoint(i, extractedPolygon[i*2], extractedPolygon[i*2+1]);

          Polygon->Paint();
          fCanvas->Update();

          ExtractionComplete = true; //ready
          return;
        }
    }

  //if no match - using base raster window class to activate Root event
  RasterWindowBaseClass::mousePressEvent(event);
}

void RasterWindowGraphClass::mouseMoveEvent(QMouseEvent *event)
{
  //qDebug() << "RasterGraph -> Mouse move event";
  if (!fCanvas) return;
  fCanvas->cd();

  double x = fCanvas->AbsPixeltoX(event->x());
  double y = fCanvas->AbsPixeltoY(event->y());
  if (fCanvas->GetLogx()) x = TMath::Power(10.0, x);
  if (fCanvas->GetLogy()) y = TMath::Power(10.0, y);

  if (ShowCursorPosition)
  {
      //QString str = "Cursor coordinates: " + QString::number(x, 'g', 4);
      //str += " : " + QString::number(y, 'g', 4);
      //MasterWindow->setWindowTitle(str);
      emit reportCursorPosition(x, y, true);
  }
  else emit reportCursorPosition(0, 0, false);

  //first block - if does not matter buttons are pressed or not
  if (ExtractionOf2DEllipsePhase == 2)
   {
       //resizeingthe ellipse - do not care if a button is pressed
      double dx = extracted2DEllipseX - x;
      double dy = extracted2DEllipseY - y;

      double sinA = sin(-extracted2DEllipseTheta*3.1415926535/180.);
      double cosA = cos(-extracted2DEllipseTheta*3.1415926535/180.);
      extracted2DEllipseR1 = fabs(dx * cosA - dy * sinA);
      extracted2DEllipseR2 = fabs(dx * sinA + dy * cosA);
//        qDebug()<<"current R1 and R2:"<<extracted2DEllipseR1<<extracted2DEllipseR2;

      Ellipse->SetR1(extracted2DEllipseR1);
      Ellipse->SetR2(extracted2DEllipseR2);
      Ellipse->Paint();
      fCanvas->Update();
      return;
   }

  if (ExtractionOf2DPolygonStarted)
   {
      int size = extractedPolygon.size(); //end point:   x=[size-2], y=[size-1]
      extractedPolygon[size-4] = x;
      extractedPolygon[size-3] = y;

      for (int i=0; i<extractedPolygon.size()/2; i++)
         Polygon->SetPoint(i, extractedPolygon[i*2], extractedPolygon[i*2+1]);

//        qDebug()<<"-list-"<<extractedPolygon.size()<<extractedPolygon.length()<<"=Poly="<<Polygon->Size();

      Polygon->Paint();
    //  fCanvas->SetCursor(kPointer);
      fCanvas->Update();
      return;
   }


  // MOUSE PRESSED MOVE
  if (event->buttons() & Qt::LeftButton)
    {
      if (ExtractionOfXStarted)
       {
         extractedX = x;
         VertLine1->SetX1(extractedX);
         VertLine1->SetX2(extractedX);
         VertLine1->Paint();
         fCanvas->Update();
         return;
       }

      if (ExtractionOf2DLineStarted)
       {
         Line2DstopX  = x;
         Line2DstopY  = y;
         Line2D->SetX2(Line2DstopX);
         Line2D->SetY2(Line2DstopY);
         Line2D->Paint();
         fCanvas->Update();
         return;
       }

      if (ExtractionOf2DBoxStarted)
       {
         extractedX2  = x;
         extractedY2  = y;
         Box2D->SetX2(extractedX2);
         Box2D->SetY2(extractedY2);
         Box2D->Paint();
         fCanvas->Update();
         return;
       }

      if (ExtractionOf2DEllipsePhase == 1)
       {
          //making line which defines ellipse orientation
         Line2DstopX  = x;
         Line2DstopY  = y;
         Line2D->SetX2(Line2DstopX);
         Line2D->SetY2(Line2DstopY);
         Line2D->Paint();
         fCanvas->Update();
         return;
       }

      //fCanvas->HandleInput(kButton1Motion, event->x(), event->y());
     }
   else
    {
      //fCanvas->HandleInput(kMouseMotion, event->x(), event->y());
    }

  //to avoid cursor change in these modes
  if (ExtractionOfXPending || ExtractionOf2DLinePending || ExtractionOf2DEllipsePending || ExtractionOf2DBoxPending || ExtractionOf2DPolygonPending) return;

  //if no match found, using base class handler to report move to Root system
  RasterWindowBaseClass::mouseMoveEvent(event);
}

void RasterWindowGraphClass::mouseReleaseEvent(QMouseEvent *event)
{
  //qDebug()<<"raster -> Mouse release event!";
  if (!fCanvas) return;
  fCanvas->cd();

  double x = fCanvas->AbsPixeltoX(event->x());
  double y = fCanvas->AbsPixeltoY(event->y());
  if (fCanvas->GetLogx()) x = TMath::Power(10.0, x);
  if (fCanvas->GetLogy()) y = TMath::Power(10.0, y);

  //if LEFT button released
  if (event->button() == Qt::LeftButton)
    {
      if (ExtractionOfXStarted)
       {
         extractedX = x;
         VertLine1->SetX1(extractedX);
         VertLine1->SetX2(extractedX);
         VertLine1->Paint();
         fCanvas->Update();
         ExtractionOfXStarted = false;

         ExtractionComplete = true; //ready
         return;
       }

      if (ExtractionOf2DLineStarted)
       {
         Line2DstopX  = x;
         Line2DstopY  = y;
         Line2D->SetX2(Line2DstopX);
         Line2D->SetY2(Line2DstopY);
         Line2D->Paint();
         fCanvas->Update();
         ExtractionOf2DLineStarted = false;

         //calculating line coordinates
         extracted2DLineA = Line2DstopY - Line2DstartY;
         extracted2DLineB = Line2DstartX - Line2DstopX;
         extracted2DLineC = extracted2DLineA*Line2DstartX + extracted2DLineB*Line2DstartY;

         ExtractionComplete = true; //ready
         return;
       }

      if (ExtractionOf2DBoxStarted)
       {
         extractedX2  = x;
         extractedY2  = y;
         Box2D->SetX2(extractedX2);
         Box2D->SetY2(extractedY2);
         Box2D->Paint();
         fCanvas->Update();
         ExtractionOf2DBoxStarted = false;

         if (extractedX1 > extractedX2)
           {
             double tmp = extractedX2;
             extractedX2 = extractedX1;
             extractedX1 = tmp;
           }
         if (extractedY1 > extractedY2)
           {
             double tmp = extractedY2;
             extractedY2 = extractedY1;
             extractedY1 = tmp;
           }

         ExtractionComplete = true; //ready
         return;
       }

      if (ExtractionOf2DEllipsePhase == 1)
       {
         Line2DstopX  = x;
         Line2DstopY  = y;
         Line2D->SetX2(Line2DstopX);
         Line2D->SetY2(Line2DstopY);
         Line2D->Paint();
         fCanvas->Update();

         //calculating ellipse angle
         double dx = Line2DstopX - Line2DstartX;
         double dy = Line2DstopY - Line2DstartY;
         qDebug()<<" dx, dy -- "<<dx<<dy;
         if ( fabs(dx) < 1.0e-10) extracted2DEllipseTheta = 90.0;
         else extracted2DEllipseTheta = TMath::ATan(dy/dx) *180.0/3.1415926535;
         qDebug()<<"Angle obtained:"<<extracted2DEllipseTheta;

         double sinA = sin(-extracted2DEllipseTheta*3.1415926535/180.0);
         double cosA = cos(-extracted2DEllipseTheta*3.1415926535/180.0);
         extracted2DEllipseR1 = fabs(dx * cosA - dy * sinA);
         extracted2DEllipseR2 = fabs(dx * sinA + dy * cosA);
         qDebug()<<"R1 and R2 obtained:"<<extracted2DEllipseR1<<extracted2DEllipseR2;

         Line2D->SetLineColor(0);
         RasterWindowGraphClass::DrawEllipse();
         ExtractionOf2DEllipsePhase = 2; //starting resize of the ellipse
//              qDebug()<<"Ellipse resize mode";

         return;
       }

      if (ExtractionOf2DEllipsePhase == 3)
       {
//              qDebug()<<"Ellipse done";
          double dx = extracted2DEllipseX - x;
          double dy = extracted2DEllipseY - y;
          double sinA = sin(-extracted2DEllipseTheta*3.1415926535/180.);
          double cosA = cos(-extracted2DEllipseTheta*3.1415926535/180.);
          extracted2DEllipseR1 = fabs(dx * cosA - dy * sinA);
          extracted2DEllipseR2 = fabs(dx * sinA + dy * cosA);

          Ellipse->SetR1(extracted2DEllipseR1);
          Ellipse->SetR2(extracted2DEllipseR2);
          Ellipse->Paint();
          fCanvas->Update();

          ExtractionOf2DEllipsePhase = 0;
          ExtractionComplete = true; //ready
          return;
       }

    //     fCanvas->HandleInput(kButton1Up, event->x(), event->y());
    }
    // if (event->button() == Qt::RightButton) fCanvas->HandleInput(kButton3Up, event->x(), event->y());

  //if no match found, using base class handler to report move to Root system
  RasterWindowBaseClass::mouseReleaseEvent(event);
}

void RasterWindowGraphClass::DrawVerticalLine()
{
  fCanvas->cd();
  if (VertLine1) delete VertLine1;
  VertLine1 = new TLine(extractedX, -1e10, extractedX, 1e10);
  VertLine1->SetLineColor(kRed);
  VertLine1->Draw();
  fCanvas->Update();
}

void RasterWindowGraphClass::Draw2DLine()
{
  fCanvas->cd();
  if (Line2D) delete Line2D;
  Line2D = new TLine(Line2DstartX, Line2DstartY, Line2DstopX, Line2DstopY);
  Line2D->SetLineColor(kRed);
  Line2D->SetLineStyle(2);
  Line2D->Draw();
  fCanvas->Update();
}

void RasterWindowGraphClass::Draw2DBox()
{
  fCanvas->cd();
  if (Box2D) delete Box2D;
  Box2D = new TBox(extractedX1, extractedY1, extractedX2, extractedY2);
  Box2D->SetLineColor(kRed);
  Box2D->SetLineStyle(2);
  Box2D->SetFillStyle(0);
  Box2D->Draw();
  fCanvas->Update();
}

void RasterWindowGraphClass::Draw2DPolygon()
{
  fCanvas->cd();
  if (Polygon) delete Polygon;
  Polygon = new TPolyLine();
  Polygon->SetLineColor(kRed);
  Polygon->SetLineStyle(2);
  Polygon->SetFillStyle(0);
  Polygon->Draw();
  fCanvas->Update();
}

void RasterWindowGraphClass::DrawEllipse()
{
  fCanvas->cd();
  if (Ellipse) delete Ellipse;
  Ellipse = new TEllipse(extracted2DEllipseX, extracted2DEllipseY, extracted2DEllipseR1, extracted2DEllipseR2, 0, 360, extracted2DEllipseTheta);
  Ellipse->SetLineColor(kRed);
  Ellipse->SetLineStyle(2);
  Ellipse->SetFillStyle(4000);
  Ellipse->Draw();
  fCanvas->Update();
}

void RasterWindowGraphClass::ExtractX()
{
  ExtractionComplete = false;
  ExtractionOfXStarted = false;
  ExtractionOfXPending = true;
}

void RasterWindowGraphClass::Extract2DLine()
{
  ExtractionComplete = false;
  ExtractionOf2DLineStarted = false;
  ExtractionOf2DLinePending = true;
}

void RasterWindowGraphClass::Extract2DEllipse()
{
  ExtractionComplete = false;
  ExtractionOf2DEllipsePhase = 0;
  ExtractionOf2DEllipsePending = true;
}

void RasterWindowGraphClass::Extract2DBox()
{
  ExtractionComplete = false; 
  ExtractionOf2DBoxStarted = false;
  ExtractionOf2DBoxPending = true;
}

void RasterWindowGraphClass::Extract2DPolygon()
{
  ExtractionComplete = false;
  ExtractionOf2DPolygonStarted = false;
  ExtractionOf2DPolygonPending = true;
}

double RasterWindowGraphClass::getCanvasMinX()
{
  return fCanvas->GetUxmin();
}

double RasterWindowGraphClass::getCanvasMaxX()
{
  return fCanvas->GetUxmax();
}

double RasterWindowGraphClass::getCanvasMinY()
{
  if (fCanvas->GetLogy())
    return TMath::Power(10.0, fCanvas->GetUymin());

  return fCanvas->GetUymin();
}

double RasterWindowGraphClass::getCanvasMaxY()
{
  if (fCanvas->GetLogy())
    return TMath::Power(10.0, fCanvas->GetUymax());

  return fCanvas->GetUymax();
}

void RasterWindowGraphClass::PixelToXY(int ix, int iy, double &x, double &y) const
{
  x = fCanvas->AbsPixeltoX(ix);
  y = fCanvas->AbsPixeltoY(iy);
  if (fCanvas->GetLogx()) x = TMath::Power(10.0, x);
  if (fCanvas->GetLogy()) y = TMath::Power(10.0, y);
}

void RasterWindowGraphClass::XYtoPixel(double x, double y, int &ix, int &iy) const
{
  //TO DO exp scale
  ix = fCanvas->XtoAbsPixel(x);
  iy = fCanvas->YtoAbsPixel(y);
}

void RasterWindowGraphClass::getRange(double &x1, double &y1, double &x2, double &y2) const
{
    fCanvas->GetRange(x1, y1, x2, y2);
}

void RasterWindowGraphClass::getRangeLogAware(double &x1, double &y1, double &x2, double &y2) const
{
    fCanvas->GetRange(x1, y1, x2, y2);
    if (fCanvas->GetLogx())
    {
        x1 = TMath::Power(10.0, x1);
        x2 = TMath::Power(10.0, x2);
    }
    if (fCanvas->GetLogy())
    {
        y1 = TMath::Power(10.0, y1);
        y2 = TMath::Power(10.0, y2);
    }
}

double RasterWindowGraphClass::getXperPixel() const
{
  double xmin = fCanvas->GetUxmin();
  double xmax = fCanvas->GetUxmax();

  int dix = fCanvas->XtoAbsPixel(xmax) - fCanvas->XtoAbsPixel(xmin);

  return ( xmax - xmin) / dix;
}

double RasterWindowGraphClass::getYperPixel() const
{
  double ymin = fCanvas->GetUymin();
  double ymax = fCanvas->GetUymax();

  int diy = - fCanvas->YtoAbsPixel(ymax) + fCanvas->YtoAbsPixel(ymin);

  return ( ymax - ymin) / diy;
}

bool RasterWindowGraphClass::isLogX() const
{
    return fCanvas->GetLogx();
}

bool RasterWindowGraphClass::isLogY() const
{
    return fCanvas->GetLogy();
}
