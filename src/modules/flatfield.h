#ifndef FLATFIELD_H
#define FLATFIELD_H

#include <vector>
#include <QVector>

struct Point {
        double x, y;
        int id;
        int hull;
        bool operator <(const Point &p) const {
                return x < p.x || (x == p.x && y < p.y);
        }
};

struct compare {
    bool operator() (double *one, double *two) {
        return (one[k] < two[k]);
    }
    int k;
};

class FlatField {
    public:
        FlatField(std::vector <double> xx_, std::vector <double> yy_);
           FlatField(); //--//
        ~FlatField();
           void Init(std::vector <double> xx_, std::vector <double> yy_, double Rmax, double Threshold); //--//
        void SetEvents(std::vector <int> plist_, QVector< QVector< float> > *events);
        void SetRmax(double r) {rmax = r;}
        void SetThreshold(double t) {thresh.resize(npmt, t);}
        void SetThreshold(std::vector <double> t) {thresh = t;}

        void rotate(Point &pt, double phi);
        std::vector <double*> make_cream(std::vector <double*> d, int bins, double frac, int i1, int i2);
        void trim (std::vector <double*> &a, double frac, int n);
        void trim (std::vector <double> &a, double frac);
        int rawdump(std::vector <double*> dat, int len, char *fname);
        std::vector <int> sortpmt(double* d);
        std::vector <double> solve4gains(std::vector <double*> matrix, int nc);
        Point solve4pos(std::vector <double*> matrix, double *resnorm);
        double distance(int i, int j);
        double distance(int i, double x, double y);
//        std::vector <int*> find_triads(std::vector <int> pmlist, double R);
        std::vector <int*> find_triads(double R);
           void find_triads();  //--//
        std::vector <int*> find_quads(int upto, double R);
        void prn2 (std::vector <double*> v);
        std::vector <int*> find_rings(int upto, double R);
        std::vector <double*> mkline(int bins, double frac, int i0, int i1, int i2, int i3);
        Point triad_center(int a0, int a1, int a2, std::vector <Point> &shell, Point *rms, Point *guess);
        Point uv_ring(double *d, int *ring);
        std::vector <double> find_gains();
        Point guess_xy(int i, std::vector <int*> rings);

        double cross(const Point &O, const Point &A, const Point &B);
        std::vector <Point> convex_hull(std::vector<Point> P);
        Point PolygonCentroid(std::vector <Point> vertices);

           std::vector <int*> *getTriads() {return &triads;} //--//
           void findRelativeGains(int iTriad, double* gainrat10, double* gainrat20); //--//
    private:
        int npmt;
        int MARK;
        double rmax;
        std::vector <double> xx;
        std::vector <double> yy;
        std::vector <double> thresh;
        std::vector <int> plist;
        int nevt;
        std::vector <double*> data;
        compare comp;

        std::vector <int*> triads; //--//
};

#endif // FLATFIELD_H
