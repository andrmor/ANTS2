/*
 * TPspline3M.cpp
 * 
 * Copyright 2013 Vova <vova@lexx>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */

#include "tpspline3d.h"

TPspline3D::TPspline3D(double xmin, double xmax, int n_intx, double ymin, double ymax, int n_inty,
                       double zmin, double zmax, int n_intz) :
                    xl(xmin), yl(ymin), zl(zmin), xr(xmax), yr(ymax), zr(zmax),
                    nintx(n_intx), ninty(n_inty), nintz(n_intz), bsz(zmin, zmax, n_intz),
                    bsxy(xmin, xmax, n_intx, ymin, ymax, n_inty)
{
    dx = xr-xl; dy = yr-yl; dz = zr-zl;
    nboxes = n_intx*n_inty*n_intz;
    nbasxy = (n_intx + 3)*(n_inty + 3);
    nbasz = n_intz + 3;
    nbas = nbasxy*nbasz;

    for (int i=0; i<nbasz; i++) {
        Zplane.push_back(new TPspline3(xmin, xmax, n_intx, ymin, ymax, n_inty));
    }

// construct UCBS matrix
    B <<    1, -3,  3, -1,
            4,  0, -6,  3,
            1,  3,  3, -3,
            0,  0,  0,  1;
    B /= 6.;
}

// for the following basis functions
// nxy = n%nbasxy; nz = n/nbasxy;
double TPspline3D::Basis(double x, double y, double z, int n)
{
    return bsxy.Basis(x, y, n%nbasxy)*bsz.Basis(z, n/nbasxy);
}

double TPspline3D::BasisDrvX(double x, double y, double z, int n)
{
    return bsxy.BasisDrvX(x, y, n%nbasxy)*bsz.Basis(z, n/nbasxy);
}

double TPspline3D::BasisDrvY(double x, double y, double z, int n)
{
    return bsxy.BasisDrvX(x, y, n%nbasxy)*bsz.Basis(z, n/nbasxy);
}

double TPspline3D::BasisDrvZ(double x, double y, double z, int n)
{
    return bsxy.Basis(x, y, n%nbasxy)*bsz.BasisDrv(z, n/nbasxy);
}

double TPspline3D::BasisDrv2XX(double x, double y, double z, int n)
{
    return bsxy.BasisDrv2XX(x, y, n%nbasxy)*bsz.Basis(z, n/nbasxy);
}

double TPspline3D::BasisDrv2YY(double x, double y, double z, int n)
{
    return bsxy.BasisDrv2YY(x, y, n%nbasxy)*bsz.Basis(z, n/nbasxy);
}

double TPspline3D::BasisDrv2ZZ(double x, double y, double z, int n)
{
    return bsxy.Basis(x, y, n%nbasxy)*bsz.BasisDrv2(z, n/nbasxy);
}

double TPspline3D::BasisDrv2XY(double x, double y, double z, int n)
{
    return bsxy.BasisDrv2XY(x, y, n%nbasxy)*bsz.Basis(z, n/nbasxy);
}

double TPspline3D::BasisDrv2XZ(double x, double y, double z, int n)
{
    return bsxy.BasisDrvX(x, y, n%nbasxy)*bsz.BasisDrv(z, n/nbasxy);
}

double TPspline3D::BasisDrv2YZ(double x, double y, double z, int n)
{
    return bsxy.BasisDrvY(x, y, n%nbasxy)*bsz.BasisDrv(z, n/nbasxy);
}

Vector4d TPspline3D::PowerVec(double x) const
{
    Vector4d X;
    double x2 = x*x;
    double x3 = x2*x;
    X << 1., x, x2, x3;
    return X;
}

Vector4d TPspline3D::PowerVecDrv(double x) const
{
    Vector4d X;
    X << 0., 1., x+x, 3*x*x;
    return X;
}

// translate z coordinates into interval number iz and position inside the interval zf
// returns true if inside the domain
bool TPspline3D::LocateZ(double z, int *iz, double *zf) const
{
    double zi = (z-zl)/dz*nintz; *iz = (int)zi;
    if (*iz < 0 || *iz >= nintz)
        return false;

    *zf = zi - *iz;
    return true;
}

double TPspline3D::Eval(double x, double y, double z) const
{
    int iz;
    double zf;
    bool inside = LocateZ(z, &iz, &zf);
    if (!inside)
        return 0.;

    Vector4d xy;
    for (int i=0; i<4; i++)
        xy(i) = Zplane[iz+i]->Eval(x, y);

    return xy.dot(B*PowerVec(zf));
}

double TPspline3D::EvalDrvX(double x, double y, double z)
{
    int iz;
    double zf;
    bool inside = LocateZ(z, &iz, &zf);
    if (!inside)
        return 0.;

    Vector4d xy;
    for (int i=0; i<4; i++)
        xy(i) = Zplane[iz+i]->EvalDrvX(x, y);

    return xy.dot(B*PowerVec(zf));
}

double TPspline3D::EvalDrvY(double x, double y, double z)
{
    int iz;
    double zf;
    bool inside = LocateZ(z, &iz, &zf);
    if (!inside)
        return 0.;

    Vector4d xy;
    for (int i=0; i<4; i++)
        xy(i) = Zplane[iz+i]->EvalDrvY(x, y);

    return xy.dot(B*PowerVec(zf));
}

double TPspline3D::EvalDrvZ(double x, double y, double z)
{
    int iz;
    double zf;
    bool inside = LocateZ(z, &iz, &zf);
    if (!inside)
        return 0.;

    Vector4d xy;
    for (int i=0; i<4; i++)
        xy(i) = Zplane[iz+i]->Eval(x, y);

    return xy.dot(B*PowerVecDrv(zf));
}

void TPspline3D::SetCoef(int iz, double *c)
{
    if (iz>=0 && iz<Zplane.size())
        Zplane[iz]->SetCoef(c);
}

void TPspline3D::SetZplane(int iz, TPspline3 *tps)
{
    if (iz>=0 && iz<Zplane.size()) {
        if (Zplane[iz])
            delete Zplane[iz];
        Zplane[iz]=tps;
    }
}


/*
void TPspline3D::SetCoef(std::vector <double> c)
{
    if ((int)c.size() == nbas)
        SetCoef(&c[0]);
}

void TPspline3D::GetCoef(double *c)
{
	for (int i=0; i<nbas; i++)
        c[i] = C(i%nbasx, i/nbasx);
}

const std::vector<double> &TPspline3D::GetCoef() const
{
    for (int i=0; i<nbas; i++)
        coef[i] = C(i%nbasx, i/nbasx);
    return coef;
}

*/
