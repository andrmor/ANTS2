#include "lrfaxial.h"
#include "spline.h"
#include "bspline3.h"
#include "jsonparser.h"

#include <math.h>

#include <QDebug>
#include <QJsonObject>

LRFaxial::LRFaxial(double r, int nint) :
            LRF2()
{
    rmax = r;
    rmax2 = r*r;
    this->nint = nint;
    bsr = 0;
    bse = 0;
    flattop = true;
}

LRFaxial::LRFaxial(QJsonObject &json) : LRF2(), bsr(NULL), bse(NULL)
{
    JsonParser parser(json);
    QJsonObject jsobj, splineobj;

    parser.ParseObject("rmax", rmax);
    rmax2 = rmax*rmax;

// read response
    if ( parser.ParseObject("response", jsobj) ) {
        JsonParser tmp(jsobj);
        if (tmp.ParseObject("bspline3", splineobj))
            bsr = read_bspline3_json(splineobj);
    }

// try to read error
    if ( parser.ParseObject("error", jsobj) ) {
        JsonParser tmp(jsobj);
        if (tmp.ParseObject("bspline3", splineobj))
            bse = read_bspline3_json(splineobj);
    }

// final touches   
    valid = (!parser.GetErrType() && bsr) ? true : false;
    if (bsr) nint = bsr->GetNint();
}

LRFaxial::~LRFaxial()
{
    if (bsr)
        delete bsr;
    if (bse)
        delete bse;
}

bool LRFaxial::inDomain(double x, double y, double /*z*/) const
{
    return x*x+y*y < rmax2;
}

double LRFaxial::eval(double x, double y, double /*z*/) const
{
    // TODO: insert check for bsr with popup on failure
    double r = hypot(x, y);
    if (r > rmax)
        return 0.;
    return bsr->Eval(compress(r));
}

double LRFaxial::evalErr(double x, double y, double /*z*/) const
{
    if (!bse) return 0.;

    double r = hypot(x, y);
    if (r > rmax)
        return 0.;
    return bse->Eval(compress(r));
}

double LRFaxial::evalDrvX(double x, double y, double /*z*/) const
{
// TODO: insert check for bsr with popup on failure
    double r = hypot(x, y);
    if (r > rmax)
        return 0.;
    return bsr->EvalDrv(compress(r))*comprDev(r)*x/r;
}

double LRFaxial::evalDrvY(double x, double y, double /*z*/) const
{
// TODO: insert check for bsr with popup on failure
    double r = hypot(x, y);
    if (r > rmax)
        return 0.;
    return bsr->EvalDrv(compress(r))*comprDev(r)*y/r;
}

double LRFaxial::eval(double x, double y, double /*z*/, double *err) const
{
    double r = hypot(x, y);
    if (r > rmax) {
        *err = 0.;
        return 0.;
    }
    *err = bse ? bse->Eval(compress(r)) : 0.;
    return bsr->Eval(compress(r));
}

double LRFaxial::fit(int npts, const double *x, const double *y, const double* /*z*/, const double *data, bool grid)
{
  //qDebug() << "fit axial start, flattop flag:"<<flattop;
  std::vector <double> vr;
  std::vector <double> va;
  try
    {
      vr.reserve(npts);
      va.reserve(npts);
    }
  catch(...)
    {
        qDebug()<< "AxialFit: not enough memory!";
        return -1;
    }

    for (int i=0; i<npts; i++) {
        if (!inDomain(x[i], y[i]))
            continue;
        double r = hypot(x[i], y[i]);
        vr.push_back(compress(r));
        va.push_back(data[i]);
    }
 //qDebug() << "fit vectors ready";
    bsr = new Bspline3(0., compress(rmax), nint);
 //qDebug() << "bsr created";
    valid = true;

    if (!grid)
        return fit_bspline3(bsr, va.size(), &vr[0], &va[0], flattop);
    else
        return fit_bspline3_grid(bsr, va.size(), &vr[0], &va[0], flattop);
}

double LRFaxial::fitRData(int npts, const double *r, const double *data)
{
    std::vector <double> vr;
    std::vector <double> va;

    for (int i=0; i<npts; i++) {
        vr.push_back(compress(r[i]));
        va.push_back(data[i]);
    }

    bsr = new Bspline3(0., compress(rmax), nint);
    valid = true;

    return fit_bspline3(bsr, va.size(), &vr[0], &va[0], flattop);
}

double LRFaxial::fitError(int npts, const double *x, const double *y, const double * /*z*/, const double *data, bool grid)
{
    std::vector <double> vr;
    std::vector <double> va;
    for (int i=0; i<npts; i++) {
        if (!inDomain(x[i], y[i]))
            continue;
        double rc = compress(hypot(x[i], y[i]));
        vr.push_back(rc);
        double err = data[i] - bsr->Eval(rc);
//        va.push_back(fabs(err));
        va.push_back(err*err);
    }
    qDebug() << "Performing Error fitting";

    bse = new Bspline3(0., compress(rmax), nint);

    if (!grid)
        return fit_bspline3(bse, va.size(), &vr[0], &va[0], false);
    else
        return fit_bspline3_grid(bse, va.size(), &vr[0], &va[0], false, true);
}

void LRFaxial::setSpline(Bspline3 *bs)
{
    bsr = bs;
    double dummy;
    bsr->GetRange(&dummy, &rmax);
}

const Bspline3 *LRFaxial::getSpline() const
{
    return bsr;
}

void LRFaxial::writeJSON(QJsonObject &json) const
{
    json["type"] = QString(type());
    json["rmax"] = rmax;
    if (bsr) {
        QJsonObject response, spline;
        write_bspline3_json(bsr, spline);
        response["bspline3"] = spline;
        json["response"] = response;
    }
    if (bse) {
        QJsonObject error, spline;
        write_bspline3_json(bse, spline);
        error["bspline3"] = spline;
        json["error"] = error;
      }
}

QJsonObject LRFaxial::reportSettings() const
{
    QJsonObject json;
    json["type"] = QString(type());
    json["n1"] = nint;

    return json;
}

double LRFaxial::comprDev(double /*r*/) const
{
    return 1.0;
}
