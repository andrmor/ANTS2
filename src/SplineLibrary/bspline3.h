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

#ifndef BSPLINE3_H
#define BSPLINE3_H

#include <vector>

/*	here is an example how to do draw basis functions it in CINT:
	Bspline3 *bs = new Bspline3(0., 10., 10);  // create the user function class
	TF1 * f = new TF1("f", bs, &Bspline3::Basis, 0. ,10. , 1 , "Bspline3", "Basis");
	// now we want to plot 6-th basis function
	f->SetParameter(0, 6);	
	f->Draw();
*/

// coefficients for a uniform 3rd degree B-spline
// (divide by 6 to get right normalization)
//const double BSM[4][4] = {1., 4., 1., 0.,
//                        -3., 0., 3., 0.,
//                        3., -6., 3., 0.,
//                        -1., 3., -3., 1.};

// (GCC tries to be more Catholic than the Pope here
// and wants inner braces not required by the standard)

const double BSM[4][4] = {{1., 4., 1., 0.},
                          {-3., 0., 3., 0.},
                          {3., -6., 3., 0.},
                          {-1., 3., -3., 1.}};

class Bspline3 {
//For now it's faster than float! Probably because of conversions to/from double
public: typedef double value_type;
private: struct Poly { value_type p[4]; };
public:
	Bspline3();
	Bspline3(double xmin, double xmax, int n_int);
	bool isInvalid() const { return nint == 0; }
	void SetRange(double xmin, double xmax) {xl = xmin; xr = xmax; dx = xr-xl;}
	void GetRange(double *xmin, double *xmax) {*xmin = xl;	*xmax = xr;}
	double GetXmin() const {return xl;}
	double GetXmax() const {return xr;}
	int GetNint() const {return nint;}
	int GetNbas() const {return nbas;}
	// get coeff. at x^pow for interval i
	value_type BasisPol(int i, int pow) {return BSM[pow][i]/6.;}
	value_type Basis(value_type x, int n);
	double Basis(double *x, double *p); // insertable into a ROOT function
	value_type BasisDrv(value_type x, int n) const;
    value_type BasisDrv2(value_type x, int n) const;
	double BasisDrv(double *x, double *p); // insertable into a ROOT function
	value_type Eval_slow(value_type x);
	value_type Eval_greedy(value_type x);
	value_type Eval_x_phobic(value_type x);
	value_type Eval(value_type x) const;
	double Eval(double *x, double *p);	// insertable into a ROOT function
	Bspline3::value_type EvalDrv(value_type x);
	double EvalDrv(double *x, double *p);	// insertable into a ROOT function
	void SetCoef(value_type *c);
	void GetCoef(value_type *c);
	void SetCoef(std::vector<value_type> c);
	std::vector<Bspline3::value_type> GetCoef() const;
	Bspline3::value_type GetCoef(int i) const {return coef[i];}
	std::vector <std::vector <value_type> > GetPoly() const;

private:
  std::vector <value_type> coef;
  std::vector <Poly> poly;
  value_type xl;
  value_type xr;
  value_type dx;
  int nint;	// number of intervals
  int nbas;	// number of basis splines
};

#endif /* BSPLINE3_H */ 
