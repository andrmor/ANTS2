#ifndef BSFIT123_H
#define BSFIT123_H

#include <vector>
#include <string>
#include <Eigen/Dense>

class Bspline1d;
class Bspline2d;
class Bspline3d;
class ProfileHist1D;
class ProfileHist2D;
class ProfileHist3D;

// Base class

class BSfit
{
protected:
// number of basis functions
    int nbas;
// Linear Least Squares
// components of the linear system
    Eigen::MatrixXd A;
    Eigen::VectorXd y;
    Eigen::VectorXd x;
// feedback from linear solver
    double residual;
    bool status;
// Quadratic Programming        
// constraints    
    bool non_negative;
    bool non_decreasing;
    bool non_increasing;
    bool even;
// variables   
    Eigen:: MatrixXd G;
    Eigen::VectorXd g0;
// ineqaulity constraints
    Eigen::MatrixXd CI;
    Eigen::VectorXd ci0;
// equality constraints
    Eigen::MatrixXd CE;
    Eigen::VectorXd ce0;
// Error reporting       
    std::string error_msg;
// Options
    bool sparse;     // use sparse QR for unconstrained fit - EXPERIMENTAL    
public:
    BSfit();
    void SetConstraintNonNegative() {non_negative=true;}
    bool Fit();
    double SolveSVD();
    bool SolveQR();
    bool SolveSparseQR();
    void AddConstraintCIneq(Eigen::MatrixXd DI, Eigen::VectorXd di0);
    void AddConstraintCEq(Eigen::MatrixXd DE, Eigen::VectorXd de0);
    void SetNonNegative();
    void SetNonIncreasing();
    void SetNonDecreasing();
    void DumpQuadProg();
    void SolveQuadProg();
    double GetResidual() {return residual;}
    bool GetStatus() {return status;}
    std::string GetError() {return error_msg;}
//public:
//    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

class BSfit1D : public BSfit
{
public:
    BSfit1D(Bspline1d *bs_);
    ~BSfit1D();
    void AddData(int npts, double const *datax, double const *datay);
    bool SetBinning(int bins);
    void SetBinningAuto();
    void SetConstraintNonIncreasing() {non_increasing=true;}
    void SetConstraintNonDecreasing() {non_decreasing=true;}
    void SetConstraintEven() {even=true;}
    bool Fit(int npts, double const *datax, double const *data, double const *dataw = 0);
    void MkLinSystem(int npts, double const *datax, double const *data, double const *dataw = 0);
    bool Fit();
    void SetEven();
private:
    Bspline1d *bs;
    int nbins;
    ProfileHist1D *h1;
// constraints ?
//public:
//    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

class BSfit2D : public BSfit
{
public:
    BSfit2D(Bspline2d *bs_);
    ~BSfit2D();
    void AddData(int npts, double const *datax, double const *datay, double const *dataz);
    bool SetBinning(int binsx, int binsy);
    void SetBinningAuto();
    void SetConstraintNonIncreasingX() {non_increasing_x=true;}
    void SetConstraintDdxAt0() {flat_top_x=true;}
    void SetConstraintSlopeY(int val) {slope_y = val;}
    void SetConstraintTopDown(double x, double y) {x0 = x; y0 = y; top_down = true;}
    void MkLinSystem(int npts, double const *datax, double const *datay, double const *data, double const *dataw);
    bool Fit(int npts, double const *datax, double const *datay, double const *data, double const *dataw = 0);
    bool Fit();
    void SetNonIncreasingX();
    void SetSlopeY();
    void SetTopDown();
    void SetFlatTopX();

private:
    double r(double x, double y) {return hypot(x-x0, y-y0);}
    Bspline2d *bs;
    int nbasx;
    int nbasy;
    int nbinsx;
    int nbinsy;
    bool non_increasing_x;
    bool flat_top_x; // force d/dx = 0 at x=0
    int slope_y;     // force d/dy positive (1) or negative (2) at x = 0
    double x0, y0;   // coordinates of the "top" point
    bool top_down;   // force upproximately non-increasing behavior with distance from (x0,y0)
    ProfileHist2D *h2;
};

class BSfit3D : public BSfit
{
public:
    BSfit3D(Bspline3d *bs_);
    ~BSfit3D();
    void AddData(int npts, double const *x, double const *y, double const *z, double const *data);
    bool SetBinning(int binsx, int binsy, int binsz);
    void SetBinningAuto();
    void MkLinSystem(int npts, double const *datax, double const *datay, double const *dataz, double const *data, double const *dataw);
    bool Fit(int npts, double const *x, double const *y, double const *z, double const *data, double const *dataw = 0);
    bool Fit();
private:
    Bspline3d *bs;
    int nbas;
    int nbasx;
    int nbasy;
    int nbasz;
    int nbasxy;
    int nbinsx;
    int nbinsy;
    int nbinsz;
    ProfileHist3D *h3;
};

#endif // !BSFIT123_H
