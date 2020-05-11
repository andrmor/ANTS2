#include "agraphwin_si.h"
#include "mainwindow.h"
#include "graphwindowclass.h"
#include "reconstructionwindow.h"

#include <QJsonObject>
#include <QJsonArray>

AGraphWin_SI::AGraphWin_SI(MainWindow *MW)
  : MW(MW)
{
  Description = "Access to the Graph window of GUI";

  H["SaveImage"] = "Save image currently shown on the graph window to an image file.\nTip: use .png extension";
  H["GetAxis"] = "Returns an object with the values presented to user in 'Range' boxes.\n"
                 "They can be accessed with min/max X/Y/Z (e.g. grwin.GetAxis().maxY).\n"
                 "The values can be 'undefined'";
  H["AddLegend"] = "Adds a temporary (not savable yet!) legend to the graph.\n"
      "x1,y1 and x2,y2 are the bottom-left and top-right corner coordinates (0..1)";
}

void AGraphWin_SI::Show()
{
  MW->GraphWindow->showNormal();
}

void AGraphWin_SI::Hide()
{
  MW->GraphWindow->hide();
}

void AGraphWin_SI::PlotDensityXY()
{
  MW->Rwindow->DoPlotXY(0);
}

void AGraphWin_SI::PlotEnergyXY()
{
  MW->Rwindow->DoPlotXY(2);
}

void AGraphWin_SI::PlotChi2XY()
{
    MW->Rwindow->DoPlotXY(3);
}

void AGraphWin_SI::ConfigureXYplot(int binsX, double X0, double X1, int binsY, double Y0, double Y1)
{
   MW->Rwindow->ConfigurePlotXY(binsX, X0, X1, binsY, Y0, Y1);
   MW->Rwindow->updateGUIsettingsInConfig();
}

void AGraphWin_SI::ConfigureXYplotExtra(bool suppress0, bool plotVsTrue, bool showPMs, bool showManifest, bool invertX, bool invertY)
{
    MW->Rwindow->ConfigurePlotXYextra(suppress0, plotVsTrue, showPMs, showManifest, invertX, invertY);
    MW->Rwindow->updateGUIsettingsInConfig();
}

void AGraphWin_SI::SetLog(bool Xaxis, bool Yaxis)
{
    MW->GraphWindow->SetLog(Xaxis, Yaxis);
}

void AGraphWin_SI::SetStatPanelVisible(bool flag)
{
    MW->GraphWindow->SetStatPanelVisible(flag);
}

void AGraphWin_SI::AddLegend(double x1, double y1, double x2, double y2, QString title)
{
    MW->GraphWindow->AddLegend(x1, y1, x2, y2, title);
}

void AGraphWin_SI::SetLegendBorder(int color, int style, int size)
{
    MW->GraphWindow->SetLegendBorder(color, style, size);
}

void AGraphWin_SI::AddText(QString text, bool Showframe, int Alignment_0Left1Center2Right)
{
    MW->GraphWindow->ShowTextPanel(text, Showframe, Alignment_0Left1Center2Right);
}

void AGraphWin_SI::AddTextScreenXY(QString text, bool Showframe, int Alignment_0Left1Center2Right, double x1, double y1, double x2, double y2)
{
    MW->GraphWindow->ShowTextPanel(text, Showframe, Alignment_0Left1Center2Right, x1, y1, x2, y2, "NDC");
}

void AGraphWin_SI::AddTextTrueXY(QString text, bool Showframe, int Alignment_0Left1Center2Right, double x1, double y1, double x2, double y2)
{
    MW->GraphWindow->ShowTextPanel(text, Showframe, Alignment_0Left1Center2Right, x1, y1, x2, y2, "BR");
}

void AGraphWin_SI::AddLine(double x1, double y1, double x2, double y2, int color, int width, int style)
{
    MW->GraphWindow->AddLine(x1, y1, x2, y2, color, width, style);
}

void AGraphWin_SI::AddToBasket(QString Title)
{
  MW->GraphWindow->AddCurrentToBasket(Title);
}

void AGraphWin_SI::ClearBasket()
{
  MW->GraphWindow->ClearBasket();
}

void AGraphWin_SI::SaveImage(QString fileName)
{
    MW->GraphWindow->SaveGraph(fileName);
}

void AGraphWin_SI::ExportTH2AsText(QString fileName)
{
    MW->GraphWindow->ExportTH2AsText(fileName);
}

QVariant AGraphWin_SI::GetProjection()
{
    QVector<double> vec = MW->GraphWindow->Get2DArray();
    QJsonArray arr;
    for (auto v : vec) arr << v;
    QJsonValue jv = arr;
    QVariant res = jv.toVariant();
    return res;
}

QVariant AGraphWin_SI::GetAxis()
{
  bool ok;
  QJsonObject result;

  result["minX"] = MW->GraphWindow->getMinX(&ok);
  if (!ok) result["minX"] = QJsonValue();
  result["maxX"] = MW->GraphWindow->getMaxX(&ok);
  if (!ok) result["maxX"] = QJsonValue();

  result["minY"] = MW->GraphWindow->getMinY(&ok);
  if (!ok) result["minY"] = QJsonValue();
  result["maxY"] = MW->GraphWindow->getMaxY(&ok);
  if (!ok) result["maxY"] = QJsonValue();

  result["minZ"] = MW->GraphWindow->getMinZ(&ok);
  if (!ok) result["minZ"] = QJsonValue();
  result["maxZ"] = MW->GraphWindow->getMaxZ(&ok);
  if (!ok) result["maxZ"] = QJsonValue();

  return QJsonValue(result).toVariant();
}
