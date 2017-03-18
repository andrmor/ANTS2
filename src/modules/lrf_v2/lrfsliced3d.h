#ifndef LRFSLICED3D_H
#define LRFSLICED3D_H

#include "lrf3d.h"
#include <vector>

class TPspline3;

class LRFsliced3D : public LRF3d
{
public:
    LRFsliced3D(double x_min, double x_max, int n_intx, double y_min, double y_max, int n_inty,
                double z_min, double z_max, int n_intz, bool log = false);
    LRFsliced3D(QJsonObject &json);
    ~LRFsliced3D();

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
    double getSliceMedianZ(int iz) {return zbot+dz*iz;}
    virtual double eval(double x, double y, double z) const;
    virtual double evalErr(double x, double y, double z=0.) const;
    virtual double evalDrvX(double x, double y, double z) const;
    virtual double evalDrvY(double x, double y, double z) const;
    virtual double eval(double x, double y, double z, double *err) const;
    virtual double fit(int npts, const double *x, const double *y, const double *z, const double *data, bool grid);
    void setSpline(TPspline3 *bs, int iz);
    TPspline3 *getSpline(int iz) {if (iz<nintz) return bsr[iz]; else return 0;}
    std::vector <TPspline3*> getSpline() const {return bsr;}
    virtual const char *type() const {return "Sliced3D";}
    virtual void writeJSON(QJsonObject &json) const;
    virtual QJsonObject reportSettings() const;

private:
    double xmin, xmax; 	// xrange
    double ymin, ymax; 	// yrange
    double zmin, zmax; 	// zrange
    int nintx, ninty;	// intervals
    int nintz;	// slices
    float dz, zbot, ztop; // needed for interpolation
    std::vector <TPspline3*> bsr; 	// vector of 2D splines
    std::vector <TPspline3*> bse; 	// vector of 2D error splines
    bool logscale;	// spline stores logarithm

    void get_layers (double z, int *l1, int *l2, double *frac) const;
};


#endif // LRFSLICED3D_H
