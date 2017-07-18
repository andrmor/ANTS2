/*
 * TPspline3M.cpp
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

#include "tpspline3m.h"

TPspline3::TPspline3(double xmin, double xmax, int n_intx, double ymin, double ymax, int n_inty) :
                    xl(xmin), yl(ymin), xr(xmax), yr(ymax), nintx(n_intx), ninty(n_inty),
                    bsx(xmin, xmax, n_intx), bsy(ymin, ymax, n_inty)
{
	dx = xr-xl; dy = yr-yl;
	nrec = n_intx*n_inty;
	nbasx = n_intx + 3;
	nbas = (n_intx + 3)*(n_inty + 3);
    C = MatrixXd::Zero(n_intx + 3, n_inty + 3);
    P.resize(nrec);
    coef.resize(nbas);
	wrapy = false;
// construct UCBS matrix
    B <<    1, -3,  3, -1,
            4,  0, -6,  3,
            1,  3,  3, -3,
            0,  0,  0,  1;
    B /= 6.;
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

// for the following basis functions
// nx = n%nbasx; ny = n/nbasx;
double TPspline3::Basis(double x, double y, int n)
{
    return bsx.Basis(x, n%nbasx)*bsy.Basis(y, n/nbasx);
}

double TPspline3::BasisDrvX(double x, double y, int n)
{
    return bsx.BasisDrv(x, n%nbasx)*bsy.Basis(y, n/nbasx);
}

double TPspline3::BasisDrvY(double x, double y, int n)
{
    return bsx.Basis(x, n%nbasx)*bsy.BasisDrv(y, n/nbasx);
}

double TPspline3::BasisDrv2XX(double x, double y, int n)
{
    return bsx.BasisDrv2(x, n%nbasx)*bsy.Basis(y, n/nbasx);
}

double TPspline3::BasisDrv2YY(double x, double y, int n)
{
    return bsx.Basis(x, n%nbasx)*bsy.BasisDrv2(y, n/nbasx);
}

double TPspline3::BasisDrv2XY(double x, double y, int n)
{
    return bsx.BasisDrv(x, n%nbasx)*bsy.BasisDrv(y, n/nbasx);
}

double TPspline3::Basis(double *x, double *p)  // insertable into ROOT function
{
    return Basis(x[0], x[1], (int)p[0]);
}

double TPspline3::Eval_slow(double x, double y)
{
	double sum = 0;
	for (int i=0; i<nbas; i++)
        sum += C(i%nbasx, i/nbasx)*Basis(x, y, i);
	return sum;	
}

Vector4d TPspline3::PowerVec(double x) const
{
    Vector4d X;
    double x2 = x*x;
    double x3 = x2*x;
    X << 1., x, x2, x3;
    return X;
}

Vector4d TPspline3::PowerVecDrv(double x) const
{
    Vector4d X;
    X << 0., 1., x+x, 3*x*x;
    return X;
}

// translate xy coordinates into rectangle position (ix, iy) and
// position inside the rectangle (xf, yf)
// returns true if inside the domain
bool TPspline3::Locate(double x, double y, int *ix, int *iy, double *xf, double *yf) const
{
    double xi = (x-xl)/dx*nintx; *ix = (int)xi;
    if (*ix < 0 || *ix >= nintx)
        return false;
    double yi = (y-yl)/dy*ninty; *iy = (int)yi;
    if (*iy < 0 || *iy >= nintx)
        return false;

    *xf = xi - *ix;
    *yf = yi - *iy;
    return true;
}

double TPspline3::Eval(double x, double y) const
{
    int ix, iy;
    double xf, yf;
    bool inside = Locate(x, y, &ix, &iy, &xf, &yf);
    if (!inside)
        return 0.;

    return PowerVec(xf).transpose()*P[ix + iy*nintx]*PowerVec(yf);
}

// this eval tries to get best speed with reduced shared memory usage
double TPspline3::Eval_greedy(double x, double y) const
{
    int ix, iy;
    double xf, yf;
    bool inside = Locate(x, y, &ix, &iy, &xf, &yf);
    if (!inside)
        return 0.;

    Vector4d xx = B*PowerVec(xf);
    Vector4d yy = B*PowerVec(yf);

    return xx.transpose()*C.block<4,4>(ix,iy)*yy;
}

double TPspline3::EvalDrvX(double x, double y)
{
    int ix, iy;
    double xf, yf;
    bool inside = Locate(x, y, &ix, &iy, &xf, &yf);
    if (!inside)
        return 0.;

    return PowerVecDrv(xf).transpose()*P[ix + iy*nintx]*PowerVec(yf);
}

double TPspline3::EvalDrvY(double x, double y)
{
    int ix, iy;
    double xf, yf;
    bool inside = Locate(x, y, &ix, &iy, &xf, &yf);
    if (!inside)
        return 0.;

    return PowerVec(xf).transpose()*P[ix + iy*nintx]*PowerVecDrv(yf);
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
        C(i%nbasx, i/nbasx) = c[i];

    for (int iy=0; iy<ninty; iy++)
      for (int ix=0; ix<nintx; ix++)
          P[ix + iy*nintx] = B.transpose()*C.block<4,4>(ix,iy)*B;
}

void TPspline3::SetCoef(std::vector <double> c)
{
    if ((int)c.size() == nbas)
        SetCoef(&c[0]);
}

void TPspline3::GetCoef(double *c)
{
	for (int i=0; i<nbas; i++)
        c[i] = C(i%nbasx, i/nbasx);
}

const std::vector<double> &TPspline3::GetCoef() const
{
    for (int i=0; i<nbas; i++)
        coef[i] = C(i%nbasx, i/nbasx);
    return coef;
}


