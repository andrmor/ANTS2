#ifndef AROOTGRAPHCOLLECTION_H
#define AROOTGRAPHCOLLECTION_H

#include "arootobjbase.h"

#include <QVector>
#include <QString>
#include <QMutex>

#include <TObject.h>

class ARootGraphRecord : public ARootObjBase
{
public:
    ARootGraphRecord(TObject* graph, const QString& name, const QString& type);

    TObject* GetObject() override;  // unasve for multithread (draw on queued signal), only GUI thread can trigger draw

    void     SetMarkerProperties(int markerColor, int markerStyle, int markerSize);
    void     SetLineProperties(int lineColor, int lineStyle, int lineWidth);
    void     SetAxisTitles(const QString& titleX, const QString& titleY);

    // Protected by Mutex
    void     AddPoint(double x, double y);
    void     AddPoints(const QVector<double> &xArr, const QVector<double> &yArr);
    void     Sort();
    void     SetYRange(double min, double max);
    void     SetXRange(double min, double max);

private:
    QString  TitleX, TitleY;
    int      MarkerColor = 4, MarkerStyle = 20, MarkerSize = 1;
    int      LineColor = 4,   LineStyle = 1,    LineWidth = 1;
};

#endif // AROOTGRAPHCOLLECTION_H
