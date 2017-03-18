#include "rasterwindowbaseclass.h"

#include <QDebug>
#include <QMouseEvent>
#include <QMainWindow>

#include "TCanvas.h"
#include "TView.h"
#include "TView3D.h"

RasterWindowBaseClass::RasterWindowBaseClass(QMainWindow *parent) : QWindow()
{
  //qDebug()<<"->Creating raster window";
  MasterWindow = parent;
  fBlockEvents = false;
  fInvertedXYforDrag = false;
  create();

  setGeometry(50, 50, 500, 500);

  wid = gVirtualX->AddWindow(QWindow::winId(), QWindow::width(), QWindow::height());
  fCanvas = new TCanvas("fCanvas", 100, 100, wid);

  PressEventRegistered = false;
  //qDebug() << "  ->Root canvas created";
}

RasterWindowBaseClass::~RasterWindowBaseClass()
{
  //qDebug()<< "     <--Starting cleanup for raster window base...";

  fCanvas->Clear();
  delete fCanvas;
  //qDebug()<<"        canvas deleted";

  gVirtualX->RemoveWindow(wid);
  //qDebug()<<"        window unregistered in Root";
  //qDebug()<< "  <-Done";
}

void RasterWindowBaseClass::SetAsActiveRootWindow()
{
  fCanvas->cd();
}

void RasterWindowBaseClass::ClearRootCanvas()
{
  fCanvas->Clear();
}

void RasterWindowBaseClass::UpdateRootCanvas()
{
  //fCanvas->Modified();
  fCanvas->Update();
}

void RasterWindowBaseClass::exposeEvent(QExposeEvent *)
{
  if (!fCanvas) return;
  if (fBlockEvents) return;
  //qDebug() << "raster window -> Expose event";
  if (isExposed()) fCanvas->Update();
}

void RasterWindowBaseClass::mouseMoveEvent(QMouseEvent *event)
{
    //qDebug() << "Base: Mouse move event";
    if (!fCanvas) return;
    fCanvas->cd();
    if (fBlockEvents) return;

    if (event->buttons() & Qt::LeftButton)
      {
        //qDebug() << "Mouse left-pressed move event";
        //left-pressed move
        if (!PressEventRegistered) return;

        if (fInvertedXYforDrag)
            fCanvas->HandleInput(kButton1Motion, event->y(), event->x());
        else
            fCanvas->HandleInput(kButton1Motion, event->x(), event->y());

        if (!fCanvas->HasViewer3D() || !fCanvas->GetView()) return;
        //qDebug() << "-->"<<fCanvas->GetView();
        Double_t centerX, centerY, viewSizeX, viewSizeY;
        fCanvas->GetView()->GetWindow(centerX, centerY, viewSizeX, viewSizeY);
        Double_t theta = fCanvas->GetView()->GetLatitude();
        Double_t phi = fCanvas->GetView()->GetLongitude();
        emit UserChangedWindow(centerX, centerY, viewSizeX, viewSizeY, phi, theta);
        //qDebug() << "--<";
      }
    else if (event->buttons() & Qt::RightButton)
      {
        //qDebug() << "Mouse right-pressed move event";  // not implemented by root!
        //right-pressed move
        //if (!PressEventRegistered) return;
        //fCanvas->HandleInput(kButton3Motion, event->x(), event->y());
      }
    else if (event->buttons() & Qt::MidButton)
      {
        //middle-pressed move
        if (!PressEventRegistered) return;
        if (!fCanvas->HasViewer3D()) return;
        //fCanvas->HandleInput(kButton2Motion, event->x(), event->y());
        int x = event->x();
        int y = event->y();
        int dx = x-lastX;
        int dy = y-lastY;
        Double_t centerX, centerY, viewSizeX, viewSizeY;
        fCanvas->cd();
        if (fCanvas->GetView())
          {
            fCanvas->GetView()->GetWindow(centerX, centerY, viewSizeX, viewSizeY);

            Double_t theta = fCanvas->GetView()->GetLatitude();
            Double_t phi = fCanvas->GetView()->GetLongitude();
            double x0 = lastCenterX - 2.0*dx/(double)this->width() *viewSizeX;
            double y0 = lastCenterY + 2.0*dy/(double)this->height() *viewSizeY;

            fCanvas->GetView()->SetWindow(x0, y0, viewSizeX, viewSizeY);
            fCanvas->GetView()->RotateView(phi,theta);
            emit UserChangedWindow(x0, y0, viewSizeX, viewSizeY, phi, theta);
          }
      }
    else
      {
        //move
        //qDebug() << "mouse plain move event"<<event->x() << event->y();
        fCanvas->HandleInput(kMouseMotion, event->x(), event->y());        
      }
    //qDebug() << "done";
}

void RasterWindowBaseClass::mousePressEvent(QMouseEvent *event)
{
  if (!fCanvas) return;
  fCanvas->cd();
  if (fBlockEvents) return;
  //qDebug() << "Mouse press event";
  PressEventRegistered = true;
  if (event->button() == Qt::LeftButton)  fCanvas->HandleInput(kButton1Down, event->x(), event->y());
  if (event->button() == Qt::RightButton) fCanvas->HandleInput(kButton3Down, event->x(), event->y());

  if (!fCanvas->HasViewer3D() || !fCanvas->GetView()) return;
  if (event->button() == Qt::MidButton)
    {
      //fCanvas->HandleInput(kButton2Down, event->x(), event->y());
      lastX = event->x();
      lastY = event->y();
      Double_t viewSizeX, viewSizeY;
      fCanvas->cd();
      fCanvas->GetView()->GetWindow(lastCenterX, lastCenterY, viewSizeX, viewSizeY);      
    }
 // qDebug() << "done";
}

void RasterWindowBaseClass::mouseReleaseEvent(QMouseEvent *event)
{
  if (!fCanvas) return;
  fCanvas->cd();
  if (fBlockEvents) return;
  //qDebug() << "Mouse release event";
  if (!PressEventRegistered) return;
  PressEventRegistered = false;
  if (event->button() == Qt::LeftButton) fCanvas->HandleInput(kButton1Up, event->x(), event->y());
  if (event->button() == Qt::RightButton) fCanvas->HandleInput(kButton3Up, event->x(), event->y());

  if (event->button() == Qt::LeftButton) emit LeftMouseButtonReleased();
}

void RasterWindowBaseClass::wheelEvent(QWheelEvent *event)
{
  if (!fCanvas) return;
  fCanvas->cd();
  if (fBlockEvents) return;
  //qDebug() << "Mouse wheel event";

  //if (event->delta()>0) fCanvas->GetView()->Execute("ZoomIn", "");
  //else fCanvas->GetView()->Execute("ZoomOut", "");

  if (!fCanvas->HasViewer3D() || !fCanvas->GetView()) return;
  fCanvas->cd();
  //int x = event->x();
  //int y = event->y();
  double factor = (event->delta()<0) ? 1.25 : 0.8;

  fCanvas->GetView()->ZoomView(0, 1.0/factor);

  Double_t centerX, centerY, viewSizeX, viewSizeY;
  fCanvas->cd();
  fCanvas->GetView()->GetWindow(centerX, centerY, viewSizeX, viewSizeY);
  Double_t theta = fCanvas->GetView()->GetLatitude();
  Double_t phi = fCanvas->GetView()->GetLongitude();

  emit UserChangedWindow(centerX, centerY, viewSizeX, viewSizeY, phi, theta);
}

void RasterWindowBaseClass::ForceResize()
{
  if (fCanvas)
    {
      fCanvas->Resize();
      //fCanvas->Modified();
      fCanvas->Update();
    }
}

void RasterWindowBaseClass::SaveAs(const QString filename)
{
  //converting QString to char
  QByteArray ba = filename.toLocal8Bit();
  char *name = ba.data();

  fCanvas->SaveAs(name);
}

void RasterWindowBaseClass::SetWindowTitle(const QString &title)
{
  MasterWindow->setWindowTitle(title);
}

void RasterWindowBaseClass::getWindowProperties(Double_t &centerX, Double_t &centerY, Double_t &hWidth, Double_t &hHeight, Double_t &phi, Double_t &theta)
{
  if (!fCanvas->HasViewer3D() || !fCanvas->GetView()) return;
  fCanvas->cd();
  fCanvas->GetView()->GetWindow(centerX, centerY, hWidth, hHeight);
  theta = fCanvas->GetView()->GetLatitude();
  phi = fCanvas->GetView()->GetLongitude();
}

void RasterWindowBaseClass::setWindowProperties(Double_t centerX, Double_t centerY, Double_t hWidth, Double_t hHeight, Double_t phi, Double_t theta)
{
  if (!fCanvas->HasViewer3D() || !fCanvas->GetView()) return;
  fCanvas->cd();
  fCanvas->GetView()->SetWindow(centerX, centerY, hWidth, hHeight);
  fCanvas->GetView()->RotateView(phi,theta);
}
