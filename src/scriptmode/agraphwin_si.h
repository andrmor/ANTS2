#ifndef AGRAPHWIN_SI_H
#define AGRAPHWIN_SI_H

#include "ascriptinterface.h"

#include <QString>
#include <QVariant>
#include <QVariantList>

class MainWindow;

class AGraphWin_SI : public AScriptInterface
{
  Q_OBJECT

public:
  AGraphWin_SI(MainWindow* MW);
  ~AGraphWin_SI(){}

public slots:
  void Show();
  void Hide();

  void PlotDensityXY();
  void PlotEnergyXY();
  void PlotChi2XY();
  void ConfigureXYplot(int binsX, double X0, double X1, int binsY, double Y0, double Y1);
  void ConfigureXYplotExtra(bool suppress0, bool plotVsTrue, bool showPMs, bool showManifest, bool invertX, bool invertY);

  void SetLog(bool Xaxis, bool Yaxis);

  void SetStatPanelVisible(bool flag);

  void AddLegend(double x1, double y1, double x2, double y2, QString title);
  void SetLegendBorder(int color, int style, int size);

  void AddText(QString text, bool Showframe, int Alignment_0Left1Center2Right);
  void AddTextScreenXY(QString text, bool Showframe, int Alignment_0Left1Center2Right, double x1, double y1, double x2, double y2);
  void AddTextTrueXY(QString text, bool Showframe, int Alignment_0Left1Center2Right, double x1, double y1, double x2, double y2);

  void AddLine(double x1, double y1, double x2, double y2, int color, int width, int style);
  void AddArrow(double x1, double y1, double x2, double y2, int color, int width, int style);

  //basket operation
  void AddToBasket(QString Title);
  void ClearBasket();

  void SaveImage(QString fileName);
  void ExportTH2AsText(QString fileName);
  QVariant GetProjection();

  QVariant GetAxis();

  QVariantList GetContent();

private:
  MainWindow* MW;
};

#endif // AGRAPHWIN_SI_H
