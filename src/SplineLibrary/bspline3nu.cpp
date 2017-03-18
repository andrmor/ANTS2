/*
 * bspline3nu.cpp
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

#ifndef WIN32

#include "bspline3nu.h"
#include <string.h>


Bspline3nu::Bspline3nu(const double * const breakpts, const int nbreak)
{
	xl = breakpts[0];
	xr = breakpts[nbreak-1];
	
	nbas = nbreak + K - 2;
	coeff = gsl_vector_alloc(nbas);
	Bk = gsl_vector_alloc(K);
	bw = gsl_bspline_alloc(K, nbreak);
	knots = bw->knots;
	dw = gsl_bspline_deriv_alloc(K);
	gsl_vector_const_view breakpts_v = gsl_vector_const_view_array(breakpts, nbreak);	
	gsl_bspline_knots(&breakpts_v.vector, bw);
	
	const double delta_knotl = breakpts[1]-breakpts[0];
	const double delta_knotr = breakpts[nbreak-1]-breakpts[nbreak-2];
	for(int i = 0; i < ORDER; i++)
	{
		gsl_vector_set(knots, i, breakpts[0]-(ORDER-i)*delta_knotl);
		gsl_vector_set(knots, nbreak+ORDER+i, breakpts[nbreak-1]+(i+1)*delta_knotr);
	}

	knotsd = new double[knots->size];
	inv_delta_knots = new double[knots->size];
	for(int i = 0; i < knots->size-1; i++)
	{
		inv_delta_knots[i] = 1./(gsl_vector_get(knots, i+1) - gsl_vector_get(knots, i));
		knotsd[i] = gsl_vector_get(knots, i);
	}
	knotsd[knots->size-1] = gsl_vector_get(knots, knots->size-1);

	polysize = nbreak-1;
	poly = new double[polysize][K];
}

Bspline3nu::~Bspline3nu()
{
	gsl_bspline_free(bw);
	gsl_bspline_deriv_free(dw);
	gsl_vector_free(coeff);
	gsl_vector_free(Bk);
	free(inv_delta_knots);
	free(poly);
}

double Bspline3nu::Basis_slow(double x, int n)
{
	return Basis(x, n);
}

double Bspline3nu::Basis(double x, int n)
{
#if 0
	const double * const t = &knotsd[n];
	const double * const idt = &inv_delta_knots[n];
	if(n < 0 || n >= nbas || x > t[4] || x < t[0])
		return 0;
	const double dx[] = { x-t[0], x-t[1], x-t[2], x-t[3], x-t[4] };
	const double dt11 = 1./(t[3]-t[1]);
	const double dx4odt42 = dx[4]/(t[4]-t[2]);
	const double order4[] = { dx[0]/(t[3]-t[0]), dx[4]/(t[4]-t[1]) };
	const double order3[] = { dx[0]/(t[2]-t[0]), dx[3]*dt11 };
	if(dx[3] > 0)
		return -order4[1]*dx[4]*dx4odt42*idt[3];
	if(dx[2] > 0)
		return (order4[0]*dx[3]*order3[1] + order4[1]*(dx[1]*order3[1] + dx[2]*dx4odt42))*idt[2];
	if(dx[1] > 0)
		return -(order4[0]*(order3[0]*dx[2] + order3[1]*dx[1]) + order4[1]*dx[1]*dx[1]*dt11)*idt[1];
	if(dx[0] > 0)
		return order4[0]*order3[0]*dx[0]*idt[0];
	return 0;
#endif
	size_t istart, iend;
	if (x<xl || x>xr) {
		return 0.;
	}
	gsl_bspline_eval_nonzero(x, Bk, &istart, &iend, bw);
	if(n < istart || n > iend)
		return 0.;
	return gsl_vector_get(Bk, n-istart);
}

double Bspline3nu::Basis(double *x, double *p)  // insertable into ROOT function
{
	return Basis(x[0], p[0]);
}

double Bspline3nu::BasisDrv(double x, int n)
{
	size_t istart, iend;
	if (x<xl || x>xr) {
		return 0.;
	}
	gsl_matrix *dB = gsl_matrix_alloc(K, 2);
	gsl_bspline_deriv_eval_nonzero(x, 1, dB, &istart, &iend, bw, dw);
	if(n < istart || n > iend)
		return 0.;
	return gsl_matrix_get(dB, n-istart, 1);
}

double Bspline3nu::BasisDrv(double *x, double *p)  // insertable into ROOT function
{
	return BasisDrv(x[0], p[0]);
}

double Bspline3nu::Eval(double x)
{
	if(x < xl || x > xr)
		return 0.;
	int i, imin = ORDER+1;
	int imax = nbas; //+ORDER for &breakpts[0] = &knots[ORDER], -ORDER for nbreak-1 = nbas-K+2-1 = nbas-(K-1) = nbas-ORDER
	do {
		i = (imax + imin)>>1;
		if(x < knotsd[i])
			imax = i-1;
		else
			imin = i+1;
	} while(imin < imax);
	//i = (x < knotsd[imin]) ? imin-1 : imin;
	//double xf = (x-gsl_vector_get(knots, i))*inv_delta_knots[i];
	double *pi = poly[(x < knotsd[imin]) ? imin-1-ORDER : imin-ORDER];

	return pi[0] + x*(pi[1] + x*(pi[2] + x*pi[3]));
}

double Bspline3nu::Eval(double *x, double *p)	// insertable into ROOT function
{
	return Eval(x[0]);
}

double Bspline3nu::EvalDrv(double x)
{
	if(x < xl || x > xr)
		return 0.;
	int i, imin = ORDER+1;
	int imax = nbas; //+ORDER for &breakpts[0] = &knots[ORDER], -ORDER for nbreak-1 = nbas-K+2-1 = nbas-(K-1) = nbas-ORDER
	do {
		i = (imax + imin)>>1;
		if(x < gsl_vector_get(knots, i))
			imax = i-1;
		else
			imin = i+1;
	} while(imin < imax);
	i = (x < gsl_vector_get(knots, imin)) ? imin-1 : imin;
	//double xf = (x-gsl_vector_get(knots, i))*inv_delta_knots[i];
	double *pi = poly[i-ORDER];

	return pi[1] + x*(2.*pi[2] + x*3.*pi[3]);
}

double Bspline3nu::EvalDrv(double *x, double *p)	// insertable into ROOT function
{
	return EvalDrv(x[0]);
}

void Bspline3nu::SetCoef(double *c)
{
	gsl_vector_view coef = gsl_vector_view_array(c, nbas);
	SetCoef(&coef.vector);
}

void Bspline3nu::SetCoef(gsl_vector *c)
{
	size_t istart, iend;
	const double xr = 0.5;
	gsl_matrix *dB = gsl_matrix_alloc(K, K); //K non-zero elements, up to third derivative (0th, 1st, 2nd, 3rd), which is also K
	double inv_fact[K], derivs[K];
	inv_fact[0] = 1;
	for(int i = 1; i < K; i++)
		inv_fact[i] = inv_fact[i-1] / i;

	gsl_vector_memcpy(coeff, c);

	//gsl_vector_set_basis(c, K);

	for(int i = 0; i < polysize; i++)
	{
		const double x1 = gsl_vector_get(knots, i+ORDER+1);
		const double x0 = gsl_vector_get(knots, i+ORDER);
		const double x = (x1+x0)*0.5;
		gsl_bspline_deriv_eval_nonzero(x, ORDER, dB, &istart, &iend, bw, dw);
		for(int j = ORDER; j >= 0; j--)
		{
			double cur_deriv = 0;
			for(int k = istart; k <=iend; k++)
				cur_deriv += gsl_vector_get(c, k) * gsl_matrix_get(dB, k-istart, j);

			double other_parcels = 0;
			for(int k = ORDER; k > j; k--) {
				other_parcels = (other_parcels + derivs[k]*inv_fact[k-j]) * x;
			}

			derivs[j] = cur_deriv - other_parcels;
			poly[i][j] = derivs[j] * inv_fact[j];
		}
	}
}

void Bspline3nu::GetCoef(double *c)
{
	for (int i=0; i<nbas; i++)
		c[i] = gsl_vector_get(coeff, i);
}

double Bspline3nu::Fit(int npts, double *datax, double *datay)
{
	double chisq;
	size_t istart, iend;
	gsl_multifit_linear_workspace *mw = gsl_multifit_linear_alloc(npts, nbas);
	gsl_matrix *cov = gsl_matrix_alloc(nbas, nbas);

	gsl_matrix *X = gsl_matrix_calloc(npts, nbas);
	for(int i = 0; i < npts; i++)
	{
		gsl_bspline_eval_nonzero(datax[i], Bk, &istart, &iend, bw);
		for(int j = istart; j <= iend; j++)
			gsl_matrix_set(X, i, j, gsl_vector_get(Bk, j-istart));
	}

	gsl_vector_const_view y_v= gsl_vector_const_view_array(datay, npts);
	gsl_multifit_linear(X, &y_v.vector, coeff, cov, &chisq, mw);

	gsl_multifit_linear_free(mw);
	gsl_matrix_free(X);
	gsl_matrix_free(cov);

	SetCoef(coeff);

	return chisq;
}

#endif
