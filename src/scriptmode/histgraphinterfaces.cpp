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
#include "TGraphErrors.h"
#include "TF1.h"
#include "TFile.h"
#include "TKey.h"

//----------------- HIST  -----------------
AInterfaceToHist::AInterfaceToHist(TmpObjHubClass* TmpHub)
    : TmpHub(TmpHub)
{
    Description = "CERN ROOT histograms - TH1D and TH2D";

    H["FitGauss"] = "Fit histogram with a Gaussian. The returned result (is successful) contains an array [Constant,Mean,Sigma,ErrConstant,ErrMean,ErrSigma]"
                    "\nOptional 'options' parameter is directly forwarded to TH1::Fit()";
    H["FitGaussWithInit"] = "Fit histogram with a Gaussian. The returned result (is successful) contains an array [Constant,Mean,Sigma,ErrConstant,ErrMean,ErrSigma]"
                            "\nInitialParValues is an array of initial parameters of the values [Constant,Mean,Sigma]"
                            "\nOptional 'options' parameter is directly forwarded to TH1::Fit()";
}

AInterfaceToHist::AInterfaceToHist(const AInterfaceToHist &other) :
    AScriptInterface(other),
    TmpHub(other.TmpHub) {}

void AInterfaceToHist::NewHist(const QString& HistName, int bins, double start, double stop)
{
    if (!bGuiThread)
    {
        abort("Threads cannot create/delete/draw histograms!");
        return;
    }

    TH1D* hist = new TH1D("", HistName.toLatin1().data(), bins, start, stop);
    ARootHistRecord* rec = new ARootHistRecord(hist, HistName, "TH1D");

    bool bOK = TmpHub->Hists.append(HistName, rec, bAbortIfExists);
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
    if (!bGuiThread)
    {
        abort("Threads cannot create/delete/draw histograms!");
        return;
    }

    TH2D* hist = new TH2D("", HistName.toLatin1().data(), binsX, startX, stopX, binsY, startY, stopY);
    ARootHistRecord* rec = new ARootHistRecord(hist, HistName, "TH2D");

    bool bOK = TmpHub->Hists.append(HistName, rec, bAbortIfExists);
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

void AInterfaceToHist::SetXCustomLabels(const QString &HistName, QVariantList Labels)
{
    if (!bGuiThread)
    {
        abort("Cannot perform this operation in non-GUI thread!");
        return;
    }

    ARootHistRecord* r = dynamic_cast<ARootHistRecord*>(TmpHub->Hists.getRecord(HistName));
    if (!r) abort("Histogram " + HistName + " not found!");
    else
    {
        QVector<QString> lab;
        for (int i=0; i<Labels.size(); i++)
            lab << Labels.at(i).toString();
        r->SetXLabels(lab);
    }
}

void AInterfaceToHist::SetTitle(const QString &HistName, const QString &Title)
{
    ARootHistRecord* r = dynamic_cast<ARootHistRecord*>(TmpHub->Hists.getRecord(HistName));
    if (!r)
        abort("Histogram " + HistName + " not found!");
    else
        r->SetTitle(Title);
}

void AInterfaceToHist::SetTitles(const QString& HistName, QString X_Title, QString Y_Title, QString Z_Title)
{
    ARootHistRecord* r = dynamic_cast<ARootHistRecord*>(TmpHub->Hists.getRecord(HistName));
    if (!r)
        abort("Histogram " + HistName + " not found!");
    else
        r->SetAxisTitles(X_Title, Y_Title, Z_Title);
}

void AInterfaceToHist::SetLineProperties(const QString &HistName, int LineColor, int LineStyle, int LineWidth)
{
    ARootHistRecord* r = dynamic_cast<ARootHistRecord*>(TmpHub->Hists.getRecord(HistName));
    if (!r)
        abort("Histogram " + HistName + " not found!");
    else
        r->SetLineProperties(LineColor, LineStyle, LineWidth);
}

void AInterfaceToHist::SetMarkerProperties(const QString &HistName, int MarkerColor, int MarkerStyle, double MarkerSize)
{
    ARootHistRecord* r = dynamic_cast<ARootHistRecord*>(TmpHub->Hists.getRecord(HistName));
    if (!r)
        abort("Histogram " + HistName + " not found!");
    else
        r->SetMarkerProperties(MarkerColor, MarkerStyle, MarkerSize);
}

void AInterfaceToHist::SetFillColor(const QString &HistName, int Color)
{
    ARootHistRecord* r = dynamic_cast<ARootHistRecord*>(TmpHub->Hists.getRecord(HistName));
    if (!r) abort("Histogram " + HistName + " not found!");
    else    r->SetFillColor(Color);
}

void AInterfaceToHist::SetMaximum(const QString &HistName, double max)
{
    ARootHistRecord* r = dynamic_cast<ARootHistRecord*>(TmpHub->Hists.getRecord(HistName));
    if (!r) abort("Histogram " + HistName + " not found!");
    else    r->SetMax(max);
}

void AInterfaceToHist::SetMinimum(const QString &HistName, double min)
{
    ARootHistRecord* r = dynamic_cast<ARootHistRecord*>(TmpHub->Hists.getRecord(HistName));
    if (!r) abort("Histogram " + HistName + " not found!");
    else    r->SetMin(min);
}

void AInterfaceToHist::SetXDivisions(const QString &HistName, int primary, int secondary, int tertiary, bool canOptimize)
{
    ARootHistRecord* r = dynamic_cast<ARootHistRecord*>(TmpHub->Hists.getRecord(HistName));
    if (!r) abort("Histogram " + HistName + " not found!");
    else    r->SetXDivisions(primary, secondary, tertiary, canOptimize);
}

void AInterfaceToHist::SetYDivisions(const QString &HistName, int primary, int secondary, int tertiary, bool canOptimize)
{
    ARootHistRecord* r = dynamic_cast<ARootHistRecord*>(TmpHub->Hists.getRecord(HistName));
    if (!r) abort("Histogram " + HistName + " not found!");
    else    r->SetYDivisions(primary, secondary, tertiary, canOptimize);
}

void AInterfaceToHist::SetXLabelProperties(const QString &HistName, double size, double offset)
{
    ARootHistRecord* r = dynamic_cast<ARootHistRecord*>(TmpHub->Hists.getRecord(HistName));
    if (!r) abort("Histogram " + HistName + " not found!");
    else    r->SetXLabelProperties(size, offset);
}

void AInterfaceToHist::SetYLabelProperties(const QString &HistName, double size, double offset)
{
    ARootHistRecord* r = dynamic_cast<ARootHistRecord*>(TmpHub->Hists.getRecord(HistName));
    if (!r) abort("Histogram " + HistName + " not found!");
    else    r->SetYLabelProperties(size, offset);
}

void AInterfaceToHist::Fill(const QString &HistName, double val, double weight)
{
    ARootHistRecord* r = dynamic_cast<ARootHistRecord*>(TmpHub->Hists.getRecord(HistName));
    if (!r)
        abort("Histogram " + HistName + " not found!");
    else
        r->Fill(val, weight);
}

void AInterfaceToHist::Fill2D(const QString &HistName, double x, double y, double weight)
{
    ARootHistRecord* r = dynamic_cast<ARootHistRecord*>(TmpHub->Hists.getRecord(HistName));
    if (!r)
        abort("Histogram " + HistName + " not found!");
    else
        r->Fill2D(x, y, weight);
}

void AInterfaceToHist::Smooth(const QString &HistName, int times)
{
    ARootHistRecord* r = dynamic_cast<ARootHistRecord*>(TmpHub->Hists.getRecord(HistName));
    if (!r)
        abort("Histogram " + HistName + " not found!");
    else
    {
        r->Smooth(times);
        if (bGuiThread) emit RequestDraw(0, "", true); //to update
    }
}

void AInterfaceToHist::ApplyMedianFilter(const QString &HistName, int span)
{
    ApplyMedianFilter(HistName, span, -1);
}

void AInterfaceToHist::ApplyMedianFilter(const QString &HistName, int spanLeft, int spanRight)
{
    ARootHistRecord* r = dynamic_cast<ARootHistRecord*>(TmpHub->Hists.getRecord(HistName));
    if (!r)
        abort("Histogram " + HistName + " not found!");
    else
    {
        bool bOK = r->MedianFilter(spanLeft, spanRight);
        if (!bOK) abort("Failed - Median filter is currently implemented only for 1D histograms (TH1)");
    }
}

void AInterfaceToHist::FillArr(const QString &HistName, const QVariant XY_Array)
{
    ARootHistRecord* r = dynamic_cast<ARootHistRecord*>(TmpHub->Hists.getRecord(HistName));
    if (!r)
        abort("Histogram " + HistName + " not found!");
    else
    {
        QVariantList VarList = XY_Array.toList();
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

void AInterfaceToHist::FillArr(const QString &HistName, const QVariantList X_Array, const QVariantList Y_Array)
{
    ARootHistRecord* r = dynamic_cast<ARootHistRecord*>(TmpHub->Hists.getRecord(HistName));

    if (!r)
    {
        abort("Histogram " + HistName + " not found!");
        return;
    }

    const int size = X_Array.size();
    if (size == 0) return;
    if (size != Y_Array.size())
    {
        abort("Mismatch in array sizes in FillArr");
        return;
    }

    QVector<double> val, weight;
    bool bOK1, bOK2;
    for (int i=0; i < size; i++)
    {
        double v = X_Array.at(i).toDouble(&bOK1);
        double w = Y_Array.at(i).toDouble(&bOK2);
        if (!bOK1 || !bOK2)
        {
            abort("FillArr: Error in conversion to number");
            return;
        }

        val << v;
        weight << w;
        continue;
    }

    r->FillArr(val, weight);
}

void AInterfaceToHist::Fill2DArr(const QString &HistName, const QVariant Array)
{
    ARootHistRecord* r = dynamic_cast<ARootHistRecord*>(TmpHub->Hists.getRecord(HistName));
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

void AInterfaceToHist::Divide(const QString &HistName, const QString &HistToDivideWith)
{
    ARootHistRecord* r1 = dynamic_cast<ARootHistRecord*>(TmpHub->Hists.getRecord(HistName));
    ARootHistRecord* r2 = dynamic_cast<ARootHistRecord*>(TmpHub->Hists.getRecord(HistToDivideWith));
    if (!r1) abort("Histogram " + HistName + " not found!");
    if (!r2) abort("Histogram " + HistToDivideWith + " not found!");
    else
    {
        bool bOK = r1->Divide(r2);
        if (!bOK) abort("Histogram division failed: " + HistName + " by " + HistToDivideWith);
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

const QVariant AInterfaceToHist::FitGauss(const QString &HistName, const QString options)
{
    ARootHistRecord* r = dynamic_cast<ARootHistRecord*>(TmpHub->Hists.getRecord(HistName));
    if (!r)
    {
        abort("Histogram " + HistName + " not found!");
        return ReturnNanArray(6);
    }
    else
    {
        const QVector<double> vec = r->FitGauss(options);
        if (vec.isEmpty()) return ReturnNanArray(6);

        if (bGuiThread) emit RequestDraw(0, "", true); //to update

        QJsonArray ar;
        for (int i=0; i<6; i++) ar << vec.at(i);

        QJsonValue jv = ar;
        QVariant res = jv.toVariant();
        return res;
    }
}

const QVariant AInterfaceToHist::FitGaussWithInit(const QString &HistName, const QVariant InitialParValues, const QString options)
{
    ARootHistRecord* r = dynamic_cast<ARootHistRecord*>(TmpHub->Hists.getRecord(HistName));
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

    if (bGuiThread) emit RequestDraw(0, "", true); //to update

    QJsonArray ar;
    for (int i=0; i<6; i++) ar << vec.at(i);

    QJsonValue jv = ar;
    QVariant res = jv.toVariant();
    return res;
}

const QVariant AInterfaceToHist::FindPeaks(const QString &HistName, double sigma, double threshold)
{
    if (threshold <= 0 || threshold >= 1.0)
    {
        abort("hist.FindPeaks() : Threshold should be larger than 0 and less than 1.0");
        return 0;
    }
    if (sigma <= 0)
    {
        abort("hist.FindPeaks() : Sigma should be positive");
        return 0;
    }
    ARootHistRecord* r = dynamic_cast<ARootHistRecord*>(TmpHub->Hists.getRecord(HistName));
    if (!r)
    {
        abort("Histogram " + HistName + " not found!");
        return 0;
    }

    const QVector<double> vec = r->FindPeaks(sigma, threshold);
    QVariantList res;
    for (const double& d : vec) res << d;
    return res;
}

int AInterfaceToHist::GetNumberOfEntries(const QString &HistName)
{
    ARootHistRecord* r = dynamic_cast<ARootHistRecord*>(TmpHub->Hists.getRecord(HistName));
    if (!r)
    {
        abort("Histogram " + HistName + " not found!");
        return 1.0;
    }
    else
        return r->GetEntries();
}

void AInterfaceToHist::SetNumberOfEntries(const QString &HistName, int numEntries)
{
    ARootHistRecord* r = dynamic_cast<ARootHistRecord*>(TmpHub->Hists.getRecord(HistName));
    if (!r) abort("Histogram " + HistName + " not found!");
    else r->SetEntries(numEntries);
}

double AInterfaceToHist::GetIntegral(const QString &HistName, bool MultiplyByBinWidth)
{
    ARootHistRecord* r = dynamic_cast<ARootHistRecord*>(TmpHub->Hists.getRecord(HistName));
    if (!r)
    {
        abort("Histogram " + HistName + " not found!");
        return 1.0;
    }
    else
        return r->GetIntegral(MultiplyByBinWidth);
}

double AInterfaceToHist::GetMaximum(const QString &HistName)
{
    ARootHistRecord* r = dynamic_cast<ARootHistRecord*>(TmpHub->Hists.getRecord(HistName));
    if (!r)
    {
        abort("Histogram " + HistName + " not found!");
        return 1.0;
    }
    else
        return r->GetMaximum();
}

void AInterfaceToHist::Scale(const QString& HistName, double ScaleIntegralTo, bool DividedByBinWidth)
{
    ARootHistRecord* r = dynamic_cast<ARootHistRecord*>(TmpHub->Hists.getRecord(HistName));
    if (!r)
        abort("Histogram " + HistName + " not found!");
    else
        r->Scale(ScaleIntegralTo, DividedByBinWidth);
}

void AInterfaceToHist::Save(const QString &HistName, const QString& fileName)
{
    ARootHistRecord* r = dynamic_cast<ARootHistRecord*>(TmpHub->Hists.getRecord(HistName));
    if (!r) abort("Histogram " + HistName + " not found!");
    else    r->Save(fileName);
}

void AInterfaceToHist::Load(const QString &HistName, const QString &fileName, const QString histNameInFile)
{
    if (!bGuiThread)
    {
        abort("Threads cannot load histograms!");
        return;
    }

    ARootHistRecord* r = dynamic_cast<ARootHistRecord*>(TmpHub->Hists.getRecord(HistName));
    if (r && bAbortIfExists)
    {
        abort("Histogram " + HistName + " already exists!");
        return;
    }

    TFile* f = new TFile(fileName.toLatin1());
    if (!f)
    {
        abort("File not found or cannot be opened: " + fileName);
        return;
    }

    const int numKeys = f->GetListOfKeys()->GetEntries();
    qDebug() << "File contains" << numKeys << "TKeys";

    ARootHistRecord * rec = nullptr;
    bool bFound = false;
    for (int i=0; i<numKeys; i++)
    {
        TKey *key = (TKey*)f->GetListOfKeys()->At(i);
        QString Type = key->GetClassName();
        QString Name = key->GetName();
        qDebug() << i << Type << Name;

        if (!histNameInFile.isEmpty() && Name != histNameInFile) continue;
        bFound = true;

        if (Type == "TH1D")
        {
            TH1D * hist = (TH1D*)key->ReadObj();
            rec = new ARootHistRecord(hist, HistName, "TH1D");
            hist->GetYaxis()->SetTitleOffset(1.30f);
            break;
        }
            //else if (Type=="TProfile") p = (TProfile*)key->ReadObj();
            //else if (Type=="TProfile2D") p = (TProfile2D*)key->ReadObj();
        else if (Type == "TH2D")
        {
            TH2D * hist = (TH2D*)key->ReadObj();
            rec = new ARootHistRecord(hist, HistName, "TH2D");
            //hist->GetYaxis()->SetTitleOffset(1.30f);
            break;
        }
    }
    f->Close();
    delete f;

    if (!rec)
    {
        if (!histNameInFile.isEmpty() && !bFound)
            abort("Histogram with name " + histNameInFile + " not found in file " + fileName);
        else
            abort("Error loading histogram.\nNote that currently supported histogram types are TH1D and TH2D");
    }
    else
    {
        bool bOK = TmpHub->Hists.append(HistName, rec, false);
        if (!bOK)
        {
            delete rec;
            abort("Load histogram from file " + fileName + " failed!");
        }
    }
}

bool AInterfaceToHist::Delete(const QString &HistName)
{
    if (!bGuiThread)
    {
        abort("Threads cannot create/delete/draw histograms!");
        return false;
    }

    return TmpHub->Hists.remove(HistName);
}

void AInterfaceToHist::DeleteAllHist()
{
    if (!bGuiThread)
        abort("Threads cannot create/delete/draw histograms!");
    else
        TmpHub->Hists.clear();
}

void AInterfaceToHist::Draw(const QString &HistName, const QString options)
{
    if (!bGuiThread)
    {
        abort("Threads cannot create/delete/draw histograms!");
        return;
    }

    ARootHistRecord* r = dynamic_cast<ARootHistRecord*>(TmpHub->Hists.getRecord(HistName));
    if (!r)
        abort("Histogram " + HistName + " not found!");
    else
    {
        TObject * copy = r->GetObject()->Clone(r->GetObject()->GetName());
        emit RequestDraw(copy, options, true);
    }
}

QVariantList AInterfaceToHist::GetContent(const QString& HistName)
{
    QVariantList vl;
    ARootHistRecord* r = dynamic_cast<ARootHistRecord*>(TmpHub->Hists.getRecord(HistName));
    if (!r) abort("Histogram " + HistName + " not found!");
    else
    {
        if (r->is1D())
        {
            QVector<double> x, y;
            const bool bOK = r->GetContent(x, y);
            if (bOK)
            {
                for (int i=0; i<x.size(); i++)
                {
                    QVariantList el;
                    el << x.at(i) << y.at(i);
                    vl.push_back(el);
                }
            }
        }
        else if (r->is2D())
        {
            QVector<double> x, y, z;
            const bool bOK = r->GetContent2D(x, y, z);
            if (bOK)
            {
                for (int i=0; i<x.size(); i++)
                {
                    QVariantList el;
                    el << x.at(i) << y.at(i) << z.at(i);
                    vl.push_back(el);
                }
            }
        }
        else abort("GetContent method is currently implemented only for TH1D and TH2D histograms");
    }
    return vl;
}

double AInterfaceToHist::GetUnderflowBin(const QString& HistName)
{
    double val = 0;
    ARootHistRecord* r = dynamic_cast<ARootHistRecord*>(TmpHub->Hists.getRecord(HistName));
    if (!r) abort("Histogram " + HistName + " not found!");
    else
    {
        if (!r->GetUnderflow(val)) abort("Failed to get undeflow - the method is curretly implemented only for TH1");
    }
    return val;
}

double AInterfaceToHist::GetOverflowBin(const QString& HistName)
{
    double val = 0;
    ARootHistRecord* r = dynamic_cast<ARootHistRecord*>(TmpHub->Hists.getRecord(HistName));
    if (!r) abort("Histogram " + HistName + " not found!");
    else
    {
        if (!r->GetOverflow(val)) abort("Failed to get overflow - the method is curretly implemented only for TH1");
    }
    return val;
}

// --------------------- End of HIST ------------------------

AInterfaceToGraph::AInterfaceToGraph(TmpObjHubClass *TmpHub)
    : TmpHub(TmpHub)
{
    Description = "CERN ROOT graphs - TGraph";

    H["NewGraph"] = "Creates a new graph (Root TGraph object)";
    H["SetMarkerProperties"] = "Default marker properties are 1, 20, 1";
    H["SetLineProperties"] = "Default line properties are 1, 1, 2";
    H["Draw"] = "Draws the graph (use \"APL\" options if in doubt)";

    H["AddPoints"] = "Can add arrays of point using two options:\n"
            "1. Provide two arrays - one for X and one for Y coordinates of the points;\n"
            "2. Provide an array of [x,y] arrays of points;\n"
            "3. Provide an array of Y coordinates. In this case X scale will be 0, 1, 2, etc.";
}

AInterfaceToGraph::AInterfaceToGraph(const AInterfaceToGraph &other) :
    AScriptInterface(other),
    TmpHub(other.TmpHub) {}

void AInterfaceToGraph::NewGraph(const QString &GraphName)
{
    if (!bGuiThread)
    {
        abort("Threads cannot create/delete/draw graphs!");
        return;
    }

    TGraph* gr = new TGraph();
    ARootGraphRecord* rec = new ARootGraphRecord(gr, GraphName, "TGraph");
    bool bOK = TmpHub->Graphs.append(GraphName, rec, bAbortIfExists);
    if (!bOK)
    {
        delete gr;
        delete rec;
        abort("Graph "+GraphName+" already exists!");
    }
    else
    {
        gr->SetFillColor(0);
        gr->SetFillStyle(0);
    }
}

void AInterfaceToGraph::NewGraphErrors(const QString &GraphName)
{
    if (!bGuiThread)
    {
        abort("Threads cannot create/delete/draw graphs!");
        return;
    }

    TGraphErrors * gr = new TGraphErrors();
    ARootGraphRecord* rec = new ARootGraphRecord(gr, GraphName, "TGraphErrors");
    bool bOK = TmpHub->Graphs.append(GraphName, rec, bAbortIfExists);
    if (!bOK)
    {
        delete gr;
        delete rec;
        abort("Graph "+GraphName+" already exists!");
    }
    else
    {
        gr->SetFillColor(0);
        gr->SetFillStyle(0);
    }
}

void AInterfaceToGraph::SetMarkerProperties(QString GraphName, int MarkerColor, int MarkerStyle, double MarkerSize)
{
    ARootGraphRecord* r = dynamic_cast<ARootGraphRecord*>(TmpHub->Graphs.getRecord(GraphName));
    if (!r)
        abort("Graph "+GraphName+" not found!");
    else
        r->SetMarkerProperties(MarkerColor, MarkerStyle, MarkerSize);
}

void AInterfaceToGraph::SetLineProperties(QString GraphName, int LineColor, int LineStyle, int LineWidth)
{
    ARootGraphRecord* r = dynamic_cast<ARootGraphRecord*>(TmpHub->Graphs.getRecord(GraphName));
    if (!r)
        abort("Graph "+GraphName+" not found!");
    else
        r->SetLineProperties(LineColor, LineStyle, LineWidth);
}

void AInterfaceToGraph::SetTitles(QString GraphName, QString X_Title, QString Y_Title, QString GraphTitle)
{
    ARootGraphRecord* r = dynamic_cast<ARootGraphRecord*>(TmpHub->Graphs.getRecord(GraphName));
    if (!r)
        abort("Graph "+GraphName+" not found!");
    else
        r->SetTitles(X_Title, Y_Title, GraphTitle);
}

void AInterfaceToGraph::AddPoint(QString GraphName, double x, double y)
{
    ARootGraphRecord* r = dynamic_cast<ARootGraphRecord*>(TmpHub->Graphs.getRecord(GraphName));
    if (!r)
        abort("Graph "+GraphName+" not found!");
    else
        r->AddPoint(x, y);
}

void AInterfaceToGraph::AddPoint(QString GraphName, double x, double y, double errorY)
{
    ARootGraphRecord* r = dynamic_cast<ARootGraphRecord*>(TmpHub->Graphs.getRecord(GraphName));
    if (!r)
        abort("Graph "+GraphName+" not found!");
    else
        r->AddPoint(x, y, 0, errorY);
}

void AInterfaceToGraph::AddPoint(QString GraphName, double x, double y, double errorX, double errorY)
{
    ARootGraphRecord* r = dynamic_cast<ARootGraphRecord*>(TmpHub->Graphs.getRecord(GraphName));
    if (!r)
        abort("Graph "+GraphName+" not found!");
    else
        r->AddPoint(x, y, errorX, errorY);
}

void AInterfaceToGraph::AddPoints(QString GraphName, QVariantList vx, QVariantList vy)
{
    if (vx.isEmpty() || vx.size() != vy.size())
    {
        abort("Empty array or mismatch in array sizes in AddPoints for graph " + GraphName);
        return;
    }

    ARootGraphRecord* r = dynamic_cast<ARootGraphRecord*>(TmpHub->Graphs.getRecord(GraphName));
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

void AInterfaceToGraph::AddPoints(QString GraphName, QVariantList vx, QVariantList vy, QVariantList vEy)
{
    if (vx.isEmpty() || vx.size() != vy.size() || vx.size() != vEy.size())
    {
        abort("Empty array or mismatch in array sizes in AddPoints for graph " + GraphName);
        return;
    }

    ARootGraphRecord* r = dynamic_cast<ARootGraphRecord*>(TmpHub->Graphs.getRecord(GraphName));
    if (!r)
        abort("Graph "+GraphName+" not found!");
    else
    {
        QVector<double> xArr(vx.size());
        QVector<double> yArr(vx.size());
        QVector<double> xErrArr(vx.size());
        QVector<double> yErrArr(vx.size());

        bool bValidX, bValidY, bValidYerr;

        for (int i=0; i<vx.size(); i++)
        {
            double x    = vx.at(i).toDouble(&bValidX);
            double y    = vy.at(i).toDouble(&bValidY);
            double yerr = vEy.at(i).toDouble(&bValidYerr);
            if (bValidX && bValidY && bValidYerr)
            {
                xArr[i] = x;
                yArr[i] = y;
                xErrArr[i] = 0;
                yErrArr[i] = yerr;
            }
            else
            {
                abort("Not numeric value found in AddPoints() for " + GraphName);
                return;
            }
        }
        r->AddPoints(xArr, yArr, xErrArr, yErrArr);
    }
}

void AInterfaceToGraph::AddPoints(QString GraphName, QVariantList vx, QVariantList vy, QVariantList vEx, QVariantList vEy)
{
    if (vx.isEmpty() || vx.size() != vy.size() || vx.size() != vEx.size() || vx.size() != vEy.size())
    {
        abort("Empty array or mismatch in array sizes in AddPoints for graph " + GraphName);
        return;
    }

    ARootGraphRecord* r = dynamic_cast<ARootGraphRecord*>(TmpHub->Graphs.getRecord(GraphName));
    if (!r)
        abort("Graph "+GraphName+" not found!");
    else
    {
        QVector<double> xArr(vx.size());
        QVector<double> yArr(vx.size());
        QVector<double> xErrArr(vx.size());
        QVector<double> yErrArr(vx.size());

        bool bValidX, bValidY, bValidXerr, bValidYerr;

        for (int i=0; i<vx.size(); i++)
        {
            double x    = vx.at(i).toDouble(&bValidX);
            double y    = vy.at(i).toDouble(&bValidY);
            double xerr = vEx.at(i).toDouble(&bValidXerr);
            double yerr = vEy.at(i).toDouble(&bValidYerr);
            if (bValidX && bValidY && bValidXerr && bValidYerr)
            {
                xArr[i] = x;
                yArr[i] = y;
                xErrArr[i] = xerr;
                yErrArr[i] = yerr;
            }
            else
            {
                abort("Not numeric value found in AddPoints() for " + GraphName);
                return;
            }
        }
        r->AddPoints(xArr, yArr, xErrArr, yErrArr);
    }
}

void AInterfaceToGraph::AddPoints(QString GraphName, QVariantList v)
{
    if (v.isEmpty())
    {
        abort("Empty array in AddPoints for graph " + GraphName);
        return;
    }

    bool bOK = false;
    v.at(0).toDouble(&bOK);
    if (bOK)
    {
        QVariantList vl;
        for (int i=0; i<v.size(); i++) vl << i;
        AddPoints(GraphName, vl, v);
        return;
    }

    bool bError = false;
    bool bValidX, bValidY, bValidErrX, bValidErrY;
    QVector<double> xArr(v.size()), yArr(v.size()), xErrArr(v.size()), yErrArr(v.size());

    const QVariantList vFirst = v.at(0).toList();
    int Length = vFirst.size();
    if (Length < 2 || Length > 4)
    {
        abort("Invalid array in AddPoints() for graph: each entry should be X,Y  [or X,Y,Xerr or X,Y,Xerr,Yerr for TGraphError]" + GraphName);
        return;
    }

    for (int i=0; i<v.size(); i++)
    {
        const QVariantList vxy = v.at(i).toList();
        if (vxy.size() < Length)
        {
            bError = true;
            break;
        }
        double x = vxy.at(0).toDouble(&bValidX);
        double y = vxy.at(1).toDouble(&bValidY);
        double xErr = 0; bValidErrX = true;
        double yErr = 0; bValidErrY = true;
        if (Length == 3) yErr = vxy.at(2).toDouble(&bValidErrY);
        if (Length == 4)
        {
            xErr = vxy.at(2).toDouble(&bValidErrX);
            yErr = vxy.at(3).toDouble(&bValidErrY);
        }

        if (bValidX && bValidY && bValidErrX && bValidErrY)
        {
            xArr[i] = x;
            yArr[i] = y;
            xErrArr[i] = xErr;
            yErrArr[i] = yErr;
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

    ARootGraphRecord* r = dynamic_cast<ARootGraphRecord*>(TmpHub->Graphs.getRecord(GraphName));
    if (!r)
        abort("Graph "+GraphName+" not found!");
    else
        r->AddPoints(xArr, yArr, xErrArr, yErrArr);
}

void AInterfaceToGraph::SetYRange(const QString &GraphName, double min, double max)
{
    ARootGraphRecord* r = dynamic_cast<ARootGraphRecord*>(TmpHub->Graphs.getRecord(GraphName));
    if (!r)
        abort("Graph "+GraphName+" not found!");
    else
        r->SetYRange(min, max);
}

void AInterfaceToGraph::SetMinimum(const QString &GraphName, double min)
{
    ARootGraphRecord* r = dynamic_cast<ARootGraphRecord*>(TmpHub->Graphs.getRecord(GraphName));
    if (!r)
        abort("Graph "+GraphName+" not found!");
    else
        r->SetMinimum(min);
}

void AInterfaceToGraph::SetMaximum(const QString &GraphName, double max)
{
    ARootGraphRecord* r = dynamic_cast<ARootGraphRecord*>(TmpHub->Graphs.getRecord(GraphName));
    if (!r)
        abort("Graph "+GraphName+" not found!");
    else
        r->SetMaximum(max);
}

void AInterfaceToGraph::SetXRange(const QString &GraphName, double min, double max)
{
    ARootGraphRecord* r = dynamic_cast<ARootGraphRecord*>(TmpHub->Graphs.getRecord(GraphName));
    if (!r)
        abort("Graph "+GraphName+" not found!");
    else
        r->SetXRange(min, max);
}

void AInterfaceToGraph::SetXDivisions(const QString &GraphName, int numDiv)
{
    ARootGraphRecord* r = dynamic_cast<ARootGraphRecord*>(TmpHub->Graphs.getRecord(GraphName));
    if (!r)
        abort("Graph "+GraphName+" not found!");
    else
        r->SetXDivisions(numDiv);
}

void AInterfaceToGraph::SetYDivisions(const QString &GraphName, int numDiv)
{
    ARootGraphRecord* r = dynamic_cast<ARootGraphRecord*>(TmpHub->Graphs.getRecord(GraphName));
    if (!r)
        abort("Graph "+GraphName+" not found!");
    else
        r->SetYDivisions(numDiv);
}

void AInterfaceToGraph::Sort(const QString &GraphName)
{
    ARootGraphRecord* r = dynamic_cast<ARootGraphRecord*>(TmpHub->Graphs.getRecord(GraphName));
    if (!r)
        abort("Graph "+GraphName+" not found!");
    else
        r->Sort();
}

void AInterfaceToGraph::Draw(QString GraphName, QString options)
{
    if (!bGuiThread)
    {
        abort("Threads cannot create/delete/draw graphs!");
        return;
    }

    ARootGraphRecord* r = dynamic_cast<ARootGraphRecord*>(TmpHub->Graphs.getRecord(GraphName));
    if (!r)
        abort("Graph "+GraphName+" not found!");
    else
    {
        TObject * copy = r->GetObject()->Clone(r->GetObject()->GetName());
        emit RequestDraw(copy, options, true);
        r->LastDrawOption = options;
    }
}

#include "TFile.h"
#include "TKey.h"
void AInterfaceToGraph::LoadTGraph(const QString &NewGraphName, const QString &FileName)
{
    if (!bGuiThread)
    {
        abort("Threads cannot load graphs!");
        return;
    }

    TFile* f = new TFile(FileName.toLocal8Bit().data());
    if (!f)
    {
        abort("Cannot open file " + FileName);
        return;
    }

    const int numKeys = f->GetListOfKeys()->GetEntries();

    TGraph* g = 0;
    for (int ikey = 0; ikey < numKeys; ikey++)
    {
        TKey *key = (TKey*)f->GetListOfKeys()->At(ikey);
        qDebug() << "Key->  name:" << key->GetName() << " class:" << key->GetClassName() <<" title:"<< key->GetTitle();

        const QString Type = key->GetClassName();
        if (Type != "TGraph") continue;
        g = dynamic_cast<TGraph*>(key->ReadObj());
        if (g) break;
    }
    if (!g) abort(FileName + " does not contain TGraphs");
    else
    {
        ARootGraphRecord* rec = new ARootGraphRecord(g, NewGraphName, "TGraph");
        bool bOK = TmpHub->Graphs.append(NewGraphName, rec, bAbortIfExists);
        if (!bOK)
        {
            delete g;
            delete rec;
            abort("Graph "+NewGraphName+" already exists!");
        }
        else
        {
            qDebug() << "Draw opt:"<<g->GetDrawOption() << g->GetOption();
            //g->Dump();
            g->SetFillColor(0);
            g->SetFillStyle(0);
        }
    }

    f->Close();
    delete f;
}

void AInterfaceToGraph::Save(const QString &GraphName, const QString &FileName)
{
    if (!bGuiThread)
    {
        abort("Threads cannot save graphs!");
        return;
    }

    ARootGraphRecord* r = dynamic_cast<ARootGraphRecord*>(TmpHub->Graphs.getRecord(GraphName));
    if (!r)
        abort("Graph "+GraphName+" not found!");
    else
        r->Save(FileName);
}

const QVariant AInterfaceToGraph::GetPoints(const QString &GraphName)
{
    QVariantList res;

    ARootGraphRecord* r = dynamic_cast<ARootGraphRecord*>(TmpHub->Graphs.getRecord(GraphName));
    if (!r)
        abort("Graph "+GraphName+" not found!");
    else
    {
        const QVector<QPair<double, double> > vec = r->GetPoints();
        for (const QPair<double, double>& pair : vec)
        {
            QVariantList el;
            el << pair.first << pair.second;
            res.push_back(el); // creates nested array!
        }
    }

    return res;
}

bool AInterfaceToGraph::Delete(QString GraphName)
{
    if (!bGuiThread)
    {
        abort("Threads cannot create/delete/draw graphs!");
        return false;
    }

    return TmpHub->Graphs.remove(GraphName);
}

void AInterfaceToGraph::DeleteAllGraph()
{
    if (!bGuiThread)
        abort("Threads cannot create/delete/draw graphs!");
    else
        TmpHub->Graphs.clear();
}
