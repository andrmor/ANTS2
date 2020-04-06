#ifndef LRFXY_H
#define LRFXY_H

#include "lrf2.h"

class Bspline2d;

class LRFxy : public LRF2
{
public:
    LRFxy(double x_min, double x_max, int n_intx, double y_min, double y_max, int n_inty);
    LRFxy(QJsonObject &json);

    ~LRFxy();
    virtual bool inDomain(double x, double y, double z=0.) const;
    virtual bool isReady () const;
    virtual bool errorDefined() const;
    virtual double getRmax() const;
    int getNintX() const {return nintx;}
    int getNintY() const {return ninty;}
    virtual double eval(double x, double y, double z=0.) const;
    virtual double evalErr(double x, double y, double z=0.) const;
    virtual double evalDrvX(double x, double y, double z=0.) const;
    virtual double evalDrvY(double x, double y, double z=0.) const;
    virtual double eval(double x, double y, double z, double *err) const;
    virtual double fit(int npts, const double *x, const double *y, const double *z, const double *data, bool grid);
    virtual double fitError(int npts, const double *x, const double *y, const double *z, const double *data, bool grid);
    void setSpline(Bspline2d *bs);
    Bspline2d *getSpline() const { return bsr; }
    virtual const char *type() const {return "XY";}
    virtual void writeJSON(QJsonObject &json) const;
    virtual QJsonObject reportSettings() const;

    void SetNonNegative(bool val) {non_negative = val;}
    void SetTopDown(bool val, double x, double y) {top_down = val; x0 = x; y0 = y;}

private:
// rectangular domain
    int nintx, ninty;	// intervals
    Bspline2d *bsr = 0; 	// 2D spline
    Bspline2d *bse = 0; 	// 2D error spline
    bool non_negative = false;
    double x0 = 0.; // coordinates of the maximum for top-down
    double y0 = 0.;
    bool top_down = false;
};

#endif // LRFXY_H
