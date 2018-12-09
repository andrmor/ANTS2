/*
 * spline.h
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


#ifndef SPLINE_H
#define SPLINE_H

//#define USE_PUGIXML

#include "bspline123d.h"

#ifdef TPS3M
#include "bspline123d.h"
#else
#include "bspline123d.h"
#endif

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <QJsonObject>

struct APoint;

void write_bspline3_json(const Bspline1d *bs, QJsonObject &json);
namespace FromJson {
  Bspline1d mkBspline3(QJsonObject &json);
  Bspline2d mkTPspline3(QJsonObject &json);
}
Bspline1d *read_bspline3_json(QJsonObject &json);
void write_tpspline3_json(const Bspline2d *bs, QJsonObject &json);
Bspline2d *read_tpspline3_json(QJsonObject &json);

#ifdef CRAP
double fit_bspline3(Bspline1d *bs, int npts, const double *datax, const double *datay, bool even = false);
double fit_bspline3(Bspline1d *bs, int npts, const double *datax, const double *datay, const double *weight, bool even);
double fit_bspline3_grid(Bspline1d *bs, int npts, const double *datax, const double *datay, bool even = false, bool takeroot = false);
double fit_tpspline3(Bspline2d *bs, int npts, const double *datax, const double *datay, const double *dataz, const double *w = NULL, bool even = false);
double fit_tpspline3_grid(Bspline2d *bs, int npts, const double *datax, const double *datay, const double *dataz, bool even = false);
double fit_tpspline3_grid(Bspline2d *bs, int npts, const APoint *data, const double *dataz, bool even = false);
#endif

#endif /* SPLINE_H */ 
