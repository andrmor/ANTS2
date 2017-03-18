//ANTS2 modules and windows
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "pms.h"
#include "pmtypeclass.h"
#include "geometrywindowclass.h"
#include "detectorclass.h"
#include "amessage.h"

//Qt
#include <QDebug>

//Root
#include "TVirtualGeoTrack.h"
#include "TGeoManager.h"

void MainWindow::ShowPMnumbers()
{
    QVector<QString> tmp(0);
    for (int i=0; i<PMs->count(); i++) tmp.append( QString::number(i) );

    MainWindow::ShowTextOnPMs(tmp, kBlack);
}

void MainWindow::ShowTextOnPMs(QVector<QString> strData, Color_t color)
{
    //protection
    int numPMs = PMs->count();
    if (strData.size() != numPMs)
      {
        message("Mismatch between the text vector and number of PMs!", this);
        return;
      }

    Detector->GeoManager->ClearTracks();
    if (!GeometryWindow->isVisible()) GeometryWindow->show();

    //font size
      //checking minimum PM size
    double minSize = 1e10;
    for (int i=0; i<numPMs; i++)
      {
        int typ = PMs->at(i).type;
        PMtypeClass tp = *PMs->getType(typ);
        if (tp.SizeX<minSize) minSize = tp.SizeX;
        int shape = tp.Shape; //0 box, 1 round, 2 hexa
        if (shape == 0)
          if (tp.SizeY<minSize) minSize = tp.SizeY;
      }
      //max number of digits
    int symbols = 0;
    for (int i=0; i<numPMs; i++)
        if (strData[i].size() > symbols) symbols = strData[i].size();
    if (symbols == 0) symbols++;
//        qDebug()<<"Max number of symbols"<<symbols;
    double size = minSize/5.0/(0.5+0.5*symbols);

    //graphics ofr symbols
    QVector< QVector < double > > numbersX;
    QVector< QVector < double > > numbersY;
    QVector< double > tmp;
    numbersX.resize(0);
    numbersY.resize(0);

    //0
    tmp.resize(0);
    tmp<<-1<<1<<1<<-1<<-1;
    numbersX.append(tmp);
    tmp.resize(0);
    tmp<<1.62<<1.62<<-1.62<<-1.62<<1.62;
    numbersY.append(tmp);
    //1
    tmp.resize(0);
    //tmp<<0<<0;
    tmp<<-0.3<<0.3<<0.3;
    numbersX.append(tmp);
    tmp.resize(0);
    //tmp<<1.62<<-1.62;
    tmp<<0.42<<1.62<<-1.62;
    numbersY.append(tmp);
    //2
    tmp.resize(0);
    tmp<<-1<<1<<1<<-1<<-1<<1;
    numbersX.append(tmp);
    tmp.resize(0);
    tmp<<1.62<<1.62<<0<<0<<-1.62<<-1.62;
    numbersY.append(tmp);
    //3
    tmp.resize(0);
    tmp<<-1<<1<<1<<-1<<1<<1<<-1;
    numbersX.append(tmp);
    tmp.resize(0);
    tmp<<1.62<<1.62<<0<<0<<0<<-1.62<<-1.62;
    numbersY.append(tmp);
    //4
    tmp.resize(0);
    tmp<<-1<<-1<<1<<1<<1;
    numbersX.append(tmp);
    tmp.resize(0);
    tmp<<1.62<<0<<0<<1.62<<-1.62;
    numbersY.append(tmp);
    //5
    tmp.resize(0);
    tmp<<1<<-1<<-1<<1<<1<<-1;
    numbersX.append(tmp);
    tmp.resize(0);
    tmp<<1.62<<1.62<<0<<0<<-1.62<<-1.62;
    numbersY.append(tmp);
    //6
    tmp.resize(0);
    tmp<<1<<-1<<-1<<1<<1<<-1;
    numbersX.append(tmp);
    tmp.resize(0);
    tmp<<1.62<<1.62<<-1.62<<-1.62<<0<<0;
    numbersY.append(tmp);
    //7
    tmp.resize(0);
    tmp<<-1<<1<<1;
    numbersX.append(tmp);
    tmp.resize(0);
    tmp<<1.62<<1.62<<-1.62;
    numbersY.append(tmp);
    //8
    tmp.resize(0);
    tmp<<-1<<1<<1<<-1<<-1<<1<<1;
    numbersX.append(tmp);
    tmp.resize(0);
    tmp<<0<<0<<-1.62<<-1.62<<1.62<<1.62<<0;
    numbersY.append(tmp);
    //9
    tmp.resize(0);
    tmp<<-1<<1<<1<<-1<<-1<<1;
    numbersX.append(tmp);
    tmp.resize(0);
    tmp<<-1.62<<-1.62<<1.62<<1.62<<0<<0;
    numbersY.append(tmp);
    //.
    tmp.resize(0);
    tmp<<-0.2<<0.2<<0.2<<-0.2<<-0.2;
    numbersX.append(tmp);
    tmp.resize(0);
    tmp<<-1.2<<-1.2<<-1.6<<-1.6<<-1.2;
    numbersY.append(tmp);
    //-
    tmp.resize(0);
    tmp<<-1<<+1;
    numbersX.append(tmp);
    tmp.resize(0);
    tmp<<0<<0;
    numbersY.append(tmp);

    //mapping
    QVector<QString> SymbolMap(0);
    SymbolMap <<"0"<<"1"<<"2"<<"3"<<"4"<<"5"<<"6"<<"7"<<"8"<<"9"<<"."<<"-";

    for (int ipm=0; ipm<PMs->count(); ipm++)
      {
        double Xcenter = PMs->X(ipm);
        double Ycenter = PMs->Y(ipm);
        double Zcenter = PMs->Z(ipm);

        QString str = strData[ipm];
        if (str.isEmpty()) continue;
        int numDigits = str.size();
        if (str.right(1) == "F") numDigits--;
//        qDebug()<<"PM number: "<<ipm<<"    string="<<str<<"  digits="<<numDigits<<"size"<<size;

        int lineWidth = 2;
        if (size<2) lineWidth = 1;

        for (int idig=0; idig<numDigits; idig++)
          {
            QString str1 = str.mid(idig,1);

            int isymbol = -1;
            for (int i=0; i<SymbolMap.size(); i++)
              if (str1 == SymbolMap[i]) isymbol = i;

//            qDebug()<<"position="<<idig<<  "  To show: str="<<str1<<"index of mapping="<<isymbol;

            Int_t track_index = Detector->GeoManager->AddTrack(2,22); //  Here track_index is the index of the newly created track in the array of primaries. One can get the pointer of this track and make it known as current track by the manager class:
            TVirtualGeoTrack *track = Detector->GeoManager->GetTrack(track_index);
            if (str.right(1) == "F")
                track->SetLineColor(kRed);
            else
                track->SetLineColor(color);
            track->SetLineWidth(lineWidth);

            if (isymbol > -1)
             for (int i=0; i<numbersX[isymbol].size(); i++)
              {
                double x = Xcenter - 2.6*size*(0.5*(numDigits-1) - 1.0*idig) + size*numbersX[isymbol][i];
                double y = Ycenter + size*numbersY[isymbol][i];
                track->AddPoint(x, y, Zcenter, 0);
              }
          }
      }

    MainWindow::ShowGeometry(false);
    Detector->GeoManager->DrawTracks();
    GeometryWindow->UpdateRootCanvas();
    //GeometryWindow->raise();
}

void MainWindow::AddLineToGeometry(QPointF& start, QPointF& end, Color_t color, int width)
{
  Int_t track_index = Detector->GeoManager->AddTrack(2,22); //  Here track_index is the index of the newly created track in the array of primaries. One can get the pointer of this track and make it known as current track by the manager class:
  TVirtualGeoTrack *track = Detector->GeoManager->GetTrack(track_index);
  track->SetLineColor(color);
  track->SetLineWidth(width);

  track->AddPoint(start.x(), start.y(), 0, 0);
  track->AddPoint(end.x(), end.y(), 0, 0);
}

void MainWindow::AddPolygonfToGeometry(QPolygonF& poly, Color_t color, int width)
{
  if (poly.size()<2) return;
  for (int i=0; i<poly.size()-1; i++)
   MainWindow::AddLineToGeometry(poly[i], poly[i+1], color, width);
}

