#include "rasterwindowbaseclass.h"

#include <QDebug>
#include <QMouseEvent>
#include <QMainWindow>

#include "TCanvas.h"
#include "TView.h"
#include "TView3D.h"

RasterWindowBaseClass::RasterWindowBaseClass(QMainWindow *MasterWindow) : QWidget(MasterWindow), MasterWindow(MasterWindow)
{
  qDebug()<<"->Creating raster window";

  // set options needed to properly update the canvas when resizing the widget
  // and to properly handle context menus and mouse move events
  setAttribute(Qt::WA_PaintOnScreen, false);
  setAttribute(Qt::WA_OpaquePaintEvent, true);
  setAttribute(Qt::WA_NativeWindow, true);
  setUpdatesEnabled(false);
  setMouseTracking(true);
  setMinimumSize(300, 200);

  // register the QWidget in TVirtualX, giving its native window id
  int wid = gVirtualX->AddWindow((ULong_t)winId(), width(), height());
  // create the ROOT TCanvas, giving as argument the QWidget registered id
  fCanvas = new TCanvas("Root Canvas", width(), height(), wid);
  //TQObject::Connect("TGPopupMenu", "PoppedDown()", "TCanvas", fCanvas, "Update()");

  fCanvas->SetBorderMode(0);
  //fCanvas->SetRightMargin(0.10);
  //fCanvas->SetLeftMargin(0.10);
  fCanvas->SetFillColor(0);

  qDebug() << "  ->Root canvas created";
}

RasterWindowBaseClass::~RasterWindowBaseClass()
{
  //qDebug()<< "     <--Starting cleanup for raster window base...";

  fCanvas->Clear();
  delete fCanvas; fCanvas = 0;
  //qDebug()<<"        canvas deleted";

  //gVirtualX->RemoveWindow(wid); //causes strange warnings !!!***
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
  fCanvas->Modified();
  fCanvas->Update();
}

/*
void RasterWindowBaseClass::exposeEvent(QExposeEvent *)
{
  if (!fCanvas) return;
  if (fBlockEvents) return;
  //qDebug() << "raster window -> Expose event";
  if (isExposed()) fCanvas->Update();
}
*/

void RasterWindowBaseClass::mouseMoveEvent(QMouseEvent *event)
{
    //qDebug() << "Mouse move event";
    if (!fCanvas) return;
    fCanvas->cd();
    if (fBlockEvents) return;

    if (event->buttons() & Qt::LeftButton)
    {
        //qDebug() << "-->Mouse left-pressed move event";
        if (!PressEventRegistered) return;

        if (fInvertedXYforDrag) fCanvas->HandleInput(kButton1Motion, event->y(), event->x());
        else                    fCanvas->HandleInput(kButton1Motion, event->x(), event->y());

        onViewChanged();
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
        //qDebug() << "-->Mid button move!";
        //fCanvas->HandleInput(kButton2Motion, event->x(), event->y());
        if (!PressEventRegistered) return;
        if (!fCanvas->HasViewer3D()) return;
        int x = event->x();
        int y = event->y();
        int dx = x - lastX;
        int dy = y - lastY;
        if (fCanvas->GetView())
        {
            ViewParameters.read(fCanvas);
            ViewParameters.WinX = lastCenterX - 2.0 * dx / (double)this->width()  * ViewParameters.WinW;
            ViewParameters.WinY = lastCenterY + 2.0 * dy / (double)this->height() * ViewParameters.WinH;
            ViewParameters.apply(fCanvas);

            onViewChanged();
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
      bBlockZoom = true;
    }
 // qDebug() << "done";

  // future: maybe it is possible to hide menu on left click using signal-slot mechanism?
  //TQObject::Connect("TGPopupMenu", "PoppedDown()", "TCanvas", fCanvas, "Update()");
}

#include <QTimer>
void RasterWindowBaseClass::mouseReleaseEvent(QMouseEvent *event)
{
  if (!fCanvas) return;
  fCanvas->cd();
  if (fBlockEvents) return;
  if (!PressEventRegistered) return;
  PressEventRegistered = false;
  if (event->button() == Qt::LeftButton)
  {
      fCanvas->HandleInput(kButton1Up, event->x(), event->y());
      emit LeftMouseButtonReleased();
  }
  else if (event->button() == Qt::RightButton) fCanvas->HandleInput(kButton3Up, event->x(), event->y());
  else if (event->button() == Qt::MiddleButton) QTimer::singleShot(300, this, &RasterWindowBaseClass::releaseZoomBlock);
}

void RasterWindowBaseClass::wheelEvent(QWheelEvent *event)
{
  if (!fCanvas) return;
  if (fBlockEvents || bBlockZoom) return;

  if (!fCanvas->HasViewer3D() || !fCanvas->GetView()) return;
  fCanvas->cd();

  double factor = ( event->delta() < 0 ? 1.25 : 0.8 );
  fCanvas->GetView()->ZoomView(0, 1.0/factor);

  onViewChanged();
}

void RasterWindowBaseClass::paintEvent(QPaintEvent * /*event*/)
{
    if (fCanvas)
    {
       fCanvas->Resize();
       fCanvas->Update();
    }
}

void RasterWindowBaseClass::resizeEvent(QResizeEvent *event)
{
    if (fCanvas)
    {
       fCanvas->SetCanvasSize(event->size().width(), event->size().height());
       fCanvas->Resize();
       fCanvas->Update();
    }
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

void RasterWindowBaseClass::setWindowProperties()
{
    ViewParameters.apply(fCanvas);
}

void RasterWindowBaseClass::onViewChanged()
{
    fCanvas->cd();
    ViewParameters.read(fCanvas);
    emit userChangedWindow();
}

void AGeoViewParameters::read(TCanvas * Canvas)
{
    TView3D * v = dynamic_cast<TView3D*>(Canvas->GetView());
    if (!v) return;

    v->GetRange(RangeLL, RangeUR);

    for (int i = 0; i < 3; i++)
        RotCenter[i] = 0.5 * (RangeUR[i] + RangeLL[i]);

    v->GetWindow(WinX, WinY, WinW, WinH);

    Lat  = v->GetLatitude();
    Long = v->GetLongitude();
    Psi  = v->GetPsi();
}

void AGeoViewParameters::apply(TCanvas *Canvas) const
{
    TView3D * v = dynamic_cast<TView3D*>(Canvas->GetView());
    if (!v) return;

    v->SetRange(RangeLL, RangeUR);
    v->SetWindow(WinX, WinY, WinW, WinH);
    int err;
    v->SetView(Long, Lat, Psi, err);

    Canvas->Modified();
    Canvas->Update();
}
