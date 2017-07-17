/*
 * tpspline3m.h
 * 
 * Copyright 2013 Vova <vova@lexx>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */

#ifndef TPSPLINE3M_H
#define TPSPLINE3M_H

#include "bspline3.h"
#include <Eigen/Dense>
// the following is needed to ensure proper alignment of std::vector <Matrix4d>
#include <Eigen/StdVector>

using Eigen::MatrixXd;
using Eigen::Matrix4d;
using Eigen::Vector4d;

class TPspline3
{
	public:
        TPspline3(double xmin, double xmax, int n_intx, double ymin, double ymax, int n_inty);
				void SetRangeX(double xmin, double xmax);
		void SetRangeY(double ymin, double ymax);
		void GetRangeX(double *xmin, double *xmax) {*xmin = xl;	*xmax = xr;}
        void GetRangeY(double *ymin, double *ymax) {*ymin = yl;	*ymax = yr;}
        double GetXmin() const {return xl;}
        double GetXmax() const {return xr;}
        double GetYmin() const {return yl;}
        double GetYmax() const {return yr;}
        int GetNintX() const {return nintx;}
        int GetNintY() const {return ninty;}
        int GetNbas() {return nbas;}
        bool isInvalid() const { return nintx==0 || ninty==0 || C.rows() != nintx+3 || C.cols() != ninty+3; }
		double Basis(double x, double y, int n);
        double BasisDrvX(double x, double y, int n);
        double BasisDrvY(double x, double y, int n);
        double BasisDrv2XX(double x, double y, int n);
        double BasisDrv2YY(double x, double y, int n);
        double BasisDrv2XY(double x, double y, int n);
		double Basis(double *x, double *p);  // insertable into ROOT function
		double Eval_slow(double x, double y);
        double Eval(double x, double y) const;
        double Eval_greedy(double x, double y) const;
        double EvalDrvX(double x, double y);
        double EvalDrvY(double x, double y);
		double Eval(double *x, double *p);	// insertable into ROOT function
        double EvalDrvX(double *x, double *p);	// insertable into ROOT function
        double EvalDrvY(double *x, double *p);	// insertable into ROOT function
		void SetCoef(double *c);
		void GetCoef(double *c);
        double GetCoef(int i) const {return C(i%nbasx, i/nbasx);}
        void SetCoef(std::vector <double> c);
        const std::vector <double> &GetCoef() const;
        const Bspline3 &GetBSX() const {return bsx;}
        const Bspline3 &GetBSY() const {return bsy;}
        void SetWrapY(bool w) {wrapy = w;}
        bool GetWrapY() {return wrapy;}
        int GetWrapped(int n);
    private:
        Vector4d PowerVec(double x) const;
        Vector4d PowerVecDrv(double x) const;
        bool Locate(double x, double y, int *ix, int *iy, double *xf, double *yf) const;

	private:
		Bspline3 bsx, bsy;
// this version uses Eigen marices to store spline and polynomial coefficients
// the matrices are organized as follows: X - rows, Y - columns
        MatrixXd C;                 // matrix of coefficients, corresponds to old "coef"
        std::vector <Matrix4d, Eigen::aligned_allocator<Matrix4d> > P; // vector of matrices with polynomial coefficients (old "poly")
//        std::vector <Matrix4d> P;   // vector of matrices with polynomial coefficients (old "poly")
        Matrix4d B;                 // matrix of UCBS coefficients
        mutable std::vector <double> coef;  // needed for backward compatibility
//		double *coef;
//		double **poly;
        double xl, yl;
        double xr, yr;
		double dx, dy;
		int nintx, ninty;
		int nrec;	// number of rectangles
		int nbas;	// number of basis splines
		int nbasx;  // columns in spline grid
        bool wrapy;
    public:
        EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

#endif /* TPSPLINE3M_H */
