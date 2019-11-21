#ifndef AROOTGRAPHCOLLECTION_H
#define AROOTGRAPHCOLLECTION_H

#include "arootobjbase.h"

#include <QVector>
#include <QPair>
#include <QString>
#include <QMutex>

#include <TObject.h>

class ARootGraphRecord : public ARootObjBase
{
public:
    ARootGraphRecord(TObject* graph, const QString& name, const QString& type);

    TObject* GetObject() override;  // unasve for multithread (draw on queued signal), only GUI thread can trigger draw

    void     SetMarkerProperties(int markerColor, int markerStyle, double markerSize);
    void     SetLineProperties(int lineColor, int lineStyle, int lineWidth);
    void     SetTitles(const QString& titleX, const QString& titleY, const QString graphTitle = "");

    // Protected by Mutex
    void     AddPoint(double x, double y, double errorX = 0, double errorY = 0);
    void     AddPoints(const QVector<double> &xArr, const QVector<double> &yArr);
    void     AddPoints(const QVector<double> &xArr, const QVector<double> &yArr, const QVector<double> &xErrArr, const QVector<double> &yErrArr);
    void     Sort();
    void     SetYRange(double min, double max);
    void     SetMinimum(double min);
    void     SetMaximum(double max);
    void     SetXRange(double min, double max);
    void     SetXDivisions(int numDiv);
    void     SetYDivisions(int numDiv);

    const QVector<QPair<double, double> > GetPoints();
    void     Save(const QString& fileName);

    QString  LastDrawOption;

private:
    QString  TitleX, TitleY;
    int      MarkerColor = 4, MarkerStyle = 20;
    double   MarkerSize = 1.0;
    int      LineColor = 4,   LineStyle = 1,    LineWidth = 1;
};

#endif // AROOTGRAPHCOLLECTION_H
