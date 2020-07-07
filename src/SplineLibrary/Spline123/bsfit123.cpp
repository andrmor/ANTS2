#include "bsfit123.h"
#include "bspline123d.h"
#include "eiquadprog.hpp"
#include "profileHist.h"

#include <Eigen/Sparse>
#include <Eigen/SparseQR>
#include <Eigen/OrderingMethods>

#include <iostream>

using Eigen::MatrixXd;
using Eigen::VectorXd;
using Eigen::JacobiSVD;
using Eigen::Triplet;
using Eigen::SparseQR;
using Eigen::SparseMatrix;

// ================ Constraints =================

void Constraints::AddInequality(MatrixXd DI, VectorXd di0)
{
    if (ci0.size() == 0) { // first constraint -> just copy
        CI = DI;
        ci0 = di0;
    } else { // not first -> append new columns
        // resize is destructive, must store CI and ci0 temporarily
        MatrixXd TI = CI;
        VectorXd ti0 = ci0;
        CI.resize(CI.rows(), CI.cols()+DI.cols());
        ci0.resize(ci0.rows()+di0.rows());
        CI << TI, DI;
        ci0 << ti0, di0;
    }
}

void Constraints::AddEquality(MatrixXd DE, VectorXd de0)
{
    if (ce0.size() == 0) { // first constraint -> just copy
        CE = DE;
        ce0 = de0;
    } else { // not first -> append new columns
        // resize is destructive, must store CE and ce0 temporarily
        MatrixXd TE = CE;
        VectorXd te0 = ce0;
        CE.resize(CE.rows(), CE.cols()+DE.cols());
        ce0.resize(ce0.cols()+de0.cols());
        CE << TE, DE;
        ce0 << te0, de0;
    }
}

// the constraints are specified in the form 
//    CE^T x + ce0 = 0 and CI^T x + ci0 >= 0
// from eiquadprog.hpp:
//	   If the constraints of your problem are specified in the form 
//	   A^T x = b and C^T x >= d, then you should set ce0 = -b and ci0 = -d.

void Constraints::SetMinimum(double min)
{
    MatrixXd TI = MatrixXd::Zero(nbas, nbas);
    VectorXd ti0 = VectorXd::Constant(nbas, -min);

    for (int i = 0; i<nbas; i++)
        TI(i, i) = 1.;

    AddInequality(TI, ti0); 
}

void Constraints::SetMaximum(double max)
{
    MatrixXd TI = MatrixXd::Zero(nbas, nbas);
    VectorXd ti0 = VectorXd::Constant(nbas, max);

    for (int i = 0; i<nbas; i++)
        TI(i, i) = -1.;

    AddInequality(TI, ti0); 
}

void ConstrainedFit1D::ForceNonIncreasing()
{
    MatrixXd TI = MatrixXd::Zero(nbas, nbas);
    VectorXd ti0 = VectorXd::Zero(nbas);

    for (int i = 0; i<nbas-1; i++) {
        TI(i, i) = 1.;
        TI(i+1, i) = -1.;
    }

    cstr->AddInequality(TI, ti0);
}

void ConstrainedFit1D::ForceNonDecreasing()
{
    MatrixXd TI = MatrixXd::Zero(nbas, nbas);
    VectorXd ti0 = VectorXd::Zero(nbas);

    for (int i = 1; i<nbas; i++) {
        TI(i, i) = 1.;
        TI(i-1, i) = -1.;
    }

    cstr->AddInequality(TI, ti0);
}

void ConstrainedFit1D::FixAt(double x, double f)
{
    if (x < bs->GetXmin() || x > bs->GetXmax())
        return;

    MatrixXd TE = MatrixXd::Zero(nbas, 1);
    VectorXd te0 = VectorXd::Constant(1, -f);

    for (int k=0; k<nbas; k++)
        TE(k, 0) = bs->Basis(x, k);
    
    cstr->AddEquality(TE, te0);
}

void ConstrainedFit1D::FixDrvAt(double x, double f)
{
    if (x < bs->GetXmin() || x > bs->GetXmax())
        return;

    MatrixXd TE = MatrixXd::Zero(nbas, 1);
    VectorXd te0 = VectorXd::Constant(1, -f);

    for (int k=0; k<nbas; k++)
        TE(k, 0) = bs->BasisDrv(x, k);
    
    cstr->AddEquality(TE, te0);
}

void ConstrainedFit2D::ForceNonIncreasingX()
{
    int nbasx = bs->GetBSX().GetNbas();
    int nbasy = bs->GetBSY().GetNbas();

    MatrixXd TI = MatrixXd::Zero(nbas, nbas);
    VectorXd ti0 = VectorXd::Zero(nbas);

    for (int iy = 0; iy<nbasy; iy++)
        for (int ix = 0; ix<nbasx-1; ix++) {
            int i = ix + iy*nbasx;
            TI(i, i) = 1.;
            TI(i+1, i) = -1.;
    }

    cstr->AddInequality(TI, ti0);
}

void ConstrainedFit2D::SetSlopeY(int slope_y)
{
    if (slope_y != 1 && slope_y != -1)
        return;
// slope == 1 => increasing with y

    int nbasx = bs->GetBSX().GetNbas();
    int nbasy = bs->GetBSY().GetNbas();

    MatrixXd TI = MatrixXd::Zero(nbas, (nbasy-1)*3);
    VectorXd ti0 = VectorXd::Zero((nbasy-1)*3);

    for (int iy = 0; iy<nbasy-1; iy++)
        for (int i=0; i<3; i++) {
        TI(iy*nbasx+i, iy*3+i) = -slope_y;
        TI((iy+1)*nbasx+i, iy*3+i) = slope_y;
    }

    cstr->AddInequality(TI, ti0);
}

// this is a special constraint requiring the spline to have maximum around (x0, y0)
// and decrease from there in any direction
// it is only compatible with non_negative and overrides everything else
void ConstrainedFit2D::ForceTopDown(double x0, double y0)
{
    this->x0 = x0;
    this->y0 = y0;

    int nbasx = bs->GetBSX().GetNbas();
    int nbasy = bs->GetBSY().GetNbas();;
    int nintx = bs->GetNintX();
    int ninty = bs->GetNintX();

    double xmin = bs->GetXmin();
    double xmax = bs->GetXmax();
    double ymin = bs->GetYmin();
    double ymax = bs->GetYmax();
    double dx = (xmax-xmin)/nintx;
    double dy = (ymax-ymin)/ninty;

    int cols = (nbasx-1)*(nbasy-1)*2;
    int shift = (nbasx-1)*(nbasy-1); // shift to the second group of inequalities

    MatrixXd TI = MatrixXd::Zero(nbas, cols);
    VectorXd ti0 = VectorXd::Zero(cols);

    for (int iy = 0; iy<nbasy-1; iy++)
        for (int ix = 0; ix<nbasx-1; ix++) {
            int i = ix + iy*nbasx;
//                                        int i = ix + iy*(nbasx-1);
            double x = xmin + dx*(ix-1);
            double y = ymin + dy*(iy-1);

// 1st group of inequalities
            if (r(x, y) > r(x, y+dy)) {
                TI(i, i) = -1; TI(i+nbasx, i)=1;
            } else {
                TI(i, i) = 1; TI(i+nbasx, i)=-1;
            }
// 2nd group of inequalities
            if (r(x, y) > r(x+dx, y)) {
                TI(i, i+shift) = -1; TI(i+1, i+shift)=1;
            } else {
                TI(i, i+shift) = 1; TI(i+1, i+shift)=-1;
            }
    }
    cstr->AddInequality(TI, ti0);
}

// force the function even by constraining d/dx to 0 at x=0
// let's do it for the center of each interval in y
void ConstrainedFit2D::ForceFlatTopX()
{
    int nbasy = bs->GetBSY().GetNbas();
    double ymin = bs->GetYmin();
    double ymax = bs->GetYmax();

    double dy = (ymax-ymin)/nbasy;
    MatrixXd TE = MatrixXd::Zero(nbas, nbasy);
    VectorXd te0 = VectorXd::Zero(nbasy);
    for (int iy=0; iy<nbasy; iy++) {
        double y = ymin + (iy+0.5)*dy;
        for (int k=0; k<nbas; k++)
            TE(k, iy) = bs->BasisDrvX(0., y, k);
    }

    cstr->AddEquality(TE, te0);
}

// ================ Base class functions =================

BSfit::Method BSfit::SelectMethod()
{
    if (method != Auto)
        return method;

    if (nbas < 16) 
        return SVD;
    else if (nbas < 400) 
        return QR;
    else
        return QR_Sparse;
}

bool BSfit::SolveLinSystem()
{
    switch (SelectMethod()) {
        case SVD:
            return SolveSVD();
            break;
        case QR:
            return SolveQR();
            break;
        case QR_Sparse:
            return SolveSparseQR();
            break;          
        default:
            return false;
            break;
    }
}

bool BSfit::SolveSVD()
{
// solve the system using SVD
    JacobiSVD <MatrixXd> svd(A, Eigen::ComputeThinU | Eigen::ComputeThinV);
    VectorXd singval = svd.singularValues();
//std::cout << "Singular values: " << svd.singularValues();
    svd_condition = singval(0)/singval(singval.size()-1);
    x = svd.solve(y);

    VectorXd r = A*x - y;
    residual = sqrt(r.squaredNorm());
    return true;
}

bool BSfit::SolveQR()
{
// solve the system using QR decomposition
    x = A.colPivHouseholderQr().solve(y);
    VectorXd r = A*x - y;
    residual = sqrt(r.squaredNorm()); 
    return true;
}

bool BSfit::SolveSparseQR()
{
// sparse version - EXPERIMENTAL
    int nz_elem = 0;
    for (int i=0; i<A.rows(); i++)
        for (int j=0; j<A.cols(); j++)
            if (A(i,j) != 0.)
                nz_elem++;

    std::vector <Eigen::Triplet<double> > tripletList;
    tripletList.reserve(nz_elem);
    for (int i=0; i<A.rows(); i++)
        for (int j=0; j<A.cols(); j++)
            if (A(i,j) != 0.)
                tripletList.push_back(Eigen::Triplet<double>(i,j,A(i,j)));

    SparseMatrix <double> A_sp(A.rows(),A.cols());
    A_sp.setFromTriplets(tripletList.begin(), tripletList.end());

    SparseQR <SparseMatrix<double>, Eigen::COLAMDOrdering<int> > solver;
    solver.compute(A_sp);
    if(solver.info()!=Eigen::Success) {
        error_msg = "SparseQR: decomposition failed";
        return false;
    }

    x = solver.solve(y);
    if(solver.info()!=Eigen::Success) {
        error_msg = "SparseQR: solving failed";
        return false;
    }

    VectorXd r = A_sp*x - y;
    residual = sqrt(r.squaredNorm());
    return true;
}  

bool BSfit::SolveQuadProg(MatrixXd &CI, VectorXd &ci0, MatrixXd &CE, VectorXd &ce0)
{
// constrained fit

// solve the system using quadratic programming, i.e.
// minimize 1/2 * x G x + g0 x subject to some inequality and equality constraints,
// where G = A'A and g0 = -y'A (' denotes transposition)
    G = A.transpose()*A;
    g0 = (y.transpose()*A)*(-1.);

// all prepared - call the solver
    residual = solve_quadprog(G, g0,  CE, ce0,  CI, ci0, x);
    return true;
}

// void BSfit::DumpQuadProg()
// {
//     std::cout << "A:" << std::endl;
//     std::cout << A << std::endl;

//     std::cout << "y:" << std::endl;
//     std::cout << y << std::endl;

//     std::cout << "G:" << std::endl;
//     std::cout << G << std::endl;

//     std::cout << "g0:" << std::endl;
//     std::cout << g0 << std::endl;

//     std::cout << "CE:" << std::endl;
//     std::cout << CE << std::endl;

//     std::cout << "CI:" << std::endl;
//     std::cout << CI << std::endl;
// }

// =============== Fit 1D spline ===============

BSfit1D::BSfit1D(Bspline1d *bs)
{
    this->bs = new BsplineBasis1d(*bs);
    Init();
}

BSfit1D::BSfit1D(double xmin, double xmax, int n_int)
{
    bs = new BsplineBasis1d(xmin, xmax, n_int);
    Init();
}

BSfit1D::~BSfit1D()
{
    delete h1;
    delete bs;
}

BSfit1D* BSfit1D::clone() const 
{ 
    BSfit1D *copy = new BSfit1D(*this); 
    copy->h1 = h1 ? new ProfileHist1D(*h1) : nullptr;
    copy->bs = bs ? new BsplineBasis1d(*bs) : nullptr;
    return copy;
}

void BSfit1D::Init()
{
    nbas = bs->GetNbas();
    h1 = 0;
    nbins = 0;  
    SetBinningAuto();
}

bool BSfit1D::SetBinning(int bins)
{
// we need at least nbas bins to get a unique solution    
    if (bins < nbas)
        return false;
    nbins = bins;    
// (re)create profile histogram for storing the binned data
    delete h1;
    h1 = new ProfileHist1D(nbins, bs->GetXmin(), bs->GetXmax());
    return true;
}

void BSfit1D::SetBinningAuto()
{
    // number of intervals can't be less then number of basis functions in the spline
    // we set it (somewhat arbitrarily) to 4 times number of intervals in the spline
    SetBinning(bs->GetNint()*4);
}

void BSfit1D::AddData(double const x, double const f)
{
    h1->Fill(x, f);
}

void BSfit1D::AddData(int npts, double const *datax, double const *datay)
{
    for (int i=0; i<npts; i++)
        h1->Fill(datax[i], datay[i]);
}

void BSfit1D::MkLinSystem(int npts, double const *datax, double const *data, double const *dataw)
{
// linear least squares: matrix eqation Ax=y
// we've got npts equations with nbas unknowns
    A.resize(npts, nbas);
    y.resize(npts);
 
// fill the equations
    if (dataw == 0) {   // for not weighted data
        for (int i=0; i<npts; i++) {
            y(i) = data[i];
            for (int k=0; k<nbas; k++)
                A(i, k) = bs->Basis(datax[i], k);
        }
    } else {            // for weighted data
        for (int i=0; i<npts; i++) {
            if (dataw[i] > 0.5) { // normal point (W >= 1)
                y(i) = data[i] * dataw[i];
                for (int k=0; k<nbas; k++)
                    A(i, k) = bs->Basis(datax[i], k) * dataw[i];
            } else {            // missing point (W == 0) - make it smooth by setting 2nd derivative to zero
                y(i) = 0.;
                for (int k=0; k<nbas; k++)
                    A(i, k) = bs->BasisDrv2(datax[i], k);
            }
        }
    }
}

// this is the default fitting routine to be performed on the binned data
bool BSfit1D::BinnedFit()
{
    if (!h1 || h1->GetEntries() == 0. ) {
        error_msg = "BSfit1D: No binned data to fit. Call AddData(npts, datax, datay) first";
        return false;
    }

    std::vector<double> vx, vdata, vw;
// extract the accumulated binned data from the profile histogram
    for (int ix=0; ix<nbins; ix++) {
        vx.push_back(h1->GetBinCenterX(ix));
        vdata.push_back(h1->GetBinMean(ix));
        vw.push_back(h1->GetBinEntries(ix));
    }

    return WeightedFit(vw.size(), &vx[0], &vdata[0], &vw[0]);
}

bool BSfit1D::WeightedFit(int npts, double const *datax, double const *data, double const *dataw)
{
    MkLinSystem(npts, datax, data, dataw);
    status = SolveLinSystem();
    return status;
}

std::vector<double> BSfit1D::GetCoef() const
{
    std::vector <double> c(nbas, 0.);
    for (int i=0; i<nbas; i++)
        c[i] = x(i);       
    return c;
}

Bspline1d *BSfit1D::MakeSpline() const
{
    Bspline1d *bspl = new Bspline1d(*bs);
    std::vector <double> c = GetCoef();
    bspl->SetCoef(c);
    return bspl;    
}

Bspline1d *BSfit1D::FitAndMakeSpline(std::vector <double> &datax, std::vector <double> &data)
{
    bool status = Fit(datax, data);
    return status ? MakeSpline() : nullptr;
}

ProfileHist *BSfit1D::GetHist()
{
    return h1; //dynamic_cast <ProfileHist*>(h1);
}

ConstrainedFit1D::ConstrainedFit1D(double xmin, double xmax, int n_int) : BSfit1D(xmin, xmax, n_int)
{
    cstr = new Constraints(bs->GetNbas());
}

ConstrainedFit1D::ConstrainedFit1D(Bspline1d *bs_) : BSfit1D(bs_)
{
    cstr = new Constraints(bs->GetNbas());
}

ConstrainedFit1D* ConstrainedFit1D::clone() const 
{ 
    ConstrainedFit1D *copy = new ConstrainedFit1D(*this); 
    copy->h1 = h1 ? new ProfileHist1D(*h1) : nullptr;
    copy->bs = bs ? new BsplineBasis1d(*bs) : nullptr;
    copy->cstr = cstr ? new Constraints(*cstr) : nullptr;
    return copy;
}

bool ConstrainedFit1D::SolveLinSystem()
{
    return SolveQuadProg(cstr->CI, cstr->ci0, cstr->CE, cstr->ce0);
}

// =============== Fit 2D spline ===============

BSfit2D::BSfit2D(Bspline2d *bs)
{
    this->bs = new BsplineBasis2d(*bs);
    Init();
}

BSfit2D::BSfit2D(double xmin, double xmax, int n_intx, double ymin, double ymax, int n_inty)
{
    bs = new BsplineBasis2d(xmin, xmax, n_intx, ymin, ymax, n_inty);
    Init();
}

BSfit2D::~BSfit2D()
{
    delete h2;
    delete bs;
}

BSfit2D* BSfit2D::clone() const 
{ 
    BSfit2D *copy = new BSfit2D(*this); 
    copy->h2 = h2 ? new ProfileHist2D(*h2) : nullptr;
    copy->bs = bs ? new BsplineBasis2d(*bs) : nullptr;
    return copy;
}

void BSfit2D::Init()
{
    nbas = bs->GetNbas();
    nbasx = bs->GetBSX().GetNbas();
    nbasy = bs->GetBSY().GetNbas();

// set binning to zero -- ToDo: probably not needed now that auto binning is default
    nbinsx = 0;
    nbinsy = 0;
    h2 = 0;
    SetBinningAuto();
}

bool BSfit2D::SetBinning(int binsx, int binsy)
{
// we need at least nbas bins to get a unique solution
// it also must be a bad idea to have less than nbasx(y) intervals 
// in each direction => flagging this situation as invalid    
    if (binsx < nbasx || binsy < nbasy)
        return false;
    nbinsx = binsx;
    nbinsy = binsy;
// (re)create profile histogram for storing the binned data
    delete(h2);
    h2 = new ProfileHist2D(nbinsx, bs->GetXmin(), bs->GetXmax(),
                                    nbinsy, bs->GetYmin(), bs->GetYmax());
    return true;
}

void BSfit2D::SetBinningAuto()
{
    // number of bins can't be less then number of basis functions in the spline
    // we set it (somewhat arbitrarily) to 2 times number of intervals in each direction
    // this gives 4 bins per elementary rectange (safe down to 3x3 TP splines)
    SetBinning(bs->GetNintX()*2, bs->GetNintY()*2);
}

void BSfit2D::AddData(double const x, double const y, double const f)
{
    h2->Fill(x, y, f);
}

void BSfit2D::AddData(int npts, double const *datax, double const *datay, const double *dataz)
{
    for (int i=0; i<npts; i++)
        h2->Fill(datax[i], datay[i], dataz[i]);
}

void BSfit2D::MkLinSystem(int npts, double const *datax, double const *datay, double const *data, double const *dataw)
{
// for the case of weighted data with missing points,
// we need to make a provision for additional 2nd derivative equations
    int missing = 0;
    int missing_ptr = npts; // put the additional equations in the end

    if (dataw != 0) {
        for (int i=0; i<npts; i++)
            if (dataw[i] < 0.5) // missing point
                missing++;
    }

// linear least squares
// we've got nbas unknowns to determine from npts equations
// (plus additional 2 equations per each missing point because of partial derivatives)
    A.resize(npts+missing*2, nbas);
    y.resize(npts+missing*2);

// fill the equations
    if (dataw == 0) {   // for not weighted data
        for (int i=0; i<npts; i++) {
            y(i) = data[i];
            for (int k=0; k<nbas; k++)
                A(i, k) = bs->Basis(datax[i], datay[i], k);
        }
    } else {            // for weighted data
        for (int i=0; i<npts; i++) {
            if (dataw[i] > 0.5) { // normal point (W >= 1)
                y(i) = data[i] * dataw[i];
                for (int k=0; k<nbas; k++)
                    A(i, k) = bs->Basis(datax[i], datay[i], k) * dataw[i];
            } else {            // missing point (W == 0) - make it smooth by setting 2nd derivatives to zero
// in 2D case it will be d2/dxdy, d2/dx2 and d2/dy2
// the first goes in place the of the original equation
// and the other two to the additional rows reserved in the end
                y(i) = 0.;
                for (int k=0; k<nbas; k++)
                    A(i, k) = bs->BasisDrv2XY(datax[i], datay[i], k);

                y(missing_ptr) = 0.;
                for (int k=0; k<nbas; k++)
                    A(missing_ptr, k) = bs->BasisDrv2XX(datax[i], datay[i], k);
                missing_ptr++;

                y(missing_ptr) = 0.;
                for (int k=0; k<nbas; k++)
                    A(missing_ptr, k) = bs->BasisDrv2YY(datax[i], datay[i], k);
                missing_ptr++;
            }
        }
    }
}



// this is the default fitting routine to be performed on the binned data
bool BSfit2D::BinnedFit()
{
    if (!h2 || h2->GetEntries() == 0. ) {
        error_msg = "BSfit2D: No data to fit. Call AddData(npts, datax, datay, dataz) first\n";
        return false;
    }
    std::vector<double> vx, vy, vdata, vw;
// extract the accumulated binned data from the profile histogram
    for (int ix=0; ix<nbinsx; ix++)
      for (int iy=0; iy<nbinsy; iy++) {
        vx.push_back(h2->GetBinCenterX(ix));
        vy.push_back(h2->GetBinCenterY(iy));
        vdata.push_back(h2->GetBinMean(ix, iy));
        vw.push_back(h2->GetBinEntries(ix, iy));
    }

    return WeightedFit(vw.size(), &vx[0], &vy[0], &vdata[0], &vw[0]);
}

bool BSfit2D::WeightedFit(int npts, double const *datax, double const *datay, double const *data, double const *dataw)
{
    MkLinSystem(npts, datax, datay, data, dataw);
    return SolveLinSystem();
}

bool BSfit2D::Fit(std::vector <double> &datax, std::vector <double> &datay, std::vector <double> &data)
{
    int npts = std::min({datax.size(), datay.size(), data.size()});
    return Fit(npts, &datax[0], &datay[0], &data[0]);
}

std::vector<double> BSfit2D::GetCoef() const
{
    std::vector <double> c(nbas, 0.);
    for (int i=0; i<nbas; i++)
        c[i] = x(i);       
    return c;
}

Bspline2d *BSfit2D::MakeSpline() const
{
    Bspline2d *bspl = new Bspline2d(*bs);
    std::vector <double> c = GetCoef();
    bspl->SetCoef(c);
    return bspl;    
}

Bspline2d *BSfit2D::FitAndMakeSpline(std::vector <double> &datax, std::vector <double> &datay, std::vector <double> &data)
{
    bool status = Fit(datax, datay, data);
    return status ? MakeSpline() : nullptr;
}

ProfileHist *BSfit2D::GetHist()
{
    return h2; // dynamic_cast <ProfileHist*>(h2);
}

ConstrainedFit2D::ConstrainedFit2D(double xmin, double xmax, int n_intx, double ymin, double ymax, int n_inty) : 
        BSfit2D(xmin, xmax, n_intx, ymin, ymax, n_inty)
{
    cstr = new Constraints(bs->GetNbas());
}

ConstrainedFit2D* ConstrainedFit2D::clone() const 
{ 
    ConstrainedFit2D *copy = new ConstrainedFit2D(*this); 
    copy->h2 = h2 ? new ProfileHist2D(*h2) : nullptr;
    copy->bs = bs ? new BsplineBasis2d(*bs) : nullptr;
    copy->cstr = cstr ? new Constraints(*cstr) : nullptr;
    return copy;
}

bool ConstrainedFit2D::SolveLinSystem()
{
    return SolveQuadProg(cstr->CI, cstr->ci0, cstr->CE, cstr->ce0);
}


// =============== Fit 3D spline ===============

BSfit3D::BSfit3D(Bspline3d *bs)
{
    this->bs = new BsplineBasis3d(*bs);
    Init();
}

BSfit3D::BSfit3D(double xmin, double xmax, int n_intx, double ymin, double ymax, int n_inty, double zmin, double zmax, int n_intz)
{
    bs = new BsplineBasis3d(xmin, xmax, n_intx, ymin, ymax, n_inty, zmin, zmax, n_intz);
    Init();
}

BSfit3D::~BSfit3D()
{
    delete h3;
    delete bs;
}

BSfit3D* BSfit3D::clone() const 
{ 
    BSfit3D *copy = new BSfit3D(*this); 
    copy->h3 = h3 ? new ProfileHist3D(*h3) : nullptr;
    copy->bs = bs ? new BsplineBasis3d(*bs) : nullptr;
    return copy;
}

void BSfit3D::Init()
{
    nbas = bs->GetNbas();
    nbasx = bs->GetBSXY().GetBSX().GetNbas();
    nbasy = bs->GetBSXY().GetBSY().GetNbas();
    nbasxy = bs->GetNbasXY();
    nbasz = bs->GetNbasZ();
// no binning by default
    nbinsx = 0;
    nbinsy = 0;
    nbinsz = 0;
    h3 = 0;
    SetBinningAuto();
}

bool BSfit3D::SetBinning(int binsx, int binsy, int binsz)
{
// we need at least nbas bins to get a unique solution
// it also must be a bad idea to have less than nbasx(y,z) intervals 
// in each direction => flagging this situation as invalid    
    if (binsx < nbasx || binsy < nbasy || binsz < nbasz)
        return false;
    nbinsx = binsx;
    nbinsy = binsy;
    nbinsz = binsz;
// (re)create profile histogram for storing the binned data
    if (h3 != 0) 
        delete(h3);
    h3 = new ProfileHist3D(nbinsx, bs->GetXmin(), bs->GetXmax(),
                                    nbinsy, bs->GetYmin(), bs->GetYmax(),
                                    nbinsz, bs->GetZmin(), bs->GetZmax());

    return true;
}

void BSfit3D::SetBinningAuto()
{
    // number of bins can't be less then number of basis functions in the spline
    // we set it (somewhat arbitrarily) to 2 times number of intervals in each direction
    // this gives 8 bins per elementary box (safe down to 3x3x3 3D splines)
    SetBinning(bs->GetNintX()*2, bs->GetNintY()*2, bs->GetNintZ()*2);
}

void BSfit3D::AddData(int npts, double const *x, double const *y, const double *z, const double *data)
{
    if (nbinsx == 0)
        SetBinningAuto();

    for (int i=0; i<npts; i++)
        h3->Fill(x[i], y[i], z[i], data[i]);
}

void BSfit3D::MkLinSystem(int npts, double const *datax, double const *datay, double const *dataz, double const *data, double const *dataw)
{
// for the case of weighted data with missing points,
// we need to make a provision for additional 2nd derivative equations
    int missing = 0;
    int missing_ptr = npts; // put the additional equations in the end

    if (dataw != 0) {
        for (int i=0; i<npts; i++)
            if (dataw[i] < 0.5) // missing point
                missing++;
    }

// linear least squares
// we've got nbas unknowns to determine from npts equations
// (plus additional 5 equations per each missing point because of partial derivatives)
    A.resize(npts+missing*5, nbas);
    y.resize(npts+missing*5);

// fill the equations
    if (dataw == 0) {   // for not weighted data
        for (int i=0; i<npts; i++) {
            y(i) = data[i];
            for (int k=0; k<nbas; k++)
                A(i, k) = bs->Basis(datax[i], datay[i], dataz[i], k);
        }
    } else {            // for weighted data
        for (int i=0; i<npts; i++) {
            if (dataw[i] > 0.5) { // normal point (W >= 1)
                y(i) = data[i] * dataw[i];
                for (int k=0; k<nbas; k++)
                    A(i, k) = bs->Basis(datax[i], datay[i], dataz[i], k) * dataw[i];
            } else {            // missing point (W == 0) - make it smooth by setting 2nd derivatives to zero
// in 3D case it will be d2/dxdy, d2/dxdz, d2/dydz, d2/dx2, d2/dy2 and d2/dz2
// the first goes in place the of the original equation
// and the other five to the additional rows reserved in the end
                y(i) = 0.;
                for (int k=0; k<nbas; k++)
                    A(i, k) = bs->BasisDrv2XY(datax[i], datay[i], dataz[i], k);

                y(missing_ptr) = 0.;
                for (int k=0; k<nbas; k++)
                    A(missing_ptr, k) = bs->BasisDrv2XX(datax[i], datay[i], dataz[i], k);
                missing_ptr++;

                y(missing_ptr) = 0.;
                for (int k=0; k<nbas; k++)
                    A(missing_ptr, k) = bs->BasisDrv2YY(datax[i], datay[i], dataz[i], k);
                missing_ptr++;

                y(missing_ptr) = 0.;
                for (int k=0; k<nbas; k++)
                    A(missing_ptr, k) = bs->BasisDrv2ZZ(datax[i], datay[i], dataz[i], k);
                missing_ptr++;

                y(missing_ptr) = 0.;
                for (int k=0; k<nbas; k++)
                    A(missing_ptr, k) = bs->BasisDrv2XZ(datax[i], datay[i], dataz[i], k);
                missing_ptr++;

                y(missing_ptr) = 0.;
                for (int k=0; k<nbas; k++)
                    A(missing_ptr, k) = bs->BasisDrv2YZ(datax[i], datay[i], dataz[i], k);
                missing_ptr++;
            }
        }
    }
}

// this is the default fitting routine to be performed on the binned data
bool BSfit3D::BinnedFit()
{
    if (!h3 || h3->GetEntries() == 0. ) {
        error_msg = "BSfit3D: No data to fit. Call AddData(npts, datax, datay, dataz) first\n";
        return false;
    }
    std::vector<double> vx, vy, vz, vdata, vw;
// extract the accumulated binned data from the profile histogram
    for (int ix=0; ix<nbinsx; ix++)
      for (int iy=0; iy<nbinsy; iy++)
        for (int iz=0; iz<nbinsz; iz++) {
            vx.push_back(h3->GetBinCenterX(ix));
            vy.push_back(h3->GetBinCenterY(iy));
            vz.push_back(h3->GetBinCenterZ(iz));
            vdata.push_back(h3->GetBinMean(ix, iy, iz));
            vw.push_back(h3->GetBinEntries(ix, iy, iz));
    }

    return WeightedFit(vw.size(), &vx[0], &vy[0], &vz[0], &vdata[0], &vw[0]);
}

bool BSfit3D::WeightedFit(int npts, double const *datax, double const *datay, double const *dataz, double const *data, double const *dataw)
{
    MkLinSystem(npts, datax, datay, dataz, data, dataw);
    return SolveLinSystem();
}

bool BSfit3D::Fit(std::vector <double> &datax, std::vector <double> &datay, std::vector <double> &dataz, std::vector <double> &data)
{
    int npts = std::min({datax.size(), datay.size(), dataz.size(), data.size()});
    return Fit(npts, &datax[0], &datay[0], &dataz[0], &data[0]);
}

std::vector<double> BSfit3D::GetCoef(int iz) const
{
    std::vector <double> c(nbasxy, 0.);
    for (int i=0; i<nbasxy; i++)
        c[i] = x(iz*nbasxy + i);      
    return c;
}

Bspline3d *BSfit3D::MakeSpline() const
{
    Bspline3d *bspl = new Bspline3d(*bs);
    for (int iz=0; iz<nbasz; iz++) {
        std::vector <double> c = GetCoef(iz);
        bspl->SetZplaneCoef(iz, c);
    }
    return bspl;    
}

Bspline3d *BSfit3D::FitAndMakeSpline(std::vector <double> &datax, std::vector <double> &datay, std::vector <double> &dataz, std::vector <double> &data)
{
    bool status = Fit(datax, datay, dataz, data);
    return status ? MakeSpline() : nullptr;
}

ProfileHist *BSfit3D::GetHist()
{
    return h3; // dynamic_cast <ProfileHist*>(h3);
}

ConstrainedFit3D::ConstrainedFit3D(double xmin, double xmax, int n_intx, double ymin, double ymax, int n_inty, double zmin, double zmax, int n_intz) : 
        BSfit3D(xmin, xmax, n_intx, ymin, ymax, n_inty, zmin, zmax, n_intz)
{
    cstr = new Constraints(bs->GetNbas());
}

ConstrainedFit3D* ConstrainedFit3D::clone() const 
{ 
    ConstrainedFit3D *copy = new ConstrainedFit3D(*this); 
    copy->h3 = h3 ? new ProfileHist3D(*h3) : nullptr;
    copy->bs = bs ? new BsplineBasis3d(*bs) : nullptr;
    copy->cstr = cstr ? new Constraints(*cstr) : nullptr;
    return copy;
}

bool ConstrainedFit3D::SolveLinSystem()
{
    return SolveQuadProg(cstr->CI, cstr->ci0, cstr->CE, cstr->ce0);
}


