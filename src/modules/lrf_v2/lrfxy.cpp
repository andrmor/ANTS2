#include "lrfxy.h"
#include "jsonparser.h"
#include "spline.h"
#include "bspline123d.h"
#include "bsfit123.h"

#include <QJsonObject>

#include <math.h>


LRFxy::LRFxy(double x_min, double x_max, int n_intx, double y_min,
            double y_max, int n_inty) : LRF2(),
//            xmin(x_min), xmax(x_max), ymin(y_min), ymax(y_max),
            nintx(n_intx), ninty(n_inty), bsr(NULL), bse(NULL)
{
    xmin = x_min; xmax = x_max;
    ymin = y_min; ymax = y_max;
}

LRFxy::LRFxy(QJsonObject &json) : LRF2(), bsr(NULL), bse(NULL)
{
    JsonParser parser(json);
    QJsonObject jsobj, splineobj;

    parser.ParseObject("xmin", xmin);
    parser.ParseObject("xmax", xmax);
    parser.ParseObject("ymin", ymin);
    parser.ParseObject("ymax", ymax);

// read response
    if ( parser.ParseObject("response", jsobj) ) {
        JsonParser tmp(jsobj);
        if (tmp.ParseObject("tpspline3", splineobj)) {
            bsr = read_tpspline3_json(splineobj);
        }
    }

// try to read error
    if ( parser.ParseObject("error", jsobj) ) {
        JsonParser tmp(jsobj);
        if (tmp.ParseObject("tpspline3", splineobj))
            bse = read_tpspline3_json(splineobj);
    }

// final touches
    valid = (!parser.GetErrType() && bsr) ? true : false;
    if (bsr) {
        nintx = bsr->GetNintX();
        ninty = bsr->GetNintY();
    }
}

LRFxy::~LRFxy()
{
    if (bsr)
        delete bsr;
    if (bse)
        delete bse;
}

bool LRFxy::errorDefined() const
{
    return (bse != 0) && bse->IsReady();
}

bool LRFxy::isReady() const
{
    return (bsr != 0) && bsr->IsReady();
}

bool LRFxy::inDomain(double x, double y, double /*z*/) const
{
    return x>xmin && x<xmax && y>ymin && y<ymax;
}

// TODO: Check WTF is Rmax for
double LRFxy::getRmax() const
{
    return std::max(std::max(fabs(xmin), fabs(ymin)), std::max(fabs(xmax), fabs(ymax)));
}

double LRFxy::eval(double x, double y, double /*z*/) const
{
    return isReady() ? bsr->Eval(x, y) : 0.;
}

double LRFxy::evalErr(double x, double y, double /*z*/) const
{
    return errorDefined() ? bse->Eval(x, y) : 0.;
}

double LRFxy::evalDrvX(double x, double y, double /*z*/) const
{
    return isReady() ? bsr->EvalDrvX(x, y) : 0.;
}

double LRFxy::evalDrvY(double x, double y, double /*z*/) const
{
    return isReady() ? bsr->EvalDrvY(x, y) : 0.;
}

double LRFxy::eval(double x, double y, double /*z*/, double *err) const
{
    *err = errorDefined() ? bse->Eval(x, y) : 0.;
    return isReady() ? bsr->Eval(x, y) : 0.;
}

double LRFxy::fit(int npts, const double *x, const double *y, const double * /*z*/, const double *data, bool grid)
{
    std::vector <double> vx;
    std::vector <double> vy;
    std::vector <double> va;
    for (int i=0; i<npts; i++) {
        if (!inDomain(x[i], y[i]))
            continue;
        vx.push_back(x[i]);
        vy.push_back(y[i]);
        va.push_back(data[i]);
    }

    bsr = new Bspline2d(xmin, xmax, nintx, ymin, ymax, ninty);
//    valid = true;

    BSfit2D F(bsr);
    if (non_negative)
        F.SetConstraintNonNegative();

    if (top_down)
        F.SetConstraintTopDown(x0, y0);

    if (!grid) {
        F.Fit(va.size(), &vx[0], &vy[0], &va[0]);
        return F.GetResidual();
    } else {
        F.AddData(va.size(), &vx[0], &vy[0], &va[0]);
        F.Fit();
        return F.GetResidual();
    }
}

double LRFxy::fitError(int npts, const double *x, const double *y, const double * /*z*/, const double *data, bool grid)
{
    std::vector <double> vx;
    std::vector <double> vy;
    std::vector <double> va;
    for (int i=0; i<npts; i++) {
        if (!inDomain(x[i], y[i]))
            continue;
        vx.push_back(x[i]);
        vy.push_back(y[i]);
        double err = data[i] - bsr->Eval(x[i], y[i]);
//        va.push_back(fabs(err));
        va.push_back(err*err);
    }

    bse = new Bspline2d(xmin, xmax, nintx, ymin, ymax, ninty);

    BSfit2D F(bse);

    if (!grid) {
        F.Fit(va.size(), &vx[0], &vy[0], &va[0]);
        return F.GetResidual();
    } else {
        F.AddData(va.size(), &vx[0], &vy[0], &va[0]);
        F.Fit();
        return F.GetResidual();
    }
}

void LRFxy::setSpline(Bspline2d *bs)
{
    bsr = bs;
    bsr->GetRangeX(&xmin, &xmax);
    bsr->GetRangeY(&ymin, &ymax);
}

void LRFxy::writeJSON(QJsonObject &json) const
{
    json["type"] = QString(type());
    json["xmin"] = xmin;
    json["xmax"] = xmax;
    json["ymin"] = ymin;
    json["ymax"] = ymax;

    if (bsr) {
        QJsonObject response, spline;
        write_tpspline3_json(bsr, spline);
        response["tpspline3"] = spline;
        json["response"] = response;
    }
    if (bse) {
        QJsonObject error, spline;
        write_tpspline3_json(bse, spline);
        error["tpspline3"] = spline;
        json["error"] = error;
      }
}

QJsonObject LRFxy::reportSettings() const
{
  QJsonObject json;
  json["type"] = QString(type());
  json["n1"] = nintx;
  json["n2"] = ninty;

  return json;
}
