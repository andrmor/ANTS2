#ifndef APEAKFINDER_H
#define APEAKFINDER_H

#include <QVector>

class TH1;

class APeakFinder
{
public:
    APeakFinder(const TH1* hist);

    //return vector of peak positions, sorted by peak amplitude (strongest first)
    const QVector<double> findPeaks(const double sigma = 2.0,       //sigma of searched peaks
                                    const double threshold = 0.02,  //peaks with amplitude less than threshold*highest_peak are discarded. 0<threshold<1
                                    const int MaxNumberOfPeaks = 30,
                                    bool SuppressDraw = true) const;

private:
    const TH1* H;
};

#endif // APEAKFINDER_H
