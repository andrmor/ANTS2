#include "lrfaxial3d.h"
#include "spline.h"
#include "jsonparser.h"
#include "tps3fit.h"

#include <math.h>

#include <QJsonObject>

#define NEWFIT

LRFaxial3d::LRFaxial3d(double r, int nint_, double z_min,
            double z_max, int n_intz, bool log) : LRF3d(), rmax(r), nint(nint_),
            zmin(z_min), zmax(z_max), nintz(n_intz), bsr(NULL), bse(NULL)
{
    flat_top = false;
    non_increasing = false;
    non_negative = false;
    z_slope = 0;
}

LRFaxial3d::LRFaxial3d(QJsonObject &json) : LRF3d(), bsr(NULL), bse(NULL)
{
    JsonParser parser(json);
    QJsonObject jsobj, splineobj;

    parser.ParseObject("rmax", rmax);
    parser.ParseObject("zmin", zmin);
    parser.ParseObject("zmax", zmax);

// read response
    if ( parser.ParseObject("response", jsobj) ) {
        JsonParser tmp(jsobj);
        if (tmp.ParseObject("tpspline3", splineobj))
            bsr = read_tpspline3_json(splineobj);
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
        nint = bsr->GetNintX();
        nintz = bsr->GetNintY();
    }
}

LRFaxial3d::~LRFaxial3d()
{
    if (bsr)
        delete bsr;
    if (bse)
        delete bse;
}

bool LRFaxial3d::inDomain(double x, double y, double z) const
{
    return hypot(x,y)<rmax*rmax && z>zmin && z<zmax;
}

double LRFaxial3d::eval(double x, double y, double z) const
{
// TODO: insert check for bsr with popup on failure
    double r = hypot(x, y);
    if (r > rmax || z<zmin || z>zmax)
        return 0.;

    return bsr->Eval(compress(r), z);
}

double LRFaxial3d::evalErr(double /*x*/, double /*y*/, double /*z*/) const
{
    return 0.;
}

double LRFaxial3d::evalDrvX(double x, double y, double z) const
{
    // TODO: insert check for bsr with popup on failure
    double r = hypot(x, y);
    if (r > rmax || z<zmin || z>zmax)
        return 0.;
// TODO: handle logscale if possible
    return bsr->EvalDrvX(compress(r), z)*comprDev(r)*x/r;
}

double LRFaxial3d::evalDrvY(double x, double y, double z) const
{
// TODO: insert check for bsr with popup on failure
    double r = hypot(x, y);
    if (r > rmax || z<zmin || z>zmax)
        return 0.;
// TODO: handle logscale if possible
    return bsr->EvalDrvX(compress(r), z)*comprDev(r)*y/r;
}

double LRFaxial3d::eval(double x, double y, double z, double *err) const
{
    double r = hypot(x, y);
    if (r > rmax || z<zmin || z>zmax) {
        *err = 0.;
        return 0.;
    }
    *err = bse ? bse->Eval(compress(r), z) : 0.;
    return bsr->Eval(compress(r), z);
}

#ifdef NEWFIT
double LRFaxial3d::fit(int npts, const double *x, const double *y, const double *z, const double *data, bool grid)
{
    std::vector <double> vr;
    std::vector <double> vz;
    std::vector <double> va;
    for (int i=0; i<npts; i++) {
        if (!inDomain(x[i], y[i], z[i]))
            continue;
        vr.push_back(compress(hypot(x[i], y[i])));
        vz.push_back(z[i]);
        va.push_back(data[i]);
    }

    bsr = new TPspline3(0., compress(rmax), nint, zmin, zmax, nintz);
    valid = true;

    TPS3fit F(bsr);
    if (flat_top)
        F.SetConstraintDdxAt0();
    if (non_negative)
        F.SetConstraintNonNegative();
    if (non_increasing)
        F.SetConstraintNonIncreasingX();
    if (z_slope)
        F.SetConstraintSlopeY(z_slope);

    if (!grid) {
        F.Fit(va.size(), &vr[0], &vz[0], &va[0]);
        return F.GetResidual();
    } else {
        F.AddData(va.size(), &vr[0], &vz[0], &va[0]);
        F.Fit();
        return F.GetResidual();
    }
}

#else

double LRFaxial3d::fit(int npts, const double *x, const double *y, const double *z, const double *data, bool grid)
{  
    std::vector <double> vr;
    std::vector <double> vz;
    std::vector <double> va;
    for (int i=0; i<npts; i++) {
        if (!inDomain(x[i], y[i], z[i]))
            continue;
        vr.push_back(compress(hypot(x[i], y[i])));
        vz.push_back(z[i]);
        va.push_back(data[i]);
    }

    bsr = new TPspline3(0., compress(rmax), nint, zmin, zmax, nintz);
    valid = true;

    if (!grid)
        return fit_tpspline3(bsr, va.size(), &vr[0], &vz[0], &va[0], NULL, true);
    else
        return fit_tpspline3_grid(bsr, va.size(), &vr[0], &vz[0], &va[0], true);
}
#endif

void LRFaxial3d::setSpline(TPspline3 *bs, bool log)
{
    bsr = bs;
    double dummy;
    bsr->GetRangeX(&dummy, &rmax);
    bsr->GetRangeY(&zmin, &zmax);
}

void LRFaxial3d::writeJSON(QJsonObject &json) const
{
    json["type"] = QString(type());
    json["rmax"] = rmax;
    json["zmin"] = zmin;
    json["zmax"] = zmax;
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

QJsonObject LRFaxial3d::reportSettings() const
{
  QJsonObject json;
  json["type"] = QString(type());
  json["n1"] = nint;
  json["n2"] = nintz;

  return json;
}

double LRFaxial3d::comprDev(double /*r*/) const
{
    return 1.0;
}
