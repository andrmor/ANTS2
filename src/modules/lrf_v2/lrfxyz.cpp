#include "lrfxyz.h"
#include "jsonparser.h"
#include "spline.h"
#include "bspline123d.h"
#include "bsfit123.h"

#include <QJsonObject>

#include <math.h>

LRFxyz::LRFxyz(double x_min, double x_max, int n_intx, double y_min,
            double y_max, int n_inty, double z_min, double z_max, int n_intz) : LRF2(),
//            xmin(x_min), xmax(x_max), ymin(y_min), ymax(y_max), zmin(z_min), zmax(z_max),
            nintx(n_intx), ninty(n_inty), nintz(n_intz)
{
    non_negative = false;
    nbasz = nintz+3;
    xmin = x_min; xmax = x_max;
    ymin = y_min; ymax = y_max;
    zmin = z_min; zmax = z_max;
    bsr = new Bspline3d(xmin, xmax, nintx, ymin, ymax, ninty, zmin, zmax, nintz);
    bse = 0;
}

LRFxyz::LRFxyz(QJsonObject &json) : LRF2()
{
    JsonParser parser(json);
    QJsonObject jsobj, splineobj;
    QJsonArray resparr, errarr;

    parser.ParseObject("xmin", xmin);
    parser.ParseObject("xmax", xmax);
    parser.ParseObject("ymin", ymin);
    parser.ParseObject("ymax", ymax);
    parser.ParseObject("zmin", zmin);
    parser.ParseObject("zmax", zmax);
    parser.ParseObject("xintervals", nintx);
    parser.ParseObject("yintervals", ninty);
    parser.ParseObject("zintervals", nintz);

    non_negative = false;
    nbasz = nintz+3;
    bsr = new Bspline3d(xmin, xmax, nintx, ymin, ymax, ninty, zmin, zmax, nintz);
    bse = 0;

// read response
    if ( parser.ParseObject("response", resparr) ) {
        QVector <QJsonObject> stack;
        if (parser.ParseArray(resparr, stack))
            if (stack.size() >= nbasz)
                for (int i=0; i<nbasz; i++) {
                    parser.SetObject(stack[i]);
                    if (parser.ParseObject("tpspline3", splineobj))
                        bsr->SetZplane(i, read_tpspline3_json(splineobj));
        }
    }

// try to read error
    parser.SetObject(json);
    if ( parser.ParseObject("error", errarr) ) {
        bse = new Bspline3d(xmin, xmax, nintx, ymin, ymax, ninty, zmin, zmax, nintz);
        QVector <QJsonObject> stack;
        if (parser.ParseArray(errarr, stack))
            if (stack.size() >= nbasz)
                for (int i=0; i<nbasz; i++) {
                    parser.SetObject(stack[i]);
                    if (parser.ParseObject("tpspline3", splineobj))
                        bse->SetZplane(i, read_tpspline3_json(splineobj));
        }
    }

// final touches
    valid = (!parser.GetErrType()) ? true : false;
}

LRFxyz::~LRFxyz()
{
    if (bsr)
        delete bsr;
    if (bse)
        delete bse;
}

bool LRFxyz::errorDefined() const
{
    return false;
    //    return bse[0] != NULL;
}

double LRFxyz::getRmax() const
{
  return std::max(std::max(fabs(xmin), fabs(ymin)), std::max(fabs(xmax), fabs(ymax)));
}


double LRFxyz::eval(double x, double y, double z) const
{
// TODO: insert check for bsr with popup on failure
    if (!inDomain(x, y, z))
        return 0.;

    return bsr->Eval(x, y, z);
}

double LRFxyz::evalErr(double x, double y, double z) const
{
    return bse ? bse->Eval(x, y, z) : 0.;
}

double LRFxyz::evalDrvX(double x, double y, double z) const
{
    // TODO: insert check for bsr with popup on failure
    if (!inDomain(x, y, z))
        return 0.;

    return bsr->EvalDrvX(x, y, z);
}

double LRFxyz::evalDrvY(double x, double y, double z) const
{
// TODO: insert check for bsr with popup on failure
    if (!inDomain(x, y, z))
        return 0.;

    return bsr->EvalDrvY(x, y, z);
}

double LRFxyz::evalDrvZ(double x, double y, double z) const
{
// TODO: insert check for bsr with popup on failure
    if (!inDomain(x, y, z))
        return 0.;

    return bsr->EvalDrvZ(x, y, z);
}

double LRFxyz::eval(double x, double y, double z, double *err) const
{
    if (!inDomain(x, y, z)) {
        *err = 0.;
        return 0.;
    }

    *err = bse ? bse->Eval(x, y, z) : 0.;
    return bsr->Eval(x, y, z);
}

double LRFxyz::fit(int npts, const double *x, const double *y, const double *z, const double *data, bool grid)
{
    std::vector <double> vx;
    std::vector <double> vy;
    std::vector <double> vz;
    std::vector <double> va;
    for (int i=0; i<npts; i++) {
        if (!inDomain(x[i], y[i], z[i]))
            continue;
        vx.push_back(x[i]);
        vy.push_back(y[i]);
        vz.push_back(z[i]);
        va.push_back(data[i]);
    }

    valid = true;

    BSfit3D F(bsr);

    if (non_negative)
        F.SetConstraintNonNegative();

    if (!grid) {
        F.Fit(va.size(), &vx[0], &vy[0], &vz[0], &va[0]);
        return F.GetResidual();
    } else {
        F.AddData(va.size(), &vx[0], &vy[0], &vz[0], &va[0]);
        F.Fit();
        return F.GetResidual();
    }
}

void LRFxyz::writeJSON(QJsonObject &json) const
{
    json["type"] = QString(type());
    json["xmin"] = xmin;
    json["xmax"] = xmax;
    json["ymin"] = ymin;
    json["ymax"] = ymax;
    json["zmin"] = zmin;
    json["zmax"] = zmax;
    json["xintervals"] = nintx;
    json["yintervals"] = ninty;
    json["zintervals"] = nintz;

    {
        QJsonObject response, spline;
        QJsonArray stack;
        for (int i=0; i<nbasz; i++) {
            write_tpspline3_json(bsr->GetZplane(i), spline);
            response["tpspline3"] = spline;
            stack.append(response);
        }
        json["response"] = stack;
    }
    // TODO: add else {} with a warning
    if (bse) {
        QJsonObject response, spline;
        QJsonArray stack;
        for (int i=0; i<nbasz; i++) {
            write_tpspline3_json(bse->GetZplane(i), spline);
            response["tpspline3"] = spline;
            stack.append(response);
        }
        json["error"] = stack;
    }
    // TODO: add else {} with a warning
}

QJsonObject LRFxyz::reportSettings() const
{
   QJsonObject json;
   json["type"] = QString(type());
   json["n1"] = nintx;
  // json["n2"] = ninty;
   json["n2"] = nintz;

   return json;
}
