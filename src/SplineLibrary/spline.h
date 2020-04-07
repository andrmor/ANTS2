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

#include "bspline123d.h"

//#include <stdio.h>    // Andr ! moved to cpp
//#include <iostream>
//#include <fstream>
//#include <sstream>
#include <QJsonObject>

//struct APoint;  // And ! commented away

void write_bspline3_json(const Bspline1d *bs, QJsonObject &json);
namespace FromJson {
  Bspline1d mkBspline3(QJsonObject &json);
  Bspline2d mkTPspline3(QJsonObject &json);
}
Bspline1d *read_bspline3_json(QJsonObject &json);
void write_tpspline3_json(const Bspline2d *bs, QJsonObject &json);
Bspline2d *read_tpspline3_json(QJsonObject &json);

#endif /* SPLINE_H */ 
