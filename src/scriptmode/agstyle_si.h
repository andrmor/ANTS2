#ifndef AINTERFACETOGSTYLESCRIPT_H
#define AINTERFACETOGSTYLESCRIPT_H

#include "ascriptinterface.h"

#include <QObject>
#include <QString>

class AGStyle_SI : public AScriptInterface
{
  Q_OBJECT
public:
    AGStyle_SI();

public slots:
  //https://root.cern.ch/root/html/TStyle.html
  void SetAxisColor(int color = 1, QString axis = "X");
  void SetBarOffset(float baroff = 0.5);
  void SetBarWidth(float barwidth = 0.5);
  void SetCanvasBorderMode(int mode = 1);
  void SetCanvasBorderSize(int size = 1);
  void SetCanvasColor(int color = 19);
  void SetCanvasDefH(int h = 500);
  void SetCanvasDefW(int w = 700);
  void SetCanvasDefX(int topx = 10);
  void SetCanvasDefY(int topy = 10);
  void SetCanvasPreferGL(bool prefer = true);
  void SetColorModelPS(int c = 0);
  void SetDateX(float x = 0.01f);
  void SetDateY(float y = 0.01f);
  void SetDrawBorder(int drawborder = 1);
  void SetEndErrorSize(float np = 2);
  void SetErrorX(float errorx = 0.5);
  void SetFitFormat(QString format = "5.4g");
  void SetFrameBorderMode(int mode = 1);
  void SetFrameBorderSize(int size = 1);
  void SetFrameFillColor(int color = 1);
  void SetFrameFillStyle(int styl = 0);
  void SetFrameLineColor(int color = 1);
  void SetFrameLineStyle(int styl = 0);
  void SetFrameLineWidth(int width = 1);
  void SetFuncColor(int color = 1);
  void SetFuncStyle(int style = 1);
  void SetFuncWidth(int width = 4);
  void SetGridColor(int color = 0);
  void SetGridStyle(int style = 3);
  void SetGridWidth(int width = 1);
  void SetHatchesLineWidth(int l);
  void SetHatchesSpacing(double h);
  void SetHeaderPS(QString header);
  void SetHistFillColor(int color = 1);
  void SetHistFillStyle(int styl = 0);
  void SetHistLineColor(int color = 1);
  void SetHistLineStyle(int styl = 0);
  void SetHistLineWidth(int width = 1);
  void SetHistMinimumZero(bool zero = true);
  void SetHistTopMargin(double hmax = 0.05);
  void SetIsReading(bool reading = true);
  void SetLabelColor(int color = 1, QString axis = "X");
  void SetLabelFont(int font = 62, QString axis = "X");
  void SetLabelOffset(float offset = 0.005f, QString axis = "X");
  void SetLabelSize(float size = 0.04f, QString axis = "X");
  void SetLegendBorderSize(int size = 4);
  void SetLegendFillColor(int color = 0);
  void SetLegendFont(int font = 62);
  void SetLegoInnerR(float rad = 0.5);
  void SetLineScalePS(float scale = 3);
  void SetLineStyleString(int i, QString text);
  void SetNdivisions(int n = 510, QString axis = "X");
  void SetNumberContours(int number = 20);
  void SetOptDate(int datefl = 1);
  void SetOptFile(int file = 1);
  void SetOptFit(int fit = 1);
  void SetOptLogx(int logx = 1);
  void SetOptLogy(int logy = 1);
  void SetOptLogz(int logz = 1);
  void SetOptStat(int stat = 1);
  void SetOptStat(QString stat);
  void SetOptTitle(int tit = 1);
  void SetPadBorderMode(int mode = 1);
  void SetPadBorderSize(int size = 1);
  void SetPadBottomMargin(float margin = 0.1f);
  void SetPadColor(int color = 19);
  void SetPadGridX(bool gridx);
  void SetPadGridY(bool gridy);
  void SetPadLeftMargin(float margin = 0.1f);
  void SetPadRightMargin(float margin = 0.1f);
  void SetPadTickX(int tickx);
  void SetPadTickY(int ticky);
  void SetPadTopMargin(float margin = 0.1f);
  void SetPaintTextFormat(QString format = "g");
  void SetPalette(int scheme); //51 - 56, default 1
  void SetPaperSize(float xsize = 20, float ysize = 26);
  void SetScreenFactor(float factor = 1);
  void SetStatBorderSize(int size = 2);
  void SetStatColor(int color = 19);
  void SetStatFont(int font = 62);
  void SetStatFontSize(float size = 0);
  void SetStatFormat(QString format = "6.4g");
  void SetStatH(float h = 0.1f);
  void SetStatStyle(int style = 1001);
  void SetStatTextColor(int color = 1);
  void SetStatW(float w = 0.19f);
  void SetStatX(float x = 0);
  void SetStatY(float y = 0);
  void SetStripDecimals(bool strip = true);
  void SetTickLength(float length = 0.03f, QString axis = "X");
  void SetTimeOffset(double toffset);
  void SetTitleAlign(int a = 13);
  void SetTitleBorderSize(int size = 2);
  void SetTitleColor(int color = 1, QString axis = "X");
  void SetTitleFillColor(int color = 1);
  void SetTitleFont(int font = 62, QString axis = "X");
  void SetTitleFontSize(float size = 0);
  void SetTitleH(float h = 0);
  void SetTitleOffset(float offset = 1, QString axis = "X");
  void SetTitlePS(QString pstitle);
  void SetTitleSize(float size = 0.02f, QString axis = "X");
  void SetTitleStyle(int style = 1001);
  void SetTitleTextColor(int color = 1);
  void SetTitleW(float w = 0);
  void SetTitleX(float x = 0);
  void SetTitleXOffset(float offset = 1);
  void SetTitleXSize(float size = 0.02f);
  void SetTitleY(float y = 0.985f);
  void SetTitleYOffset(float offset = 1.0f);
  void SetTitleYSize(float size = 0.02f);
};

#endif // AINTERFACETOGSTYLESCRIPT_H
