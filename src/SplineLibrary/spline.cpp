/*
 * spline.cpp
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

#include "spline.h"
#include <string.h>
#include <QVector>
#include <QtAlgorithms>
#include "jsonparser.h"
#include <QDebug>

// TODO:
// - Consider encapsulating this fuctionality in classes derived from BSpline and TPSpline

void write_bspline3_json(const Bspline1d *bs, QJsonObject &json)
{
    int nint = bs->GetNint();
    QJsonArray data;

    json["xmin"] = bs->GetXmin();
    json["xmax"] = bs->GetXmax();
    json["intervals"] = nint;
    for (int i=0; i<nint+3; i++)
        data.append(bs->GetCoef(i));
    json["data"] = data;
}

#include <stdio.h>    // Andr ! moved to cpp
//#include <iostream>
#include <fstream>
//#include <sstream>
void write_bspline3_json(const Bspline1d *bs, std::ofstream *out)
{
    *out << "{" << std::endl;
    *out << "\"xmin\": " << bs->GetXmin() << "," << std::endl;
    *out << "\"xmax\": " << bs->GetXmax() << "," << std::endl;
    *out << "\"intervals\": " << bs->GetNint() << "," << std::endl;
    *out << "\"data\": [" << std::endl;
    int nint = bs->GetNint();
    for (int i=0; i<nint+3; i++)
        *out << bs->GetCoef(i) << ", ";
    *out << std::endl;
    *out << "]" << std::endl;
    *out << "}" << std::endl;
}

void write_tpspline3_json(const Bspline2d *ts, QJsonObject &json)
{
    int nintx = ts->GetNintX();
    int ninty = ts->GetNintY();
    QJsonArray datax, datay;

    json["xmin"] = ts->GetXmin();
    json["xmax"] = ts->GetXmax();
    json["ymin"] = ts->GetYmin();
    json["ymax"] = ts->GetYmax();
    json["xintervals"] = nintx;
    json["yintervals"] = ninty;

    for (int i=0; i<ninty+3; i++) {
        for (int j=0; j<nintx+3; j++)
            datax.append(ts->GetCoef(i*(nintx+3)+j));
        datay.append(datax);
        for (int j=0; j<nintx+3; j++)   // couldn't find a shorter way
            datax.removeLast();         // to clear a QJsonArray
    }
    json["data"] = datay;
}

Bspline1d FromJson::mkBspline3(QJsonObject &json)
{
  double xmin = json["xmin"].toDouble();
  double xmax = json["xmax"].toDouble();
  int n_int = json["intervals"].toInt();

  std::vector<double> coef;
  for(auto c : json["data"].toArray())
    coef.push_back(c.toDouble());
  if((int)coef.size()<n_int+3)
    return Bspline1d(0, 0, 0);

  Bspline1d bs(xmin, xmax, n_int);
  bs.SetCoef(coef);
  return bs;
}


Bspline2d FromJson::mkTPspline3(QJsonObject &json)
{
    JsonParser parser(json);
    double xmin, xmax, ymin, ymax;
    int nintx, ninty;
    QJsonArray data;
    QVector <double> coef, onerow;
    QVector <QJsonArray> rows;

    parser.ParseObject("xmin", xmin);
    parser.ParseObject("xmax", xmax);
    parser.ParseObject("ymin", ymin);
    parser.ParseObject("ymax", ymax);
    parser.ParseObject("xintervals", nintx);
    parser.ParseObject("yintervals", ninty);
    if (!parser.ParseObject("data", data))
        return Bspline2d(0, 0, 0, 0, 0, 0);
    if (!parser.ParseArray(data, rows))
        return Bspline2d(0, 0, 0, 0, 0, 0);
    if (rows.size()<ninty+3)
        return Bspline2d(0, 0, 0, 0, 0, 0);
    for (int i=0; i<ninty+3; i++)
    {
        onerow.clear();
        parser.ParseArray(rows[i], onerow);
        if (onerow.size()<nintx+3)
            return Bspline2d(0, 0, 0, 0, 0, 0);
        coef += onerow;
    }
    if (parser.GetError())
        return Bspline2d(0, 0, 0, 0, 0, 0);

    Bspline2d ts(xmin, xmax, nintx, ymin, ymax, ninty);
    std::vector <double> c(coef.size(), 0.);
    for (int i=0; i<coef.size(); i++)
        c[i] = coef[i];
    ts.SetCoef(c);
    return ts;
}

Bspline1d *read_bspline3_json(QJsonObject &json)
{
    JsonParser parser(json);
    double xmin, xmax;
    int n_int;
    QJsonArray data;
    QVector<double> coef;

    parser.ParseObject("xmin", xmin);
    parser.ParseObject("xmax", xmax);
    parser.ParseObject("intervals", n_int);
    parser.ParseObject("data", data);
    parser.ParseArray(data, coef);
    if (parser.GetError() || coef.size()<n_int+3)
        return NULL;

    Bspline1d *bs = new Bspline1d(xmin, xmax, n_int);
    std::vector <double> c(coef.size(), 0.);
    for (int i=0; i<coef.size(); i++)
        c[i] = coef[i];
    bs->SetCoef(c);
    return bs;
}

Bspline2d *read_tpspline3_json(QJsonObject &json)
{
    JsonParser parser(json);
    double xmin, xmax, ymin, ymax;
    int nintx, ninty;
    QJsonArray data;
    QVector <double> coef, onerow;
    QVector <QJsonArray> rows;

    parser.ParseObject("xmin", xmin);
    parser.ParseObject("xmax", xmax);
    parser.ParseObject("ymin", ymin);
    parser.ParseObject("ymax", ymax);
    parser.ParseObject("xintervals", nintx);
    parser.ParseObject("yintervals", ninty);
    if (!parser.ParseObject("data", data))
        return NULL;
    if (!parser.ParseArray(data, rows))
        return NULL;
    if (rows.size()<ninty+3) {
        qDebug() << rows.size() << " rows";
        return NULL;
    }
    for (int i=0; i<ninty+3; i++) {
        onerow.clear();
        parser.ParseArray(rows[i], onerow);
        if (onerow.size()<nintx+3)
            return NULL;
        coef += onerow;
    }
    if (parser.GetError())
        return NULL;

    Bspline2d *ts = new Bspline2d(xmin, xmax, nintx, ymin, ymax, ninty);
    std::vector <double> c(coef.size(), 0.);
    for (int i=0; i<coef.size(); i++)
        c[i] = coef[i];
    ts->SetCoef(c);

    return ts;
}
