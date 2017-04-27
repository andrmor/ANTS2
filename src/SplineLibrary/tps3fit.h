#ifndef TPS3FIT_H
#define TPS3FIT_H


#include <vector>

class TPspline3;
class TProfile2D;

class TPS3fit
{
public:
    TPS3fit(TPspline3 *bs_);
    ~TPS3fit();
    void AddData(int npts, double const *datax, double const *datay, double const *dataz);
    void SetBinning(int binsx, int binsy);
    void SetBinningAuto();
    void SetConstraintNonNegative() {non_negative=true;}
    void SetConstraintNonIncreasingX() {non_increasing_x=true;}
    void SetConstraintDdxAt0() {flat_top_x=true;}
    void SetConstraintSlopeY(int val) {slope_y = val;}
    bool Fit(int npts, double const *datax, double const *datay, double const *dataz, double const *dataw = 0);
    bool Fit();
    double GetResidual() {return residual;}
private:
    TPspline3 *bs;
    int nbas;
    int nbinsx;
    int nbinsy;
    bool non_negative;
    bool non_increasing_x;
    bool flat_top_x; // force d/dx = 0 at x=0
    int slope_y;     // force d/dy positive (1) or negative (2) at x = 0
    double residual;
    bool status;
    TProfile2D *h1;
    std::vector<double> vx;
    std::vector<double> vy;
    std::vector<double> vz;
    std::vector<double> vw;

    void BinData();
};

#endif // TPS3FIT_H
