#ifndef AHISTOGRAM_H
#define AHISTOGRAM_H

#include <QVector>
#include <QPair>

class AHistogram1D
{
public:
    AHistogram1D(int Bins, double From, double To);
    void setBufferSize(int size) {bufferSize = size;}

    void Fill(double x, double val);

    const QVector<double> getContent(); // [0] - underflow, [1] - bin#0, ..., [bins] - bin#(bins-1), [bins+1] - overflow
    const QVector<double> getStat();    // [0] - sumVals, [1] - sumVals2, [2] - sumValX, [3] - sumValX2, [4] - # entries

private:
    int    bins;
    double from;
    double to;
    double deltaBin;

    double entries = 0;
    double sumVal = 0;
    double sumVal2 = 0;
    double sumValX = 0;
    double sumValX2 = 0;

    bool   bFixedRange = true;
    int    bufferSize = 1000;

    QVector<double> data; // [0] - underflow, [bins+1] - overflow
    QVector<QPair<double, double>> buffer;

private:
    void fillFixed(double x, double val);
    void processBuffer();

};

#endif // AHISTOGRAM_H
