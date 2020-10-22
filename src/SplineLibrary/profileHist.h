#ifndef PROFILEHIST_H
#define PROFILEHIST_H

#include <vector>
#include <cmath>

//   H(J)  =  sum Y                  E(J)  =  sum (Y^2)
//   l(J)  =  sum l                  L(J)  =  sum l
//   h(J)  =  H(J)/L(J)                     mean of Y,
//   s(J)  =  sqrt(E(J)/L(J)- h(J)**2)      standard deviation of Y  (e.g. RMS)
//   e(J)  =  s(J)/sqrt(L(J))               standard error on the mean

class PHCell
{
    friend class ProfileHist;
protected:
    double sum;
    double sum2;
    double cnt;
public:
    PHCell() : sum(0.), sum2(0.), cnt(0.) {};   // Andr ! make direct and remove constructor
    void Add(double x) {sum += x; sum2 += x*x; cnt += 1.0;}
    void Add(double x, double w) {sum += x*w; sum2 += x*x*w; cnt += w;}
    double GetEntries() const {return cnt;}
    double GetMean() const {return sum/cnt;}
    double GetSigma() const {return sqrt((sum2-sum*sum/cnt)/cnt);}
};

// Base class, generic 3D implementation
class ProfileHist
{
private:                // Andr ! make all getters const
    int ndim; // dimensionality: 1D, 2D or 3D
    int xdim, ydim, zdim;
    double xmin, ymin, zmin;
    double xmax, ymax, zmax;
    double dx, dy, dz;
    std::vector <PHCell> data;
public:
    ProfileHist(int x_dim, double x_min, double x_max);
    ProfileHist(int x_dim, double x_min, double x_max, int y_dim, double y_min, double y_max);
    ProfileHist(int x_dim, double x_min, double x_max, int y_dim, double y_min, double y_max,
                int z_dim, double z_min, double z_max);
    int LocateX(double x) {return (int)((x-xmin)/dx*xdim);}
    int LocateY(double y) {return ndim>=2 ? (int)((y-ymin)/dy*ydim) : 0;}
    int LocateZ(double z) {return ndim>=3 ? (int)((z-zmin)/dz*zdim) : 0;}
    bool Fill(double x, double t);
    bool Fill(double x, double y, double t);
    bool Fill(double x, double y, double z, double t);  
//    bool FillW(double x, double t, double w);
//    bool FillW(double x, double y, double t, double w);
//    bool FillW(double x, double y, double z, double t, double w); 
    double GetBinCenterX(int ix) {return xmin+(ix+0.5)*dx/xdim;}
    double GetBinCenterY(int iy) {return ndim>=2 ? ymin+(iy+0.5)*dy/ydim : 0.;}
    double GetBinCenterZ(int iz) {return ndim>=3 ? zmin+(iz+0.5)*dz/zdim : 0.;}
    double GetBinEntries(int ix, int iy, int iz);
    double GetBinMean(int ix, int iy, int iz); 
    double GetBinSigma(int ix, int iy, int iz);
    double GetEntries();
};

class ProfileHist1D : public ProfileHist
{
public:  // Andr ! make all getters const
    ProfileHist1D(int x_dim, double x_min, double x_max) : ProfileHist(x_dim, x_min, x_max) {}
    bool Fill(double x, double t) {return ProfileHist::Fill(x, t);} 
    double GetBinEntries(int ix) {return ProfileHist::GetBinEntries(ix, 0, 0);}
    double GetBinMean(int ix) {return ProfileHist::GetBinMean(ix, 0, 0);} 
    double GetBinSigma(int ix) {return ProfileHist::GetBinSigma(ix, 0, 0);} 
};

class ProfileHist2D : public ProfileHist
{
public:  // Andr ! make all getters const
    ProfileHist2D(int x_dim, double x_min, double x_max, int y_dim, double y_min, double y_max) : 
                    ProfileHist(x_dim, x_min, x_max, y_dim, y_min, y_max) {}
    bool Fill(double x, double y, double t) {return ProfileHist::Fill(x, y, t);} 
    double GetBinEntries(int ix, int iy) {return ProfileHist::GetBinEntries(ix, iy, 0);}
    double GetBinMean(int ix, int iy) {return ProfileHist::GetBinMean(ix, iy, 0);} 
    double GetBinSigma(int ix, int iy) {return ProfileHist::GetBinSigma(ix, iy, 0);} 
};

class ProfileHist3D : public ProfileHist
{
public:
    ProfileHist3D(int x_dim, double x_min, double x_max, int y_dim, double y_min, double y_max, 
                  int z_dim, double z_min, double z_max) : ProfileHist(x_dim, x_min, x_max, 
                  y_dim, y_min, y_max, z_dim, z_min, z_max) {}
    bool Fill(double x, double y, double z, double t) {return ProfileHist::Fill(x, y, z, t);} 
};

#endif // !PROFILEHIST_H
