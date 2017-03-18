#ifndef LRFAXIAL_H
#define LRFAXIAL_H

#include "lrf2.h"

class Bspline3;
class QJsonObject;

class LRFaxial : public LRF2
{
public:
    LRFaxial(double r, int nint);
    LRFaxial(QJsonObject &json);
    ~LRFaxial();

    virtual bool inDomain(double x, double y, double z=0.) const;
    virtual bool errorDefined() const { return bse != 0; }
    virtual double getRmax() const { return rmax; }
    virtual double getXmin() const { return -rmax; }
    virtual double getXmax() const { return +rmax; }
    virtual double getYmin() const { return -rmax; }
    virtual double getYmax() const { return +rmax; }
    int getNint() const { return nint; }
    virtual double eval(double x, double y, double z=0.) const;
    virtual double evalErr(double x, double y, double z=0.) const;
    virtual double evalDrvX(double x, double y, double z=0.) const;
    virtual double evalDrvY(double x, double y, double z=0.) const;
    virtual double eval(double x, double y, double z, double *err) const;
    virtual double fit(int npts, const double *x, const double *y, const double *z, const double *data, bool grid);
    double fitRData(int npts, const double *r, const double *data);
    virtual double fitError(int npts, const double *x, const double *y, const double *z, const double *data, bool grid);
    void setSpline(Bspline3 *bs);
    const Bspline3 *getSpline() const;
    virtual const char *type() const { return "Axial"; }
    virtual void writeJSON(QJsonObject &json) const;
    virtual QJsonObject reportSettings() const;

    virtual double compress(double r) const { return r; }
    virtual double comprDev(double r) const;
    void SetFlatTop(bool val) {flattop = val;}

protected:
    double rmax;	// domain
    double rmax2;	// domain
    int nint;		// intervals
    Bspline3 *bsr; 	// spline describing radial dependence
    Bspline3 *bse; 	// spline describing radial error dependence
    bool flattop;   // set to true if you want to have zero derivative at the origin
};

#endif // LRFAXIAL_H
