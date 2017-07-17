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

#ifndef TPSPLINE3D_H
#define TPSPLINE3D_H

#include "bspline3.h"
#include "tpspline3m.h"
#include <Eigen/Dense>
using Eigen::MatrixXd;
using Eigen::Matrix4d;
using Eigen::Vector4d;

class TPspline3D
{
	public:
        TPspline3D(double xmin, double xmax, int n_intx, double ymin, double ymax, int n_inty,
                   double zmin, double zmax, int n_intz);

        void GetRangeX(double *xmin, double *xmax) {*xmin = xl;	*xmax = xr;}
        void GetRangeY(double *ymin, double *ymax) {*ymin = yl;	*ymax = yr;}
        void GetRangeZ(double *zmin, double *zmax) {*zmin = zl;	*zmax = zr;}
        double GetXmin() const {return xl;}
        double GetXmax() const {return xr;}
        double GetYmin() const {return yl;}
        double GetYmax() const {return yr;}
        double GetZmin() const {return zl;}
        double GetZmax() const {return zr;}
        int GetNintX() const {return nintx;}
        int GetNintY() const {return ninty;}
        int GetNintZ() const {return nintz;}
        int GetNbas() {return nbas;}
        int GetNbasXY() {return nbasxy;}
        int GetNbasZ() {return nbasz;}
// FIXME:
//        bool isInvalid() const { return nintx==0 || ninty==0 || C.rows() != nintx+3 || C.cols() != ninty+3; }
        double Basis(double x, double y, double z, int n);
        double BasisDrvX(double x, double y, double z, int n);
        double BasisDrvY(double x, double y, double z, int n);
        double BasisDrvZ(double x, double y, double z, int n);
        double BasisDrv2XX(double x, double y, double z, int n);
        double BasisDrv2YY(double x, double y, double z, int n);
        double BasisDrv2ZZ(double x, double y, double z, int n);
        double BasisDrv2XY(double x, double y, double z, int n);
        double BasisDrv2XZ(double x, double y, double z, int n);
        double BasisDrv2YZ(double x, double y, double z, int n);
        double Eval(double x, double y, double z) const;
        double Eval_greedy(double x, double y, double z) const;
        double EvalDrvX(double x, double y, double z);
        double EvalDrvY(double x, double y, double z);
        double EvalDrvZ(double x, double y, double z);
        void SetCoef(int iz, double *c);
        void SetZplane(int iz, TPspline3 *tps);
//        void AddZplane(TPspline3 *tps) {Zplane.push_back(tps);}
// Hope to manage these through 2D class
//		void GetCoef(double *c);
//        double GetCoef(int i) const {return C(i%nbasx, i/nbasx);}
//        void SetCoef(std::vector <double> c);
//        const std::vector <double> &GetCoef() const;
        const TPspline3 &GetBSXY() const {return bsxy;}
        const Bspline3 &GetBSZ() const {return bsz;}
        const TPspline3 *GetZplane(int iz) const {return Zplane[iz];}
    private:
        Vector4d PowerVec(double x) const;
        Vector4d PowerVecDrv(double x) const;
        bool LocateZ(double z, int *iz, double *zf) const;

	private:
        TPspline3 bsxy;
        Bspline3 bsz;
        std::vector <TPspline3*> Zplane; // using 2D splines to do part of the housekeeping
        Matrix4d B;                 // matrix of UCBS coefficients

        double xl, yl, zl;
        double xr, yr, zr;
        double dx, dy, dz;
        int nintx, ninty, nintz;
        int nboxes;	// number of elementary boxes (formed by intersections of the interval grids on 3 axes)
        int nbas;	// total number of 3D basis splines
        int nbasxy;  // number of 2D basis splines in one XY plane
        int nbasz;
    public:
        EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

#endif /* TPSPLINE3D_H */
