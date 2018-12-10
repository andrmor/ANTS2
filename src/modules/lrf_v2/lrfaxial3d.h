#ifndef LRFAXIAL3D_H
#define LRFAXIAL3D_H

#include "lrfaxial.h"

class Bspline2d;

class LRFaxial3d : public LRFaxial
{
public:
    LRFaxial3d(double r, int nint, double zmin, double zmax, int nintz, double x0 = 0., double y0 = 0.);
    LRFaxial3d(QJsonObject &json);
    ~LRFaxial3d();

    virtual bool inDomain(double x, double y, double z) const;
    bool isReady () const;
    virtual bool errorDefined() const;

    int getNintZ() const {return nintz;}
    virtual double eval(double x, double y, double z) const;
    virtual double evalErr(double x, double y, double z=0.) const;
    virtual double evalDrvX(double x, double y, double z) const;
    virtual double evalDrvY(double x, double y, double z) const;
    virtual double eval(double x, double y, double z, double *err) const;
    virtual double fit(int npts, const double *x, const double *y, const double *z, const double *data, bool grid);
    virtual double fitError(int npts, const double *x, const double *y, const double *z, const double *data, bool grid);
    void setSpline(Bspline2d *bs);
    const Bspline2d *getSpline() const;
    virtual const char *type() const { return "Axial3D"; }
    virtual void writeJSON(QJsonObject &json) const;
    virtual QJsonObject reportSettings() const;

    void SetForcedZSlope(int val) {z_slope = val;}

private:
    int nintz;	// intervals
    Bspline2d *bs2r; 	// 2D spline
    Bspline2d *bs2e; 	// 2D error spline
    int z_slope;
};

#endif // LRFAXIAL3D_H
