#ifndef CURVEFIT_H
#define CURVEFIT_H

#include <QVector>

class Bspline1d;

class CurveFit
{
public:
    CurveFit(double x_min, double x_max, int n_intervals, QVector <double> &x0, QVector <double> &y0);
    ~CurveFit();
    Bspline1d *GetSpline();

    double eval(double x);

private:
    Bspline1d *bs = 0;
};

#endif // CURVEFIT_H
