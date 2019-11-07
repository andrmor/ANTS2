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
        fillFixed(x, val);
    else
    {
        buffer.push_back( std::pair<double,double>(x, val) );
        if (buffer.size() == bufferSize)
            processBuffer();
    }
}

const std::vector<double> &AHistogram1D::getContent()
{
    if (!bFixedRange) processBuffer();
    return data;
}

const std::vector<double> AHistogram1D::getStat()
{
    return {sumVal, sumVal2, sumValX, sumValX2, entries};
}

void AHistogram1D::fillFixed(double x, double val)
{
    if (x < from)
        data[0] += val;
    else if (x > to)
        data[bins+1] += val;
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
    if (buffer.empty()) return;

    from = to = buffer[0].first;
    for (size_t i=1; i<buffer.size(); i++)
    {
        const double & x = buffer[i].first;
        if      (x < from) from = x;
        else if (x > to)   to   = x;
    }

    if (from == to)
    {
        from *= 0.995;
        to   *= 1.005;
    }

    deltaBin = (to - from) / bins + std::numeric_limits<double>::epsilon();

    for (size_t i=0; i<buffer.size(); i++)
    {
        const double & x   = buffer[i].first;
        const double & val = buffer[i].second;
        fillFixed(x, val);
    }

    bFixedRange = true;
}

AHistogram2D::AHistogram2D(int XBins, double XFrom, double XTo, int YBins, double YFrom, double YTo) :
    xbins(XBins), xfrom(XFrom), xto(XTo), ybins(YBins), yfrom(YFrom), yto(YTo)
{
    if (xbins < 1) xbins = 1;
    if (ybins < 1) ybins = 1;
    data.resize(ybins + 2);
    for (auto & v : data)
        v.resize(xbins + 2);

    if (xto > xfrom && yto > yfrom)
    {
        xdeltaBin = (xto - xfrom) / xbins;
        ydeltaBin = (yto - yfrom) / ybins;
    }
    else
    {
        bFixedRange = false;
        buffer.reserve(bufferSize);
    }
}

void AHistogram2D::Fill(double x, double y, double val)
{
    entries += 1.0;

    if (bFixedRange)
        fillFixed(x, y, val);
    else
    {
        buffer.push_back( std::tuple<double,double,double>(x, y, val) );
        if (buffer.size() == bufferSize)
            processBuffer();
    }
}

const std::vector<std::vector<double> > & AHistogram2D::getContent()
{
    if (!bFixedRange) processBuffer();
    return data;
}

const std::vector<double> AHistogram2D::getStat()
{
    return {sumVal, sumVal2, sumValX, sumValX2, sumValY, sumValY2, entries};
}

void AHistogram2D::fillFixed(double x, double y, double val)
{
    int ixbin, iybin;
    bool bGood = true;

    if      (x < xfrom)
    {
        ixbin = 0;
        bGood = false;
    }
    else if (x > xto)
    {
        ixbin = xbins + 1;
        bGood = false;
    }
    else
        ixbin = 1 + (x - xfrom)/xdeltaBin;

    if      (y < yfrom)
    {
        iybin = 0;
        bGood = false;
    }
    else if (y > yto)
    {
        iybin = ybins + 1;
        bGood = false;
    }
    else
        iybin = 1 + (y - yfrom)/ydeltaBin;

    data[iybin][ixbin] += val;

    if (bGood)
    {
        sumVal   += val;
        sumVal2  += val*val;
        sumValX  += val*x;
        sumValX2 += val*x*x;
        sumValY  += val*y;
        sumValY2 += val*y*y;
    }
}

void AHistogram2D::processBuffer()
{
    if (buffer.empty()) return;

    xfrom = xto = std::get<0>(buffer[0]);
    yfrom = yto = std::get<1>(buffer[0]);
    for (size_t i=1; i<buffer.size(); i++)
    {
        const double & x = std::get<0>(buffer[i]);
        const double & y = std::get<1>(buffer[i]);
        if      (x < xfrom) xfrom = x;
        else if (x > xto)   xto   = x;
        if      (y < yfrom) yfrom = y;
        else if (y > yto)   yto   = y;
    }

    xdeltaBin = (xto - xfrom) / xbins + std::numeric_limits<double>::epsilon();
    ydeltaBin = (yto - yfrom) / ybins + std::numeric_limits<double>::epsilon();

    for (size_t i=0; i<buffer.size(); i++)
    {
        const double & x   = std::get<0>(buffer[i]);
        const double & y   = std::get<1>(buffer[i]);
        const double & val = std::get<2>(buffer[i]);
        fillFixed(x, y, val);
    }

    bFixedRange = true;
}

// --------------------------------------------------------------------------------------


ATH1D::ATH1D(const char *name, const char *title, int bins, double from, double to) :
    TH1D(name, title, bins, from, to) {}

ATH1D::ATH1D(const TH1D &other) :
    TH1D(other) {}

const QString ATH1D::Import(double from, double to, const std::vector<double> & binContent, const std::vector<double> & stats)
{
    const int size = binContent.size();
    if (size < 3) return QString("Number of bins should be at lreast 3 (1+under+over), here it is %1").arg(size);
    if (from >= to) return "'from' has to be smaller than 'to'";
    if (stats.size() != 5) return "stats should have size of 5";

    SetBins(size-2, from, to);
    for (int i=0; i<size; i++)
        SetBinContent(i, binContent[i]);
    SetStatistic(stats);
    return "";
}

void ATH1D::setStats(double *statsArray)
{
    fTsumw   = statsArray[0];
    fTsumw2  = statsArray[1];
    fTsumwx  = statsArray[2];
    fTsumwx2 = statsArray[3];
    SetEntries(statsArray[0]);
}

void ATH1D::SetStatistic(const std::vector<double> & stats)
{
    fTsumw   = stats[0];
    fTsumw2  = stats[1];
    fTsumwx  = stats[2];
    fTsumwx2 = stats[3];
    SetEntries(stats[4]);
}

ATH2D::ATH2D(const char *name, const char *title, int xbins, double xfrom, double xto, int ybins, double yfrom, double yto) :
    TH2D(name, title, xbins, xfrom, xto, ybins, yfrom, yto){}

const QString ATH2D::Import(double xfrom, double xto, double yfrom, double yto, const std::vector< std::vector<double> > & binContent, const std::vector<double> &stats)
{
    const int ysize = binContent.size();
    if (ysize < 3) return QString("Number of bins should be at lreast 3 (1+under+over), here Y dimension has %1").arg(ysize);
    const int xsize = binContent[0].size();
    if (xsize < 3) return QString("Number of bins should be at lreast 3 (1+under+over), here X dimension has %1").arg(xsize);
    if (xfrom >= xto) return "'xfrom' has to be smaller than 'xto'";
    if (yfrom >= yto) return "'yfrom' has to be smaller than 'yto'";
    if (stats.size() != 7) return "stats should have size of 7";

    SetBins(xsize-2, xfrom, xto, ysize-2, yfrom, yto);
    for (int iy=0; iy<ysize; iy++)
        for (int ix=0; ix<xsize; ix++)
            SetBinContent(ix, iy, binContent[iy][ix]);
    SetStatistic(stats);
    return "";
}

void ATH2D::SetStatistic(const std::vector<double> &stats)
{
    fTsumw   = stats[0];
    fTsumw2  = stats[1];
    fTsumwx  = stats[2];
    fTsumwx2 = stats[3];
    fTsumwy  = stats[4];
    fTsumwy2 = stats[5];
    SetEntries(stats[6]);
}
