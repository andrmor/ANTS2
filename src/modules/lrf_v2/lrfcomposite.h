#ifndef LRFCOMPOSITE_H
#define LRFCOMPOSITE_H

#include "lrf2.h"

#include <vector>

class LRFcomposite : public LRF2
{
public:
    LRFcomposite(LRF2* lrf);
    LRFcomposite(QJsonObject &json);
    ~LRFcomposite();
    bool inDomain(double x, double y, double z=0.) const;
    bool errorDefined() const;
    double getRmax() const;
    double getXmin() const { return xmin; }
    double getXmax() const { return xmax; }
    double getYmin() const { return ymin; }
    double getYmax() const { return ymax; }
    double eval(double x, double y, double z=0.) const;
    double evalErr(double x, double y, double z=0.) const;
    double evalDrvX(double x, double y, double z=0.) const;
    double evalDrvY(double x, double y, double z=0.) const;
    double eval(double x, double y, double z, double *err) const;
    int getCount() const {return lrfdeck.size();}
    void add(LRF2* lrf);
    double fit(int npts, const double *x, const double *y, const double *z, const double *data, bool grid);
    const char *type() const {return "Composite";}
    void writeJSON(QJsonObject &json) const;
    virtual QJsonObject reportSettings() const;
    void setFitAll(bool key) {fit_all = key;}

    std::vector<LRF2*> lrfdeck;

private:
    void init_limits(LRF2 *lrf);
    void update_limits(LRF2 *lrf);

    double xmin, xmax; 	// xrange
    double ymin, ymax; 	// yrange
    bool fit_all;       // if false fit only the last added component
};

#endif // LRFCOMPOSITE_H
