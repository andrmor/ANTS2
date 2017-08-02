#include "tps3fit.h"
#ifdef TPS3M
#include "tpspline3m.h"
#else
#include "tpspline3.h"
#endif
#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <Eigen/SparseQR>
#include <Eigen/OrderingMethods>
#include "eiquadprog.hpp"
#include <TProfile2D.h>
#include <iostream>

#include <QDebug>
#include <QTime>


//#include <vector>

using Eigen::MatrixXd;
using Eigen::VectorXd;
using Eigen::JacobiSVD;
using Eigen::Triplet;
using Eigen::SparseQR;
using Eigen::SparseMatrix;

TPS3fit::TPS3fit(TPspline3 *bs_)
{
    bs = bs_;
    nbas = bs->GetNbas();
// no binning by default
    nbinsx = 0;
    nbinsy = 0;
// no constraints by default
    non_negative = false;
    non_increasing_x = false;
    flat_top_x = false;
    top_down = false;
    slope_y = 0;
    h1 = 0;
}

TPS3fit::~TPS3fit()
{
    if (h1)
        delete h1;
}

void TPS3fit::SetBinning(int binsx, int binsy)
{
    if (nbinsx > 0) {
        std::cout << "TPS3fit: attempt to redefine binning\n";
        return;
    }
// we need at least nbas bins to get a unique solution
// in 2D case, we assign them according to the number of intervals in each direction
    nbinsx = binsx < bs->GetNintX()+3 ? bs->GetNintX()+3 : binsx;
    nbinsy = binsy < bs->GetNintY()+3 ? bs->GetNintY()+3 : binsy;
// 2D profile histogram for storing the binned data
    h1 = new TProfile2D("bs32Daux", "", nbinsx, bs->GetXmin(), bs->GetXmax(),
                                    nbinsy, bs->GetYmin(), bs->GetYmax());
}

void TPS3fit::SetBinningAuto()
{
    // number of bins can't be less then number of basis functions in the spline
    // we set it (somewhat arbitrarily) to 2 times number of intervals in each direction
    // this gives 4 bins per elementary rectange (safe down to 3x3 TP splines)
    SetBinning(bs->GetNintX()*2, bs->GetNintY()*2);
}

void TPS3fit::AddData(int npts, double const *datax, double const *datay, const double *dataz)
{
    if (nbinsx == 0)
        SetBinningAuto();

    for (int i=0; i<npts; i++)
        h1->Fill(datax[i], datay[i], dataz[i]);
}


// this is the default fitting routine to be performed on the binned data
bool TPS3fit::Fit()
{
    if (!h1) {
        std::cout << "TPS3fit: No data to fit. Call AddData(npts, datax, datay, dataz) first\n";
        return false;
    }
// extract the accumulated binned data from the profile histogram
    for (int ix=1; ix<=nbinsx; ix++)
      for (int iy=1; iy<=nbinsy; iy++) {
        int nent = h1->GetBinEntries(h1->GetBin(ix,iy)); // number of entries in the bin (ix,iy)
//        if ( nent < 1)
//            continue;
        vx.push_back(h1->GetXaxis()->GetBinCenter(ix));
        vy.push_back(h1->GetYaxis()->GetBinCenter(iy));
        vz.push_back(h1->GetBinContent(ix, iy));
        vw.push_back(nent);
//        rms->push_back(h1->GetBinError(ix) * sqrt((double)nent));
    }

    return Fit(vw.size(), &vx[0], &vy[0], &vz[0], &vw[0]);
}


bool TPS3fit::Fit(int npts, double const *datax, double const *datay, double const *dataz, double const *dataw)
{
    QTime myTimer;

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
    MatrixXd A(npts+missing*2, nbas);
    VectorXd y(npts+missing*2);
    VectorXd x;

// fill the equations
    if (dataw == 0) {   // for not weighted data
        for (int i=0; i<npts; i++) {
            y(i) = dataz[i]; // yeah, that's not a typo :)
            for (int k=0; k<nbas; k++)
                A(i, k) = bs->Basis(datax[i], datay[i], k);
        }
    } else {            // for weighted data
        for (int i=0; i<npts; i++) {
            if (dataw[i] > 0.5) { // normal point (W >= 1)
                y(i) = dataz[i] * dataw[i];
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

    if (!non_negative && !non_increasing_x && !slope_y && !flat_top_x && !top_down) { // unconstrained fit
    // solve the system using QR decomposition (SVD is too slow)
        sparse = true;
        if (!sparse) {
            myTimer.start();
            x = A.colPivHouseholderQr().solve(y);
            qDebug() << "Solve: " << myTimer.elapsed();
            VectorXd r = A*x - y;
            residual = sqrt(r.squaredNorm());
        } else { // sparse version - EXPERIMENTAL
            int nz_elem = 0;
            for (int i=0; i<A.rows(); i++)
                for (int j=0; j<A.cols(); j++)
                    if (A(i,j) != 0.)
                        nz_elem++;

            qDebug() << "Non-zero elements: " << nz_elem;

            qDebug() << "Make triplet list";
            myTimer.start();
            std::vector <Eigen::Triplet<double> > tripletList;
            tripletList.reserve(nz_elem);
            for (int i=0; i<A.rows(); i++)
                for (int j=0; j<A.cols(); j++)
                    if (A(i,j) != 0.)
                        tripletList.push_back(Eigen::Triplet<double>(i,j,A(i,j)));
            qDebug() << "Done: " << myTimer.elapsed();
            qDebug() << "Fill sparse matrix";
            myTimer.start();
            SparseMatrix <double> A_sp(A.rows(),A.cols());
            A_sp.setFromTriplets(tripletList.begin(), tripletList.end());
            qDebug() << "Done: " << myTimer.elapsed();

            qDebug() << "Solving Sparse system";
            myTimer.start();

            SparseQR <SparseMatrix<double>, Eigen::COLAMDOrdering<int> > solver;
            solver.compute(A_sp);
            if(solver.info()!=Eigen::Success) {
              std::cout << "decomposition failed\n";
              return false;
            }
            qDebug() << "Compute QR: " << myTimer.elapsed();
            x = solver.solve(y);
            if(solver.info()!=Eigen::Success) {
              std::cout << "solving failed\n";
              return false;
            }
            qDebug() << "Solve: " << myTimer.elapsed();
            VectorXd r = A_sp*x - y;
            residual = sqrt(r.squaredNorm());
        }

    } else { // constrained fit

    // solve the system using quadratic programming, i.e.
    // minimize 1/2 * x G x + g0 x subject to some inequality and equality constraints,
    // where G = A'A and g0 = -y'A (' denotes transposition)
        MatrixXd G = A.transpose()*A;
        VectorXd g0 = (y.transpose()*A)*(-1.);

    // ineqaulity constraints:
        MatrixXd CI;
        VectorXd ci0;
    // equality constraints
        MatrixXd CE;
        VectorXd ce0;

    //  set ineqaulity constraints:
        int nbasx = bs->GetBSX().GetNbas();
        int nbasy = bs->GetBSY().GetNbas();
        // components of inequality constraints
        MatrixXd CI_a, CI_b;
        VectorXd ci0_a, ci0_b;
//        CI = MatrixXd::Zero(nbas, nbas+(nbasy-1)*3);
//        ci0 = VectorXd::Zero(nbas+(nbasy-1)*3);

        if (non_negative || non_increasing_x) {
            CI_a = MatrixXd::Zero(nbas, nbas);
            ci0_a = VectorXd::Zero(nbas);
        }
        if (non_negative) {
            std::cout << "Non-negative\n";
            for (int i = 0; i<nbas; i++)
                CI_a(i, i) = 1.;
        }
        if (non_increasing_x) {
            std::cout << "Non-increasing x\n";
            for (int iy = 0; iy<nbasy; iy++)
                for (int ix = 0; ix<nbasx-1; ix++) {
                    int i = ix + iy*nbasx;
                    CI_a(i, i) = 1.;
                    CI_a(i+1, i) = -1.;
            }
        }

        if (slope_y) {
            CI_b = MatrixXd::Zero(nbas, (nbasy-1)*3);
            ci0_b = VectorXd::Zero((nbasy-1)*3);
            double slope = slope_y == 1 ? 1. : -1.; // slope == 1 => increasing with y
            std::cout << "Slope y: " << slope <<"\n";
            for (int iy = 0; iy<nbasy-1; iy++)
              for (int i=0; i<3; i++) {
                CI_b(iy*nbasx+i, iy*3+i) = -slope;
                CI_b((iy+1)*nbasx+i, iy*3+i) = slope;
            }
        }

        if (top_down) { // this option is only compatible with non_negative, trumps everything else
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

        } else {
            // now concatenate the components into final CI matrix and ci0 vector
            int acols = CI_a.cols();
            int bcols = CI_b.cols();
            if (acols>0 && bcols>0) {
                CI = MatrixXd::Zero(nbas, acols+bcols);
                ci0 = VectorXd::Zero(acols+bcols);
                CI << CI_a, CI_b;
                ci0 << ci0_a, ci0_b;
            } else if (acols > 0) {
                CI = CI_a;
                ci0 = ci0_a;
            } else if (bcols > 0) {
                CI = CI_b;
                ci0 = ci0_b;
            }
        }

        if (flat_top_x) {
            std::cout << "Flat top (set d/dx=0 at x=0)\n";
        // force the function even by constraining d/dx to 0 at x=0
        // let's do it for the center of each interval in y
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

    /*
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
    */

    // all prepared - call the solver
        residual = solve_quadprog(G, g0,  CE, ce0,  CI, ci0, x);
    }

// fill bs from the solution vector
    double *c = new double[nbas];
    for (int i=0; i<nbas; i++)
        c[i] = x(i);
    bs->SetCoef(c);
    delete[] c;

    return true;
}
