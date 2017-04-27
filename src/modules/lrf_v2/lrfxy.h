#ifndef LRFXY_H
#define LRFXY_H

#include "lrf2.h"

class TPspline3;

class LRFxy : public LRF2
{
public:
    LRFxy(double x_min, double x_max, int n_intx, double y_min, double y_max, int n_inty, bool log = false);
    LRFxy(QJsonObject &json);

    ~LRFxy();
    virtual bool inDomain(double x, double y, double z=0.) const;
    virtual bool errorDefined() const {return bse != 0;}
    virtual double getRmax() const;
    virtual double getXmin() const {return xmin;}
    virtual double getXmax() const {return xmax;}
    virtual double getYmin() const {return ymin;}
    virtual double getYmax() const {return ymax;}
    int getNintX() const {return nintx;}
    int getNintY() const {return ninty;}
    virtual double eval(double x, double y, double z=0.) const;
    virtual double evalErr(double x, double y, double z=0.) const;
    virtual double evalDrvX(double x, double y, double z=0.) const;
    virtual double evalDrvY(double x, double y, double z=0.) const;
    virtual double eval(double x, double y, double z, double *err) const;
    virtual double fit(int npts, const double *x, const double *y, const double *z, const double *data, bool grid);
    void setSpline(TPspline3 *bs, bool log);
    TPspline3 *getSpline() const { return bsr; }
    virtual const char *type() const {return "XY";}
    virtual void writeJSON(QJsonObject &json) const;
    virtual QJsonObject reportSettings() const;
    void SetNonNegative(bool val) {non_negative = val;}

private:
// rectangular domain
    double xmin, xmax; 	// xrange
    double ymin, ymax; 	// yrange
    int nintx, ninty;	// intervals
    TPspline3 *bsr; 	// 2D spline
    TPspline3 *bse; 	// 2D error spline
    bool logscale;	// spline stores logarithm
    bool non_negative;
};

#endif // LRFXY_H
