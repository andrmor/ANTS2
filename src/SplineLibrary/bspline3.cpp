/*
 * bspline3.cpp
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

#include "bspline3.h"

Bspline3::Bspline3() : xl(0), xr(0), dx(0), nint(0), nbas(0) { }

Bspline3::Bspline3(double xmin, double xmax, int n_int) : xl(xmin), xr(xmax), nint(n_int)
{
	dx = xr-xl;
	nbas = n_int + 3;
	coef.resize(nbas);
	poly.resize(nint);
}

Bspline3::value_type Bspline3::Basis(value_type x, int n)
{
	double xi = (x-xl)/dx*nint;
	double xi_rel = xi - n + 3.; // xi - (n - 3.) -- position from beginning of spline #n 
	
	if (xi_rel < 0.)
		return 0.;
	int xr_int = (int)xi_rel;
	double xf = xi_rel - xr_int;
	
	switch (xr_int) {
		case 0:
			return xf*xf*xf/6.;
		case 1:
			return (xf*(xf*(-3.*xf + 3.) + 3.) + 1.)/6.;
		case 2:
			return (xf*xf*(3.*xf - 6.) + 4.)/6.;
		case 3: {
			double xff = 1. - xf;
			return xff*xff*xff/6.;
		}
		default:
			return 0.;
	}
}
		
double Bspline3::Basis(double *x, double *p)  // insertable into ROOT function
{
	return Basis(x[0], (int)p[0]);
}
		
Bspline3::value_type Bspline3::BasisDrv(value_type x, int n) const
{
	double xi = (x-xl)/dx*nint;
	double xi_rel = xi - n + 3.; // xi - (n - 3.) -- position from beginning of spline #n 
	
	if (xi_rel < 0.)
		return 0.;
	int xr_int = (int)xi_rel;
	double xf = xi_rel - xr_int;
	
	switch (xr_int) {
		case 0:
			return xf*xf/2.;
		case 1:
			return (xf*(-3.*xf + 2.) + 1.)/2.;
		case 2:
			return (xf*(3.*xf - 4.))/2.;
		case 3: {
			double xff = 1. - xf;
			return -xff*xff/2.;
		}
		default:
			return 0.;
		}
}

double Bspline3::BasisDrv(double *x, double *p)  // insertable into ROOT function
{
	return BasisDrv(x[0], (int)p[0]);
}

Bspline3::value_type Bspline3::BasisDrv2(value_type x, int n) const
{
    double xi = (x-xl)/dx*nint;
    double xi_rel = xi - n + 3.; // xi - (n - 3.) -- position from beginning of spline #n

    if (xi_rel < 0.)
        return 0.;
    int xr_int = (int)xi_rel;
    double xf = xi_rel - xr_int;

    switch (xr_int) {
        case 0:
            return xf;
        case 1:
            return -3.*xf + 1.;
        case 2:
            return 3.*xf - 2.;
        case 3: {
            return 1. - xf;
        }
        default:
            return 0.;
        }
}

Bspline3::value_type Bspline3::Eval_slow(value_type x)
{
	double sum = 0.;
	for (int i=0; i<nbas; i++)
		sum += coef[i]*Basis(x, i);
	return sum;	
}

// this eval tries to get best speed with reduced shared memory usage
Bspline3::value_type Bspline3::Eval_greedy(value_type x)
{
    if (x<xl || x>xr || x != x) { // range check
        return 0.;
    }
    if (x==xr)
        x -= 1.0e-7;
    double xi = (x-xl)/dx*nint;
    int ix = (int)xi;
    double xf = xi-ix;
    double xff = 1. - xf;

    double sum = coef[ix]*xff*xff*xff;
    sum += coef[ix+1]*(xf*xf*(xf + xf + xf - 6.) + 4.);
    sum += coef[ix+2]*(xf*(xf*(-xf - xf - xf + 3.) + 3.) + 1.);
    sum += coef[ix+3]*xf*xf*xf;

    return sum/6.;
}

// this eval tries to squeeze even more speed by
// reducing the number of multiply operations
Bspline3::value_type Bspline3::Eval_x_phobic(value_type x)
{
    if (x<xl || x>xr || x != x) { // range check
        return 0.;
    }
    if (x==xr)
        x -= 1.0e-7;
    double xi = (x-xl)/dx*nint;
    int ix = (int)xi;

    double xf;
    if (ix<(int)poly.size())
      xf = xi - ix;
    else {
      ix--;
      xf = 1.0;
    }

    double a0, a1, a2;
    double p0, p1, p2, p3;

    a0 = coef[ix];
    a1 = coef[ix+1];
    double c2 = coef[ix+2];
    a2 = a0 + a1 + c2;  // c0 + c1 + c2
    a1 = a1 + a1 + a1;  // 3*c1
    p0 = a2 + a1;       // c0 + 4*c1 + c2
    p2 = a2 - a1;       // c0 - 2*c1 + c2
    p2 = p2 + p2 + p2;  // 3*c0 - 6*c1 + 3*c2
    p1 = c2 - a0;       // c2 - c0
    p1 = p1 + p1 + p1;  // 3*c2 - 3*c0
    p3 = a0 + a0 - a1 - p2 + coef[ix+3]; // -c0 + 3*c1 - 3*c2 + c3

    return (p0 + xf*(p1 + xf*(p2 + xf*p3)))/6.;
}

Bspline3::value_type Bspline3::Eval(value_type x) const
{
//		assert(x>=xl && x<=xr);		// range check
  if (x<xl || x>xr || x != x) {
//		printf("x = %f\n", x);
		return 0.;
	}
	double xi = (x-xl)/dx*nint;
	int i = (int)xi;

	double xf;
	if (i<(int)poly.size())
		xf = xi - i;
	else {
		i--;
		xf = 1.0;
	}

	const value_type *pi = poly[i].p;

	return pi[0] + xf*(pi[1] + xf*(pi[2] + xf*pi[3]));
}

double Bspline3::Eval(double *x, double* /*p*/)	// insertable into ROOT function
{
	return Eval(x[0]);
}

Bspline3::value_type Bspline3::EvalDrv(value_type x)
{
//		assert(x>=xl && x<=xr);		// range check
    if (x<xl || x>xr || x != x) {
//		printf("x = %f\n", x);
        return 0.;
    }
    double xi = (x-xl)/dx*nint;
    int i = (int)xi;

    double xf;
    if (i<(int)poly.size())
      xf = xi - i;
    else {
      i--;
      xf = 1.0;
    }

    const value_type *pi = poly[i].p;

    return pi[1] + xf*(2.*pi[2] + xf*3.*pi[3]);
}

double Bspline3::EvalDrv(double *x, double* /*p*/)	// insertable into ROOT function
{
    return EvalDrv(x[0]);
}

void Bspline3::SetCoef(value_type *c)
{
	for (int i=0; i<nbas; i++)
		coef[i] = c[i];
	for (int i=0; i<nint; i++) {
		poly[i].p[0] = (c[i] + 4.*c[i+1] + c[i+2])/6.;
		poly[i].p[1] = (-3.*c[i] + 3.*c[i+2])/6.;
		poly[i].p[2] = (3.*c[i] - 6.*c[i+1] + 3.*c[i+2])/6.;
		poly[i].p[3] = (-c[i] + 3.*c[i+1] - 3.*c[i+2] + c[i+3])/6.;
	}
/*			
	for (int i=0; i<nint; i++)
		for (int j=0; j<4; j++) {
			poly[i][j] = 0.;
			for (int k=0; k<4; k++)
				poly[i][j] += c[i+k]*BSM[j][k]/6.;
	}
*/
}

void Bspline3::SetCoef(std::vector<value_type> c)
{
    if ((int)c.size() == nbas)
        SetCoef(&c[0]);
}

void Bspline3::GetCoef(value_type *c)
{
	for (int i=0; i<nbas; i++)
		c[i] = coef[i];
}

std::vector<Bspline3::value_type> Bspline3::GetCoef() const
{
    return coef;
}

std::vector<std::vector<Bspline3::value_type> > Bspline3::GetPoly() const
{
    std::vector<value_type> row;
    std::vector<std::vector <value_type> > p;
    for (int i=0; i<nint; i++) {
        row.clear();
        for (int j=0; j<4; j++)
            row.push_back(poly[i].p[j]);
        p.push_back(row);
    }
    return p;
}


