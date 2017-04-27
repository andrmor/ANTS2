#ifndef BS3FIT_H
#define BS3FIT_H

#include <vector>

class Bspline3;
class TProfile;

class BS3fit
{
public:
    BS3fit(Bspline3 *bs_);
    ~BS3fit();
    void AddData(int npts, double const *datax, double const *datay);
    void SetBinning(int bins);
    void SetBinningAuto();
    void SetConstraintNonNegative() {non_negative= true;}
    void SetConstraintNonIncreasing() {non_increasing=true;}
    void SetConstraintEven() {even=true;}
    bool Fit(int npts, double const *datax, double const *datay, double const *dataw = 0);
    bool Fit();
    double GetResidual() {return residual;}
private:
    Bspline3 *bs;
    int nbas;
    int nbins;
    bool non_negative;
    bool non_increasing;
    bool even;
    double residual;
    bool status;
    TProfile *h1;
    std::vector<double> vx;
    std::vector<double> vy;
    std::vector<double> vw;

    void BinData();
};

#endif // BS3FIT_H
