#include "tps3dfit.h"
#include "tpspline3d.h"
#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <Eigen/SparseQR>
#include <Eigen/OrderingMethods>
#include "eiquadprog.hpp"
#include <TProfile3D.h>
#include <iostream>

#include <QDebug>
#include <QElapsedTimer>

//#include <vector>

using Eigen::MatrixXd;
using Eigen::VectorXd;
using Eigen::JacobiSVD;
using Eigen::Triplet;
using Eigen::SparseQR;
using Eigen::SparseMatrix;

TPS3Dfit::TPS3Dfit(TPspline3D *bs_)
{
    bs = bs_;
    nbas = bs->GetNbas();
    nbasxy = bs->GetNbasXY();
    nbasz = bs->GetNbasZ();
// no binning by default
    nbinsx = 0;
    nbinsy = 0;
    nbinsz = 0;
// no constraints by default
    non_negative = false;
    h1 = 0;
}

TPS3Dfit::~TPS3Dfit()
{
    if (h1)
        delete h1;
}

void TPS3Dfit::SetBinning(int binsx, int binsy, int binsz)
{
    if (nbinsx > 0) {
        std::cout << "TPS3Dfit: attempt to redefine binning\n";
        return;
    }
// we need at least nbas bins to get a unique solution
// in 2D case, we assign them according to the number of intervals in each direction
    nbinsx = binsx < bs->GetNintX()+3 ? bs->GetNintX()+3 : binsx;
    nbinsy = binsy < bs->GetNintY()+3 ? bs->GetNintY()+3 : binsy;
    nbinsz = binsz < bs->GetNintZ()+3 ? bs->GetNintZ()+3 : binsz;
// 2D profile histogram for storing the binned data
    h1 = new TProfile3D("bs33Daux", "", nbinsx, bs->GetXmin(), bs->GetXmax(),
                                    nbinsy, bs->GetYmin(), bs->GetYmax(),
                                    nbinsz, bs->GetZmin(), bs->GetZmax());
}

void TPS3Dfit::SetBinningAuto()
{
    // number of bins can't be less then number of basis functions in the spline
    // we set it (somewhat arbitrarily) to 2 times number of intervals in each direction
    // this gives 8 bins per elementary box (safe down to 3x3x3 3D splines)
    SetBinning(bs->GetNintX()*2, bs->GetNintY()*2, bs->GetNintZ()*2);
}

void TPS3Dfit::AddData(int npts, double const *x, double const *y, const double *z, const double *data)
{
    if (nbinsx == 0)
        SetBinningAuto();

    for (int i=0; i<npts; i++)
        h1->Fill(x[i], y[i], z[i], data[i]);
}


// this is the default fitting routine to be performed on the binned data
bool TPS3Dfit::Fit()
{
    if (!h1) {
        std::cout << "TPS3Dfit: No data to fit. Call AddData(npts, datax, datay, dataz) first\n";
        return false;
    }
// extract the accumulated binned data from the profile histogram
    for (int ix=1; ix<=nbinsx; ix++)
      for (int iy=1; iy<=nbinsy; iy++)
       for (int iz=1; iz<=nbinsz; iz++){
        int nent = h1->GetBinEntries(h1->GetBin(ix, iy, iz)); // number of entries in the bin (ix,iy,iz)
//        if ( nent < 1)
//            continue;
        vx.push_back(h1->GetXaxis()->GetBinCenter(ix));
        vy.push_back(h1->GetYaxis()->GetBinCenter(iy));
        vz.push_back(h1->GetZaxis()->GetBinCenter(iz));
        vdata.push_back(h1->GetBinContent(ix, iy, iz));
        vw.push_back(nent);
//        rms->push_back(h1->GetBinError(ix) * sqrt((double)nent));
    }

    return Fit(vw.size(), &vx[0], &vy[0], &vz[0], &vdata[0], &vw[0]);
}

bool TPS3Dfit::Fit(int npts, double const *datax, double const *datay, double const *dataz, double const *data, double const *dataw)
{
    QElapsedTimer myTimer;
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
    MatrixXd A(npts+missing*5, nbas);
    VectorXd y(npts+missing*5);
    VectorXd x;

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

    if (!non_negative) { // unconstrained fit
    // solve the system using QR decomposition (SVD is too slow)
        sparse = true;
        if (!sparse) {
            myTimer.start();
//            MatrixXd A1 = A.transpose()*A;
//            VectorXd y1 = A.transpose()*y;
//            qDebug() << "Multiplication: " << myTimer.elapsed();
            x = A.colPivHouseholderQr().solve(y);
            qDebug() << "Compute QR & solve: " << myTimer.elapsed();
            VectorXd r = A*x - y;
            residual = sqrt(r.squaredNorm());
        } else { // sparse QR -- EXPERIMENTAL
            int nz_elem = 0;
            for (int i=0; i<A.rows(); i++)
                for (int j=0; j<A.cols(); j++)
                    if (A(i,j) != 0.)
                        nz_elem++;

            qDebug() << "A.rows(): " << A.rows();
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
            SparseMatrix <double> A1_sp = A_sp.transpose()*A_sp;
            VectorXd y1 = A.transpose()*y;
            qDebug() << "Multiply: " << myTimer.elapsed();
            SparseQR <SparseMatrix<double>, Eigen::COLAMDOrdering<int> > solver;
            solver.compute(A1_sp);
            if(solver.info()!=Eigen::Success) {
              std::cout << "decomposition failed\n";
              return false;
            }
            qDebug() << "Compute QR: " << myTimer.elapsed();
            x = solver.solve(y1);
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

        if (non_negative) {
            CI = MatrixXd::Zero(nbas, nbas);
            ci0 = VectorXd::Zero(nbas);
            std::cout << "Non-negative\n";
            for (int i = 0; i<nbas; i++)
                CI(i, i) = 1.;
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
    double *c = new double[nbasxy];
    for (int iz=0; iz<nbasz; iz++) {
        for (int i=0; i<nbasxy; i++)
            c[i] = x(iz*nbasxy + i);
        bs->SetCoef(iz, c);
    }
    delete[] c;

    return true;
}
