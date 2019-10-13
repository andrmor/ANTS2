#ifndef AHISTOGRAM_H
#define AHISTOGRAM_H

#include <vector>
#include <utility>

class AHistogram1D
{
public:
    AHistogram1D(int Bins, double From, double To);
    void setBufferSize(size_t size) {bufferSize = size;}

    void Fill(double x, double val);

    const std::vector<double> getContent(); // [0] - underflow, [1] - bin#0, ..., [bins] - bin#(bins-1), [bins+1] - overflow
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

#endif // AHISTOGRAM_H
