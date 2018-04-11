#include "arootgraphrecord.h"

#include <QMutexLocker>
#include <QDebug>

#include "TGraph.h"
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

void ARootGraphRecord::SetMarkerProperties(int markerColor, int markerStyle, int markerSize)
{
    MarkerColor = markerColor, MarkerStyle = markerStyle, MarkerSize = markerSize;
}

void ARootGraphRecord::SetLineProperties(int lineColor, int lineStyle, int lineWidth)
{
    LineColor = lineColor,   LineStyle = lineStyle,    LineWidth = lineWidth;
}

void ARootGraphRecord::SetAxisTitles(const QString &titleX, const QString &titleY)
{
    TitleX = titleX; TitleY = titleY;
}

void ARootGraphRecord::AddPoint(double x, double y)
{
    QMutexLocker locker(&Mutex);

    if (Type == "TGraph")
    {
        TGraph* g = dynamic_cast<TGraph*>(Object);
        g->SetPoint(g->GetN(), x, y);
    }
}

void ARootGraphRecord::AddPoints(const QVector<double>& xArr, const QVector<double>& yArr)
{
    if (xArr.size() != yArr.size()) return;

    QMutexLocker locker(&Mutex);

    if (Type == "TGraph")
    {
        TGraph* g = dynamic_cast<TGraph*>(Object);
        for (int i=0; i<xArr.size(); i++)
        {
            //  qDebug() << i << xArr.at(i)<<yArr.at(i);
            g->SetPoint(g->GetN(), xArr.at(i), yArr.at(i));
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
