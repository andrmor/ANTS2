#ifndef AHISTOGRAM_H
#define AHISTOGRAM_H

#include <vector>
#include <utility>
#include <cstdlib>

class AHistogram1D
{
public:
    AHistogram1D(int Bins, double From, double To);
    void setBufferSize(size_t size) {bufferSize = size;}

    void Fill(double x, double val);

    void getLimits(double & From, double & To) const {From = from; To = to;}
    const std::vector<double> & getContent(); // [0] - underflow, [1] - bin#0, ..., [bins] - bin#(bins-1), [bins+1] - overflow
    const std::vector<double> getStat();    // [0] - sumVals, [1] - sumVals2, [2] - sumValX, [3] - sumValX2, [4] - # entries

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
    size_t bufferSize = 1000;

    std::vector<double> data; // [0] - underflow, [bins+1] - overflow
    std::vector<std::pair<double, double>> buffer;

private:
    void fillFixed(double x, double val);
    void processBuffer();

};

class AHistogram2D
{
public:
    AHistogram2D(int XBins, double XFrom, double XTo, int YBins, double YFrom, double YTo);
    void setBufferSize(size_t size) {bufferSize = size;}

    void Fill(double x, double y, double val);

    void getLimits(double & Xfrom, double & Xto, double & Yfrom, double & Yto) const {Xfrom = xfrom; Xto = xto; Yfrom = yfrom; Yto = yto;}
    const std::vector< std::vector<double> > & getContent(); //[y][x]; in each X: [0] - underflow, [1] - bin#0, ..., [bins] - bin#(bins-1), [bins+1] - overflow
    const std::vector<double> getStat();    // [0] - sumVals, [1] - sumVals2, [2] - sumValX, [3] - sumValX2, [4] - sumValY, [5] - sumValY2, [6] - # entries

private:
    int    xbins;
    double xfrom;
    double xto;
    double xdeltaBin;

    int    ybins;
    double yfrom;
    double yto;
    double ydeltaBin;

    double entries = 0;
    double sumVal = 0;
    double sumVal2 = 0;
    double sumValX = 0;
    double sumValX2 = 0;
    double sumValY = 0;
    double sumValY2 = 0;

    bool   bFixedRange = true;
    size_t bufferSize = 1000;

    std::vector< std::vector<double> > data; // [y][x]; for x: [0] - underflow, [bins+1] - overflow
    std::vector<std::tuple<double, double, double>> buffer;

private:
    void fillFixed(double x, double y, double val);
    void processBuffer();

};


#include "TH1D.h"
#include "TH2D.h"
#include <QString>
class ATH1D : public TH1D
{
public:
    ATH1D(const char *name, const char *title, int bins, double from, double to);
    ATH1D(const TH1D & other);

    const QString Import(double from, double to, const std::vector<double> & binContent, const std::vector<double> & stats); // empty srtring if no error

    void setStats(double * statsArray);

private:
    void SetStatistic(const std::vector<double> & stats);
};

class ATH2D : public TH2D
{
public:
    ATH2D(const char *name, const char *title, int xbins, double xfrom, double xto, int ybins, double yfrom, double yto);

    const QString Import(double xfrom, double xto, double yfrom, double yto, const std::vector<std::vector<double> > &binContent, const std::vector<double> & stats); // empty srtring if no error

private:
    void SetStatistic(const std::vector<double> & stats);
};

#endif // AHISTOGRAM_H
