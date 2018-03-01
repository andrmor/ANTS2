#include "histgraphinterfaces.h"
#include "tmpobjhubclass.h"
#include "arootobjcollection.h"
#include "arootgraphrecord.h"

#include <QJsonArray>
#include <QJsonValue>
#include <QDebug>

#include "TObject.h"
#include "TH1D.h"
#include "TH1.h"
#include "TH2D.h"
#include "TH2.h"
#include "TF2.h"
#include "TGraph.h"
#include "TF1.h"

//----------------- HIST  -----------------
AInterfaceToHist::AInterfaceToHist(TmpObjHubClass* TmpHub)
  : TmpHub(TmpHub)
{
    H["FitGauss"] = "Fit histogram with a Gaussian. The returned result (is successful) contains an array [Constant,Mean,Sigma,ErrConstant,ErrMean,ErrSigma]"
            "\nOptional 'options' parameter is directly forwarded to TH1::Fit()";
    H["FitGaussWithInit"] = "Fit histogram with a Gaussian. The returned result (is successful) contains an array [Constant,Mean,Sigma,ErrConstant,ErrMean,ErrSigma]"
                            "\nInitialParValues is an array of initial parameters of the values [Constant,Mean,Sigma]"
                            "\nOptional 'options' parameter is directly forwarded to TH1::Fit()";
}

AInterfaceToHist::AInterfaceToHist(const AInterfaceToHist &other) :
    AScriptInterface(other)
{
    TmpHub = other.TmpHub;
    bGuiTthread = false;
}

bool AInterfaceToHist::InitOnRun()
{
  TmpHub->ScriptDrawObjects.clear();
  return true;
}

void AInterfaceToHist::NewHist(QString HistName, int bins, double start, double stop)
{
  if (!bGuiTthread)
  {
      abort("Threads cannot create/delete/draw histograms!");
      return;
  }

  int index = TmpHub->ScriptDrawObjects.findIndexOf(HistName);
  if (index != -1)
    {
      abort("Bad new hist name! Object "+HistName+" already exists");
      return;
    }

  TH1D* hist = new TH1D("", HistName.toLatin1().data(), bins, start, stop);
  hist->GetYaxis()->SetTitleOffset((Float_t)1.30);
  TmpHub->ScriptDrawObjects.append(hist, HistName, "TH1D");
}

void AInterfaceToHist::NewHist2D(QString HistName, int binsX, double startX, double stopX, int binsY, double startY, double stopY)
{
    if (!bGuiTthread)
    {
        abort("Threads cannot create/delete/draw histograms!");
        return;
    }

  int index = TmpHub->ScriptDrawObjects.findIndexOf(HistName);
  if (index != -1)
    {
      abort("Bad new hist name! Object "+HistName+" already exists");
      return;
    }

  TH2D* hist = new TH2D("", HistName.toLatin1().data(), binsX, startX, stopX, binsY, startY, stopY);
  hist->GetYaxis()->SetTitleOffset((Float_t)1.30);
  TmpHub->ScriptDrawObjects.append(hist, HistName, "TH2D");
}

void AInterfaceToHist::SetTitles(QString HistName, QString X_Title, QString Y_Title, QString Z_Title)
{
  int index = TmpHub->ScriptDrawObjects.findIndexOf(HistName);
  if (index == -1)
    {
      abort("Histogram "+HistName+" not found!");
      return;
    }

  AScriptDrawItem& r = TmpHub->ScriptDrawObjects.List[index];
  r.Xtitle = X_Title;
  r.Ytitle = Y_Title;
  r.Ztitle = Z_Title;
}

void AInterfaceToHist::SetLineProperties(QString HistName, int LineColor, int LineStyle, int LineWidth)
{
  int index = TmpHub->ScriptDrawObjects.findIndexOf(HistName);
  if (index == -1)
    {
      abort("Histogram "+HistName+" not found!");
      return;
    }

  AScriptDrawItem& r = TmpHub->ScriptDrawObjects.List[index];
  r.LineColor = LineColor;
  r.LineStyle = LineStyle;
  r.LineWidth = LineWidth;
}

void AInterfaceToHist::Fill(QString HistName, double val, double weight)
{
  int index = TmpHub->ScriptDrawObjects.findIndexOf(HistName);
  if (index == -1)
    {
      abort("Histogram "+HistName+" not found!");
      return;
    }

  AScriptDrawItem& r = TmpHub->ScriptDrawObjects.List[index];
  if (r.type == "TH1D")
    {
      TH1D* h = static_cast<TH1D*>(r.Obj);
      h->Fill(val, weight);
    }
  else
    {
      abort("TH1D histogram "+HistName+" not found!");
      return;
    }
}

void AInterfaceToHist::Fill2D(QString HistName, double x, double y, double weight)
{
  int index = TmpHub->ScriptDrawObjects.findIndexOf(HistName);
  if (index == -1)
    {
      abort("Histogram "+HistName+" not found!");
      return;
    }

  AScriptDrawItem& r = TmpHub->ScriptDrawObjects.List[index];
  if (r.type == "TH2D")
    {
      TH2D* h = static_cast<TH2D*>(r.Obj);
      h->Fill(x, y, weight);
    }
  else
    {
      abort("TH2D histogram "+HistName+" not found!");
      return;
    }
}

void AInterfaceToHist::Smooth(QString HistName, int times)
{
  int index = TmpHub->ScriptDrawObjects.findIndexOf(HistName);
  if (index == -1)
    {
      abort("Histogram "+HistName+" not found!");
      return;
    }

  AScriptDrawItem& r = TmpHub->ScriptDrawObjects.List[index];
  if (r.type == "TH1D")
    {
      TH1D* h = static_cast<TH1D*>(r.Obj);
      h->Smooth(times);
      emit RequestDraw(0, "", true); //to update
    }
  else if (r.type == "TH2D")
    {
      TH2D* h = static_cast<TH2D*>(r.Obj);
      h->Smooth(times);
      emit RequestDraw(0, "", true); //to update
    }
  else
    {
      abort("Object "+HistName+": unknown histogram type!");
      return;
  }
}

QVariant ReturnNanArray(int num)
{
    QJsonArray ar;
    for (int i=0; i<num; i++) ar << std::numeric_limits<double>::quiet_NaN();
    QJsonValue jv = ar;
    QVariant res = jv.toVariant();
    return res;
}

QVariant AInterfaceToHist::FitGauss(QString HistName, QString options)
{
    int index = TmpHub->ScriptDrawObjects.findIndexOf(HistName);
    if (index == -1)
      {
        abort("Histogram "+HistName+" not found!");
        return ReturnNanArray(6);
      }

    AScriptDrawItem& r = TmpHub->ScriptDrawObjects.List[index];
    if (r.type.startsWith("TH1"))
      {
        TH1* h = static_cast<TH1*>(r.Obj);
        TF1 *f1 = new TF1("f1", "gaus");
        int status = h->Fit(f1, options.toLatin1());
        if (status != 0) return ReturnNanArray(6);

        emit RequestDraw(0, "", true); //to update

        QJsonArray ar;
        for (int i=0; i<3; i++) ar << f1->GetParameter(i);
        for (int i=0; i<3; i++) ar << f1->GetParError(i);

        QJsonValue jv = ar;
        QVariant res = jv.toVariant();
        return res;
      }
    else
      {
        abort("Object "+HistName+": unsupported histogram type!");
        return ReturnNanArray(6);
    }
}

QVariant AInterfaceToHist::FitGaussWithInit(QString HistName, QVariant InitialParValues, QString options)
{
    int index = TmpHub->ScriptDrawObjects.findIndexOf(HistName);
    if (index == -1)
      {
        abort("Histogram "+HistName+" not found!");
        return ReturnNanArray(6);
      }

    QString type = InitialParValues.typeName();
    if (type != "QVariantList")
    {
        abort("InitialParValues has to be an array of three numeric values");
        return ReturnNanArray(6);
    }

    QVariantList vl = InitialParValues.toList();
    QJsonArray ar = QJsonArray::fromVariantList(vl);
    if (ar.size() < 3)
    {
        abort("InitialParValues has to be an array of three numeric values");
        return ReturnNanArray(6);
    }
    if (!ar[0].isDouble() || !ar[1].isDouble() || !ar[2].isDouble() )
    {
        abort("InitialParValues has to be an array of three numeric values");
        return ReturnNanArray(6);
    }

    AScriptDrawItem& r = TmpHub->ScriptDrawObjects.List[index];
    if (r.type.startsWith("TH1"))
      {
        TH1* h = static_cast<TH1*>(r.Obj);

        TF1 *f1 = new TF1("f1","[0]*exp(-0.5*((x-[1])/[2])^2)");
        f1->SetParameters(ar[0].toDouble(), ar[1].toDouble(), ar[2].toDouble());

        int status = h->Fit(f1, options.toLatin1());
        if (status != 0) return ReturnNanArray(6);

        emit RequestDraw(0, "", true); //to update

        QJsonArray ar;
        for (int i=0; i<3; i++) ar << f1->GetParameter(i);
        for (int i=0; i<3; i++) ar << f1->GetParError(i);

        QJsonValue jv = ar;
        QVariant res = jv.toVariant();
        return res;
      }
    else
      {
        abort("Object "+HistName+": unsupported histogram type!");
        return ReturnNanArray(6);
    }
}

bool AInterfaceToHist::Delete(QString HistName)
{
    if (!bGuiTthread)
    {
        abort("Threads cannot create/delete/draw histograms!");
        return false;
    }

    return TmpHub->ScriptDrawObjects.remove(HistName);
}

void AInterfaceToHist::DeleteAllHist()
{
    if (!bGuiTthread)
    {
        abort("Threads cannot create/delete/draw histograms!");
        return;
    }

    TmpHub->ScriptDrawObjects.removeAllHists();
}

void AInterfaceToHist::Draw(QString HistName, QString options)
{
    if (!bGuiTthread)
    {
        abort("Threads cannot create/delete/draw histograms!");
        return;
    }

  int index = TmpHub->ScriptDrawObjects.findIndexOf(HistName);
  if (index == -1)
    {
      abort("Histogram "+HistName+" not found!");
      return;
    }

  AScriptDrawItem& r = TmpHub->ScriptDrawObjects.List[index];
  if (r.type == "TH1D")
    {
      TH1D* h = static_cast<TH1D*>(r.Obj);
      h->SetXTitle(r.Xtitle.toLatin1().data());
      h->SetYTitle(r.Ytitle.toLatin1().data());
      h->SetLineColor(r.LineColor);
      h->SetLineWidth(r.LineWidth);
      h->SetLineStyle(r.LineStyle);
      emit RequestDraw(h, options, true);
    }
  else if (r.type == "TH2D")
    {
      TH2D* h = static_cast<TH2D*>(r.Obj);
      h->SetXTitle(r.Xtitle.toLatin1().data());
      h->SetYTitle(r.Ytitle.toLatin1().data());
      h->SetZTitle(r.Ztitle.toLatin1().data());
      h->SetLineColor(r.LineColor);
      h->SetLineWidth(r.LineWidth);
      h->SetLineStyle(r.LineStyle);
      emit RequestDraw(h, options, true);
    }
  else
    {
      abort("Object "+HistName+": unknown histogram type!");
      return;
    }
}

// --------------------- End of HIST ------------------------

//----------------------------------
AInterfaceToGraph::AInterfaceToGraph(TmpObjHubClass *TmpHub)
  : TmpHub(TmpHub)
{
  H["NewGraph"] = "Creates a new graph (Root TGraph object)";
  H["SetMarkerProperties"] = "Default marker properties are 1, 20, 1";
  H["SetLineProperties"] = "Default line properties are 1, 1, 2";
  H["Draw"] = "Draws the graph (use \"APL\" options if in doubt)";
}

bool AInterfaceToGraph::InitOnRun()
{
  TmpHub->ScriptDrawObjects.clear();
  return true;
}

void AInterfaceToGraph::NewGraph(const QString &GraphName)
{
    TGraph* gr = new TGraph();
    bool bOK = TmpHub->Graphs.append(gr, GraphName, "TGraph");
    if (!bOK)
    {
        delete gr;
        abort("Graph "+GraphName+" already exists!");
    }
    else
    {
        gr->SetFillColor(0);
        gr->SetFillStyle(0);
    }
}

void AInterfaceToGraph::SetMarkerProperties(QString GraphName, int MarkerColor, int MarkerStyle, int MarkerSize)
{
    ARootGraphRecord* r = TmpHub->Graphs.getRecord(GraphName);
    if (!r)
        abort("Graph "+GraphName+" not found!");
    else
        r->SetMarkerProperties(MarkerColor, MarkerStyle, MarkerSize);
}

void AInterfaceToGraph::SetLineProperties(QString GraphName, int LineColor, int LineStyle, int LineWidth)
{
    ARootGraphRecord* r = TmpHub->Graphs.getRecord(GraphName);
    if (!r)
        abort("Graph "+GraphName+" not found!");
    else
        r->SetLineProperties(LineColor, LineStyle, LineWidth);
}

void AInterfaceToGraph::SetTitles(QString GraphName, QString X_Title, QString Y_Title)
{
    ARootGraphRecord* r = TmpHub->Graphs.getRecord(GraphName);
    if (!r)
        abort("Graph "+GraphName+" not found!");
    else
        r->SetAxisTitles(X_Title, Y_Title);
}

void AInterfaceToGraph::AddPoint(QString GraphName, double x, double y)
{
    ARootGraphRecord* r = TmpHub->Graphs.getRecord(GraphName);
    if (!r)
        abort("Graph "+GraphName+" not found!");
    else
        r->AddPoint(x, y);
}

void AInterfaceToGraph::AddPoints(QString GraphName, QVariant xArray, QVariant yArray)
{
    const QVariantList vx = xArray.toList();
    const QVariantList vy = yArray.toList();
    if (vx.isEmpty() || vx.size() != vy.size())
    {
        abort("Empty array or mismatch in array sizes in AddPoints for graph " + GraphName);
        return;
    }

    ARootGraphRecord* r = TmpHub->Graphs.getRecord(GraphName);
    if (!r)
        abort("Graph "+GraphName+" not found!");
    else
    {
        QVector<double> xArr(vx.size());
        QVector<double> yArr(vx.size());
        bool bValidX, bValidY;

        for (int i=0; i<vx.size(); i++)
        {
            double x = vx.at(i).toDouble(&bValidX);
            double y = vy.at(i).toDouble(&bValidY);
            if (bValidX && bValidY)
            {
                //  qDebug() << i << x << y;
                xArr[i] = x;
                yArr[i] = y;
            }
            else
            {
                abort("Not numeric value found in AddPoints() for " + GraphName);
                return;
            }
        }
        r->AddPoints(xArr, yArr);
    }
}

void AInterfaceToGraph::AddPoints(QString GraphName, QVariant xyArray)
{
    const QVariantList v = xyArray.toList();
    if (v.isEmpty())
    {
        abort("Empty array in AddPoints for graph " + GraphName);
        return;
    }

    bool bError = false;
    bool bValidX, bValidY;
    QVector<double> xArr(v.size()), yArr(v.size());
    for (int i=0; i<v.size(); i++)
    {
        const QVariantList vxy = v.at(i).toList();
        if (vxy.size() != 2)
        {
            bError = true;
            break;
        }
        double x = vxy.at(0).toDouble(&bValidX);
        double y = vxy.at(1).toDouble(&bValidY);
        if (bValidX && bValidY)
        {
            xArr[i] = x;
            yArr[i] = y;
        }
        else
        {
            bError = true;
            break;
        }
    }
    if (bError)
    {
        abort("Invalid array in AddPoints() for graph " + GraphName);
        return;
    }

    ARootGraphRecord* r = TmpHub->Graphs.getRecord(GraphName);
    if (!r)
        abort("Graph "+GraphName+" not found!");
    else
        r->AddPoints(xArr, yArr);
}

void AInterfaceToGraph::Sort(const QString &GraphName)
{
    ARootGraphRecord* r = TmpHub->Graphs.getRecord(GraphName);
    if (!r)
        abort("Graph "+GraphName+" not found!");
    else
        r->Sort();
}

void AInterfaceToGraph::Draw(QString GraphName, QString options)
{
    ARootGraphRecord* r = TmpHub->Graphs.getRecord(GraphName);
    if (!r)
        abort("Graph "+GraphName+" not found!");
    else
    {
        TObject* g = r->GetGraphForDrawing();
        if (!g)
        {   //paranoic
            abort("Error: Graph "+GraphName+" was created with empty object!");
            return;
        }

        emit RequestDraw(g, options, true);
    }
}

bool AInterfaceToGraph::Delete(QString GraphName)
{
    return TmpHub->Graphs.remove(GraphName);
}

void AInterfaceToGraph::DeleteAllGraph()
{
    TmpHub->Graphs.clear();
}
