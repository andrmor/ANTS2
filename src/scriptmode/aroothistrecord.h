#ifndef AROOTHISTRECORD_H
#define AROOTHISTRECORD_H

#include "arootobjbase.h"

#include <QVector>
#include <QString>
#include <QMutex>

#include <TObject.h>

class ARootHistRecord : public ARootObjBase
{
public:
    ARootHistRecord(TObject* hist, const QString&  title, const QString& type);

    TObject* GetObjForDrawing() override;  // unsave for multithread (draw on queued signal), only GUI thread can trigger draw

    void SetTitles(const QString X_Title, const QString Y_Title, const QString Z_Title = "");
    void SetLineProperties(int LineColor, int LineStyle, int LineWidth);

    void Fill(double val, double weight);
    void Fill2D(double x, double y, double weight);

    void Smooth(int times);
    const QVector<double> FitGauss(const QString& options = "");
    const QVector<double> FitGaussWithInit(const QVector<double>& InitialParValues, const QString options = "");

};

#endif // AROOTHISTRECORD_H
