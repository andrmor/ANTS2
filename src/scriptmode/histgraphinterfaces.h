#ifndef HISTGRAPHINTERFACES_H
#define HISTGRAPHINTERFACES_H

#include "ascriptinterface.h"

#include <QString>
#include <QVariant>

class TmpObjHubClass;
class TObject;

class AInterfaceToHist : public AScriptInterface
{
  Q_OBJECT

public:
  AInterfaceToHist(TmpObjHubClass *TmpHub);
  AInterfaceToHist(const AInterfaceToHist& other);
  ~AInterfaceToHist(){}

  virtual bool InitOnRun() override;
  virtual bool IsMultithreadCapable() const override {return true;}

public slots:
  void NewHist(QString HistName, int bins, double start, double stop);
  void NewHist2D(QString HistName, int binsX, double startX, double stopX, int binsY, double startY, double stopY);

  void SetTitles(QString HistName, QString X_Title, QString Y_Title, QString Z_Title = "");
  void SetLineProperties(QString HistName, int LineColor, int LineStyle, int LineWidth);

  void Fill(QString HistName, double val, double weight);
  void Fill2D(QString HistName, double x, double y, double weight);

  void Draw(QString HistName, QString options);

  void Smooth(QString HistName, int times);
  QVariant FitGauss(QString HistName, QString options="");
  QVariant FitGaussWithInit(QString HistName, QVariant InitialParValues, QString options="");

  bool Delete(QString HistName);
  void DeleteAllHist();

signals:
  void RequestDraw(TObject* obj, QString options, bool fFocus);

private:
  TmpObjHubClass *TmpHub;
  bool bGuiTthread = true;
};

// ---- G R A P H S ---- (TGraph of ROOT)
class AInterfaceToGraph : public AScriptInterface
{
  Q_OBJECT

public:
  AInterfaceToGraph(TmpObjHubClass *TmpHub);
  ~AInterfaceToGraph(){}

  virtual bool InitOnRun() override;
  virtual bool IsMultithreadCapable() const override {return true;}

public slots:
  void NewGraph(QString GraphName);
  void SetMarkerProperties(QString GraphName, int MarkerColor, int MarkerStyle, int MarkerSize);
  void SetLineProperties(QString GraphName, int LineColor, int LineStyle, int LineWidth);
  void SetTitles(QString GraphName, QString X_Title, QString Y_Title);

  void AddPoint(QString GraphName, double x, double y);
  void AddPoints(QString GraphName, QVariant xArray, QVariant yArray);
  void AddPoints(QString GraphName, QVariant xyArray);

  void Draw(QString GraphName, QString options);

  bool Delete(QString GraphName);
  void DeleteAllGraph();

signals:
  void RequestDraw(TObject* obj, QString options, bool fFocus);

private:
  TmpObjHubClass *TmpHub;
};

#endif // HISTGRAPHINTERFACES_H
