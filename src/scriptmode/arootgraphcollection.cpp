#include "arootgraphcollection.h"

#include <QMutexLocker>
#include <QDebug>

#include "TGraph.h"
#include "TAxis.h"

ARootGraphCollection::~ARootGraphCollection()
{
    clear();
}

bool ARootGraphCollection::append(TObject *obj, const QString& name, const QString& type)
{
    if (!obj) return false;

    QMutexLocker locker(&Mutex);

    if (Collection.contains(name)) return false;

    AGraphRecord* rec = new AGraphRecord(obj, name, type);
    Collection.insert(name, rec);
    return true;
}

bool ARootGraphCollection::isExist(const QString &name)
{
    QMutexLocker locker(&Mutex);
    return Collection.contains(name);
}

AGraphRecord *ARootGraphCollection::getRecord(const QString &name)
{
    QMutexLocker locker(&Mutex);
    return Collection.value(name, 0);
}

bool ARootGraphCollection::remove(const QString &name)
{
    QMutexLocker locker(&Mutex);
    if (Collection.contains(name)) return false;
    Collection.remove(name);
    return true;
}

void ARootGraphCollection::clear()
{
    QMutexLocker locker(&Mutex);
    QMapIterator<QString, AGraphRecord*> iter(Collection);
    while (iter.hasNext())
    {
        iter.next();
        delete iter.value();
    }
    Collection.clear();
}

AGraphRecord::AGraphRecord(TObject *graph, QString name, QString type) :
    Graph(graph), Name(name), Type(type) {}

TObject *AGraphRecord::GetGraphForDrawing()
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

void AGraphRecord::SetMarkerProperties(int markerColor, int markerStyle, int markerSize)
{
    MarkerColor = markerColor, MarkerStyle = markerStyle, MarkerSize = markerSize;
}

void AGraphRecord::SetLineProperties(int lineColor, int lineStyle, int lineWidth)
{
    LineColor = lineColor,   LineStyle = lineStyle,    LineWidth = lineWidth;
}

void AGraphRecord::SetAxisTitles(const QString &titleX, const QString &titleY)
{
    TitleX = titleX; TitleY = titleY;
}

void AGraphRecord::AddPoint(double x, double y)
{
    if (!Graph) return;

    QMutexLocker locker(&Mutex);

    if (Type == "TGraph")
    {
        TGraph* g = dynamic_cast<TGraph*>(Graph);
        g->SetPoint(g->GetN(), x, y);
    }
}

void AGraphRecord::AddPoints(const QVector<double>& xArr, const QVector<double>& yArr)
{
    if (xArr.size() != yArr.size()) return;
    if (!Graph) return;

    QMutexLocker locker(&Mutex);

    if (Type == "TGraph")
    {
        TGraph* g = dynamic_cast<TGraph*>(Graph);
        for (int i=0; i<xArr.size(); i++)
            g->SetPoint(g->GetN(), xArr.at(i), yArr.at(i));
    }
}

void AGraphRecord::Sort()
{
    if (!Graph) return;

    QMutexLocker locker(&Mutex);

    if (Type == "TGraph")
    {
        TGraph* g = dynamic_cast<TGraph*>(Graph);
        qDebug() << "sorting";
        g->Sort();
    }
}
