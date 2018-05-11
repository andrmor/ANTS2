#ifndef AINTERFACETOGSTYLESCRIPT_H
#define AINTERFACETOGSTYLESCRIPT_H

#include "ascriptinterface.h"
#include "TStyle.h"

#include <QObject>

class AInterfaceToGStyleScript : public AScriptInterface
{
  Q_OBJECT
public:
    AInterfaceToGStyleScript();

public slots:
  //https://root.cern.ch/root/html/TStyle.html
  void SetAxisColor(int color = 1, QString axis = "X") {gStyle->SetAxisColor(color, axis.toLocal8Bit());}
  void SetBarOffset(float baroff = 0.5) {gStyle->SetBarOffset(baroff);}
  void SetBarWidth(float barwidth = 0.5) {gStyle->SetBarWidth(barwidth);}
  void SetCanvasBorderMode(int mode = 1) {gStyle->SetCanvasBorderMode(mode);}
  void SetCanvasBorderSize(int size = 1) {gStyle->SetCanvasBorderSize(size);}
  void SetCanvasColor(int color = 19) {gStyle->SetCanvasColor(color);}
  void SetCanvasDefH(int h = 500) {gStyle->SetCanvasDefH(h);}
  void SetCanvasDefW(int w = 700) {gStyle->SetCanvasDefW(w);}
  void SetCanvasDefX(int topx = 10) {gStyle->SetCanvasDefX(topx);}
  void SetCanvasDefY(int topy = 10) {gStyle->SetCanvasDefY(topy);}
  void SetCanvasPreferGL(bool prefer = true) {gStyle->SetCanvasPreferGL(prefer);}
  void SetColorModelPS(int c = 0) {gStyle->SetColorModelPS(c);}
  void SetDateX(float x = 0.01) {gStyle->SetDateX(x);}
  void SetDateY(float y = 0.01) {gStyle->SetDateY(y);}
  void SetDrawBorder(int drawborder = 1) {gStyle->SetDrawBorder(drawborder);}
  void SetEndErrorSize(float np = 2) {gStyle->SetEndErrorSize(np);}
  void SetErrorX(float errorx = 0.5) {gStyle->SetErrorX(errorx);}
  void SetFitFormat(QString format = "5.4g") {gStyle->SetFitFormat(format.toLocal8Bit());}
  void SetFrameBorderMode(int mode = 1) {gStyle->SetFrameBorderMode(mode);}
  void SetFrameBorderSize(int size = 1) {gStyle->SetFrameBorderSize(size);}
  void SetFrameFillColor(int color = 1) {gStyle->SetFrameFillColor(color);}
  void SetFrameFillStyle(int styl = 0) {gStyle->SetFrameFillStyle(styl);}
  void SetFrameLineColor(int color = 1) {gStyle->SetFrameLineColor(color);}
  void SetFrameLineStyle(int styl = 0) {gStyle->SetFrameLineStyle(styl);}
  void SetFrameLineWidth(int width = 1) {gStyle->SetFrameLineWidth(width);}
  void SetFuncColor(int color = 1) {gStyle->SetFuncColor(color);}
  void SetFuncStyle(int style = 1) {gStyle->SetFuncStyle(style);}
  void SetFuncWidth(int width = 4) {gStyle->SetFuncWidth(width);}
  void SetGridColor(int color = 0) {gStyle->SetGridColor(color);}
  void SetGridStyle(int style = 3) {gStyle->SetGridStyle(style);}
  void SetGridWidth(int width = 1) {gStyle->SetGridWidth(width);}
  void SetHatchesLineWidth(int l) {gStyle->SetHatchesLineWidth(l);}
  void SetHatchesSpacing(double h) {gStyle->SetHatchesSpacing(h);}
  void SetHeaderPS(QString header) {gStyle->SetHeaderPS(header.toLocal8Bit());}
  void SetHistFillColor(int color = 1) {gStyle->SetHistFillColor(color);}
  void SetHistFillStyle(int styl = 0) {gStyle->SetHistFillStyle(styl);}
  void SetHistLineColor(int color = 1) {gStyle->SetHistLineColor(color);}
  void SetHistLineStyle(int styl = 0) {gStyle->SetHistLineStyle(styl);}
  void SetHistLineWidth(int width = 1) {gStyle->SetHistLineWidth(width);}
  void SetHistMinimumZero(bool zero = true) {gStyle->SetHistMinimumZero(zero);}
  void SetHistTopMargin(double hmax = 0.05) {gStyle->SetHistTopMargin(hmax);}
  void SetIsReading(bool reading = true) {gStyle->SetIsReading(reading);}
  void SetLabelColor(int color = 1, QString axis = "X") {gStyle->SetLabelColor(color, axis.toLocal8Bit());}
  void SetLabelFont(int font = 62, QString axis = "X") {gStyle->SetLabelFont(font, axis.toLocal8Bit());}
  void SetLabelOffset(float offset = 0.005, QString axis = "X") {gStyle->SetLabelOffset(offset, axis.toLocal8Bit());}
  void SetLabelSize(float size = 0.04, Option_t* axis = "X") {gStyle->SetLabelSize(size, axis);}
  void SetLegendBorderSize(int size = 4) {gStyle->SetLegendBorderSize(size);}
  void SetLegendFillColor(int color = 0){gStyle->SetLegendFillColor(color);}
  void SetLegendFont(int font = 62) {gStyle->SetLegendFont(font);}
  void SetLegoInnerR(float rad = 0.5){gStyle->SetLegoInnerR(rad);}
  void SetLineScalePS(float scale = 3) {gStyle->SetLineScalePS(scale);}
  void SetLineStyleString(int i, QString text){gStyle->SetLineStyleString(i, text.toLocal8Bit());}
  void SetNdivisions(int n = 510, QString axis = "X") {gStyle->SetNdivisions(n, axis.toLocal8Bit());}
  void SetNumberContours(int number = 20)  {gStyle->SetNumberContours(number);}
  void SetOptDate(int datefl = 1) {gStyle->SetOptDate(datefl);}
  void SetOptFile(int file = 1) {gStyle->SetOptFile(file);}
  void SetOptFit(int fit = 1) {gStyle->SetOptFit(fit);}
  void SetOptLogx(int logx = 1) {gStyle->SetOptLogx(logx);}
  void SetOptLogy(int logy = 1) {gStyle->SetOptLogy(logy);}
  void SetOptLogz(int logz = 1) {gStyle->SetOptLogz(logz);}
  void SetOptStat(int stat = 1) {gStyle->SetOptStat(stat);}
  void SetOptStat(QString stat) {gStyle->SetOptStat(stat.toLocal8Bit().constData());}
  void SetOptTitle(int tit = 1) {gStyle->SetOptTitle(tit);}
  void SetPadBorderMode(int mode = 1) {gStyle->SetPadBorderMode(mode);}
  void SetPadBorderSize(int size = 1) {gStyle->SetPadBorderSize(size);}
  void SetPadBottomMargin(float margin = 0.1) {gStyle->SetPadBottomMargin(margin);}
  void SetPadColor(int color = 19) {gStyle->SetPadColor(color);}
  void SetPadGridX(bool gridx) {gStyle->SetPadGridX(gridx);}
  void SetPadGridY(bool gridy) {gStyle->SetPadGridY(gridy);}
  void SetPadLeftMargin(float margin = 0.1) {gStyle->SetPadLeftMargin(margin);}
  void SetPadRightMargin(float margin = 0.1) {gStyle->SetPadRightMargin(margin);}
  void SetPadTickX(int tickx) {gStyle->SetPadTickX(tickx);}
  void SetPadTickY(int ticky) {gStyle->SetPadTickY(ticky);}
  void SetPadTopMargin(float margin = 0.1) {gStyle->SetPadTopMargin(margin);}
  void SetPaintTextFormat(QString format = "g") {gStyle->SetPaintTextFormat(format.toLocal8Bit());}
  void SetPalette(int scheme) {gStyle->SetPalette(scheme);} //51 - 56, default 1
  //void SetPaperSize(TStyle::EPaperSize size)
  void SetPaperSize(float xsize = 20, float ysize = 26) {gStyle->SetPaperSize(xsize, ysize);}
  void SetScreenFactor(float factor = 1) {gStyle->SetScreenFactor(factor);}
  void SetStatBorderSize(int size = 2) {gStyle->SetStatBorderSize(size);}
  void SetStatColor(int color = 19) {gStyle->SetStatColor(color);}
  void SetStatFont(int font = 62) {gStyle->SetStatFont(font);}
  void SetStatFontSize(float size = 0) {gStyle->SetStatFontSize(size);}
  void SetStatFormat(QString format = "6.4g") {gStyle->SetStatFormat(format.toLocal8Bit());}
  void SetStatH(float h = 0.1) {gStyle->SetStatH(h);}
  void SetStatStyle(int style = 1001) {gStyle->SetStatStyle(style);}
  void SetStatTextColor(int color = 1) {gStyle->SetStatTextColor(color);}
  void SetStatW(float w = 0.19) {gStyle->SetStatW(w);}
  void SetStatX(float x = 0) {gStyle->SetStatX(x);}
  void SetStatY(float y = 0) {gStyle->SetStatY(y);}
  void SetStripDecimals(bool strip = true) {gStyle->SetStripDecimals(strip);}
  void SetTickLength(float length = 0.03, QString axis = "X") {gStyle->SetTickLength(length, axis.toLocal8Bit());}
  void SetTimeOffset(double toffset) {gStyle->SetTimeOffset(toffset);}
  void SetTitleAlign(int a = 13) {gStyle->SetTitleAlign(a);}
  void SetTitleBorderSize(int size = 2) {gStyle->SetTitleBorderSize(size);}
  void SetTitleColor(int color = 1, QString axis = "X") {gStyle->SetTitleColor(color, axis.toLocal8Bit());}
  void SetTitleFillColor(int color = 1) {gStyle->SetTitleFillColor(color);}
  void SetTitleFont(int font = 62, QString axis = "X") {gStyle->SetTitleFont(font, axis.toLocal8Bit());}
  void SetTitleFontSize(float size = 0) {gStyle->SetTitleFontSize(size);}
  void SetTitleH(float h = 0) {gStyle->SetTitleH(h);}
  void SetTitleOffset(float offset = 1, QString axis = "X") {gStyle->SetTitleOffset(offset, axis.toLocal8Bit());}
  void SetTitlePS(QString pstitle) {gStyle->SetTitlePS(pstitle.toLocal8Bit());}
  void SetTitleSize(float size = 0.02, QString axis = "X") {gStyle->SetTitleSize(size, axis.toLocal8Bit());}
  void SetTitleStyle(int style = 1001) {gStyle->SetTitleStyle(style);}
  void SetTitleTextColor(int color = 1) {gStyle->SetTitleTextColor(color);}
  void SetTitleW(float w = 0) {gStyle->SetTitleW(w);}
  void SetTitleX(float x = 0) {gStyle->SetTitleX(x);}
  void SetTitleXOffset(float offset = 1) {gStyle->SetTitleXOffset(offset);}
  void SetTitleXSize(float size = 0.02) {gStyle->SetTitleXSize(size);}
  void SetTitleY(float y = 0.985) {gStyle->SetTitleY(y);}
  void SetTitleYOffset(float offset = 1) {gStyle->SetTitleYOffset(offset);}
  void SetTitleYSize(float size = 0.02) {gStyle->SetTitleYSize(size);}
};

#endif // AINTERFACETOGSTYLESCRIPT_H
