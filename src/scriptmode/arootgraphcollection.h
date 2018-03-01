#ifndef AROOTGRAPHCOLLECTION_H
#define AROOTGRAPHCOLLECTION_H

#include <QMap>
#include <QVector>
#include <QString>
#include <QMutex>

#include <TObject.h>

class AGraphRecord;

class ARootGraphCollection
{
public:
    ARootGraphCollection() {}
    ~ARootGraphCollection();

    bool          append(TObject* obj, const QString& name, const QString& type);
    bool          isExist(const QString& name);

    AGraphRecord* getRecord(const QString& name);  // Unlocks mutex on return!

    bool          remove(const QString& name);     // Not multithread-safe: graph can be in use by the GUI
    void          clear();                         // Not multithread-safe: graph can be in use by the GUI

private:
    QMap<QString, AGraphRecord*> Collection;
    QMutex        Mutex;

};

class AGraphRecord
{
public:
    AGraphRecord(TObject* graph, QString  name, QString  type);
    AGraphRecord(){}
    ~AGraphRecord(){}   // Do not delete graph object in the destructor! Handled by the collection

    TObject* GetGraphForDrawing();  // unasve for multithread (draw on queued signal), only GUI thread can trigger draw

    void     SetMarkerProperties(int markerColor, int markerStyle, int markerSize);
    void     SetLineProperties(int lineColor, int lineStyle, int lineWidth);
    void     SetAxisTitles(const QString& titleX, const QString& titleY);

    // Protected by Mutex
    void     AddPoint(double x, double y);
    void     AddPoints(const QVector<double> &xArr, const QVector<double> &yArr);
    void     Sort();

private:
    TObject* Graph = 0;  // TGraph, TGraph2D
    QString  Name;       // it is the title
    QString  Type;       // object type (e.g. "TH1D")

    QString  TitleX, TitleY;
    int      MarkerColor = 4, MarkerStyle = 20, MarkerSize = 1;
    int      LineColor = 4,   LineStyle = 1,    LineWidth = 1;

    QMutex   Mutex;
};

#endif // AROOTGRAPHCOLLECTION_H
