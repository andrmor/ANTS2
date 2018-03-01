#ifndef AROOTHISTRECORD_H
#define AROOTHISTRECORD_H

#include <QVector>
#include <QString>
#include <QMutex>

#include <TObject.h>

class ARootHistRecord
{
public:
    ARootHistRecord(TObject* hist, const QString&  name, QString  type);
    ARootHistRecord(){}
    ~ARootHistRecord(){}   // Do not delete hist object in the destructor! Handled by the collection

    TObject* GetHistForDrawing();  // unsave for multithread (draw on queued signal), only GUI thread can trigger draw

    void SetTitles(QString X_Title, QString Y_Title, QString Z_Title = "");
    void SetLineProperties(int LineColor, int LineStyle, int LineWidth);

    void Fill(double val, double weight);
    void Fill2D(double x, double y, double weight);

    void Smooth(int times);
    const QVector<double> FitGauss(const QString& options = "");
    const QVector<double> FitGaussWithInit(const QVector<double>& InitialParValues, const QString options = "");

private:
    TObject* Hist = 0;   // TH1D, TH2D
    QString  Name;       // it is the title
    QString  Type;       // object type (e.g. "TH1D")

    QMutex   Mutex;
};

#endif // AROOTHISTRECORD_H
