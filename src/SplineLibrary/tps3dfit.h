#ifndef TPS3FIT_H
#define TPS3FIT_H


#include <vector>

class TPspline3D;
class TProfile3D;

class TPS3Dfit
{
public:
    TPS3Dfit(TPspline3D *bs_);
    ~TPS3Dfit();
    void AddData(int npts, double const *x, double const *y, double const *z, double const *data);
    void SetBinning(int binsx, int binsy, int binsz);
    void SetBinningAuto();
    void SetConstraintNonNegative() {non_negative=true;}
    bool Fit(int npts, double const *x, double const *y, double const *z, double const *data, double const *dataw = 0);
    bool Fit();
    double GetResidual() {return residual;}
private:
    TPspline3D *bs;
    int nbas;
    int nbasz;
    int nbasxy;
    int nbinsx;
    int nbinsy;
    int nbinsz;
    bool non_negative;
    bool sparse;
    double residual;
    bool status;
    TProfile3D *h1;
    std::vector<double> vx;
    std::vector<double> vy;
    std::vector<double> vz;
    std::vector<double> vdata;
    std::vector<double> vw;

    void BinData();
};

#endif // TPS3FIT_H
