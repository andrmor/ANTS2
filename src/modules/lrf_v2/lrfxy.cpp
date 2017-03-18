#include "lrfxy.h"
#include "jsonparser.h"
#include "spline.h"

#include <QJsonObject>

#include <math.h>
#include <tpspline3.h>

LRFxy::LRFxy(double x_min, double x_max, int n_intx, double y_min,
            double y_max, int n_inty, bool log) : LRF2(), xmin(x_min), xmax(x_max),
            ymin(y_min), ymax(y_max), nintx(n_intx), ninty(n_inty), bsr(NULL), bse(NULL), logscale(log)
{
}

LRFxy::LRFxy(QJsonObject &json) : LRF2(), bsr(NULL), bse(NULL), logscale(false)
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

bool LRFxy::inDomain(double x, double y, double /*z*/) const
{
    return x>xmin && x<xmax && y>ymin && y<ymax;
}

double LRFxy::getRmax() const
{
    return std::max(std::max(fabs(xmin), fabs(ymin)), std::max(fabs(xmax), fabs(ymax)));
}

double LRFxy::eval(double x, double y, double /*z*/) const
{
    // TODO: insert check for bsr with popup on failure
    if (!inDomain(x, y)) {
//        qDebug() << x << "," << y << "Not in Domain";
        return 0.;
    }
    return logscale ? exp(bsr->Eval(x, y)) : bsr->Eval(x, y);
}

double LRFxy::evalErr(double /*x*/, double /*y*/, double /*z*/) const
{
    return 0;
}

double LRFxy::evalDrvX(double x, double y, double /*z*/) const
{
    // TODO: insert check for bsr with popup on failure
    if (!inDomain(x, y))
        return 0.;
// TODO: handle logscale if possible
    return bsr->EvalDrvX(x, y);
}

double LRFxy::evalDrvY(double x, double y, double /*z*/) const
{
// TODO: insert check for bsr with popup on failure
    if (!inDomain(x, y))
        return 0.;
// TODO: handle logscale if possible
    return bsr->EvalDrvY(x, y);
}

double LRFxy::eval(double x, double y, double /*z*/, double *err) const
{
    if (!inDomain(x, y)) {
        *err = 0.;
        return 0.;
    }
    *err = bse ? bse->Eval(x, y) : 0.;
    return logscale ? exp(bsr->Eval(x, y)) : bsr->Eval(x, y);
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

    bsr = new TPspline3(xmin, xmax, nintx, ymin, ymax, ninty);
    valid = true;

    if (!grid)
        return fit_tpspline3(bsr, va.size(), &vx[0], &vy[0], &va[0]);
    else
        return fit_tpspline3_grid(bsr, va.size(), &vx[0], &vy[0], &va[0]);
}

void LRFxy::setSpline(TPspline3 *bs, bool log)
{
    bsr = bs;
    bsr->GetRangeX(&xmin, &xmax);
    bsr->GetRangeY(&ymin, &ymax);
    logscale = log;
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
  json["logscale"] = logscale;

  return json;
}
