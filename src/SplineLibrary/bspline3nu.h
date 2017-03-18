/*
 * bspline3nu.h
 * 
 * Copyright 2013 Raimundo Martins <raimundoomartins@gmail.com>
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

#ifndef BSPLINE3NU_H
#define BSPLINE3NU_H

#define GSL_RANGE_CHECK_OFF
#define HAVE_INLINE
#include <gsl/gsl_vector.h>
#include <gsl/gsl_bspline.h>
#include <gsl/gsl_multifit.h>
#undef HAVE_INLINE
#undef GSL_RANGE_CHECK_OFF

#define K 4
#define ORDER ((K)-1)
//#define CLASSNAME Bspline#ORDER


class Bspline3nu {
	public:
		//double *breakpts includes x_min and x_max aka xl and xr
		Bspline3nu(const double *breakpts, int nbreak);
		~Bspline3nu();
	  //void SetRange(double xmin, double xmax) {xl = xmin; xr = xmax; dx = xr-xl;} makes no sense here
		void GetRange(double *xmin, double *xmax) {*xmin = xl;	*xmax = xr;}
		double GetXmin() {return xl;}
		double GetXmax() {return xr;}
		int GetNbas() {return nbas;}
		// get coeff. at x^pow for interval i		
		double Basis_slow(double x, int n);
		double Basis(double x, int n);
		double Basis(double *x, double *p); // insertable into a ROOT function
		double BasisDrv(double x, int n);		
		double BasisDrv(double *x, double *p); // insertable into a ROOT function
		double Eval(double x);
		double Eval(double *x, double *p);	// insertable into a ROOT function
		double EvalDrv(double x);
		double EvalDrv(double *x, double *p);	// insertable into a ROOT function
		void SetCoef(double *c);
		void SetCoef(gsl_vector *c);
		void GetCoef(double *c);
		double GetCoef(int i) {return gsl_vector_get(coeff, i);}

		double Fit(int npts, double *datax, double *datay);

	private:
		gsl_bspline_workspace *bw;
		gsl_bspline_deriv_workspace *dw;
		gsl_vector *Bk;
		gsl_vector *coeff;

		gsl_vector *knots;
		double *knotsd;
		double *inv_delta_knots;

		size_t polysize;
		double (*poly)[K];

		double xl;
		double xr;
		int nbas;	// number of basis splines
};

#endif /* BSPLINE3NU_H */ 
