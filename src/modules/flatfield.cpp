#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include <iostream>

#include <vector>
#include <algorithm>

#include <TVectorD.h>
#include <TMatrixD.h>
#include <TDecompSVD.h>
#include <TMath.h>

#include <QDebug>

#include "flatfield.h"

const float Pi = (float)3.14159265358; //--// const
const float sqrt3 = sqrt(3.0);   //--// const

FlatField::FlatField(std::vector <double> xx_, std::vector <double> yy_):
        xx(xx_), yy(yy_)
{
    npmt = xx.size();
    MARK = npmt;
    SetThreshold(0.);
}

FlatField::FlatField()
{    
    xx.clear();
    yy.clear();
    npmt = 0;
    MARK = 0;
    thresh.clear();
}

FlatField::~FlatField()
{
    //qDebug() << nevt << data.size();
    //for (int i=0; i<nevt; i++)
    for (int i=0; i<(int)data.size(); i++)
      delete[] data[i];  //--// Shouldnt it be delete[]  ? -- yep :)
}

void FlatField::Init(std::vector<double> xx_, std::vector<double> yy_, double Rmax, double Threshold)
{
  xx = xx_;
  yy = yy_;
  npmt = xx.size();
  MARK = npmt;
  rmax = Rmax;
  thresh.resize(npmt, Threshold);
}

void FlatField::SetEvents(std::vector <int> plist_, QVector< QVector< float> > *events)
{
    plist = plist_;
    nevt = events->size();
    for (int i=0; i<nevt; i++) {
        double *d = new double[npmt+1];
        for (int j=0; j<npmt; j++)
            d[j] = events->at(i)[plist[j]];
        d[MARK] = -1;
        data.push_back(d);
    }
}

// 2D cross product of OA and OB vectors, i.e. z-component of their 3D cross product.
// Returns a positive value, if OAB makes a counter-clockwise turn,
// negative for clockwise turn, and zero if the points are collinear.
double FlatField::cross(const Point &O, const Point &A, const Point &B)
{
        return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
}

// Implementation of Andrew's monotone chain 2D convex hull algorithm.
// Asymptotic complexity: O(n log n).
// Taken from http://en.wikibooks.org/wiki/Algorithm_Implementation/Geometry/Convex_hull/Monotone_chain
// Returns a list of points on the convex hull in counter-clockwise order.
// Note: the last point in the returned list is the same as the first one.
std::vector<Point> FlatField::convex_hull(std::vector<Point> P)
{
        int n = P.size(), k = 0;
        std::vector<Point> H(2*n);

        // Sort points lexicographically
        sort(P.begin(), P.end());

        // Build lower hull
        for (int i = 0; i < n; i++) {
                while (k >= 2 && cross(H[k-2], H[k-1], P[i]) <= 0) k--;
                H[k++] = P[i];
        }

        // Build upper hull
        for (int i = n-2, t = k+1; i >= 0; i--) {
                while (k >= t && cross(H[k-2], H[k-1], P[i]) <= 0) k--;
                H[k++] = P[i];
        }

        H.resize(k);
        return H;
}

Point FlatField::PolygonCentroid(std::vector <Point> vertices)
// see http://en.wikipedia.org/wiki/Centroid#Centroid_of_polygon
// adapted from here:
// http://stackoverflow.com/questions/2792443/finding-the-centroid-of-a-polygon
{
    Point centroid;
    double signedArea = 0.0;
    double x0, y0; // Current vertex
    double x1, y1; // Next vertex X
    double a;  	   // Partial signed area

    centroid.x = 0.;
    centroid.y = 0.;

    for (int i=0; i<(int)vertices.size()-1; i++) {
        x0 = vertices[i].x;
        y0 = vertices[i].y;
        x1 = vertices[i+1].x;
        y1 = vertices[i+1].y;
        a = x0*y1 - x1*y0;
        signedArea += a;
        centroid.x += (x0 + x1)*a;
        centroid.y += (y0 + y1)*a;
    }

    signedArea *= 0.5;
    centroid.x /= (6*signedArea);
    centroid.y /= (6*signedArea);

    return centroid;
}

void FlatField::findRelativeGains(int iTriad, double *gainrat10, double *gainrat20)
{
  std::vector <Point> shell;
  std::vector <Point> crap;

  int i0 = triads[iTriad][0];
  int i1 = triads[iTriad][1];
  int i2 = triads[iTriad][2];

  shell.clear();
  Point rms;
  Point qqq = triad_center(i0, i1, i2, crap, &rms, NULL);
  //printf("Triad %d: u=%f+-%f, v=%f+-%f\n", iTriad, qqq.x, rms.x, qqq.y, rms.y);

  qqq = triad_center(i0, i1, i2, shell, &rms, &qqq);
  //printf("Triad %d: u=%f+-%f, v=%f+-%f\n", iTriad, qqq.x, rms.x, qqq.y, rms.y);

  *gainrat10 = exp(qqq.x);
  *gainrat20 = exp(0.5*qqq.x+qqq.y);
//  qDebug()<<"Triad "<<iTriad<< "--> ratios 10 and 20:"<<*gainrat10<<*gainrat20;
}

// rotates (in place !!!) the point by angle phi
void FlatField::rotate(Point &pt, double phi)
{
    double sphi = sin(phi);
    double cphi = cos(phi);
    double x = pt.x;
    double y = pt.y;

    pt.x = cphi*x - sphi*y;
    pt.y = sphi*x + cphi*y;
}

std::vector <double*> FlatField::make_cream(std::vector <double*> d, int bins, double frac, int i1, int i2)
{
    std::vector <double*> cream;

    // sort all according to the first index
    comp.k = i1;
    std::sort(d.begin(), d.end(), comp);

    // now sort each bin separately according to the second index
    comp.k = i2;
    int binsize = d.size()/bins;
//	going backwards so that erase() doesn't mess yet unprocessed vector elements
    for (int i=bins-1; i>=0; i--) {
        std::sort(d.begin() + i*binsize, d.begin() + (i+1)*binsize, comp);
        // leave only the cream
        d.erase (d.begin() + i*binsize, d.begin() + i*binsize + (int)(binsize*(1.-frac)));
    }

    return d;
}

// this one trims the vector removing the distribution tails
// the operation is performed IN PLACE!
void FlatField::trim (std::vector <double*> &a, double frac, int n)
{
    comp.k = n;
    int size = a.size();
    int len_to_trim = (int)(frac*size);

    std::sort(a.begin(), a.end(), comp);
    double minwid = a[size-1][n]-a[0][n];
//	double minwid = *a.end()[n]-*a.begin()[n];
    int imw = 0;
    for (int i=0; i<len_to_trim; i++) {
        float wid = a[size-len_to_trim+i][n]-a[i][n];
        if (wid < minwid) {
            minwid = wid;
            imw = i;
        }
    }
    a.erase(a.end() - len_to_trim + imw, a.end());
    a.erase(a.begin(), a.begin() + imw);
//	return a;
}

// same for vector <double>
void FlatField::trim (std::vector <double> &a, double frac)
{
    int size = a.size();
    int len_to_trim = (int)(frac*size);

    std::sort(a.begin(), a.end());
    double minwid = a[size-1]-a[0];
    int imw = 0;
    for (int i=0; i<len_to_trim; i++) {
        float wid = a[size-len_to_trim+i]-a[i];
        if (wid < minwid) {
            minwid = wid;
            imw = i;
        }
    }
    a.erase(a.end() - len_to_trim + imw, a.end());
    a.erase(a.begin(), a.begin() + imw);
}

int FlatField::rawdump(std::vector <double*> dat, int len, char *fname)
{
    FILE *fp = fopen(fname, "w");
    if (!fp) {
        printf ("Can't open %s for writing\n", fname);
        return -1;
    }

    for (int i=0; i<(int)dat.size(); i++) {
        double *d = dat[i];
        for (int j=0; j<len; j++)
            fprintf(fp, "%f ", d[j]);
        fprintf(fp, "\n");
    }
    fclose(fp);
    return 0;
}

/*
std::vector <int> FlatField::sortpmt(double* d)
{
    std::vector < std::pair <double, int> > snake;
    std::vector <int> out;

    for (int ipmt=0; ipmt<npmt; ipmt++)
        snake.push_back(make_pair(d[i], ipmt));
    std::sort(snake.begin(), snake.end()); // sort by R
    std::reverse(snake.begin(), snake.end());
    for (int ipmt=0; ipmt<npmt; ipmt++)
        out.push_back(snake[ipmt].second);
    return out
}
*/

std::vector <int> FlatField::sortpmt(double* d)
{
    std::vector <double*> pairs;
    std::vector <int> out;

    for (int i=0; i<npmt; i++) {
        double *pair = new double[2];
        pair[0] = i+0.01;
        pair[1] = d[i];
        pairs.push_back(pair);
    }
    comp.k = 1;
    sort(pairs.begin(), pairs.end(), comp);

    // reverse direction to have the biggest value first
    for (int i=npmt-1; i>=0; i--)
        out.push_back((int)pairs[i][0]);

    for (int i=0; i<npmt; i++)
        delete [] pairs[i];

    return out;
}

std::vector <double> FlatField::solve4gains(std::vector <double*> matrix, int nc)
{
    int len = matrix.size();
    TMatrixD A(len+1, nc);
    TVectorD y(len+1);

    for (int i=0; i<len; i++) {
        y(i) = matrix[i][nc];
        for (int k=0; k<nc; k++)
            A(i, k) = matrix[i][k];
    }
// normalization: require average gain to be 1
// TODO: try setting the refernce channel to 1
    y(len) = (double)nc;
    for (int k=0; k<nc; k++)
        A(len, k) = 1.;

    bool success;
    TDecompSVD svd(A);
    const TVectorD c_svd = svd.Solve(y, success);

    std::vector <double> out;

    for (int j=0; j<nc; j++)
        out.push_back(c_svd[j]);

    return out;
}

Point FlatField::solve4pos(std::vector <double*> matrix, double *resnorm)
{
    int len = matrix.size();

    TMatrixD A(len+1, 2);
    TVectorD y(len+1);

    for (int i=0; i<len; i++) {
        y(i) = matrix[i][2];
        A(i, 0) = matrix[i][0];
        A(i, 1) = matrix[i][1];;
    }

    bool success;
    TDecompSVD svd(A);
    const TVectorD c_svd = svd.Solve(y, success);

    Point out;
    out.x = c_svd[0];
    out.y = c_svd[1];

    TVectorD residual = A*c_svd - y;
    *resnorm = sqrt(residual.Norm2Sqr());

    return out;
}

double FlatField::distance(int i, int j)
{
    double dx = xx[i]-xx[j];
    double dy = yy[i]-yy[j];
    return sqrt(dx*dx+dy*dy);
}

double FlatField::distance(int i, double x, double y)
{
    double dx = xx[i]-x;
    double dy = yy[i]-y;
    return sqrt(dx*dx+dy*dy);
}

/*
std::vector <int*> FlatField::find_triads(std::vector <int> pmlist, double R)
{
    std::vector <int*> out;
    int upto = pmlist.size();
    for (int i=0; i<upto; i++)
        for (int j=i+1; j<upto; j++)
            for (int k=j+1; k<upto; k++) {
                int pi = pmlist[i], pj = pmlist[j], pk = pmlist[k];
                if ((distance(pi,pj) < R) && (distance(pi,pk) < R) && (distance(pj,pk) < R)) {
                    int *q = new int[3];
                    q[0] = pi; q[1] = pj; q[2] = pk;
                    out.push_back(q);
                }
    }
    return out;
}
*/
std::vector <int*> FlatField::find_triads(double R)
{
    std::vector <int*> out;
    for (int i=0; i<npmt; i++)
        for (int j=i+1; j<npmt; j++)
            for (int k=j+1; k<npmt; k++) {
                if ((distance(i,j) < R) && (distance(i,k) < R) && (distance(j,k) < R)) {
                    int *q = new int[3];
                    q[0] = i; q[1] = j; q[2] = k;
                    out.push_back(q);
                }
    }
    return out;
}

void FlatField::find_triads()
{
  triads.clear();

  for (int i=0; i<npmt; i++)
    for (int j=i+1; j<npmt; j++)
       for (int k=j+1; k<npmt; k++)
            {
              if ((distance(i,j) < rmax) && (distance(i,k) < rmax) && (distance(j,k) < rmax))
                {
                  int *q = new int[3];
                  q[0] = i; q[1] = j; q[2] = k;
                  triads.push_back(q);
                }
            }
}

std::vector <int*> FlatField::find_quads(int upto, double R)
{
    int quad[4];
    std::vector <int*> out;
    for (int i=0; i<upto; i++)
        for (int j=i+1; j<upto; j++) {
            if (distance(i,j) > R)
                continue;
            int cnt = 2;
            for (int k=0; k<upto; k++)
                if ((k!=i) && (k!=j) && (distance(i,k) < R) && (distance(j,k) < R))
                    quad[cnt++] = k;
            std::cout << i << j << cnt << "\n";
            if (cnt == 4) {
                quad[0] = i;
                quad[1] = j;
                int *q = new int[4];
                for (int n=0; n<4; n++)
                    q[n] = quad[n];
                out.push_back(q);
            }
            printf("out.size=%d\n", out.size());
    }
    return out;
}

void FlatField::prn2 (std::vector <double*> v)
{
    for (int i=0; i<(int)v.size(); i++)
        printf("%f\t%f\n", v[i][0], v[i][1]);
    printf("\n");
}

std::vector <int*> FlatField::find_rings(int upto, double R)
{
    std::vector <int*> out;
    std::vector <int> ring;
    std::vector <double*> tt;
    for (int i=0; i<upto; i++) {
        ring.clear();
        for (int j=0; j<upto; j++)
            if (j!=i && distance(i,j)<R)
                ring.push_back(j);
        if (ring.size() < 6)
                    continue;
        for (int j=0; j<6; j++) {
            int id = (int)ring[j];
            double *t = new double [2];
            t[0] = ring[j]+0.01;
            t[1] = atan2(yy[id]-yy[i], xx[id]-xx[i]);
            tt.push_back(t);
        }

        // sort according to polar angle
        comp.k = 1;
        std::sort(tt.begin(), tt.end(), comp);

        // find the angle closest to 0.
        int jmin = 0; double tmin = fabs(tt[0][1]);
        for (int j=0; j<6; j++) {
            if (fabs(tt[j][1]) < tmin) {
                jmin = j; tmin = fabs(tt[j][1]);
            }
        }

        // rotate so that phi=0 comes first
        std::rotate(tt.begin(), tt.begin()+jmin, tt.end());

        int *r = new int [7];
        r[0] = i;
        for (int j=0; j<6; j++)
            r[j+1] = (int)tt[j][0];

        out.push_back(r);

        // cleanup
        for (int j=0; j<6; j++)
            delete tt[j];
        tt.clear();
    }
    printf("out.size=%d\n", out.size());
    return out;
}

std::vector <double*> FlatField::mkline(int bins, double frac, int i0, int i1, int i2, int i3)
{
    // default values used in LUX: bins = 50, frac = 0.05
    std::vector <double*> line1 = make_cream(data, bins, frac, i0, i1);
    std::vector <double*> line2 = make_cream(data, bins, frac, i1, i0);

    std::vector <double*> out;
    std::vector <int> order;

    for (int i=0; i<(int)line1.size(); i++) {
        order = sortpmt(line1[i]);
        if ((order[0]==i0) || (order[0]==i1) || (order[0]==i2) || (order[0]==i3))
            out.push_back(line1[i]);
    }
    for (int i=0; i<(int)line2.size(); i++) {
        order = sortpmt(line2[i]);
        if ((order[0]==i0) || (order[0]==i1) || (order[0]==i2) || (order[0]==i3))
            out.push_back(line2[i]);
    }
    return out;
}

Point FlatField::triad_center(int a0, int a1, int a2, std::vector <Point> &shell, Point *rms, Point *guess)
{
    // given id's of triad
    // returns (u,v) of its center of symmetry
    std::vector <Point> cloud;
    std::vector <double> xx, yy;

    for (int i=0; i<(int)data.size(); i++)
        data[i][MARK] = 0.;

    for (int j=0; j<20; j++) {
        cloud.clear();
        for (int i=0; i<(int)data.size(); i++) {
            Point p;
            double *d = data[i];
            if (d[a0]>thresh[a0] && d[a1]>thresh[a1] && d[a2]>thresh[a2] && d[MARK] > -0.5) {
                p.x = (log(d[a1])-log(d[a0]));
                p.y = log(d[a2])-0.5*log(d[a0])-0.5*log(d[a1]);
                if (guess) {
                    double x = p.x - guess->x;
                    double y = p.y - guess->y;
                    if (sqrt(x*x + y*y) > 2.)
                        continue;
                }
                p.id = i;
                cloud.push_back(p);
            }
        }

        std::vector <Point> hull = convex_hull(cloud);
        for (int i=0; i<(int)hull.size(); i++) {
            hull[i].hull = j;
            data[hull[i].id][MARK] = -1.;
        }

        Point c = PolygonCentroid(hull);
        xx.push_back(c.x);
        yy.push_back(c.y);

        if (&shell) {
            shell.reserve(shell.size()+hull.size());
            shell.insert( shell.end(), hull.begin(), hull.end() );
        }
    }

    trim(xx, 0.5);
    trim(yy, 0.5);

    Point out;
    out.x = TMath::Mean(xx.begin(), xx.end());
    out.y = TMath::Mean(yy.begin(), yy.end());

    if (rms) {
        rms->x = TMath::RMS(xx.begin(), xx.end());
        rms->y = TMath::RMS(yy.begin(), yy.end());
    }

    return out;
}

Point FlatField::uv_ring(double *d, int *ring) {
    Point p;
    double a[7];
    p.x = p.y = 0.;

    for (int i=0; i<7; i++) {
        a[i] = d[ring[i]];
        if (a[i] <= 0.)
            return p;
    }

    p.x = log(a[1]/a[4])+0.5*log(a[2]/a[3])+0.5*log(a[6]/a[5]);
    p.y = (log(a[2]/a[5])+log(a[3]/a[6]))*sqrt(3.)/2.;

    return p;
}

std::vector <double> FlatField::find_gains()
{
    std::vector <int*> triads = find_triads(rmax);
    qDebug() << "Found " << triads.size() << " triads";
    for (int i=0; i<(int)triads.size(); i++)
        qDebug() << i << ": " << triads[i][0] << ", " << triads[i][1] << ", " << triads[i][2] << ", ";
    std::vector <double*> matrix;
    std::vector <Point> shell;
    std::vector <Point> crap;

    for (int i=0; i<(int)triads.size(); i++) {
        int i0 = triads[i][0];
        int i1 = triads[i][1];
        int i2 = triads[i][2];
        shell.clear();
        Point rms;
        Point qqq = triad_center(i0, i1, i2, crap, &rms, NULL);

        printf("Triad %d: u=%f+-%f, v=%f+-%f\n", i, qqq.x, rms.x, qqq.y, rms.y);

        qqq = triad_center(i0, i1, i2, shell, &rms, &qqq);

        printf("Triad %d: u=%f+-%f, v=%f+-%f\n", i, qqq.x, rms.x, qqq.y, rms.y);

        double rat10 = exp(qqq.x);
        double rat20 = exp(0.5*qqq.x+qqq.y);
        printf("%d:%d:%d\t%f\t%f\n", i0, i1, i2, rat10, rat20);

        double *row = new double [npmt+1](); // zero-initialized
        row[i0] = 1.; row[i1] = -1./rat10; row[npmt] = 0.;
        matrix.push_back(row);
        row = new double [npmt+1]();
        row[i0] = 1.; row[i2] = -1./rat20; row[npmt] = 0.;
        matrix.push_back(row);
    }

    std::vector <double> gain = solve4gains(matrix, npmt);

    for (int i=0; i<npmt; i++)
        qDebug() << "gain " << i << " : " << gain[i];

    return gain;
}

Point FlatField::guess_xy(int i, std::vector <int*> rings)
{
    std::vector <double*> tt;

    for (int j=0; j<(int)rings.size(); j++) {
        Point uv = uv_ring(data[i], rings[j]);
        double r = sqrt(uv.x*uv.x +uv.y*uv.y);
        double phi = atan2(uv.y, uv.x);
        double *t = new double [3];
        t[0] = rings[j][0]+0.01;
        t[1] = r;
        t[2] = phi;
        tt.push_back(t);
    }
    comp.k = 1;
    std::sort(tt.begin(), tt.end(), comp);
    std::reverse(tt.begin(), tt.end());

    std::vector <double*> matrix;
    for (int j=0; j<7; j++) {
        if (tt[j][1] == 0.)
            break;

        double xj = xx[(int)tt[j][0]];
        double yj = yy[(int)tt[j][0]];
        double phij = tt[j][2];
        double sj = sin(phij);
        double cj = cos(phij);

        double *row = new double [3]();
        row[0] = sj; row[1] = -cj;
        row[2] = sj*xj - cj*yj;
        matrix.push_back(row);
    }

//    if (matrix.size() < 2)
//        continue;

    double resnorm;
    return solve4pos(matrix, &resnorm);
}

