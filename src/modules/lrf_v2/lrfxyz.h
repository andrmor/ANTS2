#ifndef LRFXYZ_H
#define LRFXYZ_H

#include "lrf2.h"
#include <vector>

class Bspline3d;

class LRFxyz : public LRF2
{
public:
    LRFxyz(double x_min, double x_max, int n_intx, double y_min,
           double y_max, int n_inty, double z_min, double z_max, int n_intz);
    LRFxyz(QJsonObject &json);
    ~LRFxyz();

    virtual bool inDomain(double x, double y, double z) const;
    virtual bool isReady () const;
    virtual bool errorDefined() const;
    virtual double getRmax() const;
    int getNintX() const {return nintx;}
    int getNintY() const {return ninty;}
    int getNintZ() const {return nintz;}
    virtual double eval(double x, double y, double z) const;
    virtual double evalErr(double x, double y, double z=0.) const;
    virtual double evalDrvX(double x, double y, double z) const;
    virtual double evalDrvY(double x, double y, double z) const;
    virtual double evalDrvZ(double x, double y, double z) const;
    virtual double eval(double x, double y, double z, double *err) const;
    virtual double fit(int npts, const double *x, const double *y, const double *z, const double *data, bool grid);
    virtual double fitError(int npts, const double *x, const double *y, const double *z, const double *data, bool grid);
//    void setSpline(Bspline3d *bs);
    Bspline3d *getSpline() {return bsr;}
    virtual const char *type() const {return "XYZ";}
    virtual void writeJSON(QJsonObject &json) const;
    virtual QJsonObject reportSettings() const;

private:
    int nintx, ninty, nintz;	// intervals
    int nbasz;          // number of basis splines along Z

    Bspline3d *bsr = 0; 	// 3D spline
    Bspline3d *bse = 0; 	// 3D error spline

    bool non_negative = false;
};

#endif // LRFXYZ_H
