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
    double sum = 0.;
    double sum2 = 0.;
    double cnt = 0.;
public:
//    PHCell() : sum(0.), sum2(0.), cnt(0.) {};   // Andr ! make direct and remove constructor
    void Add(double x) {sum += x; sum2 += x*x; cnt += 1.0;}
    void Add(double x, double w) {sum += x*w; sum2 += x*x*w; cnt += w;}
    double GetEntries() const {return cnt;}
    double GetMean() const {return sum/cnt;}
    double GetSigma() const {return sqrt((sum2-sum*sum/cnt)/cnt);}
    void Clear() {sum = 0.; sum2 = 0.; cnt = 0.;}
};

// Base class, generic 3D implementation
class ProfileHist
{
private:
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

    int LocateX(double x) const {return (int)((x-xmin)/dx*xdim);}
    int LocateY(double y) const {return ndim>=2 ? (int)((y-ymin)/dy*ydim) : 0;}
    int LocateZ(double z) const {return ndim>=3 ? (int)((z-zmin)/dz*zdim) : 0;}

    void Clear();
    bool Fill(double x, double t);
    bool Fill(double x, double y, double t);
    bool Fill(double x, double y, double z, double t);  

    int GetNdim() const {return ndim;}
    int GetBinsX() const {return xdim;}
    int GetBinsY() const {return ydim;}
    int GetBinsZ() const {return zdim;}
    int GetBinsTotal() const {return data.size();}

    double GetBinCenterX(int ix) const {return xmin+(ix+0.5)*dx/xdim;}
    double GetBinCenterY(int iy) const {return ndim>=2 ? ymin+(iy+0.5)*dy/ydim : 0.;}
    double GetBinCenterZ(int iz) const {return ndim>=3 ? zmin+(iz+0.5)*dz/zdim : 0.;}
    double GetFlatBinEntries(int i) const {return data.at(i).GetEntries();}
    double GetFlatBinMean(int i) const {return data.at(i).GetMean();}
    double GetFlatBinSigma(int i) const {return data.at(i).GetSigma();}
    int GetFlatIndex(int ix, int iy, int iz) const {return ix + (iy + iz*ydim)*xdim;}
    double GetBinEntries(int ix, int iy, int iz) const {return GetFlatBinEntries(GetFlatIndex(ix, iy, iz));}
    double GetBinMean(int ix, int iy, int iz) const {return GetFlatBinMean(GetFlatIndex(ix, iy, iz));} 
    double GetBinSigma(int ix, int iy, int iz) const {return GetFlatBinSigma(GetFlatIndex(ix, iy, iz));}

    double GetEntries() const;   
};

class ProfileHist1D : public ProfileHist
{
public:
    ProfileHist1D(int x_dim, double x_min, double x_max) : ProfileHist(x_dim, x_min, x_max) {}
    bool Fill(double x, double t) {return ProfileHist::Fill(x, t);} 
    double GetBinEntries(int ix) const {return ProfileHist::GetBinEntries(ix, 0, 0);}
    double GetBinMean(int ix) const {return ProfileHist::GetBinMean(ix, 0, 0);} 
    double GetBinSigma(int ix) const {return ProfileHist::GetBinSigma(ix, 0, 0);} 
};

class ProfileHist2D : public ProfileHist
{
public:
    ProfileHist2D(int x_dim, double x_min, double x_max, int y_dim, double y_min, double y_max) : 
                    ProfileHist(x_dim, x_min, x_max, y_dim, y_min, y_max) {}
    bool Fill(double x, double y, double t) {return ProfileHist::Fill(x, y, t);} 
    double GetBinEntries(int ix, int iy) const {return ProfileHist::GetBinEntries(ix, iy, 0);}
    double GetBinMean(int ix, int iy) const {return ProfileHist::GetBinMean(ix, iy, 0);} 
    double GetBinSigma(int ix, int iy) const {return ProfileHist::GetBinSigma(ix, iy, 0);} 
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
