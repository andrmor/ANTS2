#include "bspline123d.h"

#ifdef BSIO
#include "json11.hpp"
#endif

// ============== Base class functions ==================
BsplineBase::BsplineBase() {
// construct UCBS matrix
    B <<    1, -3,  3, -1,
            4,  0, -6,  3,
            1,  3,  3, -3,
            0,  0,  0,  1;
    B /= 6.;
}

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
{
    Init(xmin, xmax, n_int);
}

BsplineBasis1d::BsplineBasis1d(const BsplineBasis1d &obj)
{
    Init(obj.xl, obj.xr, obj.nint);
    this->fValid = obj.fValid;
    this->fReady = obj.fReady;
}

void BsplineBasis1d::Init(double xmin, double xmax, int n_int)
{
// bail out if input is not valid, fValid stays false in this case
    if ((xmin == xmax) || (n_int < 1))
        return;

    xl = xmin;
    xr = xmax;
    nint = n_int;
    dx = xr-xl;
    nbas = nint + 3;
    fValid = true;
}

// translate x coordinates into interval index idx and position inside the interval xf
// returns true if spline is valid and x is inside the domain
bool BsplineBasis1d::Locate(double x, int *idx, double *xf) const
{
    if (!fValid)
        return false;
// special case: right edge of the domain (x == xr)
    if (x == xr) {
        *idx = nint - 1;
        *xf = 1.0;
        return true;
    }

    double xi = (x - xl)/dx*nint;
    int _idx = floor(xi);
    if (_idx < 0 || _idx >= nint)
        return false;

    *xf = xi - _idx;
    *idx = _idx;
    return true;
}

double BsplineBasis1d::Basis(double x, int n) const
{
    int ix;
    double xf;
    if (!Locate(x, &ix, &xf) || n<ix || n>ix+3)
        return 0.;

    return PowerVec(xf).dot(B.row(n-ix));
}

std::vector <double> BsplineBasis1d::Basis (std::vector <double> &vx, int n) const
{
	std::vector <double> vf;
	vf.reserve(vx.size());
	for (double x : vx) 
		vf.push_back(Basis(x, n));
		
	return vf;    
}

double BsplineBasis1d::BasisDrv(double x, int n) const
{
    int ix;
    double xf;
    if (!Locate(x, &ix, &xf) || n<ix || n>ix+3)
        return 0.;

    return PowerVecDrv(xf).dot(B.row(n-ix));
}

std::vector <double> BsplineBasis1d::BasisDrv (std::vector <double> &vx, int n) const
{
	std::vector <double> vf;
	vf.reserve(vx.size());
	for (double x : vx) 
		vf.push_back(BasisDrv(x, n));
		
	return vf;    
}

double BsplineBasis1d::BasisDrv2(double x, int n) const
{
    int ix;
    double xf;
    if (!Locate(x, &ix, &xf) || n<ix || n>ix+3)
        return 0.;

    return PowerVecDrv2(xf).dot(B.row(n-ix));
}

// --------------- Bspline1d ----------------


Bspline1d::Bspline1d(double xmin, double xmax, int n_int) : BsplineBasis1d(xmin, xmax, n_int)
{
    Init();
}

Bspline1d::Bspline1d(BsplineBasis1d &base) : BsplineBasis1d(base)
{
    Init();
}

void Bspline1d::Init()
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

    if (!Locate(x, &ix, &xf))
        return 0.;

    return PowerVec(xf).dot(P[ix]);
}

std::vector <double> Bspline1d::Eval (std::vector <double> &vx) const
{
	std::vector <double> vf;
	vf.reserve(vx.size());
	for (double x : vx) 
		vf.push_back(Eval(x));
		
	return vf;
}

double Bspline1d::Eval_compact(double x) const
{
    int ix;
    double xf;

    if (!Locate(x, &ix, &xf))
        return 0.;

    return C.segment(ix,4).dot(B*PowerVec(xf));
}

double Bspline1d::EvalDrv(double x) const
{

    int ix;
    double xf;

    if (!Locate(x, &ix, &xf))
        return 0.;

    return PowerVecDrv(xf).dot(P[ix]);
}

std::vector <double> Bspline1d::EvalDrv (std::vector <double> &vx) const
{
	std::vector <double> vf;
	vf.reserve(vx.size());
	for (double x : vx) 
		vf.push_back(EvalDrv(x));
		
	return vf;
}

bool Bspline1d::SetCoef(std::vector<double> &c)
{
    if (!fValid || (int)c.size() != nbas)
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

// JSON I/O routines
#ifdef BSIO
BsplineBasis1d::BsplineBasis1d(const Json &json)
{
    if (!json["xmin"].is_number() || !json["xmax"].is_number() || !json["intervals"].is_number())
        return;

    double xmin = json["xmin"].number_value();
    double xmax = json["xmax"].number_value();
    double xintervals = json["intervals"].int_value();

    Init(xmin, xmax, xintervals);
}

Bspline1d::Bspline1d(const Json &json) : BsplineBasis1d(json)
{
    if (!json["data"].is_array())
        return;
    Json::array data = json["data"].array_items();

    if (data.size() < nbas)
        return;

    Init();

    std::vector <double> c(nbas, 0.);
    for (int i=0; i<nbas; i++)
        c[i] = data[i].number_value();

    SetCoef(c);
}

Bspline1d::Bspline1d(std::string &json_str) : Bspline1d(Json::parse(json_str, json_err)) {}

void Bspline1d::ToJsonObject(Json::object &json) const
{
    json["xmin"] = xl; // this->GetXmin();
    json["xmax"] = xr; // this->GetXmax();
    json["intervals"] = nint;
    json["data"] = GetCoef();
}

Json::object Bspline1d::GetJsonObject() const
{
    Json::object json;
    ToJsonObject(json);
    return json;
}

std::string Bspline1d::GetJsonString() const
{
    Json::object json;
    ToJsonObject(json);
    return Json(json).dump();
}
#endif

// ============== 2D spline ==================

// ------------ BsplineBasis2d ---------------

BsplineBasis2d::BsplineBasis2d(double xmin, double xmax, int n_intx, double ymin, double ymax, int n_inty)
{
    Init(xmin, xmax, n_intx, ymin, ymax, n_inty);
}

BsplineBasis2d::BsplineBasis2d(const BsplineBasis2d &obj) 
{
    Init(obj.GetXmin(), obj.GetXmax(), obj.nintx, obj.GetYmin(), obj.GetYmax(), obj.ninty);
}

void BsplineBasis2d::Init(double xmin, double xmax, int n_intx, double ymin, double ymax, int n_inty)
{   
// bail out if input is not valid, fValid stays false in this case
    if (xmin == xmax || n_intx < 1 || ymin == ymax || n_inty < 1)
        return;

    nintx = n_intx;
    ninty = n_inty;
    nrec = nintx*ninty;
    nbasx = nintx + 3;
    nbas = (nintx + 3)*(ninty + 3);
    bsx.Init(xmin, xmax, nintx);
    bsy.Init(ymin, ymax, ninty);
    fValid = true;
}

bool BsplineBasis2d::Locate(double x, int *ix, double *xf, double y, int *iy, double *yf) const
{
    return bsx.Locate(x, ix, xf) && bsy.Locate(y, iy, yf);
}

// in the following basis functions: nx = n%nbasx; ny = n/nbasx;
double BsplineBasis2d::Basis(double x, double y, int nx, int ny) const
{
    return bsx.Basis(x, nx)*bsy.Basis(y, ny);
}

std::vector <double> BsplineBasis2d::Basis (std::vector <double> &vx, std::vector <double> &vy, int nx, int ny) const
{
    int len = std::min(vx.size(), vy.size());
	std::vector <double> vf(len);
	for (int i=0; i<len; i++)
		vf[i] = Basis(vx[i], vy[i], nx, ny);
		
	return vf;
}

double BsplineBasis2d::BasisDrvX(double x, double y, int nx, int ny) const
{
    return bsx.BasisDrv(x, nx)*bsy.Basis(y, ny);
}

std::vector <double> BsplineBasis2d::BasisDrvX (std::vector <double> &vx, std::vector <double> &vy, int nx, int ny) const
{
    int len = std::min(vx.size(), vy.size());
	std::vector <double> vf(len);
	for (int i=0; i<len; i++)
		vf[i] = BasisDrvX(vx[i], vy[i], nx, ny);
		
	return vf;
}

double BsplineBasis2d::BasisDrvY(double x, double y, int nx, int ny) const
{
    return bsx.Basis(x, nx)*bsy.BasisDrv(y, ny);
}

std::vector <double> BsplineBasis2d::BasisDrvY (std::vector <double> &vx, std::vector <double> &vy, int nx, int ny) const
{
    int len = std::min(vx.size(), vy.size());
	std::vector <double> vf(len);
	for (int i=0; i<len; i++)
		vf[i] = BasisDrvY(vx[i], vy[i], nx, ny);
		
	return vf;
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
Bspline2d::Bspline2d(double xmin, double xmax, int n_intx, double ymin, double ymax, int n_inty) :
            BsplineBasis2d(xmin, xmax, n_intx, ymin, ymax, n_inty)
{
    Init();
}

Bspline2d::Bspline2d(BsplineBasis2d &base) : BsplineBasis2d(base)
{
    Init();
}

void Bspline2d::Init()
{
    C = MatrixXd::Zero(nintx + 3, ninty + 3);
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
    if (!bsx.Locate(x, &ix, &xf) || !bsy.Locate(y, &iy, &yf))
        return 0.;

    return PowerVec(xf).transpose()*P[ix + iy*nintx]*PowerVec(yf);
}

std::vector <double> Bspline2d::Eval (std::vector <double> &vx, std::vector <double> &vy) const
{
    int len = std::min(vx.size(), vy.size());
	std::vector <double> vf(len);
	for (int i=0; i<len; i++)
		vf[i] = Eval(vx[i], vy[i]);
		
	return vf;
}

// this eval tries to get best speed with reduced shared memory usage
double Bspline2d::Eval_compact(double x, double y) const
{
    int ix, iy;
    double xf, yf;
    if (!bsx.Locate(x, &ix, &xf) || !bsy.Locate(y, &iy, &yf))
        return 0.;

    Vector4d xx = B*PowerVec(xf);
    Vector4d yy = B*PowerVec(yf);

    return xx.transpose()*C.block<4,4>(ix,iy)*yy;
}

double Bspline2d::EvalDrvX(double x, double y) const
{
    int ix, iy;
    double xf, yf;
    if (!bsx.Locate(x, &ix, &xf) || !bsy.Locate(y, &iy, &yf))
        return 0.;

    return PowerVecDrv(xf).transpose()*P[ix + iy*nintx]*PowerVec(yf);
}

std::vector <double> Bspline2d::EvalDrvX (std::vector <double> &vx, std::vector <double> &vy) const
{
    int len = std::min(vx.size(), vy.size());
	std::vector <double> vf(len);
	for (int i=0; i<len; i++)
		vf[i] = EvalDrvX(vx[i], vy[i]);
		
	return vf;
}

double Bspline2d::EvalDrvY(double x, double y) const
{
    int ix, iy;
    double xf, yf;
    if (!bsx.Locate(x, &ix, &xf) || !bsy.Locate(y, &iy, &yf))
        return 0.;

    return PowerVec(xf).transpose()*P[ix + iy*nintx]*PowerVecDrv(yf);
}

std::vector <double> Bspline2d::EvalDrvY (std::vector <double> &vx, std::vector <double> &vy) const
{
    int len = std::min(vx.size(), vy.size());
	std::vector <double> vf(len);
	for (int i=0; i<len; i++)
		vf[i] = EvalDrvY(vx[i], vy[i]);
		
	return vf;
}

bool Bspline2d::SetCoef(std::vector <double> &c)
{
    if (!fValid || (int)c.size() != nbas)
        return false;
    
    for (int i=0; i<nbas; i++)
        C(i%nbasx, i/nbasx) = c[i];

    for (int iy=0; iy<ninty; iy++)
      for (int ix=0; ix<nintx; ix++)
          P[ix + iy*nintx] = B.transpose()*C.block<4,4>(ix,iy)*B;
    fReady = true;
    return true;      
}

std::vector<double> Bspline2d::GetCoef() const
{
    std::vector <double> c(nbas);
    for (int i=0; i<nbas; i++)
        c[i] = C(i%nbasx, i/nbasx);
    return c;
}

// JSON I/O routines
#ifdef BSIO
BsplineBasis2d::BsplineBasis2d(const Json &json)
{
    if (!json["xmin"].is_number() || !json["xmax"].is_number() || !json["xintervals"].is_number())
        return;
    if (!json["ymin"].is_number() || !json["ymax"].is_number() || !json["yintervals"].is_number())
        return;

    double xmin = json["xmin"].number_value();
    double xmax = json["xmax"].number_value();
    int xintervals = json["xintervals"].int_value();
    double ymin = json["ymin"].number_value();
    double ymax = json["ymax"].number_value();
    int yintervals = json["yintervals"].int_value();

    Init(xmin, xmax, xintervals, ymin, ymax, yintervals);
}

Bspline2d::Bspline2d(const Json &json) : BsplineBasis2d(json)
{
    if (!json["data"].is_array())
        return;
    Json::array data = json["data"].array_items();

    if (data.size() < nbas)
        return;

    Init();

    std::vector <double> c(nbas, 0.);
    for (int i=0; i<nbas; i++)
        c[i] = data[i].number_value();

    SetCoef(c);
}

Bspline2d::Bspline2d(std::string &json_str) : Bspline2d(Json::parse(json_str, json_err)) {}

void Bspline2d::ToJsonObject(Json::object &json) const
{
    json["xmin"] = bsx.GetXmin();
    json["xmax"] = bsx.GetXmax();
    json["ymin"] = bsy.GetXmin();
    json["ymax"] = bsy.GetXmax();
    json["xintervals"] = nintx;
    json["yintervals"] = ninty;

    json["data"] = GetCoef();
}

Json::object Bspline2d::GetJsonObject() const
{
    Json::object json;
    ToJsonObject(json);
    return json;
}

std::string Bspline2d::GetJsonString() const
{
    Json::object json;
    ToJsonObject(json);
    return Json(json).dump();
}
#endif


// ================ 3D Spline =================

// ------------ BsplineBasis3d ---------------

BsplineBasis3d::BsplineBasis3d(double xmin, double xmax, int n_intx, double ymin, double ymax, int n_inty,
                       double zmin, double zmax, int n_intz)
{
    Init(xmin, xmax, n_intx, ymin, ymax, n_inty, zmin, zmax, n_intz);
}

BsplineBasis3d::BsplineBasis3d(const BsplineBasis3d &obj) 
{
    Init(obj.GetXmin(), obj.GetXmax(), obj.nintx, obj.GetYmin(), obj.GetYmax(), obj.ninty,
         obj.GetZmin(), obj.GetZmax(), obj.nintz);
}

void BsplineBasis3d::Init(double xmin, double xmax, int n_intx, double ymin, double ymax, int n_inty,
                          double zmin, double zmax, int n_intz)
{
// bail out if input is not valid, fValid stays false in this case
    if (xmin == xmax || n_intx < 1 || ymin == ymax || n_inty < 1 || zmin == zmax || n_intz < 1)
        return;

    nintx = n_intx; ninty = n_inty; nintz = n_intz;
    nboxes = nintx*ninty*nintz;
    nbasxy = (nintx + 3)*(ninty + 3);
    nbasz = nintz + 3;
    nbas = nbasxy*nbasz;
    bsxy.Init(xmin, xmax, nintx, ymin, ymax, ninty);
    bsz.Init(zmin, zmax, nintz);
    fValid = true;
}

// for the following basis functions
// nxy = n%nbasxy; nz = n/nbasxy;
double BsplineBasis3d::Basis(double x, double y, double z, int n) const
{
    return bsxy.Basis(x, y, n%nbasxy)*bsz.Basis(z, n/nbasxy);
}

double BsplineBasis3d::BasisDrvX(double x, double y, double z, int n) const
{
    return bsxy.BasisDrvX(x, y, n%nbasxy)*bsz.Basis(z, n/nbasxy);
}

double BsplineBasis3d::BasisDrvY(double x, double y, double z, int n) const
{
    return bsxy.BasisDrvX(x, y, n%nbasxy)*bsz.Basis(z, n/nbasxy);
}

double BsplineBasis3d::BasisDrvZ(double x, double y, double z, int n) const
{
    return bsxy.Basis(x, y, n%nbasxy)*bsz.BasisDrv(z, n/nbasxy);
}

double BsplineBasis3d::BasisDrv2XX(double x, double y, double z, int n) const
{
    return bsxy.BasisDrv2XX(x, y, n%nbasxy)*bsz.Basis(z, n/nbasxy);
}

double BsplineBasis3d::BasisDrv2YY(double x, double y, double z, int n) const
{
    return bsxy.BasisDrv2YY(x, y, n%nbasxy)*bsz.Basis(z, n/nbasxy);
}

double BsplineBasis3d::BasisDrv2ZZ(double x, double y, double z, int n) const
{
    return bsxy.Basis(x, y, n%nbasxy)*bsz.BasisDrv2(z, n/nbasxy);
}

double BsplineBasis3d::BasisDrv2XY(double x, double y, double z, int n) const
{
    return bsxy.BasisDrv2XY(x, y, n%nbasxy)*bsz.Basis(z, n/nbasxy);
}

double BsplineBasis3d::BasisDrv2XZ(double x, double y, double z, int n) const
{
    return bsxy.BasisDrvX(x, y, n%nbasxy)*bsz.BasisDrv(z, n/nbasxy);
}

double BsplineBasis3d::BasisDrv2YZ(double x, double y, double z, int n) const
{
    return bsxy.BasisDrvY(x, y, n%nbasxy)*bsz.BasisDrv(z, n/nbasxy);
}

// ------------ Bspline3d ---------------
Bspline3d::Bspline3d(double xmin, double xmax, int n_intx, double ymin, double ymax, int n_inty,
                       double zmin, double zmax, int n_intz) : BsplineBasis3d(xmin, xmax, n_intx,
                        ymin, ymax, n_inty, zmin, zmax, n_intz)
{
    Init();
}

Bspline3d::Bspline3d(BsplineBasis3d &base) : BsplineBasis3d(base)
{
    Init();
}

void Bspline3d::Init()
{
    for (int i=0; i<nbasz; i++) {
        Zplane.push_back(new Bspline2d(bsxy.GetXmin(), bsxy.GetXmax(), nintx,
                                       bsxy.GetYmin(), bsxy.GetYmax(), ninty));
    }
}

double Bspline3d::Eval(double x, double y, double z) const
{
    int ix, iy, iz;
    double xf, yf, zf;
    if (!bsxy.Locate(x, &ix, &xf, y, &iy, &yf) || !bsz.Locate(z, &iz, &zf))
        return 0.;       

    Vector4d xy;
    for (int i=0; i<4; i++)
        xy(i) = Zplane[iz+i]->Eval(x, y);

    return xy.dot(B*PowerVec(zf));
}

std::vector <double> Bspline3d::Eval (std::vector <double> &vx, std::vector <double> &vy, std::vector <double> &vz) const
{
    int len = std::min({vx.size(), vy.size(), vz.size()});
	std::vector <double> vf(len);
	for (int i=0; i<len; i++)
		vf[i] = Eval(vx[i], vy[i], vz[i]);
		
	return vf;
}

double Bspline3d::EvalDrvX(double x, double y, double z) const
{
    int ix, iy, iz;
    double xf, yf, zf;
    if (!bsxy.Locate(x, &ix, &xf, y, &iy, &yf) || !bsz.Locate(z, &iz, &zf))
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
    if (!bsxy.Locate(x, &ix, &xf, y, &iy, &yf) || !bsz.Locate(z, &iz, &zf))
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
    if (!bsxy.Locate(x, &ix, &xf, y, &iy, &yf) || !bsz.Locate(z, &iz, &zf))
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

// JSON I/O routines
#ifdef BSIO
BsplineBasis3d::BsplineBasis3d(const Json &json)
{
    if (!json["xmin"].is_number() || !json["xmax"].is_number() || !json["xintervals"].is_number())
        return;
    if (!json["ymin"].is_number() || !json["ymax"].is_number() || !json["yintervals"].is_number())
        return;
    if (!json["zmin"].is_number() || !json["zmax"].is_number() || !json["zintervals"].is_number())
        return;

    double xmin = json["xmin"].number_value();
    double xmax = json["xmax"].number_value();
    int xintervals = json["xintervals"].int_value();
    double ymin = json["ymin"].number_value();
    double ymax = json["ymax"].number_value();
    int yintervals = json["yintervals"].int_value();
    double zmin = json["zmin"].number_value();
    double zmax = json["zmax"].number_value();
    int zintervals = json["zintervals"].int_value();

    Init(xmin, xmax, xintervals, ymin, ymax, yintervals, zmin, zmax, zintervals);
}

Bspline3d::Bspline3d(const Json &json) : BsplineBasis3d(json)
{
    if (!json["data"].is_array())
        return;
    Json::array dataz = json["data"].array_items();

    if (dataz.size() < nbasz)
        return;

    Init();

    for (int i=0; i<nbasz; i++) {
        Json::array dataxy = dataz[i].array_items();
        if (dataxy.size() < nbasxy)
            return;
        std::vector <double> c(nbasxy, 0.);
        for (int j=0; j<nbasxy; j++)
            c[j] = dataxy[j].number_value();

        Zplane[i]->SetCoef(c);
    }
    fReady = true;
}

Bspline3d::Bspline3d(std::string &json_str) : Bspline3d(Json::parse(json_str, json_err)) {}

void Bspline3d::ToJsonObject(Json::object &json) const
{
    json["xmin"] = bsxy.GetXmin();
    json["xmax"] = bsxy.GetXmax();
    json["xintervals"] = nintx;
    json["ymin"] = bsxy.GetYmin();
    json["ymax"] = bsxy.GetYmax();
    json["yintervals"] = ninty;
    json["zmin"] = bsz.GetXmin();
    json["zmax"] = bsz.GetXmax();
    json["zintervals"] = nintz;

    std::vector < std::vector <double> > data;
    for (int i=0; i<nbasz; i++) {
        data.push_back(Zplane[i]->GetCoef());
    }
    json["data"] = data;
}

Json::object Bspline3d::GetJsonObject() const
{
    Json::object json;
    ToJsonObject(json);
    return json;
}

std::string Bspline3d::GetJsonString() const
{
    Json::object json;
    ToJsonObject(json);
    return Json(json).dump();
}
#endif
