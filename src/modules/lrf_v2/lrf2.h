#ifndef LRF2_H
#define LRF2_H

class QJsonObject;

class LRF2
{
public:
    LRF2() : valid(false) {}
    virtual ~LRF2() {}
    bool inDomain(double *pos) const {return inDomain(pos[0], pos[1], pos[2]);}
    double eval(double *pos) const {return eval(pos[0], pos[1], pos[2]);}
    double evalErr(double *pos) const {return evalErr(pos[0], pos[1], pos[2]);}
    double evalDrvX(double *pos) const {return evalDrvX(pos[0], pos[1], pos[2]);}
    double evalDrvY(double *pos) const {return evalDrvY(pos[0], pos[1], pos[2]);}
    double eval(double *pos, double *err) const {return eval(pos[0], pos[1], pos[2], err);}
    virtual bool inDomain(double x, double y, double z=0.) const = 0;
    virtual bool errorDefined() const = 0;
    virtual double getRmax() const = 0;
    virtual double getXmin() const {return xmin;}
    virtual double getXmax() const {return xmax;}
    virtual double getYmin() const {return ymin;}
    virtual double getYmax() const {return ymax;}
    virtual double getZmin() const {return zmin;}
    virtual double getZmax() const {return zmax;}
    virtual double eval(double x, double y, double z=0.) const = 0;
    virtual double evalErr(double x, double y, double z=0.) const = 0;
    virtual double evalDrvX(double x, double y, double z=0.) const = 0;
    virtual double evalDrvY(double x, double y, double z=0.) const = 0;
    virtual double eval(double x, double y, double z, double *err) const = 0;
    virtual double fit(int npts, const double *x, const double *y, const double *z, const double *data, bool grid) = 0;
    virtual double fitError(int npts, const double *x, const double *y, const double *z, const double *data, bool grid);
    virtual const char *type() const = 0;
    virtual void writeJSON(QJsonObject &json) const = 0;
    virtual QJsonObject reportSettings() const = 0;
//        virtual int ReadJSON(QJsonObject &json) = 0;
    virtual bool isValid() const { return valid; }

protected:
    bool valid; // indicates if the LRF can be used for reconstruction
    double xmin, xmax; 	// xrange
    double ymin, ymax; 	// yrange
    double zmin, zmax; 	// zrange
};

#endif // LRF2_H
