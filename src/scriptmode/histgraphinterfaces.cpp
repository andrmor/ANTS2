#include "histgraphinterfaces.h"
#include "tmpobjhubclass.h"
#include "arootobjcollection.h"
#include "arootgraphrecord.h"
#include "aroothistrecord.h"

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

void AInterfaceToHist::NewHist(const QString& HistName, int bins, double start, double stop)
{
    if (!bGuiTthread)
    {
        abort("Threads cannot create/delete/draw histograms!");
        return;
    }

    TH1D* hist = new TH1D("", HistName.toLatin1().data(), bins, start, stop);
    ARootHistRecord* rec = new ARootHistRecord(hist, HistName, "TH1D");

    bool bOK = TmpHub->Hists.append(HistName, rec);
    if (!bOK)
    {
        delete rec;
        abort("Histogram " + HistName+" already exists!");
    }
    else
    {
        hist->GetYaxis()->SetTitleOffset(1.30f);
    }
}

void AInterfaceToHist::NewHist2D(const QString& HistName, int binsX, double startX, double stopX, int binsY, double startY, double stopY)
{
    if (!bGuiTthread)
    {
        abort("Threads cannot create/delete/draw histograms!");
        return;
    }

    TH2D* hist = new TH2D("", HistName.toLatin1().data(), binsX, startX, stopX, binsY, startY, stopY);
    ARootHistRecord* rec = new ARootHistRecord(hist, HistName, "TH2D");

    bool bOK = TmpHub->Hists.append(HistName, rec);
    if (!bOK)
    {
        delete rec;
        abort("Histogram " + HistName+" already exists!");
    }
    else
    {
        hist->GetYaxis()->SetTitleOffset(1.30f);
    }
}

void AInterfaceToHist::SetTitles(const QString& HistName, QString X_Title, QString Y_Title, QString Z_Title)
{
    ARootHistRecord* r = static_cast<ARootHistRecord*>(TmpHub->Hists.getRecord(HistName));
    if (!r)
        abort("Histogram " + HistName + " not found!");
    else
        r->SetTitles(X_Title, Y_Title, Z_Title);
}

void AInterfaceToHist::SetLineProperties(const QString &HistName, int LineColor, int LineStyle, int LineWidth)
{
    ARootHistRecord* r = static_cast<ARootHistRecord*>(TmpHub->Hists.getRecord(HistName));
    if (!r)
        abort("Histogram " + HistName + " not found!");
    else
        r->SetLineProperties(LineColor, LineStyle, LineWidth);
}

void AInterfaceToHist::Fill(const QString &HistName, double val, double weight)
{
    ARootHistRecord* r = static_cast<ARootHistRecord*>(TmpHub->Hists.getRecord(HistName));
    if (!r)
        abort("Histogram " + HistName + " not found!");
    else
        r->Fill(val, weight);
}

void AInterfaceToHist::Fill2D(const QString &HistName, double x, double y, double weight)
{
    ARootHistRecord* r = static_cast<ARootHistRecord*>(TmpHub->Hists.getRecord(HistName));
    if (!r)
        abort("Histogram " + HistName + " not found!");
    else
        r->Fill2D(x, y, weight);
}

void AInterfaceToHist::Smooth(const QString &HistName, int times)
{
    ARootHistRecord* r = static_cast<ARootHistRecord*>(TmpHub->Hists.getRecord(HistName));
    if (!r)
        abort("Histogram " + HistName + " not found!");
    else
    {
        r->Smooth(times);
        if (bGuiTthread) emit RequestDraw(0, "", true); //to update
    }
}

void AInterfaceToHist::FillArr(const QString &HistName, const QVariant Array)
{
    ARootHistRecord* r = static_cast<ARootHistRecord*>(TmpHub->Hists.getRecord(HistName));
    if (!r)
        abort("Histogram " + HistName + " not found!");
    else
    {
        QVariantList VarList = Array.toList();
        if (VarList.isEmpty())
        {
            abort("Array (or array of arrays) is expected as the second argument in hist.FillArr()");
            return;
        }

        QVector<double> val(VarList.size()), weight(VarList.size());
        bool bOK1, bOK2;
        for (int i=0; i<VarList.size(); i++)
        {
            QVariantList element = VarList.at(i).toList();
            switch (element.size())
            {
             case 2:
               {
                    double v = element.at(0).toDouble(&bOK1);
                    double w = element.at(1).toDouble(&bOK2);
                    if (bOK1 && bOK2)
                    {
                        val[i] = v;
                        weight[i] = w;
                        continue;  // NEXT
                    }
                    break;
               }
             case 0:
               {
                    double v = VarList.at(i).toDouble(&bOK1);
                    if (bOK1)
                    {
                        val[i] = v;
                        weight[i] = 1.0;
                        continue;  // NEXT
                    }
                    break;
               }
             default:
               break;
            }

            abort("hist.FillArr(): the second argument has to be array of values (then weight=1) or arrays of [val, weight]");
            return;
        }

        r->FillArr(val, weight);
    }
}

void AInterfaceToHist::Fill2DArr(const QString &HistName, const QVariant Array)
{
    ARootHistRecord* r = static_cast<ARootHistRecord*>(TmpHub->Hists.getRecord(HistName));
    if (!r)
        abort("Histogram " + HistName + " not found!");
    else
    {
        QVariantList VarList = Array.toList();
        if (VarList.isEmpty())
        {
            abort("Array of arrays is expected as the second argument in hist.Fill2DArr()");
            return;
        }

        QVector<double> xx(VarList.size()), yy(VarList.size()), weight(VarList.size());
        bool bOK1, bOK2, bOK3;
        for (int i=0; i<VarList.size(); i++)
        {
            QVariantList element = VarList.at(i).toList();
            switch (element.size())
            {
             case 2:
               {
                    double x = element.at(0).toDouble(&bOK1);
                    double y = element.at(1).toDouble(&bOK2);
                    if (bOK1 && bOK2)
                    {
                        xx[i] = x;
                        yy[i] = y;
                        weight[i] = 1.0;
                        continue;  // NEXT
                    }
                    break;
               }
             case 3:
               {
                    double x = element.at(0).toDouble(&bOK1);
                    double y = element.at(1).toDouble(&bOK2);
                    double w = element.at(2).toDouble(&bOK3);
                    if (bOK1 && bOK2 && bOK3)
                    {
                        xx[i] = x;
                        yy[i] = y;
                        weight[i] = w;
                        continue;  // NEXT
                    }
                    break;
               }
             default:
               break;
            }

            abort("hist.Fill2DArr(): the second argument has to be array of arrays: [x, y] (then weight is 1) or [x, y, weight]");
            return;
        }

        r->Fill2DArr(xx, yy, weight);
    }
}

/*
void AInterfaceToHist::Divide(const QString &HistName, const QString &HistToDivideWith)
{
    int index = TmpHub->ScriptDrawObjects.findIndexOf(HistName);
    if (index == -1)
      {
        abort("Histogram "+HistName+" not found!");
        return;
      }
    RootDrawObj& r1 = TmpHub->ScriptDrawObjects.List[index];
    if (!r1.type.startsWith("TH"))
    {
        abort("Histogram "+HistName+" not found!");
        return;
    }

    index = TmpHub->ScriptDrawObjects.findIndexOf(HistToDivideWith);
    if (index == -1)
      {
        abort("Histogram "+HistToDivideWith+" not found!");
        return;
      }
    RootDrawObj& r2 = TmpHub->ScriptDrawObjects.List[index];
    if (!r2.type.startsWith("TH"))
    {
        abort("Histogram "+HistToDivideWith+" not found!");
        return;
    }

    TH1* h1 = dynamic_cast<TH1*>(r1.Obj);
    if (h1)
    {
        TH1* h2 = dynamic_cast<TH1*>(r2.Obj);
        if (h2)
        {
            bool bOK = h1->Divide(h2);
            if (bOK) return;
        }
    }
    abort("Division failed!");
}
*/


QVariant ReturnNanArray(int num)
{
    QJsonArray ar;
    for (int i=0; i<num; i++) ar << std::numeric_limits<double>::quiet_NaN();
    QJsonValue jv = ar;
    QVariant res = jv.toVariant();
    return res;
}

const QVariant AInterfaceToHist::FitGauss(const QString &HistName, const QString options)
{
    ARootHistRecord* r = static_cast<ARootHistRecord*>(TmpHub->Hists.getRecord(HistName));
    if (!r)
    {
        abort("Histogram " + HistName + " not found!");
        return ReturnNanArray(6);
    }
    else
    {
        const QVector<double> vec = r->FitGauss(options);
        if (vec.isEmpty()) return ReturnNanArray(6);

        if (bGuiTthread) emit RequestDraw(0, "", true); //to update

        QJsonArray ar;
        for (int i=0; i<6; i++) ar << vec.at(i);

        QJsonValue jv = ar;
        QVariant res = jv.toVariant();
        return res;
    }
}

const QVariant AInterfaceToHist::FitGaussWithInit(const QString &HistName, const QVariant InitialParValues, const QString options)
{
    ARootHistRecord* r = static_cast<ARootHistRecord*>(TmpHub->Hists.getRecord(HistName));
    if (!r)
    {
        abort("Histogram " + HistName + " not found!");
        return ReturnNanArray(6);
    }

    QString type = InitialParValues.typeName();
    if (type != "QVariantList")
    {
        abort("InitialParValues has to be an array of three numeric values");
        return ReturnNanArray(6);
    }

    QVariantList vl = InitialParValues.toList();
    QVector<double> in(3);
    if (vl.size() != 3)
    {
        abort("InitialParValues has to be an array of three numeric values");
        return ReturnNanArray(6);
    }
    bool bOK;
    for (int i=0; i<3; i++)
    {
        double d = vl.at(i).toDouble(&bOK);
        if (!bOK)
        {
            abort("InitialParValues has to be an array of three numeric values");
            return ReturnNanArray(6);
        }
        in[i] = d;
    }

    const QVector<double> vec = r->FitGaussWithInit(in, options);
    if (vec.isEmpty()) return ReturnNanArray(6);

    if (bGuiTthread) emit RequestDraw(0, "", true); //to update

    QJsonArray ar;
    for (int i=0; i<6; i++) ar << vec.at(i);

    QJsonValue jv = ar;
    QVariant res = jv.toVariant();
    return res;
}

bool AInterfaceToHist::Delete(const QString &HistName)
{
    if (!bGuiTthread)
    {
        abort("Threads cannot create/delete/draw histograms!");
        return false;
    }

    return TmpHub->Hists.remove(HistName);
}

void AInterfaceToHist::DeleteAllHist()
{
    if (!bGuiTthread)
        abort("Threads cannot create/delete/draw histograms!");
    else
        TmpHub->Hists.clear();
}

void AInterfaceToHist::Draw(const QString &HistName, const QString options)
{
    if (!bGuiTthread)
    {
        abort("Threads cannot create/delete/draw histograms!");
        return;
    }

    ARootHistRecord* r = static_cast<ARootHistRecord*>(TmpHub->Hists.getRecord(HistName));
    if (!r)
        abort("Histogram " + HistName + " not found!");
    else
        emit RequestDraw(r->GetObject(), options, true);
}

// --------------------- End of HIST ------------------------

AInterfaceToGraph::AInterfaceToGraph(TmpObjHubClass *TmpHub)
    : TmpHub(TmpHub)
{
    H["NewGraph"] = "Creates a new graph (Root TGraph object)";
    H["SetMarkerProperties"] = "Default marker properties are 1, 20, 1";
    H["SetLineProperties"] = "Default line properties are 1, 1, 2";
    H["Draw"] = "Draws the graph (use \"APL\" options if in doubt)";
}



void AInterfaceToGraph::NewGraph(const QString &GraphName)
{
    TGraph* gr = new TGraph();
    ARootGraphRecord* rec = new ARootGraphRecord(gr, GraphName, "TGraph");
    bool bOK = TmpHub->Graphs.append(GraphName, rec);
    if (!bOK)
    {
        delete rec;
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
    ARootGraphRecord* r = static_cast<ARootGraphRecord*>(TmpHub->Graphs.getRecord(GraphName));
    if (!r)
        abort("Graph "+GraphName+" not found!");
    else
        r->SetMarkerProperties(MarkerColor, MarkerStyle, MarkerSize);
}

void AInterfaceToGraph::SetLineProperties(QString GraphName, int LineColor, int LineStyle, int LineWidth)
{
    ARootGraphRecord* r = static_cast<ARootGraphRecord*>(TmpHub->Graphs.getRecord(GraphName));
    if (!r)
        abort("Graph "+GraphName+" not found!");
    else
        r->SetLineProperties(LineColor, LineStyle, LineWidth);
}

void AInterfaceToGraph::SetTitles(QString GraphName, QString X_Title, QString Y_Title)
{
    ARootGraphRecord* r = static_cast<ARootGraphRecord*>(TmpHub->Graphs.getRecord(GraphName));
    if (!r)
        abort("Graph "+GraphName+" not found!");
    else
        r->SetAxisTitles(X_Title, Y_Title);
}

void AInterfaceToGraph::AddPoint(QString GraphName, double x, double y)
{
    ARootGraphRecord* r = static_cast<ARootGraphRecord*>(TmpHub->Graphs.getRecord(GraphName));
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

    ARootGraphRecord* r = static_cast<ARootGraphRecord*>(TmpHub->Graphs.getRecord(GraphName));
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

    ARootGraphRecord* r = static_cast<ARootGraphRecord*>(TmpHub->Graphs.getRecord(GraphName));
    if (!r)
        abort("Graph "+GraphName+" not found!");
    else
        r->AddPoints(xArr, yArr);
}

void AInterfaceToGraph::Sort(const QString &GraphName)
{
    ARootGraphRecord* r = static_cast<ARootGraphRecord*>(TmpHub->Graphs.getRecord(GraphName));
    if (!r)
        abort("Graph "+GraphName+" not found!");
    else
        r->Sort();
}

void AInterfaceToGraph::Draw(QString GraphName, QString options)
{
    ARootGraphRecord* r = static_cast<ARootGraphRecord*>(TmpHub->Graphs.getRecord(GraphName));
    if (!r)
        abort("Graph "+GraphName+" not found!");
    else
        emit RequestDraw(r->GetObject(), options, true);
}

bool AInterfaceToGraph::Delete(QString GraphName)
{
    return TmpHub->Graphs.remove(GraphName);
}

void AInterfaceToGraph::DeleteAllGraph()
{
    TmpHub->Graphs.clear();
}
