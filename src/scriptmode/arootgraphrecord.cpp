#include "arootgraphrecord.h"

#include <QMutexLocker>
#include <QDebug>

#include "TGraph.h"
#include "TAxis.h"

ARootGraphRecord::ARootGraphRecord(TObject *graph, QString name, QString type) :
    Graph(graph), Name(name), Type(type) {}

TObject *ARootGraphRecord::GetGraphForDrawing()
{
    TGraph* gr = dynamic_cast<TGraph*>(Graph);
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

    return Graph;
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
    if (!Graph) return;

    QMutexLocker locker(&Mutex);

    if (Type == "TGraph")
    {
        TGraph* g = dynamic_cast<TGraph*>(Graph);
        g->SetPoint(g->GetN(), x, y);
    }
}

void ARootGraphRecord::AddPoints(const QVector<double>& xArr, const QVector<double>& yArr)
{
    if (xArr.size() != yArr.size()) return;
    if (!Graph) return;

    QMutexLocker locker(&Mutex);

    if (Type == "TGraph")
    {
        TGraph* g = dynamic_cast<TGraph*>(Graph);
        for (int i=0; i<xArr.size(); i++)
        {
            //  qDebug() << i << xArr.at(i)<<yArr.at(i);
            g->SetPoint(g->GetN(), xArr.at(i), yArr.at(i));
        }
    }
}

void ARootGraphRecord::Sort()
{
    if (!Graph) return;

    QMutexLocker locker(&Mutex);

    if (Type == "TGraph")
    {
        TGraph* g = dynamic_cast<TGraph*>(Graph);
        g->Sort();
    }
}
