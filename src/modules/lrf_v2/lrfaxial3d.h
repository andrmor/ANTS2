#ifndef LRFAXIAL3D_H
#define LRFAXIAL3D_H

#include "lrf3d.h"

class TPspline3;

class LRFaxial3d : public LRF3d
{
public:
    LRFaxial3d(double r, int nint_, double z_min, double z_max, int n_intz, bool log = false);
    LRFaxial3d(QJsonObject &json);
    ~LRFaxial3d();
    virtual bool inDomain(double x, double y, double z) const;
    virtual bool errorDefined() const {return bse != 0;}
    virtual double getRmax() const {return rmax;}
    virtual double getXmin() const {return -getRmax();}
    virtual double getXmax() const {return +getRmax();}
    virtual double getYmin() const {return -getRmax();}
    virtual double getYmax() const {return +getRmax();}
    virtual double getZmin() const {return zmin;}
    virtual double getZmax() const {return zmax;}
    int getNint() const {return nint;}
    int getNintZ() const {return nintz;}
    virtual double eval(double x, double y, double z) const;
    virtual double evalErr(double x, double y, double z=0.) const;
    virtual double evalDrvX(double x, double y, double z) const;
    virtual double evalDrvY(double x, double y, double z) const;
    virtual double eval(double x, double y, double z, double *err) const;
    virtual double fit(int npts, const double *x, const double *y, const double *z, const double *data, bool grid);
    void setSpline(TPspline3 *bs, bool log);
    virtual const char *type() const { return "Axial3D"; }
    virtual void writeJSON(QJsonObject &json) const;
    virtual QJsonObject reportSettings() const;

private:
    double rmax;	// domain
    int nint;		// intervals
    double zmin, zmax; 	// zrange
    int nintz;	// intervals
    TPspline3 *bsr; 	// spline describing radial dependence
    TPspline3 *bse; 	// spline describing radial error dependence
    bool logscale;	// spline stores logarithm
};

#endif // LRFAXIAL3D_H
