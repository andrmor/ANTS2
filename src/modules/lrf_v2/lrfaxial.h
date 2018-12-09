#ifndef LRFAXIAL_H
#define LRFAXIAL_H

#include "lrf2.h"
#include <cmath>

class Bspline1d;
class QJsonObject;

class LRFaxial : public LRF2
{
public:
    LRFaxial(double r, int nint, double x0 = 0., double y0 = 0.);
    LRFaxial(QJsonObject &json);
    ~LRFaxial();

    virtual bool inDomain(double x, double y, double z=0.) const;
    bool isReady () const;
//    virtual bool errorDefined() const { return bse != 0; }
    virtual bool errorDefined() const;
    virtual double getRmax() const { return rmax; }
    int getNint() const { return nint; }
    virtual double eval(double x, double y, double z=0.) const;
    virtual double evalErr(double x, double y, double z=0.) const;
    virtual double evalDrvX(double x, double y, double z=0.) const;
    virtual double evalDrvY(double x, double y, double z=0.) const;
    virtual double eval(double x, double y, double z, double *err) const;
    virtual double fit(int npts, const double *x, const double *y, const double *z, const double *data, bool grid);
    double fitRData(int npts, const double *r, const double *data);
    virtual double fitError(int npts, const double *x, const double *y, const double *z, const double *data, bool grid);
    void setSpline(Bspline1d *bs);
    const Bspline1d *getSpline() const;
    virtual const char *type() const { return "Axial"; }
    virtual void writeJSON(QJsonObject &json) const;
    virtual QJsonObject reportSettings() const;

    void SetFlatTop(bool val) {flattop = val;}
    void SetNonNegative(bool val) {non_negative = val;}
    void SetNonIncreasing(bool val) {non_increasing = val;}

// calculation of radius + provision for compression
    double R(double x, double y) const {return sqrt((x-x0)*(x-x0)+(y-y0)*(y-y0));}
    virtual double Rho(double r) const {return r;}
    virtual double Rho(double x, double y) const {return R(x, y);}
    virtual double RhoDrvX(double x, double y) const {return (x-x0)/R(x, y);}
    virtual double RhoDrvY(double x, double y) const {return (y-y0)/R(x, y);}

protected:
    void recalcLimits();

protected:
    double x0, y0;  // center
    double rmax;	// domain
    double rmax2;	// domain
    int nint;		// intervals
    Bspline1d *bsr; 	// spline describing radial dependence
    Bspline1d *bse; 	// spline describing radial error dependence
    bool flattop;   // set to true if you want to have zero derivative at the origin
    bool non_negative;
    bool non_increasing;
};

class LRFcAxial : public LRFaxial
{
public:

    LRFcAxial(double r, int nint,  double k, double r0, double lam, double x0 = 0., double y0 = 0.);
    virtual void writeJSON(QJsonObject &json) const;
    LRFcAxial(QJsonObject &json);
    virtual const char *type() const { return "ComprAxial"; }
    virtual QJsonObject reportSettings() const;

    double getCompr_r0() const { return r0; }
    double getCompr_a() const { return a; }
    double getCompr_b() const { return b; }
    double getCompr_lam2() const { return lam2; }

    virtual double Rho(double r) const;
    virtual double Rho(double x, double y) const;
    virtual double RhoDrvX(double x, double y) const;
    virtual double RhoDrvY(double x, double y) const;

protected:
    double a;
    double b;
    double r0;
    double lam2;

    double comp_k;
    double comp_lam;
};

#endif // LRFAXIAL_H
