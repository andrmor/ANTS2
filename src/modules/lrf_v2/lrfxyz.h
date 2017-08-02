#ifndef LRFXYZ_H
#define LRFXYZ_H

#include "lrf3d.h"
#include <vector>

class TPspline3D;

class LRFxyz : public LRF3d
{
public:
    LRFxyz(double x_min, double x_max, int n_intx, double y_min,
           double y_max, int n_inty, double z_min, double z_max, int n_intz);
    LRFxyz(QJsonObject &json);
    ~LRFxyz();

    virtual bool inDomain(double x, double y, double z) const
        {return x>xmin && x<xmax && y>ymin && y<ymax && z>zmin && z<zmax;}
    virtual bool errorDefined() const;
    virtual double getXmin() const {return xmin;}
    virtual double getXmax() const {return xmax;}
    virtual double getYmin() const {return ymin;}
    virtual double getYmax() const {return ymax;}
    virtual double getZmin() const {return zmin;}
    virtual double getZmax() const {return zmax;}
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
//    void setSpline(TPspline3D *bs);
    TPspline3D *getSpline() {return bsr;}
    virtual const char *type() const {return "XYZ";}
    virtual void writeJSON(QJsonObject &json) const;
    virtual QJsonObject reportSettings() const;

private:
    double xmin, xmax; 	// xrange
    double ymin, ymax; 	// yrange
    double zmin, zmax; 	// zrange
    int nintx, ninty, nintz;	// intervals
    int nbasz;          // number of basis splines along Z

    TPspline3D *bsr; 	// 3D spline
    TPspline3D *bse; 	// 3D error spline

    bool non_negative;
};

#endif // LRFXYZ_H
