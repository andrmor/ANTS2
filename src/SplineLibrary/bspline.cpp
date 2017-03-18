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

#include "bspline.h"

double Bspline::Basis(double *x, double *p)  // insertable into ROOT function
{
    return Basis(x[0], (int)p[0]);
}
		
double Bspline::BasisDrv(double *x, double *p)  // insertable into ROOT function
{
    return BasisDrv(x[0], (int)p[0]);
}

double Bspline::Eval(double *x, double* /*p*/)	// insertable into ROOT function
{
	return Eval(x[0]);
}

double Bspline::EvalDrv(double *x, double* /*p*/)	// insertable into ROOT function
{
    return EvalDrv(x[0]);
}


