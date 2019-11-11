#ifndef HISTGRAPHINTERFACES_H
#define HISTGRAPHINTERFACES_H

#include "ascriptinterface.h"

#include <QString>
#include <QVariant>
#include <QVariantList>

class TmpObjHubClass;
class TObject;

class AInterfaceToHist : public AScriptInterface
{
  Q_OBJECT

public:
  AInterfaceToHist(TmpObjHubClass *TmpHub);
  AInterfaceToHist(const AInterfaceToHist& other);
  ~AInterfaceToHist(){}

  //bool           InitOnRun() override {}
  bool           IsMultithreadCapable() const override {return true;}

public slots:
  void           SetAbortIfAlreadyExists(bool flag) {bAbortIfExists = flag;}

  void           NewHist(const QString& HistName, int bins, double start, double stop);
  void           NewHist2D(const QString &HistName, int binsX, double startX, double stopX, int binsY, double startY, double stopY);

  void           SetXCustomLabels(const QString &HistName, QVariantList Labels);

  void           SetTitle(const QString& HistName, const QString& Title);
  void           SetTitles(const QString& HistName, QString X_Title, QString Y_Title, QString Z_Title = "");
  void           SetLineProperties(const QString& HistName, int LineColor, int LineStyle, int LineWidth);
  void           SetMarkerProperties(const QString& HistName, int MarkerColor, int MarkerStyle, double MarkerSize);
  void           SetFillColor(const QString& HistName, int Color);
  void           SetMaximum(const QString& HistName, double max);
  void           SetMinimum(const QString& HistName, double min);
  void           SetXDivisions(const QString& HistName, int primary, int secondary, int tertiary, bool canOptimize);
  void           SetYDivisions(const QString& HistName, int primary, int secondary, int tertiary, bool canOptimize);
  void           SetXLabelProperties(const QString& HistName, double size, double offset);
  void           SetYLabelProperties(const QString& HistName, double size, double offset);

  void           Fill(const QString& HistName, double val, double weight);
  void           Fill2D(const QString& HistName, double x, double y, double weight);

  void           FillArr(const QString& HistName, const QVariant XY_Array);
  void           FillArr(const QString& HistName, const QVariantList X_Array, const QVariantList Y_Array);
  void           Fill2DArr(const QString& HistName, const QVariant Array);

  void           Divide(const QString& HistName, const QString& HistToDivideWith);

  void           Draw(const QString& HistName, const QString options = "");

  QVariantList   GetContent(const QString& HistName);
  double         GetUnderflowBin(const QString& HistName);
  double         GetOverflowBin(const QString& HistName);

  void           Smooth(const QString& HistName, int times);
  void           ApplyMedianFilter(const QString& HistName, int span);
  const QVariant FitGauss(const QString& HistName, const QString options = "");
  const QVariant FitGaussWithInit(const QString& HistName, const QVariant InitialParValues, const QString options = "");
  const QVariant FindPeaks(const QString& HistName, double sigma, double threshold);

  int            GetNumberOfEntries(const QString& HistName);
  void           SetNumberOfEntries(const QString& HistName, int numEntries);
  double         GetIntegral(const QString& HistName, bool MultiplyByBinWidth = false);
  double         GetMaximum(const QString& HistName);
  void           Scale(const QString& HistName, double ScaleIntegralTo, bool DividedByBinWidth = false);

  void           Save(const QString& HistName, const QString &fileName);
  void           Load(const QString& HistName, const QString &fileName, const QString histNameInFile = "");

  bool           Delete(const QString& HistName);
  void           DeleteAllHist();

signals:
  void           RequestDraw(TObject* obj, QString options, bool fFocus);

private:
  TmpObjHubClass *TmpHub;

  bool           bAbortIfExists = false;

};

// ---- G R A P H S ---- (TGraph and TgraphError of ROOT)
class AInterfaceToGraph : public AScriptInterface
{
  Q_OBJECT

public:
  AInterfaceToGraph(TmpObjHubClass *TmpHub);
  AInterfaceToGraph(const AInterfaceToGraph& other);
  ~AInterfaceToGraph(){}

  //virtual bool InitOnRun() override {}
  virtual bool IsMultithreadCapable() const override {return true;}

public slots:
  void SetAbortIfAlreadyExists(bool flag) {bAbortIfExists = flag;}

  void NewGraph(const QString& GraphName);
  void NewGraphErrors(const QString& GraphName);

  void SetMarkerProperties(QString GraphName, int MarkerColor, int MarkerStyle, double MarkerSize);
  void SetLineProperties(QString GraphName, int LineColor, int LineStyle, int LineWidth);
  void SetTitles(QString GraphName, QString X_Title, QString Y_Title, QString GraphTitle = "");

  void AddPoint(QString GraphName, double x, double y);
  void AddPoint(QString GraphName, double x, double y, double errorY);
  void AddPoint(QString GraphName, double x, double y, double errorX, double errorY);
  void AddPoints(QString GraphName, QVariant xArray, QVariant yArray);
  void AddPoints(QString GraphName, QVariant xArray, QVariant yArray, QVariant yErrArray);
  void AddPoints(QString GraphName, QVariant xArray, QVariant yArray, QVariant xErrArray, QVariant yErrArray);
  void AddPoints(QString GraphName, QVariant xyArray);

  void SetYRange(const QString& GraphName, double min, double max);
  void SetXRange(const QString& GraphName, double min, double max);
  void SetXDivisions(const QString& GraphName, int numDiv);
  void SetYDivisions(const QString& GraphName, int numDiv);

  void Sort(const QString& GraphName);

  void Draw(QString GraphName, QString options = "APL");

  void LoadTGraph(const QString& NewGraphName, const QString& FileName);
  void Save(const QString& GraphName, const QString& FileName);
  const QVariant GetPoints(const QString& GraphName);

  //void ImportFromBasket(const QString& NewGraphName, const QString& BasketName, int index = 0);

  bool Delete(QString GraphName);
  void DeleteAllGraph();

signals:
  void RequestDraw(TObject* obj, QString options, bool fFocus);

private:
  TmpObjHubClass *TmpHub;

  bool           bAbortIfExists = false;

};

#endif // HISTGRAPHINTERFACES_H
