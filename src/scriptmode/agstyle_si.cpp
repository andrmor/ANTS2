#include "agstyle_si.h"

#include "TStyle.h"

AGStyle_SI::AGStyle_SI() : AScriptInterface()
{
    Description = "See //https://root.cern.ch/root/html/TStyle.html - you can use practically asll Set... methods";
}

void AGStyle_SI::SetAxisColor(int color, QString axis) {gStyle->SetAxisColor(color, axis.toLocal8Bit());}

void AGStyle_SI::SetBarOffset(float baroff) {gStyle->SetBarOffset(baroff);}

void AGStyle_SI::SetBarWidth(float barwidth) {gStyle->SetBarWidth(barwidth);}

void AGStyle_SI::SetCanvasBorderMode(int mode) {gStyle->SetCanvasBorderMode(mode);}

void AGStyle_SI::SetCanvasBorderSize(int size) {gStyle->SetCanvasBorderSize(size);}

void AGStyle_SI::SetCanvasColor(int color) {gStyle->SetCanvasColor(color);}

void AGStyle_SI::SetCanvasDefH(int h) {gStyle->SetCanvasDefH(h);}

void AGStyle_SI::SetCanvasDefW(int w) {gStyle->SetCanvasDefW(w);}

void AGStyle_SI::SetCanvasDefX(int topx) {gStyle->SetCanvasDefX(topx);}

void AGStyle_SI::SetCanvasDefY(int topy) {gStyle->SetCanvasDefY(topy);}

void AGStyle_SI::SetCanvasPreferGL(bool prefer) {gStyle->SetCanvasPreferGL(prefer);}

void AGStyle_SI::SetColorModelPS(int c) {gStyle->SetColorModelPS(c);}

void AGStyle_SI::SetDateX(float x) {gStyle->SetDateX(x);}

void AGStyle_SI::SetDateY(float y) {gStyle->SetDateY(y);}

void AGStyle_SI::SetDrawBorder(int drawborder) {gStyle->SetDrawBorder(drawborder);}

void AGStyle_SI::SetEndErrorSize(float np) {gStyle->SetEndErrorSize(np);}

void AGStyle_SI::SetErrorX(float errorx) {gStyle->SetErrorX(errorx);}

void AGStyle_SI::SetFitFormat(QString format) {gStyle->SetFitFormat(format.toLocal8Bit());}

void AGStyle_SI::SetFrameBorderMode(int mode) {gStyle->SetFrameBorderMode(mode);}

void AGStyle_SI::SetFrameBorderSize(int size) {gStyle->SetFrameBorderSize(size);}

void AGStyle_SI::SetFrameFillColor(int color) {gStyle->SetFrameFillColor(color);}

void AGStyle_SI::SetFrameFillStyle(int styl) {gStyle->SetFrameFillStyle(styl);}

void AGStyle_SI::SetFrameLineColor(int color) {gStyle->SetFrameLineColor(color);}

void AGStyle_SI::SetFrameLineStyle(int styl) {gStyle->SetFrameLineStyle(styl);}

void AGStyle_SI::SetFrameLineWidth(int width) {gStyle->SetFrameLineWidth(width);}

void AGStyle_SI::SetFuncColor(int color) {gStyle->SetFuncColor(color);}

void AGStyle_SI::SetFuncStyle(int style) {gStyle->SetFuncStyle(style);}

void AGStyle_SI::SetFuncWidth(int width) {gStyle->SetFuncWidth(width);}

void AGStyle_SI::SetGridColor(int color) {gStyle->SetGridColor(color);}

void AGStyle_SI::SetGridStyle(int style) {gStyle->SetGridStyle(style);}

void AGStyle_SI::SetGridWidth(int width) {gStyle->SetGridWidth(width);}

void AGStyle_SI::SetHatchesLineWidth(int l) {gStyle->SetHatchesLineWidth(l);}

void AGStyle_SI::SetHatchesSpacing(double h) {gStyle->SetHatchesSpacing(h);}

void AGStyle_SI::SetHeaderPS(QString header) {gStyle->SetHeaderPS(header.toLocal8Bit());}

void AGStyle_SI::SetHistFillColor(int color) {gStyle->SetHistFillColor(color);}

void AGStyle_SI::SetHistFillStyle(int styl) {gStyle->SetHistFillStyle(styl);}

void AGStyle_SI::SetHistLineColor(int color) {gStyle->SetHistLineColor(color);}

void AGStyle_SI::SetHistLineStyle(int styl) {gStyle->SetHistLineStyle(styl);}

void AGStyle_SI::SetHistLineWidth(int width) {gStyle->SetHistLineWidth(width);}

void AGStyle_SI::SetHistMinimumZero(bool zero) {gStyle->SetHistMinimumZero(zero);}

void AGStyle_SI::SetHistTopMargin(double hmax) {gStyle->SetHistTopMargin(hmax);}

void AGStyle_SI::SetIsReading(bool reading) {gStyle->SetIsReading(reading);}

void AGStyle_SI::SetLabelColor(int color, QString axis) {gStyle->SetLabelColor(color, axis.toLocal8Bit());}

void AGStyle_SI::SetLabelFont(int font, QString axis) {gStyle->SetLabelFont(font, axis.toLocal8Bit());}

void AGStyle_SI::SetLabelOffset(float offset, QString axis) {gStyle->SetLabelOffset(offset, axis.toLocal8Bit());}

void AGStyle_SI::SetLabelSize(float size, QString axis) {gStyle->SetLabelSize(size, axis.toLocal8Bit());}

void AGStyle_SI::SetLegendBorderSize(int size) {gStyle->SetLegendBorderSize(size);}

void AGStyle_SI::SetLegendFillColor(int color){gStyle->SetLegendFillColor(color);}

void AGStyle_SI::SetLegendFont(int font) {gStyle->SetLegendFont(font);}

void AGStyle_SI::SetLegoInnerR(float rad){gStyle->SetLegoInnerR(rad);}

void AGStyle_SI::SetLineScalePS(float scale) {gStyle->SetLineScalePS(scale);}

void AGStyle_SI::SetLineStyleString(int i, QString text){gStyle->SetLineStyleString(i, text.toLocal8Bit());}

void AGStyle_SI::SetNdivisions(int n, QString axis) {gStyle->SetNdivisions(n, axis.toLocal8Bit());}

void AGStyle_SI::SetNumberContours(int number)  {gStyle->SetNumberContours(number);}

void AGStyle_SI::SetOptDate(int datefl) {gStyle->SetOptDate(datefl);}

void AGStyle_SI::SetOptFile(int file) {gStyle->SetOptFile(file);}

void AGStyle_SI::SetOptFit(int fit) {gStyle->SetOptFit(fit);}

void AGStyle_SI::SetOptLogx(int logx) {gStyle->SetOptLogx(logx);}

void AGStyle_SI::SetOptLogy(int logy) {gStyle->SetOptLogy(logy);}

void AGStyle_SI::SetOptLogz(int logz) {gStyle->SetOptLogz(logz);}

void AGStyle_SI::SetOptStat(int stat) {gStyle->SetOptStat(stat);}

void AGStyle_SI::SetOptStat(QString stat) {gStyle->SetOptStat(stat.toLocal8Bit().constData());}

void AGStyle_SI::SetOptTitle(int tit) {gStyle->SetOptTitle(tit);}

void AGStyle_SI::SetPadBorderMode(int mode) {gStyle->SetPadBorderMode(mode);}

void AGStyle_SI::SetPadBorderSize(int size) {gStyle->SetPadBorderSize(size);}

void AGStyle_SI::SetPadBottomMargin(float margin) {gStyle->SetPadBottomMargin(margin);}

void AGStyle_SI::SetPadColor(int color) {gStyle->SetPadColor(color);}

void AGStyle_SI::SetPadGridX(bool gridx) {gStyle->SetPadGridX(gridx);}

void AGStyle_SI::SetPadGridY(bool gridy) {gStyle->SetPadGridY(gridy);}

void AGStyle_SI::SetPadLeftMargin(float margin) {gStyle->SetPadLeftMargin(margin);}

void AGStyle_SI::SetPadRightMargin(float margin) {gStyle->SetPadRightMargin(margin);}

void AGStyle_SI::SetPadTickX(int tickx) {gStyle->SetPadTickX(tickx);}

void AGStyle_SI::SetPadTickY(int ticky) {gStyle->SetPadTickY(ticky);}

void AGStyle_SI::SetPadTopMargin(float margin) {gStyle->SetPadTopMargin(margin);}

void AGStyle_SI::SetPaintTextFormat(QString format) {gStyle->SetPaintTextFormat(format.toLocal8Bit());}

void AGStyle_SI::SetPalette(int scheme) {gStyle->SetPalette(scheme);}

void AGStyle_SI::SetPaperSize(float xsize, float ysize) {gStyle->SetPaperSize(xsize, ysize);}

void AGStyle_SI::SetScreenFactor(float factor) {gStyle->SetScreenFactor(factor);}

void AGStyle_SI::SetStatBorderSize(int size) {gStyle->SetStatBorderSize(size);}

void AGStyle_SI::SetStatColor(int color) {gStyle->SetStatColor(color);}

void AGStyle_SI::SetStatFont(int font) {gStyle->SetStatFont(font);}

void AGStyle_SI::SetStatFontSize(float size) {gStyle->SetStatFontSize(size);}

void AGStyle_SI::SetStatFormat(QString format) {gStyle->SetStatFormat(format.toLocal8Bit());}

void AGStyle_SI::SetStatH(float h) {gStyle->SetStatH(h);}

void AGStyle_SI::SetStatStyle(int style) {gStyle->SetStatStyle(style);}

void AGStyle_SI::SetStatTextColor(int color) {gStyle->SetStatTextColor(color);}

void AGStyle_SI::SetStatW(float w) {gStyle->SetStatW(w);}

void AGStyle_SI::SetStatX(float x) {gStyle->SetStatX(x);}

void AGStyle_SI::SetStatY(float y) {gStyle->SetStatY(y);}

void AGStyle_SI::SetStripDecimals(bool strip) {gStyle->SetStripDecimals(strip);}

void AGStyle_SI::SetTickLength(float length, QString axis) {gStyle->SetTickLength(length, axis.toLocal8Bit());}

void AGStyle_SI::SetTimeOffset(double toffset) {gStyle->SetTimeOffset(toffset);}

void AGStyle_SI::SetTitleAlign(int a) {gStyle->SetTitleAlign(a);}

void AGStyle_SI::SetTitleBorderSize(int size) {gStyle->SetTitleBorderSize(size);}

void AGStyle_SI::SetTitleColor(int color, QString axis) {gStyle->SetTitleColor(color, axis.toLocal8Bit());}

void AGStyle_SI::SetTitleFillColor(int color) {gStyle->SetTitleFillColor(color);}

void AGStyle_SI::SetTitleFont(int font, QString axis) {gStyle->SetTitleFont(font, axis.toLocal8Bit());}

void AGStyle_SI::SetTitleFontSize(float size) {gStyle->SetTitleFontSize(size);}

void AGStyle_SI::SetTitleH(float h) {gStyle->SetTitleH(h);}

void AGStyle_SI::SetTitleOffset(float offset, QString axis) {gStyle->SetTitleOffset(offset, axis.toLocal8Bit());}

void AGStyle_SI::SetTitlePS(QString pstitle) {gStyle->SetTitlePS(pstitle.toLocal8Bit());}

void AGStyle_SI::SetTitleSize(float size, QString axis) {gStyle->SetTitleSize(size, axis.toLocal8Bit());}

void AGStyle_SI::SetTitleStyle(int style) {gStyle->SetTitleStyle(style);}

void AGStyle_SI::SetTitleTextColor(int color) {gStyle->SetTitleTextColor(color);}

void AGStyle_SI::SetTitleW(float w) {gStyle->SetTitleW(w);}

void AGStyle_SI::SetTitleX(float x) {gStyle->SetTitleX(x);}

void AGStyle_SI::SetTitleXOffset(float offset) {gStyle->SetTitleXOffset(offset);}

void AGStyle_SI::SetTitleXSize(float size) {gStyle->SetTitleXSize(size);}

void AGStyle_SI::SetTitleY(float y) {gStyle->SetTitleY(y);}

void AGStyle_SI::SetTitleYOffset(float offset) {gStyle->SetTitleYOffset(offset);}

void AGStyle_SI::SetTitleYSize(float size) {gStyle->SetTitleYSize(size);}


