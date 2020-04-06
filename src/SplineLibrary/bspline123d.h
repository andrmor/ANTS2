#ifndef BSPLINE123D_H
#define BSPLINE123D_H

#include <vector>
#include <map>
#include <Eigen/Dense>
// the following is needed to ensure proper alignment of std::vector <Vector4d>
#include <Eigen/StdVector>

// to remove serialization routines and dependence on json11 library
// comment the following line
#define BSIO

using Eigen::MatrixXd;
using Eigen::VectorXd;
using Eigen::Matrix4d;
using Eigen::Vector4d;

#ifdef BSIO
namespace json11 {
    class Json;
}
#endif

class BsplineBase
{
    public:
        BsplineBase() {
        // construct UCBS matrix
            B <<    1, -3,  3, -1,
                    4,  0, -6,  3,
                    1,  3,  3, -3,
                    0,  0,  0,  1;
            B /= 6.;
            fValid = false;
            fReady = false;
        }

        Vector4d PowerVec(double x) const;
        Vector4d PowerVecDrv(double x) const;
        Vector4d PowerVecDrv2(double x) const;
        bool IsReady() const {return fReady;}
        bool isInvalid() const { return !fValid; }
    protected:
        Matrix4d B; // matrix of UCBS coefficients
// flags
        bool fValid; // spline is initialized -- now should be always the case
// "ready" means that the object contains a spline loaded through SetCoef() routine
        bool fReady; // true if C and P are initialized and contain non-zero data

// evaluating not-ready splines always returns zero and, optionally, raises an error

    public:
        EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

// sometimes we need spline object just to calculate basis fuctions
class BsplineBasis1d : public BsplineBase
{
public:
    BsplineBasis1d() {;}
    BsplineBasis1d(double xmin, double xmax, int n_int);
    void Init(double xmin, double xmax, int n_int);
    void SetRange(double xmin, double xmax) {xl = xmin; xr = xmax; dx = xr-xl;}
    void GetRange(double *xmin, double *xmax) const {*xmin = xl;	*xmax = xr;}
    double GetXmin() const {return xl;}
    double GetXmax() const {return xr;}
    int GetNint() const {return nint;}
    int GetNbas() const {return nbas;}
    double Basis(double x, int n) const;
    double BasisDrv(double x, int n) const;
    double BasisDrv2(double x, int n) const;
    bool Locate(double x, int *ix, double *xf) const;
#ifdef BSIO
    BsplineBasis1d(const json11::Json &json);
#endif

protected:
    double xl;
    double xr;
    double dx;
    int nint;	// number of intervals
    int nbas;	// number of basis splines
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

// full spline object stores coefficients and can do evaluaion
class Bspline1d : public BsplineBasis1d
{
    public:
        Bspline1d(double xmin, double xmax, int n_int);
        void Init();
        double Eval(double x) const;
        double Eval_slow(double x) const;
        double Eval_compact(double x) const;        
        double EvalDrv(double x) const;
        bool SetCoef(std::vector<double> c);
        std::vector<double> GetCoef() const;
        double GetCoef(int i) const {return C(i);}
        std::vector<std::vector<double> > GetPoly() const;
#ifdef BSIO
        Bspline1d(const json11::Json &json);
//        void ToJson(json11::Json::object &json);
        void ToJson(std::map<std::string, json11::Json> &json);
        std::string ToString();
#endif

    private:
        VectorXd C; // spline coefficients
        std::vector <Vector4d, Eigen::aligned_allocator<Vector4d> > P; // vector of vectors with polynomial coefficients
    public:
        EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

class BsplineBasis2d : public BsplineBase
{
	public:
        BsplineBasis2d() {;}
        BsplineBasis2d(double xmin, double xmax, int n_intx, double ymin, double ymax, int n_inty);
        void Init(double xmin, double xmax, int n_intx, double ymin, double ymax, int n_inty);
		void SetRangeX(double xmin, double xmax) {bsx.SetRange(xmin, xmax);}
		void SetRangeY(double ymin, double ymax) {bsy.SetRange(ymin, ymax);}
		void GetRangeX(double *xmin, double *xmax) const {bsx.GetRange(xmin, xmax);}
        void GetRangeY(double *ymin, double *ymax) const {bsy.GetRange(ymin, ymax);}
        double GetXmin() const {return bsx.GetXmin();}
        double GetXmax() const {return bsx.GetXmax();}
        double GetYmin() const {return bsy.GetXmin();}
        double GetYmax() const {return bsy.GetXmax();}
        int GetNintX() const {return nintx;}
        int GetNintY() const {return ninty;}
        int GetNbas() const {return nbas;}
		double Basis(double x, double y, int n) const;
        double BasisDrvX(double x, double y, int n) const;
        double BasisDrvY(double x, double y, int n) const;
        double BasisDrv2XX(double x, double y, int n) const;
        double BasisDrv2YY(double x, double y, int n) const;
        double BasisDrv2XY(double x, double y, int n) const;
        const BsplineBasis1d &GetBSX() const {return bsx;}
        const BsplineBasis1d &GetBSY() const {return bsy;}
        bool Locate(double x, int *ix, double *xf, double y, int *iy, double *yf) const;
#ifdef BSIO
        BsplineBasis2d(const json11::Json &json);
#endif

    protected:
        int nintx, ninty;
        BsplineBasis1d bsx, bsy;
		int nrec;	// number of rectangles
		int nbas;	// number of basis splines
		int nbasx;  // columns in spline grid
    public:
        EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

class Bspline2d : public BsplineBasis2d
{
    public:
        Bspline2d(double xmin, double xmax, int n_intx, double ymin, double ymax, int n_inty);
        void Init();
        double Eval_slow(double x, double y) const;
        double Eval(double x, double y) const;
        double Eval_compact(double x, double y) const;
        double EvalDrvX(double x, double y) const;
        double EvalDrvY(double x, double y) const;
        double GetCoef(int i) const {return C(i%nbasx, i/nbasx);}
        bool SetCoef(std::vector <double> c);
        const std::vector <double> GetCoef() const;
#ifdef BSIO
        Bspline2d(const json11::Json &json);
//        void ToJson(json11::Json::object &json);
        void ToJson(std::map<std::string, json11::Json> &json);
        std::string ToString();
#endif

    private:
// this version uses Eigen marices to store spline and polynomial coefficients
// the matrices are organized as follows: X - rows, Y - columns
        MatrixXd C;                 // matrix of coefficients, corresponds to old "coef"
        std::vector <Matrix4d, Eigen::aligned_allocator<Matrix4d> > P; // vector of matrices with polynomial coefficients (old "poly")

    public:
        EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

class BsplineBasis3d : public BsplineBase
{
    public:
        BsplineBasis3d(double xmin, double xmax, int n_intx, double ymin, double ymax, int n_inty,
                       double zmin, double zmax, int n_intz);
        void Init(double xmin, double xmax, int n_intx, double ymin, double ymax, int n_inty,
                  double zmin, double zmax, int n_intz);
        void SetRangeX(double xmin, double xmax) {bsxy.SetRangeX(xmin, xmax);}
        void SetRangeY(double ymin, double ymax) {bsxy.SetRangeY(ymin, ymax);}
        void SetRangeZ(double zmin, double zmax) {bsz.SetRange(zmin, zmax);}
        void GetRangeX(double *xmin, double *xmax) const {bsxy.GetRangeX(xmin, xmax);}
        void GetRangeY(double *ymin, double *ymax) const {bsxy.GetRangeY(ymin, ymax);}
        void GetRangeZ(double *zmin, double *zmax) const {bsz.GetRange(zmin, zmax);}
        double GetXmin() const {return bsxy.GetXmin();}
        double GetXmax() const {return bsxy.GetXmax();}
        double GetYmin() const {return bsxy.GetYmin();}
        double GetYmax() const {return bsxy.GetYmax();}
        double GetZmin() const {return bsz.GetXmin();}
        double GetZmax() const {return bsz.GetXmax();}
        int GetNintX() const {return nintx;}
        int GetNintY() const {return ninty;}
        int GetNintZ() const {return nintz;}
        int GetNbas() const {return nbas;}
        int GetNbasXY() const {return nbasxy;}
        int GetNbasZ() const {return nbasz;}
        double Basis(double x, double y, double z, int n) const;
        double BasisDrvX(double x, double y, double z, int n) const;
        double BasisDrvY(double x, double y, double z, int n) const;
        double BasisDrvZ(double x, double y, double z, int n) const;
        double BasisDrv2XX(double x, double y, double z, int n) const;
        double BasisDrv2YY(double x, double y, double z, int n) const;
        double BasisDrv2ZZ(double x, double y, double z, int n) const;
        double BasisDrv2XY(double x, double y, double z, int n) const;
        double BasisDrv2XZ(double x, double y, double z, int n) const;
        double BasisDrv2YZ(double x, double y, double z, int n) const;
        const BsplineBasis2d &GetBSXY() const {return bsxy;}
        const BsplineBasis1d &GetBSZ() const {return bsz;}
#ifdef BSIO
        BsplineBasis3d(const json11::Json &json);
#endif

    protected:
        int nintx, ninty, nintz;
        BsplineBasis2d bsxy;
        BsplineBasis1d bsz;
        int nboxes;	// number of elementary boxes (formed by intersections of the interval grids on 3 axes)
        int nbas;	// total number of 3D basis splines
        int nbasxy;  // number of 2D basis splines in one XY plane
        int nbasz;
    public:
        EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

class Bspline3d : public BsplineBasis3d
{
	public:
        Bspline3d(double xmin, double xmax, int n_intx, double ymin, double ymax, int n_inty,
                   double zmin, double zmax, int n_intz);
        void Init();
        double Eval(double x, double y, double z) const;
        double Eval_greedy(double x, double y, double z) const;
        double EvalDrvX(double x, double y, double z) const;
        double EvalDrvY(double x, double y, double z) const;
        double EvalDrvZ(double x, double y, double z) const;
        bool SetZplaneCoef(int iz, std::vector <double> c);
        bool SetZplane(int iz, Bspline2d *tps);
        const Bspline2d *GetZplane(int iz) const {return Zplane[iz];}
#ifdef BSIO
        Bspline3d(const json11::Json &json);
        void ToJson(std::map<std::string, json11::Json> &json);
        std::string ToString();
#endif

	private:
        std::vector <Bspline2d*> Zplane; // using 2D splines to do part of the housekeeping

    public:
        EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

#endif /* BSPLINE123D_H */
