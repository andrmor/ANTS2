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

// ================ Base class functions =================

BSfit::BSfit()
{
// no constraints by default
    non_negative = false;   // Andr ! move initialization to the header
}

double BSfit::SolveSVD()
{
// solve the system using SVD
    JacobiSVD <MatrixXd> svd(A, Eigen::ComputeThinU | Eigen::ComputeThinV);
    VectorXd singval = svd.singularValues();
//std::cout << "Singular values: " << svd.singularValues();
    double k = singval(0)/singval(singval.size()-1);
    x = svd.solve(y);

    VectorXd r = A*x - y;
    residual = sqrt(r.squaredNorm());
    return k;
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

void BSfit::SolveQuadProg()
{
// constrained fit

// solve the system using quadratic programming, i.e.
// minimize 1/2 * x G x + g0 x subject to some inequality and equality constraints,
// where G = A'A and g0 = -y'A (' denotes transposition)
    G = A.transpose()*A;
    g0 = (y.transpose()*A)*(-1.);

// all prepared - call the solver
    residual = solve_quadprog(G, g0,  CE, ce0,  CI, ci0, x);
}

void BSfit::SetNonNegative()
{
    if (ci0.size() == 0) {
        CI = MatrixXd::Zero(nbas, nbas);
        ci0 = VectorXd::Zero(nbas);
    }
    for (int i = 0; i<nbas; i++)
        CI(i, i) = 1.;
}

void BSfit::SetNonIncreasing()
{
    if (ci0.size() == 0) {
        CI = MatrixXd::Zero(nbas, nbas);
        ci0 = VectorXd::Zero(nbas);
    }
    for (int i = 0; i<nbas-1; i++) {
        CI(i, i) = 1.;
        CI(i+1, i) = -1.;
    }
}

void BSfit::SetNonDecreasing()
{
    if (ci0.size() == 0) {
        CI = MatrixXd::Zero(nbas, nbas);
        ci0 = VectorXd::Zero(nbas);
    }
    for (int i = 1; i<nbas; i++) {
        CI(i, i) = 1.;
        CI(i-1, i) = -1.;
    }
}

void BSfit::AddConstraintCIneq(MatrixXd DI, VectorXd di0)
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

void BSfit::AddConstraintCEq(MatrixXd DE, VectorXd de0)
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

void BSfit::DumpQuadProg()
{
    std::cout << "A:" << std::endl;
    std::cout << A << std::endl;

    std::cout << "y:" << std::endl;
    std::cout << y << std::endl;

    std::cout << "G:" << std::endl;
    std::cout << G << std::endl;

    std::cout << "g0:" << std::endl;
    std::cout << g0 << std::endl;

    std::cout << "CE:" << std::endl;
    std::cout << CE << std::endl;

    std::cout << "CI:" << std::endl;
    std::cout << CI << std::endl;
}

// =============== Fit 1D spline ===============

BSfit1D::BSfit1D(Bspline1d *bs_)
{
    bs = bs_;
    nbas = bs->GetNbas();
// no constraints by default
    non_negative = false;
    non_increasing = false;
    non_decreasing = false;
    even = false;
// make cure that profile histogram will be created by SetBinning()    
    h1 = 0;
    nbins = 0;  
//    SetBinningAuto();
}

BSfit1D::~BSfit1D()
{
    if (h1)
        delete h1;  // Andr : "if (h1)" has no effect, remove; in the header: h1 = nullptr
}

bool BSfit1D::SetBinning(int bins)
{
// we need at least nbas bins to get a unique solution    
    if (bins < nbas)
        return false;
    nbins = bins;    
// (re)create profile histogram for storing the binned data
    if (h1 != 0) 
        delete h1; // Andr : before was delete (h1)
    h1 = new ProfileHist1D(nbins, bs->GetXmin(), bs->GetXmax());
    return true;
}

void BSfit1D::SetBinningAuto()
{
    // number of intervals can't be less then number of basis functions in the spline
    // we set it (somewhat arbitrarily) to 4 times number of intervals in the spline
    SetBinning(bs->GetNint()*4);
}

void BSfit1D::AddData(int npts, double const *datax, double const *datay)
{
    if (nbins == 0)
        SetBinningAuto();

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

void BSfit1D::SetEven()
{
// force the function even by constraining the derivative to 0 at x=0
    CE = MatrixXd::Zero(nbas, 1);
    ce0 = VectorXd::Zero(1);
    for (int k=0; k<nbas; k++)
        CE(k, 0) = bs->BasisDrv(0., k);
}

// this is the default fitting routine to be performed on the binned data
bool BSfit1D::Fit()
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

    return Fit(vw.size(), &vx[0], &vdata[0], &vw[0]);
}

bool BSfit1D::Fit(int npts, double const *datax, double const *data, double const *dataw)
{
    MkLinSystem(npts, datax, data, dataw);

    if (!non_negative && !non_increasing && !even)  // unconstrained fit
        SolveSVD();
    else {
// set all activated constraints
        if (non_negative)
            SetNonNegative();
        if (non_increasing)
            SetNonIncreasing();
        if (non_decreasing)
            SetNonDecreasing();
        if (even)
            SetEven();

        SolveQuadProg();
    }

// fill bs from the solution vector
    std::vector <double> c(nbas, 0.);
    for (int i=0; i<nbas; i++)
        c[i] = x(i);
    bs->SetCoef(c);

    return true;
}

// =============== Fit 2D spline ===============

BSfit2D::BSfit2D(Bspline2d *bs_)
{
    bs = bs_;
    nbas = bs->GetNbas();
    nbasx = bs->GetBSX().GetNbas();
    nbasy = bs->GetBSY().GetNbas();
// no binning by default
    nbinsx = 0;
    nbinsy = 0;
// no constraints by default
    non_negative = false;
    non_increasing_x = false;
    flat_top_x = false;
    top_down = false;
    slope_y = 0;
    h2 = 0;
}

BSfit2D::~BSfit2D()
{
    if (h2)
        delete h2;
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
    if (h2 != 0) 
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

void BSfit2D::AddData(int npts, double const *datax, double const *datay, const double *dataz)
{
    if (nbinsx == 0)
        SetBinningAuto();

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

void BSfit2D::SetNonIncreasingX()
{
    MatrixXd CI_a = MatrixXd::Zero(nbas, nbas);
    VectorXd ci0_a = VectorXd::Zero(nbas);

    for (int iy = 0; iy<nbasy; iy++)
        for (int ix = 0; ix<nbasx-1; ix++) {
            int i = ix + iy*nbasx;
            CI_a(i, i) = 1.;
            CI_a(i+1, i) = -1.;
    }
    AddConstraintCIneq(CI_a, ci0_a);
}

void BSfit2D::SetSlopeY()
{
    MatrixXd CI_b = MatrixXd::Zero(nbas, (nbasy-1)*3);
    VectorXd ci0_b = VectorXd::Zero((nbasy-1)*3);
    double s = slope_y == 1 ? 1. : -1.; // slope == 1 => increasing with y
    std::cout << "Slope y: " << s <<"\n";
    for (int iy = 0; iy<nbasy-1; iy++)
        for (int i=0; i<3; i++) {
        CI_b(iy*nbasx+i, iy*3+i) = -s;
        CI_b((iy+1)*nbasx+i, iy*3+i) = s;
    }
    AddConstraintCIneq(CI_b, ci0_b);
}

// this is a special constraint requiring the spline to have maximum around (x0, y0)
// and decrese from there in any direction
// it is only compatible with non_negative and overrides everything else
void BSfit2D::SetTopDown()
{
    int cols = (nbasx-1)*(nbasy-1)*2;
    int shift = (nbasx-1)*(nbasy-1); // shift to the second group of inequalities
    if (non_negative)
        cols += 2*(nbasx+nbasy-2);
    CI = MatrixXd::Zero(nbas, cols);
    ci0 = VectorXd::Zero(cols);
    double xmin = bs->GetXmin();
    double xmax = bs->GetXmax();
    double ymin = bs->GetYmin();
    double ymax = bs->GetYmax();
//            qDebug() << xmin << ", " << xmax << ", " << ymin << ", " << ymax;
    int nintx = bs->GetNintX();
    int ninty = bs->GetNintX();
    double dx = (xmax-xmin)/nintx;
    double dy = (ymax-ymin)/ninty;
    for (int iy = 0; iy<nbasy-1; iy++)
        for (int ix = 0; ix<nbasx-1; ix++) {
            int i = ix + iy*nbasx;
//                                        int i = ix + iy*(nbasx-1);
            double x = xmin + dx*(ix-1);
            double y = ymin + dy*(iy-1);
//                    qDebug() << x << ", " << y << ", " << r(x, y) << ", " << r(x, y+dy);
// 1st group of inequalities
            if (r(x, y) > r(x, y+dy)) {
                CI(i, i) = -1; CI(i+nbasx, i)=1;
            } else {
                CI(i, i) = 1; CI(i+nbasx, i)=-1;
            }
// 2nd group of inequalities
            if (r(x, y) > r(x+dx, y)) {
                CI(i, i+shift) = -1; CI(i+1, i+shift)=1;
            } else {
                CI(i, i+shift) = 1; CI(i+1, i+shift)=-1;
            }
    }
// 3rd group of inequalities
    if (non_negative) {
        int k = (nbasx-1)*(nbasy-1)*2;
        for (int iy = 0; iy<nbasy; iy++)
            for (int ix = 0; ix<nbasx; ix++)
                if (ix == 0 || ix == nbasx-1 || iy == 0 || iy == nbasy-1)
                    CI(ix + iy*nbasx ,k++) = 1.;
    }
}

// force the function even by constraining d/dx to 0 at x=0
// let's do it for the center of each interval in y
void BSfit2D::SetFlatTopX()
{
    double ymin = bs->GetYmin();
    double ymax = bs->GetYmax();
    double dy = (ymax-ymin)/nbasy;
    CE = MatrixXd::Zero(nbas, nbasy);
    ce0 = VectorXd::Zero(nbasy);
    for (int iy=0; iy<nbasy; iy++) {
        double y = ymin + (iy+0.5)*dy;
        for (int k=0; k<nbas; k++)
            CE(k, iy) = bs->BasisDrvX(0., y, k);
    }    
}

// this is the default fitting routine to be performed on the binned data
bool BSfit2D::Fit()
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

    return Fit(vw.size(), &vx[0], &vy[0], &vdata[0], &vw[0]);
}

bool BSfit2D::Fit(int npts, double const *datax, double const *datay, double const *data, double const *dataw)
{
    MkLinSystem(npts, datax, datay, data, dataw);

    if (!non_negative && !non_increasing_x && !slope_y && !flat_top_x && !top_down) { // unconstrained fit
    // solve the system using QR decomposition (SVD is too slow)
        sparse = true;
        if (!sparse) {
            SolveQR();
        } else { 
            SolveSparseQR();
        }

    } else { // constrained fit
        // set all activated constraints
        if (non_negative)
            SetNonNegative();
        if (non_increasing_x)
            SetNonIncreasingX();
        if (slope_y)
            SetSlopeY();
        if (flat_top_x)
            SetFlatTopX();
        if (top_down)
            SetTopDown();

        SolveQuadProg();
    }

// fill bs from the solution vector
    std::vector <double> c(nbas, 0.);
    for (int i=0; i<nbas; i++)
        c[i] = x(i);
    bs->SetCoef(c);

    return true;
}

// =============== Fit 3D spline ===============

BSfit3D::BSfit3D(Bspline3d *bs_)
{
    bs = bs_;
    nbas = bs->GetNbas();
    nbasx = bs->GetBSXY().GetBSX().GetNbas();
    nbasy = bs->GetBSXY().GetBSY().GetNbas();
    nbasxy = bs->GetNbasXY();
    nbasz = bs->GetNbasZ();
// no binning by default
    nbinsx = 0;
    nbinsy = 0;
    nbinsz = 0;
// no constraints by default
    non_negative = false;
    h3 = 0;
}

BSfit3D::~BSfit3D()
{
    if (h3)
        delete h3;
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
bool BSfit3D::Fit()
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

    return Fit(vw.size(), &vx[0], &vy[0], &vz[0], &vdata[0], &vw[0]);
}

bool BSfit3D::Fit(int npts, double const *datax, double const *datay, double const *dataz, double const *data, double const *dataw)
{
    MkLinSystem(npts, datax, datay, dataz, data, dataw);

    if (!non_negative) { // unconstrained fit
    // solve the system using QR decomposition (SVD is too slow)
        sparse = true;
        if (!sparse) {
            SolveQR();
        } else { 
            SolveSparseQR();
        }

    } else { // constrained fit
        if (non_negative)
            SetNonNegative();
 
        SolveQuadProg();
    }

// fill bs from the solution vector
    std::vector <double> c(nbasxy, 0.);
    for (int iz=0; iz<nbasz; iz++) {
        for (int i=0; i<nbasxy; i++)
            c[i] = x(iz*nbasxy + i);
        bs->SetZplaneCoef(iz, c);
    }

    return true;
}
