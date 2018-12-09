#include "bspline123d.h"

// ============== Base class functions ==================

// return 4-vector of powers of x: Vx = {1, x, x^2, x^3}
Vector4d BsplineBase::PowerVec(double x) const
{
    Vector4d X;
    double x2 = x*x;
    double x3 = x2*x;
    X << 1., x, x2, x3;
    return X;
}

// return 4-vector of 1st derivatives of powers of x: Vx' = {0, 1, 2*x, 3*x^2}
Vector4d BsplineBase::PowerVecDrv(double x) const
{
    Vector4d X;
    X << 0., 1., x+x, 3.*x*x;
    return X;
}

// return 4-vector of 2nd derivatives of powers of x: Vx'' = {0, 0, 2, 6*x}
Vector4d BsplineBase::PowerVecDrv2(double x) const
{
    Vector4d X;
    X << 0., 0., 2, 6.*x;
    return X;
}

// ============== 1D spline ==================

// ------------ BsplineBasis1d ---------------

BsplineBasis1d::BsplineBasis1d(double xmin, double xmax, int n_int)
        : xl(xmin), xr(xmax), nint(n_int)
{
    dx = xr-xl;
    nbas = nint + 3;
    fValid = true;
}

// translate x coordinates into interval number ix and position inside the interval xf
// returns true if inside the domain
bool BsplineBasis1d::Locate(double x, int *ix, double *xf) const
{
    double xi = (x-xl)/dx*nint; *ix = (int)xi;
    if (*ix < 0 || *ix >= nint)
        return false;

    *xf = xi - *ix;
    return true;
}

double BsplineBasis1d::Basis(double x, int n) const
{
    int ix;
    double xf;
    bool inside = Locate(x, &ix, &xf);
    if (!inside || n<ix || n>ix+3)
        return 0.;

    return PowerVec(xf).dot(B.row(n-ix));
}

double BsplineBasis1d::BasisDrv(double x, int n) const
{
    int ix;
    double xf;
    bool inside = Locate(x, &ix, &xf);
    if (!inside || n<ix || n>ix+3)
        return 0.;

    return PowerVecDrv(xf).dot(B.row(n-ix));
}

double BsplineBasis1d::BasisDrv2(double x, int n) const
{
    int ix;
    double xf;
    bool inside = Locate(x, &ix, &xf);
    if (!inside || n<ix || n>ix+3)
        return 0.;

    return PowerVecDrv2(xf).dot(B.row(n-ix));
}

// --------------- Bspline1d ----------------

Bspline1d::Bspline1d(double xmin, double xmax, int n_int) : BsplineBasis1d(xmin, xmax, n_int)
{
    C = VectorXd::Zero(nbas);
    P.resize(nint);
    for (int i=0; i<P.size(); i++)
        P[i].setZero();
}

double Bspline1d::Eval_slow(double x) const
{
	double sum = 0.;
	for (int i=0; i<nbas; i++)
		sum += C(i)*Basis(x, i);
	return sum;	
}

double Bspline1d::Eval(double x) const
{
    int ix;
    double xf;
    bool inside = Locate(x, &ix, &xf);
    if (!inside)
        return 0.;

    return PowerVec(xf).dot(P[ix]);
}

double Bspline1d::Eval_compact(double x) const
{
    int ix;
    double xf;
    bool inside = Locate(x, &ix, &xf);
    if (!inside)
        return 0.;

    return C.segment(ix,4).dot(B*PowerVec(xf));
}

double Bspline1d::EvalDrv(double x) const
{

    int ix;
    double xf;
    bool inside = Locate(x, &ix, &xf);
    if (!inside)
        return 0.;

    return PowerVecDrv(xf).dot(P[ix]);
}

bool Bspline1d::SetCoef(std::vector<double> c)
{
    if ((int)c.size() != nbas)
        return false;

    for (int i=0; i<nbas; i++)
        C(i) = c[i];
   
    Matrix4d Bt = B.transpose();
    for (int i=0; i<nint; i++) {
        P[i] = Bt*C.segment(i,4);
    }
    fReady = true;
    return true;
}

std::vector<double> Bspline1d::GetCoef() const
{
    std::vector <double> c(nbas, 0.);
    for (int i=0; i<nbas; i++)
        c[i] = C(i);
    return c;
}

std::vector<std::vector<double> > Bspline1d::GetPoly() const
{
    std::vector<double> row;
    std::vector<std::vector <double> > p;
    for (int i=0; i<nint; i++) {
        row.clear();
        for (int j=0; j<4; j++)
            row.push_back(P[i](j));
        p.push_back(row);
    }
    return p;
}

// ============== 2D spline ==================

// ------------ BsplineBasis2d ---------------

BsplineBasis2d::BsplineBasis2d(double xmin, double xmax, int n_intx, double ymin, double ymax, int n_inty) :
                    nintx(n_intx), ninty(n_inty), bsx(xmin, xmax, n_intx), bsy(ymin, ymax, n_inty)
{
	nrec = n_intx*n_inty;
	nbasx = n_intx + 3;
	nbas = (n_intx + 3)*(n_inty + 3);
    fValid = true;
}

bool BsplineBasis2d::Locate(double x, int *ix, double *xf, double y, int *iy, double *yf) const
{
    return bsx.Locate(x, ix, xf) && bsy.Locate(y, iy, yf);
}

// for the following basis functions
// nx = n%nbasx; ny = n/nbasx;
double BsplineBasis2d::Basis(double x, double y, int n) const
{
    return bsx.Basis(x, n%nbasx)*bsy.Basis(y, n/nbasx);
}

double BsplineBasis2d::BasisDrvX(double x, double y, int n) const
{
    return bsx.BasisDrv(x, n%nbasx)*bsy.Basis(y, n/nbasx);
}

double BsplineBasis2d::BasisDrvY(double x, double y, int n) const
{
    return bsx.Basis(x, n%nbasx)*bsy.BasisDrv(y, n/nbasx);
}

double BsplineBasis2d::BasisDrv2XX(double x, double y, int n) const
{
    return bsx.BasisDrv2(x, n%nbasx)*bsy.Basis(y, n/nbasx);
}

double BsplineBasis2d::BasisDrv2YY(double x, double y, int n) const
{
    return bsx.Basis(x, n%nbasx)*bsy.BasisDrv2(y, n/nbasx);
}

double BsplineBasis2d::BasisDrv2XY(double x, double y, int n) const
{
    return bsx.BasisDrv(x, n%nbasx)*bsy.BasisDrv(y, n/nbasx);
}

// --------------- Bspline2d ----------------

//Bspline1d::Bspline1d(double xmin, double xmax, int n_int) : BsplineBasis1d(xmin, xmax, n_int)

Bspline2d::Bspline2d(double xmin, double xmax, int n_intx, double ymin, double ymax, int n_inty) :
            BsplineBasis2d(xmin, xmax, n_intx, ymin, ymax, n_inty)
{
    C = MatrixXd::Zero(n_intx + 3, n_inty + 3);
    P.resize(nrec);
    for (int i=0; i<P.size(); i++)
        P[i].setZero();
}

double Bspline2d::Eval_slow(double x, double y) const
{
    double sum = 0;
    for (int i=0; i<nbas; i++)
        sum += C(i%nbasx, i/nbasx)*Basis(x, y, i);
    return sum;
}

double Bspline2d::Eval(double x, double y) const
{
    int ix, iy;
    double xf, yf;
    bool inside = bsx.Locate(x, &ix, &xf) && bsy.Locate(y, &iy, &yf);
    if (!inside)
        return 0.;

    return PowerVec(xf).transpose()*P[ix + iy*nintx]*PowerVec(yf);
}

// this eval tries to get best speed with reduced shared memory usage
double Bspline2d::Eval_compact(double x, double y) const
{
    int ix, iy;
    double xf, yf;
    bool inside = bsx.Locate(x, &ix, &xf) && bsy.Locate(y, &iy, &yf);
    if (!inside)
        return 0.;

    Vector4d xx = B*PowerVec(xf);
    Vector4d yy = B*PowerVec(yf);

    return xx.transpose()*C.block<4,4>(ix,iy)*yy;
}

double Bspline2d::EvalDrvX(double x, double y) const
{
    int ix, iy;
    double xf, yf;
    bool inside = bsx.Locate(x, &ix, &xf) && bsy.Locate(y, &iy, &yf);
    if (!inside)
        return 0.;

    return PowerVecDrv(xf).transpose()*P[ix + iy*nintx]*PowerVec(yf);
}

double Bspline2d::EvalDrvY(double x, double y) const
{
    int ix, iy;
    double xf, yf;
    bool inside = bsx.Locate(x, &ix, &xf) && bsy.Locate(y, &iy, &yf);
    if (!inside)
        return 0.;

    return PowerVec(xf).transpose()*P[ix + iy*nintx]*PowerVecDrv(yf);
}

bool Bspline2d::SetCoef(std::vector <double> c)
{
    if ((int)c.size() != nbas)
        return false;
    
    for (int i=0; i<nbas; i++)
        C(i%nbasx, i/nbasx) = c[i];

    for (int iy=0; iy<ninty; iy++)
      for (int ix=0; ix<nintx; ix++)
          P[ix + iy*nintx] = B.transpose()*C.block<4,4>(ix,iy)*B;
    fReady = true;
    return true;      
}

const std::vector<double> Bspline2d::GetCoef() const
{
    std::vector <double> c(nbas);
    for (int i=0; i<nbas; i++)
        c[i] = C(i%nbasx, i/nbasx);
    return c;
}

// ================ 3D Spline =================

Bspline3d::Bspline3d(double xmin, double xmax, int n_intx, double ymin, double ymax, int n_inty,
                       double zmin, double zmax, int n_intz) :
                    nintx(n_intx), ninty(n_inty), nintz(n_intz),
                    bsxy(xmin, xmax, n_intx, ymin, ymax, n_inty), bsz(zmin, zmax, n_intz)
{
    nboxes = n_intx*n_inty*n_intz;
    nbasxy = (n_intx + 3)*(n_inty + 3);
    nbasz = n_intz + 3;
    nbas = nbasxy*nbasz;

    for (int i=0; i<nbasz; i++) {
        Zplane.push_back(new Bspline2d(xmin, xmax, n_intx, ymin, ymax, n_inty));
    }
}

// for the following basis functions
// nxy = n%nbasxy; nz = n/nbasxy;
double Bspline3d::Basis(double x, double y, double z, int n) const
{
    return bsxy.Basis(x, y, n%nbasxy)*bsz.Basis(z, n/nbasxy);
}

double Bspline3d::BasisDrvX(double x, double y, double z, int n) const
{
    return bsxy.BasisDrvX(x, y, n%nbasxy)*bsz.Basis(z, n/nbasxy);
}

double Bspline3d::BasisDrvY(double x, double y, double z, int n) const
{
    return bsxy.BasisDrvX(x, y, n%nbasxy)*bsz.Basis(z, n/nbasxy);
}

double Bspline3d::BasisDrvZ(double x, double y, double z, int n) const
{
    return bsxy.Basis(x, y, n%nbasxy)*bsz.BasisDrv(z, n/nbasxy);
}

double Bspline3d::BasisDrv2XX(double x, double y, double z, int n) const
{
    return bsxy.BasisDrv2XX(x, y, n%nbasxy)*bsz.Basis(z, n/nbasxy);
}

double Bspline3d::BasisDrv2YY(double x, double y, double z, int n) const
{
    return bsxy.BasisDrv2YY(x, y, n%nbasxy)*bsz.Basis(z, n/nbasxy);
}

double Bspline3d::BasisDrv2ZZ(double x, double y, double z, int n) const
{
    return bsxy.Basis(x, y, n%nbasxy)*bsz.BasisDrv2(z, n/nbasxy);
}

double Bspline3d::BasisDrv2XY(double x, double y, double z, int n) const
{
    return bsxy.BasisDrv2XY(x, y, n%nbasxy)*bsz.Basis(z, n/nbasxy);
}

double Bspline3d::BasisDrv2XZ(double x, double y, double z, int n) const
{
    return bsxy.BasisDrvX(x, y, n%nbasxy)*bsz.BasisDrv(z, n/nbasxy);
}

double Bspline3d::BasisDrv2YZ(double x, double y, double z, int n) const
{
    return bsxy.BasisDrvY(x, y, n%nbasxy)*bsz.BasisDrv(z, n/nbasxy);
}

double Bspline3d::Eval(double x, double y, double z) const
{
    int ix, iy, iz;
    double xf, yf, zf;
    bool inside = bsxy.Locate(x, &ix, &xf, y, &iy, &yf) && bsz.Locate(z, &iz, &zf);
    if (!inside)
        return 0.;       

    Vector4d xy;
    for (int i=0; i<4; i++)
        xy(i) = Zplane[iz+i]->Eval(x, y);

    return xy.dot(B*PowerVec(zf));
}

double Bspline3d::EvalDrvX(double x, double y, double z) const
{
    int ix, iy, iz;
    double xf, yf, zf;
    bool inside = bsxy.Locate(x, &ix, &xf, y, &iy, &yf) && bsz.Locate(z, &iz, &zf);
    if (!inside)
        return 0.;

    Vector4d xy;
    for (int i=0; i<4; i++)
        xy(i) = Zplane[iz+i]->EvalDrvX(x, y);

    return xy.dot(B*PowerVec(zf));
}

double Bspline3d::EvalDrvY(double x, double y, double z) const
{
    int ix, iy, iz;
    double xf, yf, zf;
    bool inside = bsxy.Locate(x, &ix, &xf, y, &iy, &yf) && bsz.Locate(z, &iz, &zf);
    if (!inside)
        return 0.;

    Vector4d xy;
    for (int i=0; i<4; i++)
        xy(i) = Zplane[iz+i]->EvalDrvY(x, y);

    return xy.dot(B*PowerVec(zf));
}

double Bspline3d::EvalDrvZ(double x, double y, double z) const
{
    int ix, iy, iz;
    double xf, yf, zf;
    bool inside = bsxy.Locate(x, &ix, &xf, y, &iy, &yf) && bsz.Locate(z, &iz, &zf);
    if (!inside)
        return 0.;

    Vector4d xy;
    for (int i=0; i<4; i++)
        xy(i) = Zplane[iz+i]->Eval(x, y);

    return xy.dot(B*PowerVecDrv(zf));
}

bool Bspline3d::SetZplaneCoef(int iz, std::vector <double> c)
{
    if (iz<0 || iz>=Zplane.size())
        return false;

    Zplane[iz]->SetCoef(c);
    return true;
}

bool Bspline3d::SetZplane(int iz, Bspline2d *tps)
{
    if (iz<0 || iz>=Zplane.size())
        return false;

    if (Zplane[iz])
        delete Zplane[iz];
    Zplane[iz]=tps;
    return true;
}
