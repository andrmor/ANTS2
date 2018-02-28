/*
 * tpspline3.cpp
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

#include "tpspline3.h"

TPspline3::TPspline3(double xmin, double xmax, int n_intx, double ymin, double ymax, int n_inty) :
                    bsx(xmin, xmax, n_intx), bsy(ymin, ymax, n_inty),
                    xl(xmin), yl(ymin),
                    xr(xmax), yr(ymax),
                    nintx(n_intx), ninty(n_inty)
{
	dx = xr-xl; dy = yr-yl;
	nrec = n_intx*n_inty;
	nbasx = n_intx + 3;
	nbas = (n_intx + 3)*(n_inty + 3);
	for (int i=0; i<nrec; i++)
		poly.push_back(Poly());
	wrapy = false;
}

void TPspline3::SetRangeX(double xmin, double xmax)
{
	xl = xmin;
	xr = xmax;
	dx = xr-xl;
	bsx.SetRange(xl, xr);
}

void TPspline3::SetRangeY(double ymin, double ymax)
{
	yl = ymin;
	yr = ymax;
	dy = yr-yl;
	bsy.SetRange(yl, yr);
}

// this function calculates equivalent basis function number
// for the case of wrapped around Y
int TPspline3::GetWrapped(int n)
{
    if (!wrapy)
        return n;

    int nx = n%nbasx;
    int ny = n/nbasx;

    ny = ny%ninty; // wrap y around
    return ny*nbasx+nx;
}

double TPspline3::Basis(double x, double y, int n)
{
	int nx = n%nbasx;
	int ny = n/nbasx;
	
	return bsx.Basis(x, nx)*bsy.Basis(y, ny);
}

double TPspline3::BasisDrvX(double x, double y, int n)
{
    int nx = n%nbasx;
    int ny = n/nbasx;

    return bsx.BasisDrv(x, nx)*bsy.Basis(y, ny);
}

double TPspline3::BasisDrvY(double x, double y, int n)
{
    int nx = n%nbasx;
    int ny = n/nbasx;

    return bsx.Basis(x, nx)*bsy.BasisDrv(y, ny);
}

double TPspline3::BasisDrv2XX(double x, double y, int n)
{
    int nx = n%nbasx;
    int ny = n/nbasx;

    return bsx.BasisDrv2(x, nx)*bsy.Basis(y, ny);
}

double TPspline3::BasisDrv2YY(double x, double y, int n)
{
    int nx = n%nbasx;
    int ny = n/nbasx;

    return bsx.Basis(x, nx)*bsy.BasisDrv2(y, ny);
}

double TPspline3::BasisDrv2XY(double x, double y, int n)
{
    int nx = n%nbasx;
    int ny = n/nbasx;

    return bsx.BasisDrv(x, nx)*bsy.BasisDrv(y, ny);
}

double TPspline3::Basis(double *x, double *p)  // insertable into ROOT function
{
    return Basis(x[0], x[1], (int)p[0]);
}

double TPspline3::Eval_slow(double x, double y)
{
	double sum = 0;
	for (int i=0; i<nbas; i++)
		sum += coef[i]*Basis(x, y, i);
	return sum;	
}

double TPspline3::Eval(double x, double y) const
{
//			assert(x>=xl && x<=xr);		// range check
//			assert(y>=yl && y<=yr);		// range check

	if (x<xl || x>xr) {
//		printf("x = %f\n", x);
		return 0.;
	}
	if (x==xr)
		x -= 1.0e-7;
		
	if (y<yl || y>yr) {
//		printf("y = %f\n", y);
		return 0.;
	}
	if (y==yr)
		y -= 1.0e-7;	
					
	double xi = (x-xl)/dx*nintx;
	int ix = (int)xi;
    if(ix >= nintx) ix = nintx - 1; //Added by raimundo. Had a very specific case where iy = ninty and made ants crash.
	double xf = xi-ix;
	
	double yi = (y-yl)/dy*ninty;
	int iy = (int)yi;
    if(iy >= ninty) iy = ninty - 1; //Added by raimundo. Had a very specific case where iy = ninty and made ants crash.
	double yf = yi-iy;
	
	const double *pi = poly[ix + iy*nintx].p;

	double sum = pi[0] + xf*(pi[1] + xf*(pi[2] + xf*pi[3]));
	sum += yf*(pi[4] + xf*(pi[5] + xf*(pi[6] + xf*pi[7])));
	sum += yf*yf*(pi[8] + xf*(pi[9] + xf*(pi[10] + xf*pi[11])));
	return sum + yf*yf*yf*(pi[12] + xf*(pi[13] + xf*(pi[14] + xf*pi[15])));
}

// this eval tries to get best speed with reduced shared memory usage
double TPspline3::Eval_greedy(double x, double y)
{
    if (x<xl || x>xr) return 0.;
    if (x==xr) x -= 1.0e-7;
    if (y<yl || y>yr) return 0.;
    if (y==yr) y -= 1.0e-7;

    double xi = (x-xl)/dx*nintx;
    int ix = (int)xi;
    double xf = xi-ix;
    double xff = 1. - xf;

    double yi = (y-yl)/dy*ninty;
    int iy = (int)yi;
    double yf = yi-iy;
    double yff = 1. - yf;

    double xx[4], yy[4];

    xx[0] = xff*xff*xff;
    xx[1] = xf*xf*(xf + xf + xf - 6.) + 4.;
    xx[2] = xf*(xf*(-xf - xf - xf + 3.) + 3.) + 1.;
    xx[3] = xf*xf*xf;

    yy[0] = yff*yff*yff;
    yy[1] = yf*yf*(yf + yf + yf - 6.) + 4.;
    yy[2] = yf*(yf*(-yf - yf - yf + 3.) + 3.) + 1.;
    yy[3] = yf*yf*yf;

    // It probably doesn't make much sense to unroll the loops here
    // the gain would be marginal (and the complier could be doing
    // it anyway)
    double sum = 0.;
    int k = iy*nbasx+ix; // current 2D base function
    for (int jy=0; jy<4; jy++) {
        for (int jx=0; jx<4; jx++)
            sum += coef[k++] * xx[jx] * yy[jy];
        k += nbasx-4;
    }

    return sum/36.;
}

double TPspline3::EvalDrvX(double x, double y)
{
//			assert(x>=xl && x<=xr);		// range check
//			assert(y>=yl && y<=yr);		// range check

    if (x<xl || x>xr) {
//		printf("x = %f\n", x);
        return 0.;
    }
    if (x==xr)
        x -= 1.0e-7;

    if (y<yl || y>yr) {
//		printf("y = %f\n", y);
        return 0.;
    }
    if (y==yr)
        y -= 1.0e-7;

    double xi = (x-xl)/dx*nintx;
    int ix = (int)xi;
    double xf = xi-ix;

    double yi = (y-yl)/dy*ninty;
    int iy = (int)yi;
    double yf = yi-iy;

    const double *pi = poly[ix + iy*nintx].p;

    double sum = pi[1] + xf*(2.*pi[2] + xf*3.*pi[3]);
    sum += yf*(pi[5] + xf*(2.*pi[6] + xf*3.*pi[7]));
    sum += yf*yf*(pi[9] + xf*(2.*pi[10] + xf*3.*pi[11]));
    return sum + yf*yf*yf*(pi[13] + xf*(2.*pi[14] + xf*3.*pi[15]));
}

double TPspline3::EvalDrvY(double x, double y)
{
//			assert(x>=xl && x<=xr);		// range check
//			assert(y>=yl && y<=yr);		// range check

    if (x<xl || x>xr) {
//		printf("x = %f\n", x);
        return 0.;
    }
    if (x==xr)
        x -= 1.0e-7;

    if (y<yl || y>yr) {
//		printf("y = %f\n", y);
        return 0.;
    }
    if (y==yr)
        y -= 1.0e-7;

    double xi = (x-xl)/dx*nintx;
    int ix = (int)xi;
    double xf = xi-ix;

    double yi = (y-yl)/dy*ninty;
    int iy = (int)yi;
    double yf = yi-iy;

    const double *pi = poly[ix + iy*nintx].p;

    double sum = pi[4] + xf*(pi[5] + xf*(pi[6] + xf*pi[7]));
    sum += 2.*yf*(pi[8] + xf*(pi[9] + xf*(pi[10] + xf*pi[11])));
    return sum + 3.*yf*yf*(pi[12] + xf*(pi[13] + xf*(pi[14] + xf*pi[15])));
}

double TPspline3::Eval(double *x, double* /*p*/)	// insertable into ROOT function
{
	return Eval(x[0], x[1]);
}

double TPspline3::EvalDrvX(double *x, double* /*p*/)	// insertable into ROOT function
{
    return EvalDrvX(x[0], x[1]);
}

double TPspline3::EvalDrvY(double *x, double* /*p*/)	// insertable into ROOT function
{
    return EvalDrvY(x[0], x[1]);
}

void TPspline3::SetCoef(double *c)
{
	for (int i=0; i<nbas; i++)
        coef.push_back(c[i]);
		
/*				
	for (int i=0; i<nint; i++)
		for (int j=0; j<4; j++) {
			poly[i][j]=0.;
			for (int k=0; k<4; k++)
				poly[i][j] += c[i+k]*BSM[j][k]/6.;
	}
*/	
	for (int iy=0; iy<ninty; iy++) 
	  for (int ix=0; ix<nintx; ix++)
	    for (int jy=0; jy<4; jy++)
		  for (int jx=0; jx<4; jx++) {
			poly[ix+iy*nintx][jx+jy*4] = 0.;
			for (int kx=0; kx<4; kx++)
			  for (int ky=0; ky<4; ky++)
				poly[ix+iy*nintx][jx+jy*4] += 
					c[ix+kx+(iy+ky)*nbasx]*BSM[jx][kx]*BSM[jy][ky]/36.;
	}		

/*
	for (int i=0; i<nrec; i++)
	  for (int j=0; j<16; j++) {
		poly[i][j] = 0.;
		for (int kx=0; kx<4; kx++)
		  for (int ky=0; ky<4; ky++)
			poly[i][j] += c[i%nintx+kx+(i/nintx+ky)*nbasx]*BSM[j%4][kx]*BSM[j/4][ky]/36.;
	}
*/				
}

void TPspline3::SetCoef(std::vector <double> c)
{
    if ((int)c.size() == nbas)
        SetCoef(&c[0]);
}

void TPspline3::GetCoef(double *c)
{
	for (int i=0; i<nbas; i++)
		c[i] = coef[i];
}

const std::vector<double> &TPspline3::GetCoef() const
{
    return coef;
}


