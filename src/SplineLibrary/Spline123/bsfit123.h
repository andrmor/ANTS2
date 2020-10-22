#ifndef BSFIT123_H
#define BSFIT123_H

#include <vector>
#include <string>
#include <Eigen/Dense>

#include "bspline123d.h"

class ProfileHist;
class ProfileHist1D;
class ProfileHist2D;
class ProfileHist3D;

// Classes managing constraints for QuqdProg

class Constraints
{
friend class ConstrainedFit1D;
friend class ConstrainedFit2D;
friend class ConstrainedFit3D;
public:
    Constraints(int nbas) {this->nbas = nbas;}
    virtual ~Constraints() {;}

    void SetMinimum(double min);
    void SetMaximum(double max);
    void ForceNonNegative() {SetMinimum(0.);}

protected:
    void AddInequality(Eigen::MatrixXd DI, Eigen::VectorXd di0);
    void AddEquality(Eigen::MatrixXd DE, Eigen::VectorXd de0);  

protected:
// ineqaulity constraints (leave zero-size if not used)
    Eigen::MatrixXd CI;
    Eigen::VectorXd ci0;
// equality constraints (leave zero-size if not used)
    Eigen::MatrixXd CE;
    Eigen::VectorXd ce0;
private:
    int nbas;
};

// Base fit class

class BSfit
{
public:
    enum Method {
        Auto,           // automatic choice
        SVD,            // more robust but slowest
        QR,             // fastest for small-size problems
        QR_Sparse,      // adequate for larger problems
        QuadProg        // use for constrained fit
    };

public:
    BSfit() {;}
    virtual ~BSfit() {;} // Andr +

    virtual BSfit* clone() const = 0;
    virtual ProfileHist *GetHist() = 0;
    virtual bool SolveLinSystem();
    bool SolveSVD();
    bool SolveQR();
    bool SolveSparseQR();
    bool SolveQuadProg(Eigen::MatrixXd &CI, Eigen::VectorXd &ci0, Eigen::MatrixXd &CE, Eigen::VectorXd &ce0);
    void SetMethod(Method m) {method = m;}
    Method SelectMethod();

//    void DumpQuadProg();
    double GetResidual() const {return residual;}    // Andr const
    bool GetStatus() const {return status;}          // Andr const
    std::string GetError() const {return error_msg;} // Andr const

protected:
    Method method = Auto;
// number of basis functions
    int nbas;
// Linear Least Squares
// components of the linear system
    Eigen::MatrixXd A;
    Eigen::VectorXd y;
    Eigen::VectorXd x;
// feedback from linear solver
    bool status;
    double residual;
    double svd_condition; // condition number from SVD
// Quadratic Programming           
    Eigen:: MatrixXd G;
    Eigen::VectorXd g0;
// Error reporting       
    std::string error_msg;
// Options
    bool sparse;     // use sparse QR for unconstrained fit - EXPERIMENTAL   
//public:
//    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

class BSfit1D : public BSfit
{
friend class ConstrainedFit1D;

public:
    BSfit1D(double xmin, double xmax, int n_int);
    BSfit1D(Bspline1d *bs_);
    ~BSfit1D();
    virtual BSfit1D* clone() const;
    void Init();
    void AddData(double const x, double const f);
    void AddData(int npts, double const *datax, double const *data);
    void AddData(std::vector <double> &datax, std::vector <double> &data) 
            {AddData(std::min(datax.size(), data.size()), &datax[0], &data[0]); }
    bool SetBinning(int bins);
    void SetBinningAuto();

    bool Fit(std::vector <double> &datax, std::vector <double> &data)
            {return Fit(std::min(datax.size(), data.size()), &datax[0], &data[0]);}
    bool Fit(int npts, double const *datax, double const *data)
            {return WeightedFit(npts, datax, data, nullptr);}
    bool WeightedFit(int npts, double const *datax, double const *data, double const *dataw); 
    bool BinnedFit();

    std::vector<double> GetCoef() const;
    Bspline1d *MakeSpline() const;
    Bspline1d *FitAndMakeSpline(std::vector <double> &datax, std::vector <double> &data);

    ProfileHist *GetHist();

protected:
    void MkLinSystem(int npts, double const *datax, double const *data, double const *dataw);
private:
    BsplineBasis1d *bs;
    int nbins;
    ProfileHist1D *h1;
//public:
//    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

class ConstrainedFit1D : public BSfit1D
{
public:
    ConstrainedFit1D(double xmin, double xmax, int n_int);
    ConstrainedFit1D(Bspline1d *bs_);
    virtual ~ConstrainedFit1D() {;}
    virtual ConstrainedFit1D* clone() const;
    bool SolveLinSystem();

// 1D constraints
    void SetMinimum(double min) {cstr->SetMinimum(min);}
    void SetMaximum(double max) {cstr->SetMaximum(max);}
    void ForceNonNegative() {cstr->ForceNonNegative();}
    void ForceNonIncreasing();
    void ForceNonDecreasing();
    void FixAt(double x, double f);
    void FixDrvAt(double x, double dfdx);
    void FixLeft(double f) {FixAt(bs->GetXmin(), f);}
    void FixDrvLeft(double dfdx) {FixDrvAt(bs->GetXmin(), dfdx);}
    void FixRight(double f) {FixAt(bs->GetXmax(), f);}
    void FixDrvRight(double dfdx) {FixDrvAt(bs->GetXmax(), dfdx);}

private:
    Constraints *cstr;
};

class BSfit2D : public BSfit
{
friend class ConstrainedFit2D;

public:
    BSfit2D(double xmin, double xmax, int n_intx, double ymin, double ymax, int n_inty);
    BSfit2D(Bspline2d *bs_);
    ~BSfit2D();
    virtual BSfit2D* clone() const;
    void Init();
    void AddData(double const x, double const y, double const f);
    void AddData(int npts, double const *datax, double const *datay, double const *dataz);
    void AddData(std::vector <double> &datax, std::vector <double> &datay, std::vector <double> &data) 
            {AddData(std::min({datax.size(), datay.size(), data.size()}), &datax[0], &datay[0], &data[0]);}
    bool SetBinning(int binsx, int binsy);
    void SetBinningAuto();

    bool Fit(std::vector <double> &datax, std::vector <double> &datay, std::vector <double> &data);
    bool Fit(int npts, double const *datax, double const *datay, double const *data)
            {return WeightedFit(npts, datax, datay, data, nullptr);}
    bool WeightedFit(int npts, double const *datax, double const *datay, double const *data, double const *dataw); 
    bool BinnedFit();

    std::vector<double> GetCoef() const;
    Bspline2d *MakeSpline() const;
    Bspline2d *FitAndMakeSpline(std::vector <double> &datax, std::vector <double> &datay, std::vector <double> &data);

    ProfileHist *GetHist();
protected:
   void MkLinSystem(int npts, double const *datax, double const *datay, double const *data, double const *dataw);

private:
    BsplineBasis2d *bs;
    int nbasx;
    int nbasy;
    int nbinsx;
    int nbinsy;
    ProfileHist2D *h2;
};

class ConstrainedFit2D : public BSfit2D
{
public:
    ConstrainedFit2D(double xmin, double xmax, int n_intx, double ymin, double ymax, int n_inty);
    virtual ~ConstrainedFit2D() {;}
    virtual ConstrainedFit2D* clone() const;
    bool SolveLinSystem();
// 2D constraints
    void SetMinimum(double min) {cstr->SetMinimum(min);}
    void SetMaximum(double max) {cstr->SetMaximum(max);}
    void ForceNonNegative() {cstr->ForceNonNegative();}
    void ForceNonIncreasingX();
    void SetSlopeY(int slope_y);
    void ForceTopDown(double x0, double y0);
    void ForceFlatTopX();

protected:
    double r(double x, double y) {return hypot(x-x0, y-y0);}
    double x0, y0; // coordinates of the "top" point for Top-Down

private:
    Constraints *cstr;
};

class BSfit3D : public BSfit
{
friend class ConstrainedFit3D;

public:
    BSfit3D(double xmin, double xmax, int n_intx, double ymin, double ymax, int n_inty, double zmin, double zmax, int n_intz);
    BSfit3D(Bspline3d *bs_);
    ~BSfit3D();
    virtual BSfit3D* clone() const;
    void Init();
    void AddData(int npts, double const *x, double const *y, double const *z, double const *data);
    bool SetBinning(int binsx, int binsy, int binsz);
    void SetBinningAuto();
    void MkLinSystem(int npts, double const *datax, double const *datay, double const *dataz, double const *data, double const *dataw);

    bool Fit(std::vector <double> &datax, std::vector <double> &datay, std::vector <double> &dataz, std::vector <double> &data);
    bool Fit(int npts, double const *datax, double const *datay, double const *dataz, double const *data)
            {return WeightedFit(npts, datax, datay, dataz, data, nullptr);}
    bool WeightedFit(int npts, double const *datax, double const *datay, double const *dataz, double const *data, double const *dataw); 
    bool BinnedFit();

    std::vector<double> GetCoef(int iz) const;
    Bspline3d *MakeSpline() const;
    Bspline3d *FitAndMakeSpline(std::vector <double> &datax, std::vector <double> &datay, std::vector <double> &dataz, std::vector <double> &data);

    ProfileHist *GetHist();

protected:
    int nbasx;
    int nbasy;
    int nbasz;
    int nbasxy;
    int nbinsx;
    int nbinsy;
    int nbinsz;
    BsplineBasis3d *bs;
    ProfileHist3D *h3;
};

class ConstrainedFit3D : public BSfit3D
{
public:
    ConstrainedFit3D(double xmin, double xmax, int n_intx, double ymin, double ymax, int n_inty, double zmin, double zmax, int n_intz);
    virtual ~ConstrainedFit3D() {;}
    virtual ConstrainedFit3D* clone() const;
    bool SolveLinSystem();

// 3D constraints
    void SetMinimum(double min) {cstr->SetMinimum(min);}
    void SetMaximum(double max) {cstr->SetMaximum(max);}
    void ForceNonNegative() {cstr->ForceNonNegative();}

private:
    Constraints *cstr;
};

#endif // !BSFIT123_H
