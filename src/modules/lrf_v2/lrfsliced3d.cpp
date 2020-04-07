#include "lrfsliced3d.h"
#include "jsonparser.h"
#include "bsfit123.h"
#include "bspline123d.h"
#include "spline.h"

#include <QJsonObject>

#include <math.h>

LRFsliced3D::LRFsliced3D(double x_min, double x_max, int n_intx, double y_min,
            double y_max, int n_inty, double z_min, double z_max, int n_intz) : LRF2(),
//            xmin(x_min), xmax(x_max), ymin(y_min), ymax(y_max), zmin(z_min), zmax(z_max),
            nintx(n_intx), ninty(n_inty), nintz(n_intz)
{
    xmin = x_min; xmax = x_max;
    ymin = y_min; ymax = y_max;
    zmin = z_min; zmax = z_max;

    bsr.resize(nintz, 0);
    bse.resize(nintz, 0);

    dz = (zmax - zmin)/nintz;   // slice width
    zbot = zmin + dz/2.;        // middle of the bottom slice
    ztop = zmax - dz/2.;        // middle of the top slice
}

LRFsliced3D::LRFsliced3D(QJsonObject &json) : LRF2()
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
    parser.ParseObject("layerz", nintz);

    bsr.resize(nintz, 0);
    bse.resize(nintz, 0);

// read response
    if ( parser.ParseObject("response", resparr) ) {
        QVector <QJsonObject> stack;
        if (parser.ParseArray(resparr, stack))
            if (stack.size() >= nintz)
                for (int i=0; i<nintz; i++) {
                    parser.SetObject(stack[i]);
                    if (parser.ParseObject("tpspline3", splineobj))
                        bsr[i] = read_tpspline3_json(splineobj);
        }

    }

// try to read error
    parser.SetObject(json);
    if ( parser.ParseObject("error", errarr) ) {
        QVector <QJsonObject> stack;
        if (parser.ParseArray(errarr, stack))
            if (stack.size() >= nintz)
                for (int i=0; i<nintz; i++) {
                    parser.SetObject(stack[i]);
                    if (parser.ParseObject("tpspline3", splineobj))
                        bse[i] = read_tpspline3_json(splineobj);
        }

    }

// final touches
    valid = (!parser.GetErrType() && (int)bsr.size()==nintz) ? true : false;
    if (bsr[0]) {
        nintx = bsr[0]->GetNintX();
        ninty = bsr[0]->GetNintY();
    }

    dz = (zmax - zmin)/nintz;   // slice width
    zbot = zmin + dz/2.;        // middle of the bottom slice
    ztop = zmax - dz/2.;        // middle of the top slice
}

LRFsliced3D::~LRFsliced3D()
{
    for (int i=0; i<nintz; i++) {
        if (bsr[i])
            delete bsr[i];
        if (bse[i])
            delete bse[i];
    }
}

bool LRFsliced3D::errorDefined() const
{
    return false;
    //    return bse[0] != NULL;
}

double LRFsliced3D::getRmax() const
{
  return std::max(std::max(fabs(xmin), fabs(ymin)), std::max(fabs(xmax), fabs(ymax)));
}

void LRFsliced3D::get_layers (double z, int *l1, int *l2, double *frac) const
{
    if (z <= zbot)
        *l1 = *l2 = 0;
    else if (z >= ztop)
        *l1 = *l2 = nintz-1;
    else {
        double zrel = (z-zbot)/dz;
        *l1 = (int)zrel;
        *l2 = *l1 + 1;
        *frac = z - *l1;
    }
}

double LRFsliced3D::eval(double x, double y, double z) const
{
// TODO: insert check for bsr with popup on failure
    if (!inDomain(x, y, z))
        return 0.;
    int lr1, lr2;
    double frac;
    get_layers(z, &lr1, &lr2, &frac);
    if (lr1 == lr2)
        return bsr[lr1]->Eval(x, y);
    else
        return (1.-frac)*bsr[lr1]->Eval(x, y) + frac*bsr[lr1]->Eval(x, y);
}

double LRFsliced3D::evalErr(double /*x*/, double /*y*/, double /*z*/) const
{
    return 0.;
}

double LRFsliced3D::evalDrvX(double x, double y, double z) const
{
    // TODO: insert check for bsr with popup on failure
    if (!inDomain(x, y, z))
        return 0.;
    int lr1, lr2;
    double frac;
    get_layers(z, &lr1, &lr2, &frac);
    if (lr1 == lr2)
        return bsr[lr1]->EvalDrvX(x, y);
    else
        return (1.-frac)*bsr[lr1]->EvalDrvX(x, y) + frac*bsr[lr1]->EvalDrvX(x, y);
}

double LRFsliced3D::evalDrvY(double x, double y, double z) const
{
// TODO: insert check for bsr with popup on failure
    if (!inDomain(x, y, z))
        return 0.;
    int lr1, lr2;
    double frac;
    get_layers(z, &lr1, &lr2, &frac);
    if (lr1 == lr2)
        return bsr[lr1]->EvalDrvY(x, y);
    else
        return (1.-frac)*bsr[lr1]->EvalDrvY(x, y) + frac*bsr[lr1]->EvalDrvY(x, y);
}

double LRFsliced3D::eval(double x, double y, double z, double *err) const
{
    if (!inDomain(x, y, z)) {
        *err = 0.;
        return 0.;
    }
    int lr1, lr2;
    double frac;
    get_layers(z, &lr1, &lr2, &frac);
    if (lr1 == lr2)
        *err = bse[lr1]->Eval(x, y);
    else {
        double a = bse[lr1]->Eval(x, y);
        double b = bse[lr2]->Eval(x, y);
        *err = sqrt((1.-frac)*a*a + frac*b*b);
    }
    return eval(x, y, z);
}

double LRFsliced3D::fit(int npts, const double *x, const double *y, const double *z, const double *data, bool grid)
{
    std::vector <std::vector <double> > vvx;
    std::vector <std::vector <double> > vvy;
    std::vector <std::vector <double> > vva;
    vvx.resize(nintz);
    vvy.resize(nintz);
    vva.resize(nintz);

    for (int i=0; i<npts; i++) {
        if (!inDomain(x[i], y[i], z[i]) || z[i] >= zmax)
            continue;
        int iz = (int) ((z[i]-zmin)/dz);
        vvx[iz].push_back(x[i]);
        vvy[iz].push_back(y[i]);
        vva[iz].push_back(data[i]);
    }

    valid = true;
    for (int iz=0; iz < nintz; iz++) {
        bsr[iz] = new Bspline2d(xmin, xmax, nintx, ymin, ymax, ninty);
        if (true) { // make a block where F is local
            BSfit2D F(bsr[iz]);
//            if (non_negative)
//                F.SetConstraintNonNegative();

//            if (top_down)
//                F.SetConstraintTopDown(x0, y0);

            if (!grid) {
                F.Fit(vva[iz].size(), &vvx[iz][0], &vvy[iz][0], &vva[iz][0]);
                return F.GetResidual();
            } else {
                F.AddData(vva[iz].size(), &vvx[iz][0], &vvy[iz][0], &vva[iz][0]);
                F.Fit();
                return F.GetResidual();
            }
        }
    }
    // Andr ! there is no default return!
}

void LRFsliced3D::setSpline(Bspline2d *bs, int iz)
{
    if (iz > nintz) // Andr ? need a warning here?
        return;
    bsr[iz] = bs;
    double amin, amax;
    bs->GetRangeX(&amin, &amax);
    if (amin<xmin) xmin = amin;
    if (amax>xmax) xmax = amax;
    bs->GetRangeY(&amin, &amax);
    if (amin<ymin) ymin = amin;
    if (amax>ymax) ymax = amax;
}

void LRFsliced3D::writeJSON(QJsonObject &json) const
{
    json["type"] = QString(type());
    json["xmin"] = xmin;
    json["xmax"] = xmax;
    json["ymin"] = ymin;
    json["ymax"] = ymax;
    json["zmin"] = zmin;
    json["zmax"] = zmax;
    json["layerz"] = nintz;

    if ((int)bsr.size() >= nintz) {
        QJsonObject response, spline;
        QJsonArray stack;
        for (int i=0; i<nintz; i++) {
            write_tpspline3_json(bsr[i], spline);
            response["tpspline3"] = spline;
            stack.append(response);
        }
        json["response"] = stack;
    }
    // TODO: add else {} with a warning
    if ((int)bse.size() >= nintz && bse[0]) {
        QJsonObject response, spline;
        QJsonArray stack;
        for (int i=0; i<nintz; i++) {
            write_tpspline3_json(bse[i], spline);
            response["tpspline3"] = spline;
            stack.append(response);
        }
        json["error"] = stack;
    }
    // TODO: add else {} with a warning
}

QJsonObject LRFsliced3D::reportSettings() const
{
   QJsonObject json;
   json["type"] = QString(type());
   json["n1"] = nintx;
  // json["n2"] = ninty;
   json["n2"] = nintz;

   return json;
}
