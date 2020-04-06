#include "lrfaxial.h"
#include "spline.h"
#include "bspline123d.h"
#include "jsonparser.h"
#include "bsfit123.h"
#include "compress.h"

#include <QDebug>
#include <QJsonObject>

LRFaxial::LRFaxial(double r, int nint, double x0, double y0)
{
    rmax = r;
    this->x0 = x0;
    this->y0 = y0;
    this->nint = nint;
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

LRFaxial::LRFaxial(QJsonObject &json) : LRF2()
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

// try to read compression
    if ( parser.ParseObject("compression", jsobj) )
        compress = Compress1d::Factory(jsobj);

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
    if (compress)
        delete compress;
}

bool LRFaxial::errorDefined() const
{
    return (bse != 0) && bse->IsReady();
}

bool LRFaxial::isReady() const
{
    return (bsr != 0) && bsr->IsReady();
}


bool LRFaxial::inDomain(double x, double y, double /*z*/) const
{
// the agony of choice:
// faster
    return (x-x0)*(x-x0)+(y-y0)*(y-y0) < rmax2;
// more consistent
//    return Rho(x, y) < rhomax;
}

double LRFaxial::Rho(double r) const
{
    return compress ? compress->Rho(r) : r;
}

double LRFaxial::Rho(double x, double y) const
{
    return compress ? compress->Rho(R(x, y)) : R(x, y);
}

double LRFaxial::RhoDrvX(double x, double y) const
{
    double drdx = (x-x0)/R(x, y);
    return compress ? compress->RhoDrv(R(x, y))*drdx : drdx;
}

double LRFaxial::RhoDrvY(double x, double y) const
{
    double drdy = (y-y0)/R(x, y);
    return compress ? compress->RhoDrv(R(x, y))*drdy : drdy;
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

// dangerous and probably not needed
//void LRFaxial::setSpline(Bspline1d *bs)
//{
//    if (bsr)
//        delete bsr;

//    bsr = bs;
//    rmax = bsr->GetXmax();
//    nint = bsr->GetNint();
//    recalcLimits();
//}

// TODO: make it safer (convert into reference?)
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
    if (compress) {
        QJsonObject compr;
        compress->writeJSON(compr);
        json["compression"] = compr;
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
// Deprecated! Left for compatibility

LRFcAxial::LRFcAxial(double r, int nint,  double k, double r0, double lam, double x0, double y0):
    LRFaxial(r, nint, x0, y0)
{
    compress = new DualSlopeCompress(k, r0, lam);
}

LRFcAxial::LRFcAxial(QJsonObject &json) : LRFaxial(json)
{
    if (!compress) { // compatibility with old saved LRFs
        double a, b, r0, lam2, comp_lam, comp_k;

        JsonParser parser(json);
        parser.ParseObject("a", a);
        parser.ParseObject("b", b);
        parser.ParseObject("r0", r0);
        parser.ParseObject("lam2", lam2);
        if(!parser.ParseObject("comp_lam", comp_lam))
            comp_lam = sqrt(lam2);
        if(!parser.ParseObject("comp_k", comp_k))
            comp_k = (a+1)/(a-1); //funny how this turned out :)

        compress = new DualSlopeCompress(comp_k, r0, comp_lam);
    }
}

QJsonObject LRFcAxial::reportSettings() const
{
    QJsonObject json(LRFaxial::reportSettings());
    QJsonObject compr(compress->reportSettings());
    foreach(const QString& key, compr.keys())
        json[key] = compr.value(key);

    return json;
}

#if 0
void LRFcAxial::writeJSON(QJsonObject &json) const
{
    LRFaxial::writeJSON(json);
    json["type"] = QString(type());
    QJsonObject compr(compress->reportSettings());
    foreach(const QString& key, compr.keys())
        json[key] = compr.value(key);
}
#endif
