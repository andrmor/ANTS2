#include "arootgraphrecord.h"

#include <QMutexLocker>
#include <QDebug>

#include "TGraph.h"
#include "TGraphErrors.h"
#include "TAxis.h"
#include "TAttLine.h"
#include "TAttMarker.h"
#include "TAttFill.h"

ARootGraphRecord::ARootGraphRecord(TObject *graph, const QString &name, const QString &type) :
    ARootObjBase(graph, name, type) {}

TObject *ARootGraphRecord::GetObject()
{
    TGraph* gr = dynamic_cast<TGraph*>(Object);
    if (gr)
    {
        gr->SetFillColor(0);
        gr->SetFillStyle(0);

        gr->SetLineColor(LineColor);
        gr->SetLineWidth(LineWidth);
        gr->SetLineStyle(LineStyle);
        gr->SetMarkerColor(MarkerColor);
        gr->SetMarkerSize(MarkerSize);
        gr->SetMarkerStyle(MarkerStyle);

        gr->SetEditable(false);

        gr->GetYaxis()->SetTitleOffset(1.30f);
        gr->GetXaxis()->SetTitle(TitleX.toLatin1().data());
        gr->GetYaxis()->SetTitle(TitleY.toLatin1().data());
    }

    return Object;
}

void ARootGraphRecord::SetMarkerProperties(int markerColor, int markerStyle, double markerSize)
{
    MarkerColor = markerColor, MarkerStyle = markerStyle, MarkerSize = markerSize;
}

void ARootGraphRecord::SetLineProperties(int lineColor, int lineStyle, int lineWidth)
{
    LineColor = lineColor,   LineStyle = lineStyle,    LineWidth = lineWidth;
}

void ARootGraphRecord::SetTitles(const QString &titleX, const QString &titleY, const QString graphTitle)
{
    TitleX = titleX; TitleY = titleY;
    QMutexLocker locker(&Mutex);

    if (Type == "TGraph" && !graphTitle.isEmpty())
    {
        TGraph* g = dynamic_cast<TGraph*>(Object);
        if (g) g->SetTitle(graphTitle.toLatin1().data());
    }
}

void ARootGraphRecord::AddPoint(double x, double y, double errorX, double errorY)
{
    QMutexLocker locker(&Mutex);

    if (Type == "TGraph")
    {
        TGraph* g = dynamic_cast<TGraph*>(Object);
        g->SetPoint(g->GetN(), x, y);
    }
    else if (Type == "TGraphErrors")
    {
        TGraphErrors* g = dynamic_cast<TGraphErrors*>(Object);
        if (g)
        {
            const int iBin = g->GetN();
            g->SetPoint(iBin, x, y);
            g->SetPointError(iBin, errorX, errorY);
        }
    }
}

void ARootGraphRecord::AddPoints(const QVector<double>& xArr, const QVector<double>& yArr)
{
    if (xArr.size() != yArr.size()) return;

    QMutexLocker locker(&Mutex);

    if (Type == "TGraph")
    {
        TGraph* g = dynamic_cast<TGraph*>(Object);
        if (g)
        {
            for (int i=0; i<xArr.size(); i++)
            {
                //  qDebug() << i << xArr.at(i)<<yArr.at(i);
                g->SetPoint(g->GetN(), xArr.at(i), yArr.at(i));
            }
        }
    }
}

void ARootGraphRecord::AddPoints(const QVector<double> &xArr, const QVector<double> &yArr, const QVector<double> &xErrArr, const QVector<double> &yErrArr)
{
    if (xArr.size() != yArr.size() || xArr.size() != xErrArr.size() || xArr.size() != yErrArr.size()) return;

    QMutexLocker locker(&Mutex);

    if (Type == "TGraph")
    {
        TGraph* g = dynamic_cast<TGraph*>(Object);
        if (g)
        {
            for (int i=0; i<xArr.size(); i++)
            {
                //  qDebug() << i << xArr.at(i)<<yArr.at(i);
                g->SetPoint(g->GetN(), xArr.at(i), yArr.at(i));
            }
        }
    }
    else if (Type == "TGraphErrors")
    {
        TGraphErrors* g = dynamic_cast<TGraphErrors*>(Object);
        if (g)
        {
            for (int i=0; i<xArr.size(); i++)
            {
                const int iBin = g->GetN();
                g->SetPoint(iBin, xArr.at(i), yArr.at(i));
                g->SetPointError(iBin, xErrArr.at(i), yErrArr.at(i));
            }
        }
    }
}

void ARootGraphRecord::Sort()
{
    QMutexLocker locker(&Mutex);

    if (Type == "TGraph")
    {
        TGraph* g = dynamic_cast<TGraph*>(Object);
        g->Sort();
    }
}

void ARootGraphRecord::SetYRange(double min, double max)
{
    QMutexLocker locker(&Mutex);

    if (Type == "TGraph")
    {
        TGraph* g = dynamic_cast<TGraph*>(Object);
        if (g)
        {
            g->SetMinimum(min);
            g->SetMaximum(max);
        }
    }
}

void ARootGraphRecord::SetMinimum(double min)
{
    QMutexLocker locker(&Mutex);

    if (Type == "TGraph")
    {
        TGraph* g = dynamic_cast<TGraph*>(Object);
        if (g)
            g->SetMinimum(min);
    }
}

void ARootGraphRecord::SetMaximum(double max)
{
    QMutexLocker locker(&Mutex);

    if (Type == "TGraph")
    {
        TGraph* g = dynamic_cast<TGraph*>(Object);
        if (g)
            g->SetMaximum(max);
    }
}

void ARootGraphRecord::SetXRange(double min, double max)
{
    QMutexLocker locker(&Mutex);

    if (Type == "TGraph")
    {
        TGraph* g = dynamic_cast<TGraph*>(Object);
        if (g)
        {
            TAxis* axis = g->GetXaxis();
            if (axis) axis->SetLimits(min, max);
        }
    }
}

void ARootGraphRecord::SetXDivisions(int numDiv)
{
    QMutexLocker locker(&Mutex);

    TGraph* g = dynamic_cast<TGraph*>(Object);
    if (g)
    {
        TAxis* ax = g->GetXaxis();
        if (ax) ax->SetNdivisions(numDiv);
    }
}

void ARootGraphRecord::SetYDivisions(int numDiv)
{
    QMutexLocker locker(&Mutex);

    TGraph* g = dynamic_cast<TGraph*>(Object);
    if (g)
    {
        TAxis* ax = g->GetYaxis();
        if (ax) ax->SetNdivisions(numDiv);
    }
}

const QVector<QPair<double, double> > ARootGraphRecord::GetPoints()
{
    QMutexLocker locker(&Mutex);

    QVector<QPair<double, double>> res;

    if (Type == "TGraph")
    {
        TGraph* g = dynamic_cast<TGraph*>(Object);
        if (g)
        {
            const int numPoints = g->GetN();
            res.resize(numPoints);
            for (int ip=0; ip<numPoints; ip++)
                g->GetPoint(ip, res[ip].first, res[ip].second);
        }
    }
    return res;
}

void ARootGraphRecord::Save(const QString &fileName)
{
    QMutexLocker locker(&Mutex);

    if (Type == "TGraph")
    {
        TGraph* g = dynamic_cast<TGraph*>(Object);
        if (g) g->SaveAs(fileName.toLatin1().data(), LastDrawOption.toLatin1().data());
    }
}
