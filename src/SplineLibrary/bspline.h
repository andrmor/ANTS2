/*
 * bspline3.h
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

#ifndef BSPLINE_H
#define BSPLINE_H

class Bspline {
	public:
		Bspline(double xmin, double xmax, int nbasis) { xl = xmin; xr = xmax; nbas = nbasis; }
		void GetRange(double *xmin, double *xmax) {*xmin = xl;	*xmax = xr;}
		double GetXmin() {return xl;}
		double GetXmax() {return xr;}
		virtual int GetNint() = 0;
		virtual int GetNbas() {return nbas;}
		virtual double Basis(double x, int n) = 0;
		double Basis(double *x, double *p); // insertable into a ROOT function
		virtual double BasisDrv(double x, int n) = 0;
		double BasisDrv(double *x, double *p); // insertable into a ROOT function
        virtual double Eval(double x) = 0;
		double Eval(double *x, double *p);	// insertable into a ROOT function
        virtual double EvalDrv(double x) = 0;
		double EvalDrv(double *x, double *p);	// insertable into a ROOT function
		virtual void SetCoef(double *c) = 0;
		virtual void GetCoef(double *c) = 0;
		virtual double GetCoef(int i) = 0;

	protected:
		double xl;
		double xr;
		int nbas;	// number of basis splines		
};

#endif /* BSPLINE_H */ 
