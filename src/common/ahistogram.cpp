#include "ahistogram.h"

#include <limits>
#include <QDebug>

AHistogram1D::AHistogram1D(int Bins, double From, double To) :
    bins(Bins), from(From), to(To)
{
    if (bins < 1) bins = 1;
    data.resize(bins + 2);

    if (to > from)
    {
        deltaBin = (to - from) / bins;
    }
    else
    {
        bFixedRange = false;
        buffer.reserve(bufferSize);
    }
}

void AHistogram1D::Fill(double x, double val)
{
    entries += 1.0;

    if (bFixedRange)
    {
        fillFixed(x, val);
    }
    else
    {
        buffer << QPair<double,double>(x, val);
        if (buffer.size() == bufferSize)
            processBuffer();
    }
}

const QVector<double> AHistogram1D::getContent()
{
    if (!bFixedRange) processBuffer();

    QVector<double> res;
    res.reserve(data.size());
    for (const double & bc : data)
        res << bc;
    return res;
}

const QVector<double> AHistogram1D::getStat()
{
    return {sumVal, sumVal2, sumValX, sumValX2, entries};
}

void AHistogram1D::fillFixed(double x, double val)
{
    if (x < from)
    {
        data[0] += val;
    }
    else if (x > to)
    {
        data[bins+1] += val;
    }
    else
    {
        data[ 1 + (x - from)/deltaBin ] += val;

        sumVal   += val;
        sumVal2  += val*val;
        sumValX  += val*x;
        sumValX2 += val*x*x;
    }
}

void AHistogram1D::processBuffer()
{
    if (buffer.isEmpty()) return;

    from = to = buffer.first().first;
    for (int i=1; i<buffer.size(); i++)
    {
        const double & x = buffer.at(i).first;
        if      (x < from) from = x;
        else if (x > to)   to   = x;
    }

    deltaBin = (to - from) / bins + std::numeric_limits<double>::epsilon();

    for (int i=0; i<buffer.size(); i++)
    {
        const double & x   = buffer.at(i).first;
        const double & val = buffer.at(i).second;
        fillFixed(x, val);
    }

    bFixedRange = true;
}
