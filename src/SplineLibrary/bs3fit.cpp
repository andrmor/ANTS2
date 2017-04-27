#include "bs3fit.h"
#include "bspline3.h"
#include <Eigen/Dense>
#include "eiquadprog.hpp"
#include <TProfile.h>
#include <iostream>

//#include <vector>

using Eigen::MatrixXd;
using Eigen::VectorXd;
using Eigen::JacobiSVD;

BS3fit::BS3fit(Bspline3 *bs_)
{
    bs = bs_;
    nbas = bs->GetNbas();
// no binning by default
    nbins = 0;
// no constraints by default
    non_negative = false;
    non_increasing = false;
    even = false;
    h1 = 0;
}

BS3fit::~BS3fit()
{
    if (h1)
        delete h1;
}

void BS3fit::SetBinning(int bins)
{
    if (nbins > 0) {
        std::cout << "BS3fit: attempt to redefine binning\n";
        return;
    }
// we need at least nbas bins to get a unique solution
    nbins = bins < nbas ? nbas : bins;
// profile histogram for storing the binned data
    h1 = new TProfile("bs3aux", "", nbins, bs->GetXmin(), bs->GetXmax());
}

void BS3fit::SetBinningAuto()
{
    // number of intervals can't be less then number of basis functions in the spline
    // we set it (somewhat arbitrarily) to 4 times number of intervals in the spline
    SetBinning(bs->GetNint()*4);
}

void BS3fit::AddData(int npts, double const *datax, double const *datay)
{
    if (nbins == 0)
        SetBinningAuto();

    for (int i=0; i<npts; i++)
        h1->Fill(datax[i], datay[i]);
}


// this is the default fitting routine to be performed on the binned data
bool BS3fit::Fit()
{
    if (!h1) {
        std::cout << "BS3fit: No data to fit. Call AddData(npts, datax, datay) first\n";
        return false;
    }
// extract the accumulated binned data from the profile histogram
    for (int ix=1; ix<=nbins; ix++) {
        int nent = h1->GetBinEntries(ix); // number of entries in the bin ix
//        if ( nent < 1)
//            continue;
        vx.push_back(h1->GetXaxis()->GetBinCenter(ix));
        vy.push_back(h1->GetBinContent(ix));
        vw.push_back(nent);
//        rms->push_back(h1->GetBinError(ix) * sqrt((double)nent));
    }

    return Fit(vw.size(), &vx[0], &vy[0], &vw[0]);
}

bool BS3fit::Fit(int npts, double const *datax, double const *datay, double const *dataw)
{
// linear least squares
// we've got npts equations with nbas unknowns
    MatrixXd A(npts, nbas);
    VectorXd y(npts);
    VectorXd x;

// fill the equations
    if (dataw == 0) {   // for not weighted data
        for (int i=0; i<npts; i++) {
            y(i) = datay[i];
            for (int k=0; k<nbas; k++)
                A(i, k) = bs->Basis(datax[i], k);
        }
    } else {            // for weighted data
        for (int i=0; i<npts; i++) {
            if (dataw[i] > 0.5) { // normal point (W >= 1)
                y(i) = datay[i] * dataw[i];
                for (int k=0; k<nbas; k++)
                    A(i, k) = bs->Basis(datax[i], k) * dataw[i];
            } else {            // missing point (W == 0) - make it smooth by setting 2nd derivative to zero
                y(i) = 0.;
                for (int k=0; k<nbas; k++)
                    A(i, k) = bs->BasisDrv2(datax[i], k);
            }
        }
    }

    if (!non_negative && !non_increasing && !even) { // unconstrained fit
    // solve the system using SVD
        JacobiSVD <MatrixXd> svd(A, Eigen::ComputeThinU | Eigen::ComputeThinV);
          //std::cout << "Singular values: " << svd.singularValues();
        x = svd.solve(y);

        VectorXd r = A*x - y;
        residual = sqrt(r.squaredNorm());

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

        if (non_negative || non_increasing) {
            CI = MatrixXd::Zero(nbas, nbas);
            ci0 = VectorXd::Zero(nbas);
        }
        if (non_negative) {
            for (int i = 0; i<nbas; i++)
                CI(i, i) = 1.;
        }
        if (non_increasing) {
            for (int i = 0; i<nbas-1; i++) {
                CI(i, i) = 1.;
                CI(i+1, i) = -1.;
            }
        }

        if (even) {
    // force the function even by constraining the derivative to 0 at x=0
            CE = MatrixXd::Zero(nbas, 1);
            ce0 = VectorXd::Zero(1);
            for (int k=0; k<nbas; k++)
                CE(k, 0) = bs->BasisDrv(0., k);
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

