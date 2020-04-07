#include "curvefit.h"
#include "bspline123d.h"
#include "bsfit123.h"
#include <QVector>

CurveFit::CurveFit(double x_min, double x_max, int n_intervals, QVector <double> &x0, QVector <double> &y0)
{
    int n_pts = std::min(x0.size(), y0.size());

    std::vector <double> x(n_pts+2), y(n_pts+2), w(n_pts+2);
    x[0] = x_min; y[0] = w[0] = 0.;
    x[n_pts+1] = x_max; y[n_pts+1] = w[n_pts+1] = 0.;
    for (int i=1; i<n_pts+1; i++) {
        x[i] = x0[i-1];
        y[i] = y0[i-1];
        w[i] = 1.;
    }

    bs = new Bspline1d(x_min, x_max, n_intervals);
    BSfit1D F(bs);
    F.Fit(n_pts+2, x.data(), y.data(), w.data());
}

CurveFit::~CurveFit()
{
    delete bs;
}

Bspline1d* CurveFit::GetSpline() const
{
    return bs;
}

double CurveFit::eval(double x)
{
    if (!bs) return 0;

    return bs->Eval(x);
}
