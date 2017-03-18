/*
 * tpspline3.h
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

#ifndef TPSPLINE3_H
#define TPSPLINE3_H

#include "bspline3.h"

class TPspline3
{
	struct Poly { double p[16]; double &operator[](int i) { return p[i]; } };
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
        bool isInvalid() const { return (nintx==0 && ninty==0) || coef.size() != (unsigned)nbas; }  //could be one dimensional XY splines?
		double Basis(double x, double y, int n);
        double BasisDrvX(double x, double y, int n);
        double BasisDrvY(double x, double y, int n);
        double BasisDrv2XX(double x, double y, int n);
        double BasisDrv2YY(double x, double y, int n);
        double BasisDrv2XY(double x, double y, int n);
		double Basis(double *x, double *p);  // insertable into ROOT function
		double Eval_slow(double x, double y);
        double Eval(double x, double y) const;
        double Eval_greedy(double x, double y);
        double EvalDrvX(double x, double y);
        double EvalDrvY(double x, double y);
		double Eval(double *x, double *p);	// insertable into ROOT function
        double EvalDrvX(double *x, double *p);	// insertable into ROOT function
        double EvalDrvY(double *x, double *p);	// insertable into ROOT function
		void SetCoef(double *c);
		void GetCoef(double *c);
		double GetCoef(int i) const {return coef[i];}
        void SetCoef(std::vector <double> c);
        const std::vector <double> &GetCoef() const;
        const Bspline3 &GetBSX() const {return bsx;}
        const Bspline3 &GetBSY() const {return bsy;}
        void SetWrapY(bool w) {wrapy = w;}
        bool GetWrapY() {return wrapy;}
        int GetWrapped(int n);
			
	private:
		Bspline3 bsx, bsy;
        std::vector <double> coef;
        std::vector <Poly> poly;
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
};

#endif /* TPSPLINE3_H */ 
