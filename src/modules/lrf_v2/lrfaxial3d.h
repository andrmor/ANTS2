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
    virtual bool isReady () const;
    virtual bool errorDefined() const;

    int getNintZ() const {return nintz;}
    virtual double eval(double x, double y, double z) const;
    virtual double evalErr(double x, double y, double z) const;
    virtual double evalDrvX(double x, double y, double z) const;
    virtual double evalDrvY(double x, double y, double z) const;
    virtual double eval(double x, double y, double z, double *err) const;
    virtual double fit(int npts, const double *x, const double *y, const double *z, const double *data, bool grid);
    virtual double fitError(int npts, const double *x, const double *y, const double *z, const double *data, bool grid);
//    void setSpline(Bspline2d *bs);
    const Bspline2d *getSpline() const;
    virtual const char *type() const { return "Axial3D"; }
    virtual void writeJSON(QJsonObject &json) const;
    virtual QJsonObject reportSettings() const;

    void SetForcedZSlope(int val) {z_slope = val;}

private:
    int nintz;	// intervals
    int z_slope = 0;
    Bspline2d *bs2r = 0; 	// 2D spline
    Bspline2d *bs2e = 0; 	// 2D error spline

};

class LRFcAxial3d : public LRFaxial3d
{
public:
    LRFcAxial3d(double r, int nint, double zmin, double zmax,
                int nintz, double k, double r0, double lam, double x0 = 0., double y0 = 0.);
    LRFcAxial3d(QJsonObject &json);
    virtual const char *type() const { return "ComprAxial3D"; }
    virtual QJsonObject reportSettings() const;
//    virtual void writeJSON(QJsonObject &json) const;
};


#endif // LRFAXIAL3D_H
