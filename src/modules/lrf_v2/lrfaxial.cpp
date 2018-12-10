#include "lrfaxial.h"
#include "spline.h"
#include "bspline123d.h"
#include "jsonparser.h"
#include "bsfit123.h"

#include <QDebug>
#include <QJsonObject>

LRFaxial::LRFaxial(double r, int nint, double x0, double y0)
{
    rmax = r;
    this->x0 = x0;
    this->y0 = y0;
    this->nint = nint;
    bsr = new Bspline1d(0., r, nint);
    bse = new Bspline1d(0., r, nint);
    flattop = true;
    non_increasing = false;
    non_negative = false;
    recalcLimits();
}

void LRFaxial::recalcLimits()
{
    rmax2 = rmax*rmax;
    xmin = x0-rmax;
    xmax = x0+rmax;
    ymin = y0-rmax;
    ymax = y0+rmax;
}

LRFaxial::LRFaxial(QJsonObject &json) : LRF2(), bsr(NULL), bse(NULL)
{
    JsonParser parser(json);
    QJsonObject jsobj, splineobj;

    x0 = y0 = 0.; // compatibility: default axis is at (0,0)
    parser.ParseObject("x0", x0);
    parser.ParseObject("y0", y0);
    parser.ParseObject("rmax", rmax);
    recalcLimits();

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

bool LRFaxial::errorDefined() const
{
    return bse->IsReady();
}

bool LRFaxial::isReady() const
{
    return (bsr != 0) && bsr->IsReady();
}

bool LRFaxial::inDomain(double x, double y, double /*z*/) const
{
    return x*x + y*y < rmax2;
}

double LRFaxial::eval(double x, double y, double /*z*/) const
{
    return isReady() ? bsr->Eval(Rho(x, y)) : 0.;
}

double LRFaxial::evalErr(double x, double y, double /*z*/) const
{
    return errorDefined() ? bse->Eval(Rho(x, y)) : 0.;
}

double LRFaxial::evalDrvX(double x, double y, double /*z*/) const
{
    return isReady() ? bsr->EvalDrv(Rho(x, y))*RhoDrvX(x, y) : 0.;
}

double LRFaxial::evalDrvY(double x, double y, double /*z*/) const
{
    return isReady() ? bsr->EvalDrv(Rho(x, y))*RhoDrvY(x, y) : 0.;
}

double LRFaxial::eval(double x, double y, double /*z*/, double *err) const
{
    double rho = Rho(x, y);
    *err = errorDefined() ? bse->Eval(rho) : 0.;
    return isReady() ? bsr->Eval(rho) : 0.;
}

double LRFaxial::fit(int npts, const double *x, const double *y, const double* /*z*/, const double *data, bool grid)
{
    std::vector <double> vr;
    std::vector <double> va;

    for (int i=0; i<npts; i++) {
        if (!inDomain(x[i], y[i]))
            continue;
        vr.push_back(Rho(x[i], y[i]));
        va.push_back(data[i]);
    }
 //qDebug() << "fit vectors ready";
    bsr = new Bspline1d(0., Rho(rmax), nint);
 //qDebug() << "bsr created";

    BSfit1D F(bsr);
    if (flattop) F.SetConstraintEven();
    if (non_negative) F.SetConstraintNonNegative();
    if (non_increasing) F.SetConstraintNonIncreasing();

    if (!grid) {
        F.Fit(va.size(), &vr[0], &va[0]);
        valid = bsr->IsReady();
        return F.GetResidual();
    } else {
        F.AddData(va.size(), &vr[0], &va[0]);
        F.Fit();
        valid = bsr->IsReady();
        return F.GetResidual();
    }
}

double LRFaxial::fitRData(int npts, const double *r, const double *data)
{
    std::vector <double> vr;
    std::vector <double> va;

    for (int i=0; i<npts; i++) {
        vr.push_back(Rho(r[i]));
        va.push_back(data[i]);
    }

    bsr = new Bspline1d(0., Rho(rmax), nint);

    BSfit1D F(bsr);
    if (flattop) F.SetConstraintEven();
    if (non_negative) F.SetConstraintNonNegative();
    if (non_increasing) F.SetConstraintNonIncreasing();

    F.Fit(va.size(), &vr[0], &va[0]);
    valid = bsr->IsReady();
    return F.GetResidual();
}

double LRFaxial::fitError(int npts, const double *x, const double *y, const double * /*z*/, const double *data, bool grid)
{
    std::vector <double> vr;
    std::vector <double> va;

    for (int i=0; i<npts; i++) {
        if (!inDomain(x[i], y[i]))
            continue;
        double rc = Rho(x[i], y[i]);
        vr.push_back(rc);
        double err = data[i] - bsr->Eval(rc);
//        va.push_back(fabs(err));
        va.push_back(err*err);
    }
    //qDebug() << "Performing Error fitting";

    bse = new Bspline1d(0., Rho(rmax), nint);

    BSfit1D F(bse);

    if (!grid) {
        F.Fit(va.size(), &vr[0], &va[0]);
        return F.GetResidual();
    } else {
        F.AddData(va.size(), &vr[0], &va[0]);
        F.Fit();
        return F.GetResidual();
    }
}

void LRFaxial::setSpline(Bspline1d *bs)
{
    if (bsr)
        delete bsr;

    bsr = bs;
    rmax = bsr->GetXmax();
    nint = bsr->GetNint();
    recalcLimits();
}

const Bspline1d *LRFaxial::getSpline() const
{
    return bsr;
}

void LRFaxial::writeJSON(QJsonObject &json) const
{
    json["type"] = QString(type());
    json["rmax"] = rmax;
    json["x0"] = x0;
    json["y0"] = y0;
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

// ================ Compressed axial =================

LRFcAxial::LRFcAxial(double r, int nint, double k, double r0, double lam, double x0, double y0) :
    LRFaxial(r, nint, x0, y0)
{
    this->r0 = r0;
    a = (k+1)/(k-1);
    lam2 = lam*lam;
    b = sqrt(r0*r0+lam2)+a*r0;

    comp_lam = lam;
    comp_k = k;
}

LRFcAxial::LRFcAxial(QJsonObject &json) : LRFaxial(json)
{
    JsonParser parser(json);
    parser.ParseObject("a", a);
    parser.ParseObject("b", b);
    parser.ParseObject("r0", r0);
    parser.ParseObject("lam2", lam2);
    if(!parser.ParseObject("comp_lam", comp_lam))
        comp_lam = sqrt(lam2);
    if(!parser.ParseObject("comp_k", comp_k))
        comp_k = (a+1)/(a-1); //funny how this turned out :)
}

QJsonObject LRFcAxial::reportSettings() const
{
   QJsonObject json(LRFaxial::reportSettings());
   json["r0"] = r0;
   json["comp_lam"] = comp_lam;
   json["comp_k"] = comp_k;

   return json;
}

double LRFcAxial::Rho(double r) const
{
    double dr = r - r0;
    return std::max(0., b + dr*a - sqrt(dr*dr + lam2));
}

double LRFcAxial::Rho(double x, double y) const
{
    double dr = R(x, y) - r0;
    return std::max(0., b + dr*a - sqrt(dr*dr + lam2));
}

double LRFcAxial::RhoDrvX(double x, double y) const
{
    double dr = R(x, y) - r0;
    double dRhodR = dr/sqrt(dr*dr + lam2) + a;
    return dRhodR*(x-x0)/R(x, y);
}

double LRFcAxial::RhoDrvY(double x, double y) const
{
    double dr = R(x, y) - r0;
    double dRhodR = dr/sqrt(dr*dr + lam2) + a;
    return dRhodR*(y-y0)/R(x, y);
}

void LRFcAxial::writeJSON(QJsonObject &json) const
{
    LRFaxial::writeJSON(json);
    json["type"] = QString(type());
    json["a"] = a;
    json["b"] = b;
    json["r0"] = r0;
    json["lam2"] = lam2;
    json["comp_lam"] = comp_lam;
    json["comp_k"] = comp_k;
}
