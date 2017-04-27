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
#include <math.h>
#include <TVectorD.h>
#include <TMatrixD.h>
#include <TDecompSVD.h>
#include <TDecompQRH.h>
#include <TProfile.h>
#include <TProfile2D.h>
#include <QVector>
#include <QtAlgorithms>
#include "jsonparser.h"
#include <QDebug>

//#define USE_EIGEN
#ifdef USE_EIGEN
#include <Eigen/Dense>
#endif

#define SECOND_DRV
//#define USE_QP

#ifdef USE_QP
#include "eiquadprog.hpp"
#endif

#include "apoint.h"

// TODO:
// - Consider encapsulating this fuctionality in classes derived from BSpline and TPSpline

void write_bspline3_json(const Bspline3 *bs, QJsonObject &json)
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

void write_bspline3_json(const Bspline3 *bs, std::ofstream *out)
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

void write_tpspline3_json(const TPspline3 *ts, QJsonObject &json)
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

Bspline3 FromJson::mkBspline3(QJsonObject &json)
{
  double xmin = json["xmin"].toDouble();
  double xmax = json["xmax"].toDouble();
  int n_int = json["intervals"].toInt();

  std::vector<Bspline3::value_type> coef;
  for(auto c : json["data"].toArray())
    coef.push_back(c.toDouble());
  if((int)coef.size()<n_int+3)
    return Bspline3(0, 0, 0);

  Bspline3 bs(xmin, xmax, n_int);
  bs.SetCoef(coef.data());
  return bs;
}


TPspline3 FromJson::mkTPspline3(QJsonObject &json)
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
        return TPspline3(0, 0, 0, 0, 0, 0);
    if (!parser.ParseArray(data, rows))
        return TPspline3(0, 0, 0, 0, 0, 0);
    if (rows.size()<ninty+3)
        return TPspline3(0, 0, 0, 0, 0, 0);
    for (int i=0; i<ninty+3; i++)
    {
        onerow.clear();
        parser.ParseArray(rows[i], onerow);
        if (onerow.size()<nintx+3)
            return TPspline3(0, 0, 0, 0, 0, 0);
        coef += onerow;
    }
    if (parser.GetError())
        return TPspline3(0, 0, 0, 0, 0, 0);

    TPspline3 ts(xmin, xmax, nintx, ymin, ymax, ninty);
    ts.SetCoef(coef.data());
    return ts;
}

Bspline3 *read_bspline3_json(QJsonObject &json)
{
    JsonParser parser(json);
    double xmin, xmax;
    int n_int;
    QJsonArray data;
    QVector<Bspline3::value_type> coef;

    parser.ParseObject("xmin", xmin);
    parser.ParseObject("xmax", xmax);
    parser.ParseObject("intervals", n_int);
    parser.ParseObject("data", data);
    parser.ParseArray(data, coef);
    if (parser.GetError() || coef.size()<n_int+3)
        return NULL;

    Bspline3 *bs = new Bspline3(xmin, xmax, n_int);
    bs->SetCoef(coef.data());
    return bs;
}

TPspline3 *read_tpspline3_json(QJsonObject &json)
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

    TPspline3 *ts = new TPspline3(xmin, xmax, nintx, ymin, ymax, ninty);
    ts->SetCoef(coef.data());

    return ts;
}

#ifdef USE_PUGIXML
void write_bspline3_xml(Bspline3 *bs, std::ofstream *out)
{
    *out << "<bspline3>" << std::endl;
    *out << "<xmin>" << bs->GetXmin() << "</xmin>" << std::endl;
    *out << "<xmax>" << bs->GetXmax() << "</xmax>" << std::endl;
    *out << "<intervals>" << bs->GetNint() << "</intervals>" << std::endl;
    *out << "<data>" << std::endl;
    int nint = bs->GetNint();
    for (int i=0; i<nint+3; i++)
        *out << bs->GetCoef(i) << " ";
    *out << std::endl;
    *out << "</data>" << std::endl;
    *out << "</bspline3>" << std::endl;
}

Bspline3 *read_bspline3_xml(pugi::xml_node node)
{
    double xmin = node.child("xmin").text().as_double();
    double xmax = node.child("xmax").text().as_double();
    int n_int = node.child("intervals").text().as_int();
    Bspline3 *bs = new Bspline3(xmin, xmax, n_int);
    std::string data(node.child("data").text().get());
    std::istringstream datastream(data);

    double *c = new double[n_int+3];
    for (int i=0; i<n_int+3; i++)
        datastream >> c[i];
    bs->SetCoef(c);
    delete[] c;

    return bs;
}

void write_tpspline3_xml(TPspline3 *ts, std::ofstream *out)
{
    *out << "<tpspline3>" << std::endl;
    *out << "<xmin>" << ts->GetXmin() << "</xmin>" << std::endl;
    *out << "<xmax>" << ts->GetXmax() << "</xmax>" << std::endl;
    *out << "<ymin>" << ts->GetYmin() << "</ymin>" << std::endl;
    *out << "<ymax>" << ts->GetYmax() << "</ymax>" << std::endl;
    *out << "<xintervals>" << ts->GetNintX() << "</xintervals>" << std::endl;
    *out << "<yintervals>" << ts->GetNintY() << "</yintervals>" << std::endl;

    int nintx = ts->GetNintX();
    int ninty = ts->GetNintY();
    *out << "<data>" << std::endl;
    for (int i=0; i<ninty+3; i++) {
        for (int j=0; j<nintx+3; j++)
            *out << ts->GetCoef(i*(nintx+3)+j) << " ";
        *out << std::endl;
    }
    *out << "</data>" << std::endl;
    *out << "</tpspline3>" << std::endl;
}

TPspline3 *read_tpspline3_xml(pugi::xml_node node)
{
    double xmin = node.child("xmin").text().as_double();
    double xmax = node.child("xmax").text().as_double();
    int nintx = node.child("xintervals").text().as_int();
    double ymin = node.child("ymin").text().as_double();
    double ymax = node.child("ymax").text().as_double();
    int ninty = node.child("yintervals").text().as_int();
    TPspline3 *ts = new TPspline3(xmin, xmax, nintx, ymin, ymax, ninty);
    std::string data(node.child("data").text().get());
    std::istringstream datastream(data);

    double *c = new double[(nintx+3)*(ninty+3)];
    for (int i=0; i<ninty+3; i++)
        for (int j=0; j<nintx+3; j++)
            datastream >> c[i*(nintx+3)+j];
    ts->SetCoef(c);
    delete[] c;

    return ts;
}
#endif

#ifdef USE_EIGEN
using Eigen::MatrixXd;
using Eigen::VectorXd;
using Eigen::JacobiSVD;
#ifndef USE_QP
double fit_bspline3(Bspline3 *bs, int npts, const double *datax, const double *datay, bool even)
{
	int nbas = bs->GetNbas(); 	// numbes of basis functions
	Bool_t success;
    int expand = even ? 2 : 0;
		
// linear least squares	
// we've got npts (+1 if forcing even) equations with nbas unknowns	
    MatrixXd A(npts+expand, nbas);
    VectorXd y(npts+expand);

// fill the equations	
	for (int i=0; i<npts; i++) {
		y(i) = datay[i];
		for (int k=0; k<nbas; k++)
			A(i, k) = bs->Basis(datax[i], k);
	}

    std::cout << "A0:" << std::endl;
    std::cout << A << std::endl;

    std::cout << "y0:" << std::endl;
    std::cout << y << std::endl;

// force the function even by setting 
// the derivative to 0 at x=0 in the last equation
	if (even) {
		double strength = ((double)npts)/nbas;
        double xmax = bs->GetXmax();
		y(npts) = 0.;
		for (int k=0; k<nbas; k++)
			A(npts, k) = bs->BasisDrv(0., k)*strength;
    // also constrain the tail
        y(npts+1) = 0.;
        for (int k=0; k<nbas; k++)
            A(npts+1, k) = bs->BasisDrv(xmax-0.0001, k)*strength;
	}
		
// solve the system
    JacobiSVD <MatrixXd> svd(A, Eigen::ComputeThinU | Eigen::ComputeThinV);
      //std::cout << "Singular values: " << svd.singularValues();
    VectorXd c_svd = svd.solve(y);
//	if (!success) {
//		printf("fit_bspline3: SVD failed\n");
//		return -1.;
//	}

    std::cout << "x0:" << std::endl;
    std::cout << c_svd << std::endl;

// fill bs from the solution vector
	double *c = new double[nbas];
	for (int i=0; i<nbas; i++)
		c[i] = c_svd(i);
	bs->SetCoef(c);
	delete[] c;
	
    VectorXd residual = A*c_svd - y;
    return sqrt(residual.squaredNorm());
}
#else
double fit_bspline3(Bspline3 *bs, int npts, const double *datax, const double *datay, bool even)
{
    int nbas = bs->GetNbas(); 	// numbes of basis functions
    Bool_t success;

// linear least squares
// we've got npts equations with nbas unknowns
    MatrixXd A(npts, nbas);
    VectorXd y(npts);

// fill the equations
    for (int i=0; i<npts; i++) {
        y(i) = datay[i];
        for (int k=0; k<nbas; k++)
            A(i, k) = bs->Basis(datax[i], k);
    }

// solve the system using quadratic programming, i.e.
// minimize 1/2 * x G x + g0 x subject to some inequality and equality constraints,
// where G = A'A and g0 = y'A (' denotes transposition)
    MatrixXd G = A.transpose()*A;  //
    VectorXd g0 = (y.transpose()*A)*(-1.);  //
    VectorXd x(nbas);

//  set ineqaulity constraints:
    MatrixXd CI = MatrixXd::Zero(nbas, nbas);
    VectorXd ci0 = VectorXd::Zero(nbas);
//  make all spline coefficients non-negative
    for (int i = 0; i<nbas; i++)
        CI(i, i) = 1.;
//  make them also non-increasing
    for (int i = 0; i<nbas-1; i++)
        CI(i+1, i) = -1.;

//  set equality constraints (unconstrained by default)
    MatrixXd CE;
    VectorXd ce0;

    if (even) {
// force the function even by constraining the derivative to 0 at x=0
        CE = MatrixXd::Zero(nbas, 1);
        ce0 = VectorXd::Zero(1);
        for (int k=0; k<nbas; k++)
            CE(k, 0) = bs->BasisDrv(0., k);
    }

/*
    std::cout << "A:" << std::endl;
    std::cout << A << std::endl;

    std::cout << "y:" << std::endl;
    std::cout << y << std::endl;

    std::cout << "G:" << std::endl;
    std::cout << G << std::endl;

    std::cout << "g0:" << std::endl;
    std::cout << g0 << std::endl;

    std::cout << "CE:" << std::endl;
    std::cout << CE << std::endl;

    std::cout << "CI:" << std::endl;
    std::cout << CI << std::endl;
*/

// all prepared - call the solver
    float res = solve_quadprog(G, g0,  CE, ce0,  CI, ci0, x);

// fill bs from the solution vector
    double *c = new double[nbas];
    for (int i=0; i<nbas; i++)
        c[i] = x(i);
    bs->SetCoef(c);
    delete[] c;

    return res;
}
#endif
#else

double fit_bspline3(Bspline3 *bs, int npts, const double *datax, const double *datay, const double *weight, bool even)
{
    int nbas = bs->GetNbas(); 	// number of basis functions
    Bool_t success;
    int expand = even ? 2 : 0;

// linear least squares
// we've got npts (+1 if forcing even) equations with nbas unknowns
    TMatrixD A(npts+expand, nbas);
    TVectorD y(npts+expand);

// fill the equations
    for (int i=0; i<npts; i++) {
        if (weight) {
            y(i) = datay[i]*weight[i];
            for (int k=0; k<nbas; k++)
                A(i, k) = bs->Basis(datax[i], k)*weight[i];
        } else {
            y(i) = datay[i];
            for (int k=0; k<nbas; k++)
                A(i, k) = bs->Basis(datax[i], k);
        }
    }

// force the function even by setting
// the derivative to 0 at x=0 in the last equation
    if (even) {
        double strength;
        if (weight) {
            double sum=0.;
            for (int i=0; i<npts; i++)
                sum += weight[i];
            strength = sum/nbas;
        } else {
            strength = ((double)npts)/nbas;
        }
        double xmax = bs->GetXmax();
        y(npts) = 0.;
        for (int k=0; k<nbas; k++)
            A(npts, k) = bs->BasisDrv(0., k)*strength;
    // also constrain the tail
        y(npts+1) = 0.;
        for (int k=0; k<nbas; k++)
            A(npts+1, k) = bs->BasisDrv(xmax-0.0001, k)*strength;
    }
/*
    for (int i=0; i<npts+expand; i++) {
        QDebug debug = qDebug();
        for (int j=0; j<nbas; j++) {
            debug << A(i,j) << ", ";
        }
    }
*/
// solve the system
    TDecompSVD svd(A);
    const TVectorD c_svd = svd.Solve(y, success);
    if (!success) {
        qDebug() << "fit_bspline3: SVD: Can't solve, the system is singular";
        return -1.;
    }

//    TVectorD sig = svd.GetSig();
//    qDebug() << "Singular values: " << sig(0) << " ... " << sig(nbas-1);

// fill bs from the solution vector
    Bspline3::value_type *c = new Bspline3::value_type[nbas];
    for (int i=0; i<nbas; i++)
        c[i] = c_svd(i);
    bs->SetCoef(c);
    delete[] c;

    TVectorD residual = A*c_svd - y;
    return sqrt(residual.Norm2Sqr());
}

// wrapper for compatibility
double fit_bspline3(Bspline3 *bs, int npts, const double *datax, const double *datay, bool even)
{
    return fit_bspline3(bs, npts, datax, datay, NULL, even);
}

#endif


int get_data_from_profile(TProfile *h, std::vector <double> *x, std::vector <double> *y,
                          std::vector <double> *w, std::vector <double> *rms)
{
    int nbins = h->GetNbinsX(); // number of bins

    for (int ix=1; ix<=nbins; ix++) {
        int nent = h->GetBinEntries(ix); // number of entries in the bin ix
        if ( nent < 1)
            continue;
        x->push_back(h->GetXaxis()->GetBinCenter(ix));
        y->push_back(h->GetBinContent(ix));
        w->push_back(nent);
        rms->push_back(h->GetBinError(ix) * sqrt((double)nent));
    }
    return w->size();
}

int get_data_from_profile2D(TProfile2D *h, std::vector <double> *x, std::vector <double> *y,
                            std::vector <double> *z, std::vector <double> *w, std::vector <double> *rms)
{
    int nbinsx = h->GetNbinsX(); // number of bins
    int nbinsy = h->GetNbinsY(); // number of bins
    int weight;

    for (int ix=1; ix<=nbinsx; ix++)
      for (int iy=1; iy<=nbinsy; iy++) {
        if ((weight = h->GetBinEntries(h->GetBin(ix,iy))) < -1)
            continue;
        x->push_back(h->GetXaxis()->GetBinCenter(ix));
        y->push_back(h->GetYaxis()->GetBinCenter(iy));
        z->push_back(h->GetBinContent(ix, iy));
        w->push_back((double)weight);
        rms->push_back(h->GetBinError(ix, iy) * sqrt((double)weight));
    }
    return w->size();
}

double fit_bspline3_grid(Bspline3 *bs, int npts, const double *datax, const double *datay, bool even, bool takeroot)
{
    // number of intervals can't be less then number of basis functions in the spline
    // we set it (somewhat arbitrarily) to 4 times number of intervals in the spline
    int nint = bs->GetNint()*4;

    //qDebug() << "Fit bslpine grid, takeroot = " << takeroot;

    TProfile *h1 = new TProfile("bs3aux", "", nint, bs->GetXmin(), bs->GetXmax());
    for (int i=0; i<npts; i++)
        h1->Fill(datax[i], datay[i]);

    std::vector <double> x, y, w, rms;
    int ngood = get_data_from_profile(h1, &x, &y, &w, &rms);
    delete h1;
    if (takeroot) {
        for (int i=0; i<(int)y.size(); i++){
            y[i] = sqrt(y[i]);
//            qDebug() << "Point " << i << ": " << x[i] << ", " << y[i];
        }
    }
    return fit_bspline3(bs, ngood, &x[0], &y[0], even);
}

#ifdef USE_EIGEN
#ifndef USE_QP
double fit_tpspline3(TPspline3 *bs, int npts, const double *datax, const double *datay, const double *dataz, const double *weight, bool even)
{
    int nbas = bs->GetNbas(); 	// numbes of basis functions
    int nbasx = bs->GetBSX().GetNbas();
    int nbasy = bs->GetBSY().GetNbas();
    Bool_t success;

    std::vector <double> vz;
    std::vector <double> vrow;
    std::vector <std::vector <double> > vmat;

// linear least squares

    // ========== fill the equations ===========

    for (int i=0; i<npts; i++) {
        int w = weight ? weight[i] : 1.;
        if (w>0.5) { // normal point
// f(x,y) = z
            vz.push_back(dataz[i]*w);
            for (int k=0; k<nbas; k++)
                vrow.push_back(bs->Basis(datax[i], datay[i], k)*w);
            vmat.push_back(vrow);
            vrow.clear();
        }
#ifdef SECOND_DRV
// Experimental !!! - VS
// forcing first derivatives to zero isn't really a good idea:
// it'll result in a distorted surface if we're on a slope
// let's try the same trick with second derivatives
        else { // empty point - force second derivatives to zero
// d2f/dx2 = 0
            vz.push_back(0.);
            for (int k=0; k<nbas; k++)
                vrow.push_back(bs->BasisDrv2XX(datax[i], datay[i], k));
            vmat.push_back(vrow);
            vrow.clear();
// d2f/dy2 = 0
            vz.push_back(0.);
            for (int k=0; k<nbas; k++)
                vrow.push_back(bs->BasisDrv2YY(datax[i], datay[i], k));
            vmat.push_back(vrow);
            vrow.clear();
// d2f/dxdy = 0
            vz.push_back(0.);
            for (int k=0; k<nbas; k++)
                vrow.push_back(bs->BasisDrv2XY(datax[i], datay[i], k));
            vmat.push_back(vrow);
            vrow.clear();
        }
#else
        else { // empty point - force second derivatives to zero
// df/dx = 0
            vz.push_back(0.);
            for (int k=0; k<nbas; k++)
                vrow.push_back(bs->BasisDrvX(datax[i], datay[i], k));
            vmat.push_back(vrow);
            vrow.clear();
// df/dy = 0
            vz.push_back(0.);
            for (int k=0; k<nbas; k++)
                vrow.push_back(bs->BasisDrvY(datax[i], datay[i], k));
            vmat.push_back(vrow);
            vrow.clear();
        }
#endif
    }
    // if required, force the function even by setting
    // the derivative d/dx to 0 along x=0 axis
    if (even) {
        double strength = ((double)npts)/nbas;
        for (int iy=0; iy<nbasy; iy++) {
            vz.push_back(0.);
            vrow.assign(nbas, 0.);
            for (int ix=0; ix<nbasx; ix++)
                vrow[iy*nbasx+ix] = bs->GetBSX().BasisDrv(0., ix)*strength;
            vmat.push_back(vrow);
            vrow.clear();
        }
    // also tell it NOT wiggle the tail by setting d/dx to 0 at x=xmax
    // (experimental feature)
        double xmax = bs->GetXmax();
        for (int iy=0; iy<nbasy; iy++) {
            vz.push_back(0.);
            vrow.assign(nbas, 0.);
            for (int ix=0; ix<nbasx; ix++)
                vrow[iy*nbasx+ix] = bs->GetBSX().BasisDrv(xmax-0.001, ix)*strength;
            vmat.push_back(vrow);
            vrow.clear();
        }
    }

// ========= solve the system using Eigen machinery =========

// we've got vz.size() equations with nbas unknowns
    MatrixXd A(vz.size(), nbas);
    VectorXd z(vz.size());

    for (int irow=0; irow<vz.size(); irow++) {
        z(irow) = vz[irow];
        for (int icol=0; icol<nbas; icol++)
            A(irow, icol) = vmat[irow][icol];
    }

//    JacobiSVD <MatrixXd> svd(A, Eigen::ComputeThinU | Eigen::ComputeThinV);
//    std::cout << "Singular values: " << svd.singularValues();
//    VectorXd c_svd = svd.solve(z);

    VectorXd c_svd = A.colPivHouseholderQr().solve(z);

//    if (!success) {
//        printf("fit_bspline3: SVD failed\n");
//        return -1.;
//    }

// fill bs from the solution vector
    double *c = new double[nbas];
    for (int i=0; i<nbas; i++)
        c[i] = c_svd(i);
    bs->SetCoef(c);
    delete[] c;

    VectorXd residual = A*c_svd - z;
    return sqrt(residual.squaredNorm());
}
#else // use QP
double fit_tpspline3(TPspline3 *bs, int npts, const double *datax, const double *datay, const double *dataz, const double *weight, bool even)
{
    int nbas = bs->GetNbas(); 	// numbes of basis functions
    int nbasx = bs->GetBSX().GetNbas();
    int nbasy = bs->GetBSY().GetNbas();
    Bool_t success;

    std::vector <double> vz;
    std::vector <double> vrow;
    std::vector <std::vector <double> > vmat;

// linear least squares

    // ========== fill the equations ===========

    for (int i=0; i<npts; i++) {
        int w = weight ? weight[i] : 1.;
        if (w>0.5) { // normal point
// f(x,y) = z
            vz.push_back(dataz[i]*w);
            for (int k=0; k<nbas; k++)
                vrow.push_back(bs->Basis(datax[i], datay[i], k)*w);
            vmat.push_back(vrow);
            vrow.clear();
        }
    }

// ========= solve the system using Eigen machinery =========

// we've got vz.size() equations with nbas unknowns
    MatrixXd A(vz.size(), nbas);
    VectorXd z(vz.size());

    for (int irow=0; irow<vz.size(); irow++) {
        z(irow) = vz[irow];
        for (int icol=0; icol<nbas; icol++)
            A(irow, icol) = vmat[irow][icol];
    }

// solve the system using quadratic programming, i.e.
// minimize 1/2 * x G x + g0 x subject to some inequality and equality constraints,
// where G = A'A and g0 = z'A (' denotes transposition)
    MatrixXd G = A.transpose()*A;  //
    VectorXd g0 = (z.transpose()*A)*(-1.);  //
    VectorXd x(nbas);

//  set ineqaulity constraints:
    MatrixXd CI = MatrixXd::Zero(nbas, nbas+(nbasy-1)*3);
    VectorXd ci0 = VectorXd::Zero(nbas+(nbasy-1)*3);
//  make all spline coefficients non-negative
    for (int i = 0; i<nbas; i++)
        CI(i, i) = 1.;
//  make them also non-increasing in x direction
    for (int iy = 0; iy<nbasy; iy++)
        for (int ix = 0; ix<nbasx-1; ix++) {
            int i = ix + iy*nbasx;
            CI(i+1, i) = -1.;
    }
// and in y direction (along the x=0 line)
    for (int iy = 0; iy<nbasy-1; iy++)
      for (int i=0; i<3; i++) {
        CI(iy*nbasx+i, iy*3+nbas+i) = -1.;
        CI((iy+1)*nbasx+i, iy*3+nbas+i) = 1.;
    }

/*
    for (int ix = 0; ix<nbasx; ix++)
        for (int iy = 1; iy<nbasy; iy++) {
            int i = ix + iy*nbasx;
            CI(i-nbasx, i) = -1.;
    }
*/
//  set equality constraints (unconstrained by default)
    MatrixXd CE;
    VectorXd ce0;


//    JacobiSVD <MatrixXd> svd(A, Eigen::ComputeThinU | Eigen::ComputeThinV);
//    std::cout << "Singular values: " << svd.singularValues();
//    VectorXd c_svd = svd.solve(z);

//    VectorXd c_svd = A.colPivHouseholderQr().solve(z);
/*
    std::cout << "A:" << std::endl;
    std::cout << A << std::endl;

    std::cout << "z:" << std::endl;
    std::cout << z << std::endl;

    std::cout << "G:" << std::endl;
    std::cout << G << std::endl;

    std::cout << "g0:" << std::endl;
    std::cout << g0 << std::endl;

    std::cout << "CE:" << std::endl;
    std::cout << CE << std::endl;

    std::cout << "CI:" << std::endl;
    std::cout << CI << std::endl;
*/
// all prepared - call the solver
    float res = solve_quadprog(G, g0,  CE, ce0,  CI, ci0, x);

// fill bs from the solution vector
    double *c = new double[nbas];
    for (int i=0; i<nbas; i++)
        c[i] = x(i);
    bs->SetCoef(c);
    delete[] c;

    return res;
}
#endif // use QP
#else // use ROOT instead of Eigen
double fit_tpspline3(TPspline3 *bs, int npts, const double *datax, const double *datay, const double *dataz, const double *weight, bool even)
{
    int nbas = bs->GetNbas(); 	// numbes of basis functions
    int nbasx = bs->GetBSX().GetNbas();
    int nbasy = bs->GetBSY().GetNbas();
    Bool_t success;

// a dirty wrap hack. removed. Raimundo
//turns out it's really dirty, and breaks things
    /*if (bs->GetWrapY()) {
        nbasy -= 3;
        nbas -= nbasx*3;
    }*/

    std::vector <double> vz;
    std::vector <double> vrow;
    std::vector <std::vector <double> > vmat;
    /*static*/ std::vector <std::vector <double> > vcache;
//    /*static*/ TDecompSVD svd;
    //TDecompQRH svd;
    TDecompSVD svd; //Slower but works... (probably because after-treatment wasn't correct with QRH). Raimundo

// linear least squares

    // ========== fill the equations ===========

    for (int i=0; i<npts; i++) {
        int w = weight ? weight[i] : 1.;
        if (w>0.5) { // normal point
// f(x,y) = z
            vz.push_back(dataz[i]*w);
            for (int k=0; k<nbas; k++)
                vrow.push_back(bs->Basis(datax[i], datay[i], bs->GetWrapped(k))*w);
            vmat.push_back(vrow);
            vrow.clear();
        }
/* This might be a good idea for polar fitting when a large contigous chunk
 * of data is missing, but in other cases may cause unnecessary oscillations.
 * Commented out for now, may convert later in an option just for polar case */
// Turns out its a bad idea... NOT Commented. Raimundo
        else { // empty point - force derivatives to zero
// df/dx = 0
            vz.push_back(0.);
            for (int k=0; k<nbas; k++)
                vrow.push_back(bs->BasisDrvX(datax[i], datay[i], bs->GetWrapped(k)));
            vmat.push_back(vrow);
            vrow.clear();
// df/dy = 0
            vz.push_back(0.);
            for (int k=0; k<nbas; k++)
                vrow.push_back(bs->BasisDrvY(datax[i], datay[i], bs->GetWrapped(k)));
            vmat.push_back(vrow);
            vrow.clear();
        }
    }
    // if required, force the function even by setting
    // the derivative d/dx to 0 along x=0 axis
    if (even) {
        double strength = ((double)npts)/nbas;
        for (int iy=0; iy<nbasy; iy++) {
            vz.push_back(0.);
            vrow.assign(nbas, 0.);
            for (int ix=0; ix<nbasx; ix++)
                vrow[iy*nbasx+ix] = bs->GetBSX().BasisDrv(0., ix)*strength;
            vmat.push_back(vrow);
            vrow.clear();
        }
    // also tell it NOT wiggle the tail by setting d/dx to 0 at x=xmax
    // (experimental feature)
        double xmax = bs->GetXmax();
        for (int iy=0; iy<nbasy; iy++) {
            vz.push_back(0.);
            vrow.assign(nbas, 0.);
            for (int ix=0; ix<nbasx; ix++)
                vrow[iy*nbasx+ix] = bs->GetBSX().BasisDrv(xmax-0.001, ix)*strength;
            vmat.push_back(vrow);
            vrow.clear();
        }
    }

// ========= solve the system using ROOT machinery =========

// we've got vz.size() equations with nbas unknowns
    TMatrixD A(vz.size(), nbas);
    TVectorD z(vz.size());


// compare the matrix with one saved from the last call (vcache)
// if they are equal we'll save a lot of time by reusing SVD
    bool use_cache = false;
/*
    if (vmat.size() == vcache.size()) {
        for (unsigned int i=0; i<vmat.size(); i++)
            if (!(vmat[i] ==vcache[i]))
                break;
        use_cache = true;
    }
*/
// matrix changed: save the new one to vcache and do SVD
    if (!use_cache) {
        vcache.clear();
        vcache.resize(vmat.size());
        for (unsigned int irow=0; irow<vz.size(); irow++) {
//            z(irow) = vz[irow];
            vcache[irow].resize(vmat[irow].size());
            for (int icol=0; icol<nbas; icol++) {
                A(irow, icol) = vcache[irow][icol] = vmat[irow][icol];
            }
        }
        svd.SetMatrix(A);
    }

// update the right-hand side
    for (unsigned int irow=0; irow<vz.size(); irow++)
        z(irow) = vz[irow];

// solve
    const TVectorD c_svd = svd.Solve(z, success);
    if (!success) {
        fprintf(stderr, "fit_bspline3: SVD failed\n");
        return -1.;
    }

    //TVectorD sig = svd.GetSig();
    //fprintf(stderr, "Singular values: %fl ... %lf\n", sig(0), sig(nbas-1));

// fill bs from the solution vector
    //nbas = bs->GetNbas(); // restore in case it was mangled by wrap hack
    double *c = new double[nbas];
    for (int i=0; i<nbas; i++)
        c[i] = c_svd(bs->GetWrapped(i));    // duplicate wrapped knots if necessary
    bs->SetCoef(c);
    delete[] c;

    TVectorD residual = A*c_svd - z;
    double resid = sqrt(residual.Norm2Sqr());
    return resid;
}
#endif

/*
double fit_tpspline3(TPspline3 *bs, int npts, double *datax, double *datay, double *dataz, double *weight, bool even)
{
	int nbas = bs->GetNbas(); 	// numbes of basis functions
    int nbasx = bs->GetBSX()->GetNbas();
    int nbasy = bs->GetBSY()->GetNbas();
	Bool_t success;
    int zeros = 0;
    if (weight)
        for (int i=0; i<npts; i++)
            if (weight[i]<0.5)
                zeros++;

    printf("zeros: %d\n", zeros);
    int expand = even ? nbasy : 0;
    expand += zeros;

// linear least squares
// we've got npts (+nbasy if forcing even) equations with nbas unknowns
    TMatrixD A(npts+expand, nbas);
    TVectorD z(npts+expand);

    std::vector <double> vx, vy;

// fill the equations	
	for (int i=0; i<npts; i++) {
        int w = weight ? weight[i] : 1.;
        if (w>0.5) {
            z(i) = dataz[i]*w;
            for (int k=0; k<nbas; k++)
                A(i, k) = bs->Basis(datax[i], datay[i], k)*w;
        } else {
            z(i) = 0.;
            for (int k=0; k<nbas; k++)
                A(i, k) = bs->BasisDrvX(datax[i], datay[i], k);
            vx.push_back(datax[i]);
            vy.push_back(datay[i]);
        }
    }

    for (int j=0; j<zeros; j++) {
        int i = npts + j;
        z(i) = 0.;
        for (int k=0; k<nbas; k++)
            A(i, k) = bs->BasisDrvY(vx[i], vy[i], k);
    }

// force the function even by setting
// the derivative d/dx to 0 along x=0 axis
    if (even) {
        double strength = ((double)npts)/nbas;
        for (int iy=0; iy<nbasy; iy++) {
            z(npts+zeros+iy) = 0.;
            for (int ix=0; ix<nbasx; ix++)
                A(npts+zeros+iy, iy*nbasx+ix) = bs->GetBSX()->BasisDrv(0., ix)*strength;
        }
    }

// solve the system
	TDecompSVD svd(A);
	const TVectorD c_svd = svd.Solve(z, success);
	if (!success) {
		printf("fit_bspline3: SVD failed\n");
		return -1.;
	} 

// fill bs from the solution vector
	double *c = new double[nbas];
	for (int i=0; i<nbas; i++)
		c[i] = c_svd(i);
	bs->SetCoef(c);
	delete[] c;
	
	TVectorD residual = A*c_svd - z;
	return sqrt(residual.Norm2Sqr());
}
*/
double fit_tpspline3_grid(TPspline3 *bs, int npts, const double *datax, const double *datay, const double *dataz, bool even)
{
    // number of intervals can't be less then number of basis functions in the spline
    // we set it (somewhat arbitrarily) to 4 times number of intervals in the spline
    int nintx = bs->GetNintX()*2;
    int ninty = bs->GetNintY()*2;
//    printf ("nx = %d, ny = %d\n", nintx, ninty);
//    printf ("ymin = %lf, ymax = %lf\n", bs->GetYmin(), bs->GetYmax());

    TProfile2D *h1 = new TProfile2D("bs32Daux", "", nintx, bs->GetXmin(), bs->GetXmax(),
                                    ninty, bs->GetYmin(), bs->GetYmax());
    for (int i=0; i<npts; i++)
        h1->Fill(datax[i], datay[i], dataz[i]);

//    h1->DrawPanel();
    std::vector <double> x, y, z, w, rms;
    int ngood = get_data_from_profile2D(h1, &x, &y, &z, &w, &rms);
//    for (int j=0; j<w.size(); j++)
//        printf("%f, %f, %f, %f\n", x[j], y[j], z[j], w[j]);
    delete h1;
    return fit_tpspline3(bs, ngood, &x[0], &y[0], &z[0], &w[0], even);
}

double fit_tpspline3_grid(TPspline3 *bs, int npts, const APoint *data, const double *dataz, bool even)
{
    // number of intervals can't be less then number of basis functions in the spline
    // we set it (somewhat arbitrarily) to 4 times number of intervals in the spline
    int nintx = bs->GetNintX()*2;
    int ninty = bs->GetNintY()*2;
//    printf ("nx = %d, ny = %d\n", nintx, ninty);
//    printf ("ymin = %lf, ymax = %lf\n", bs->GetYmin(), bs->GetYmax());

    TProfile2D *h1 = new TProfile2D("bs32Daux", "", nintx, bs->GetXmin(), bs->GetXmax(),
                                    ninty, bs->GetYmin(), bs->GetYmax());
    for (int i=0; i<npts; i++)
        h1->Fill(data[i].x(), data[i].y(), dataz[i]);

//    h1->DrawPanel();
    std::vector <double> x, y, z, w, rms;
    int ngood = get_data_from_profile2D(h1, &x, &y, &z, &w, &rms);
//    for (int j=0; j<w.size(); j++)
//        printf("%f, %f, %f, %f\n", x[j], y[j], z[j], w[j]);
    delete h1;
    return fit_tpspline3(bs, ngood, &x[0], &y[0], &z[0], &w[0], even);
}
