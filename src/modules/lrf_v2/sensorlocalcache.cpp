#include "sensorlocalcache.h"
#include "lrf2.h"
#include "lrfaxial.h"
#include "lrfxy.h"
#include "lrfxyz.h"
#include "lrfcomposite.h"
#include "lrfaxial3d.h"
#include "lrfsliced3d.h"
#include "pmsensor.h"
#include "lrffactory.h"
#include "apositionenergyrecords.h"
#include "alrffitsettings.h"

#include <math.h>

#include <QVector>
#include <QDebug>

#include <TProfile2D.h>
#include <TFile.h>
#include <TGraph.h>

SensorLocalCache::SensorLocalCache(int numGoodEvents, bool fDataRecon, bool fScaleByEnergy, const QVector<AReconRecord*> reconData,
                                   const QVector<AScanRecord*> *scan, const QVector< QVector<float> > *events, ALrfFitSettings *LRFsettings) :
    LRFsettings(LRFsettings),
    numGoodEvents(numGoodEvents), dataSize(0),
    xx(0), minx(1e10), maxx(-1e10),
    yy(0), miny(1e10), maxy(-1e10),
    zz(0), minz(1e10), maxz(-1e10),
    sigsig(0), gains(0), maxr(0), maxr2(0)

{
    //caching pointers to "Good" events, their positions and energies
    const QVector<float> **goodEvents = new const QVector<float>*[numGoodEvents];
    const double **r = new const double*[numGoodEvents];
    double *factors = new double[numGoodEvents];

    bool fEnergyFactors = false; // = fDataRecon && fScaleByEnergy;
    double energy_normalization = 1.0;
    if (fScaleByEnergy)
    {
        if (fDataRecon == 0)                    //0 - Scan, 1 - Reconstr data
        {
            fEnergyFactors = !scan->isEmpty();  //paranoid protection
            if (fEnergyFactors)
            {
                double sumEnergy = 0;
                for (int ievent = 0; ievent < events->size(); ievent++)
                {
                    if (!reconData[ievent]->GoodEvent) continue;
                    sumEnergy += (*scan).at(ievent)->Points.at(0).energy;
                }
                energy_normalization = sumEnergy / numGoodEvents;
            }
        }
        else
            fEnergyFactors = true;
    }

    int i = 0;
    for (int ievent = 0; ievent < events->size(); ievent++)
    {
        if (!reconData[ievent]->GoodEvent) continue;

        if (fDataRecon) r[i] = reconData[ievent]->Points[0].r;
        else            r[i] = (*scan)[ievent]->Points[0].r;

        if (fEnergyFactors)
        {
            if (fDataRecon)
                factors[i] = 1.0 / reconData[ievent]->Points[0].energy;
            else
                factors[i] = energy_normalization / (*scan).at(ievent)->Points.at(0).energy;
        }
        else factors[i] = 1.0;

        goodEvents[i] = &(*events)[ievent];
        i++;
    }

    //qDebug() << "official good events:"<<numGoodEvents << "size of data:" << i;
    this->goodEvents = goodEvents;
    this->r = r;
    this->factors = factors;
    //fFitOnlyLast = false;
}

SensorLocalCache::~SensorLocalCache()
{
    uncacheGroup();
    if(goodEvents) { delete[] goodEvents; goodEvents = 0; }
    if(r) { delete[] r; r = 0; }
    if(factors) { delete[] factors; factors = 0; }
    if(gains) { delete[] gains; gains = 0; }
}

void SensorLocalCache::uncacheGroup()
{
    if(xx) { delete[] xx; xx = 0; }
    if(yy) { delete[] yy; yy = 0; }
    if(zz) { delete[] zz; zz = 0; }
    if(sigsig) { delete[] sigsig; sigsig = 0; }

    minx = 1e10; maxx = -1e10;
    miny = 1e10; maxy = -1e10;
    minz = 1e10; maxz = -1e10;
    maxr = 0; maxr2 = 0;
}

bool SensorLocalCache::cacheGroup(const std::vector<PMsensor> *sensors, bool adjustGains, int igrp_)
{
    //inits
    igrp = igrp_;
    int pmsInGroup = (int)sensors->size();
    dataSize = numGoodEvents*pmsInGroup;
    if (gains) { delete[] gains; gains = 0; }
    uncacheGroup(); //TODO: realloc instead of free/malloc

    //trying to reserve to exact size - we can predict it
    try
    {
        xx = new double[dataSize];
        yy = new double[dataSize];
        zz = new double[dataSize];
        sigsig = new double[dataSize];
    }
    catch(...)
    {
        qWarning()<< "SensorLocalCache::reserve("<<numGoodEvents<<") failed...";
        uncacheGroup();
        return false;
    }
    ///this->numGoodEvents = numGoodEvents;
    //qDebug()<< "Reserved data successfully"<<xx<<yy<<zz<<sigsig;

    //cycle over all sensors in the group
    for (int ipmIndex = 0; ipmIndex < pmsInGroup; ipmIndex++)
    {
        double rloc[3]; //event position in local (of the PM) coordinates
        // for all pms in the group
        const PMsensor *pm = &(*sensors)[ipmIndex];
        int ipm = pm->GetIndex();

        //local coordinates of only good events for ipmIndex pm
        double *x_loc = &xx[ipmIndex*numGoodEvents];
        double *y_loc = &yy[ipmIndex*numGoodEvents];
        double *z_loc = &zz[ipmIndex*numGoodEvents];
        double *sig   = &sigsig[ipmIndex*numGoodEvents];
        for (int ipts = 0; ipts < numGoodEvents; ipts++)
        {
            pm->transform(r[ipts], rloc);

            x_loc[ipts] = rloc[0];
            y_loc[ipts] = rloc[1];
            z_loc[ipts] = rloc[2];
            sig[ipts] = (*goodEvents[ipts])[ipm] * factors[ipts];

            maxr2 = std::max(maxr2, rloc[0]*rloc[0] + rloc[1]*rloc[1]);
            minx = std::min(minx, rloc[0]);
            maxx = std::max(maxx, rloc[0]);
            miny = std::min(miny, rloc[1]);
            maxy = std::max(maxy, rloc[1]);
            minz = std::min(minz, rloc[2]);
            maxz = std::max(maxz, rloc[2]);
        }
    }
    maxr = sqrt(maxr2);
    //qDebug() << "caching of local data done";

    if (adjustGains && pmsInGroup >= 2)
    {
        gains = new double[sensors->size()];
        calcRelativeGains2D((int)sqrt(numGoodEvents/7.), sensors->size());

        for (int ipmIndex = 0; ipmIndex < pmsInGroup; ipmIndex++)
        {
            //fprintf(stderr, "gain %d = %lf\n", (*sensors)[ipmIndex].GetIndex(), gains[ipmIndex]);
            double invgain = 1.0/gains[ipmIndex];
            double *pmsig = &sigsig[numGoodEvents*ipmIndex];
            for (int ipts = 0; ipts < numGoodEvents; ipts++)
                pmsig[ipts] *= invgain;
        }
    }

    return true;
}

///*** does it make a problem for axial lrf?
void SensorLocalCache::calcRelativeGains2D(int ngrid, unsigned int pmsCount)
{
    gains[0] = 1.0;
    char filename[32];
    //qDebug() << "CalcGains: irgp=" << igrp << "; count: " << pmsCount;
    //sprintf(filename, "gains%d.root", igrp);
    TFile *file = new TFile(filename, "recreate");
    TProfile2D hp0("hpgains0", "hpgains0", ngrid, minx, maxx, ngrid, miny, maxy);
    for (int i = 0; i < numGoodEvents; i++)
        hp0.Fill(xx[i], yy[i], sigsig[i]);
    hp0.Write();

    for (unsigned int ipm = 1; ipm < pmsCount; ipm++)
    {
        const double *pmx = &xx[numGoodEvents*ipm];
        const double *pmy = &yy[numGoodEvents*ipm];
        const double *pmsig = &sigsig[numGoodEvents*ipm];

        char histname[32];
        sprintf(histname, "hpgains%d", ipm);
        TProfile2D hp1(histname, histname, ngrid, minx, maxx, ngrid, miny, maxy);
        for (int i = 0; i < numGoodEvents; i++)
            hp1.Fill(pmx[i], pmy[i], pmsig[i]);
        hp1.Write();

        double sumxy = 0.;
        double sumxx = 0.;
        for (int ix = 0; ix < ngrid; ix++)
            for (int iy = 0; iy < ngrid; iy++) {
                int bin0 = hp0.GetBin(ix, iy);
                int bin1 = hp1.GetBin(ix, iy);
                if (hp0.GetBinEntries(bin0) && hp1.GetBinEntries(bin1))  { // must have something in both bins
                    double z0 = hp0.GetBinContent(bin0);
                    sumxy += z0*hp1.GetBinContent(bin1);
                    sumxx += z0*z0;
                }
        }

        if (sumxx == 0) gains[ipm] = 666.0; //Vladimir claims it will be exact 0 if there is no points
        else gains[ipm] = sumxy/sumxx;
    }
    file->Close();
}

/*if (type == 2) { // Polar
    if (!use_compression) {
        LRF_Polar *mylrf = new LRF_Polar(MW->PMs->X(root), MW->PMs->Y(root),
                                           sqrt(maxr2[igrp]), nodesx, nodesy);
        FitGroup(igrp, mylrf, use_grid);
    } else {
        LRF_ComprPol *mylrf = new LRF_ComprPol(MW->PMs->X(root), MW->PMs->Y(root),
                                           sqrt(maxr2[igrp]), nodesx, nodesy, compr_k, compr_r0, compr_lam);
        FitGroup(igrp, mylrf, use_grid);
    }
}*/

LRF2 *SensorLocalCache::fitLRF(LRF2 *lrf) const
{
    //not yet in use! because we need to check if lrf is valid everywhere
    /*if(gains) //If we are trying to adjust gains
    {
        int numSensors = dataSize / numGoodEvents;
        for (int ipmIndex = 0; ipmIndex < numSensors; ipmIndex++)
        {
            //and gains aren't valid
            if(isnan(gains[ipmIndex]) || isinf(gains[ipmIndex]))
            {
                //the LRF is not valid, so skip fitting
                lrf->setValid(false);
                return lrf;
            }
        }
    }*/
    double result = lrf->fit(dataSize, xx, yy, zz, sigsig, LRFsettings->fUseGrid);
    if (result == -1) return 0;
    else
      {
        if (LRFsettings->fFitError)
          {
            result = lrf->fitError(dataSize, xx, yy, zz, sigsig, LRFsettings->fUseGrid);
            if (result == -1) return 0;
          }
      }
    return lrf;
}

LRF2 *SensorLocalCache::mkLRFaxial(int nodes, double *compr) const
{
//    qDebug() << "making axial with rmax:" << maxr;    
    //if (!compr) return (LRFaxial*)fitLRF(new LRFaxial(maxr, nodes));
    //else return (LRFcAxial*)fitLRF(new LRFcAxial(maxr, nodes, compr[0], compr[1], compr[2]));
    LRFaxial* lrf = compr ? new LRFcAxial(maxr, nodes, compr[0], compr[1], compr[2]) : new LRFaxial(maxr, nodes);
    lrf->SetFlatTop(LRFsettings->fForceZeroDeriv);
    lrf->SetNonNegative(LRFsettings->fForceNonNegative);
    lrf->SetNonIncreasing(LRFsettings->fForceNonIncreasingInR);
    return fitLRF(lrf);
}

LRF2 *SensorLocalCache::mkLRFxy(int nodesx, int nodesy) const
{
    LRFxy* lrf = new LRFxy(minx, maxx, nodesx, miny, maxy, nodesy);
    lrf->SetNonNegative(LRFsettings->fForceNonNegative);
    if (LRFsettings->fForceTopDown)
        lrf->SetTopDown(true, 0., 0.);
    return fitLRF(lrf);
}

LRF2 *SensorLocalCache::mkLRFcomposite(int nodesr, int nodesxy, double *compr) const
{  
      LRFaxial *lrf_r;
      if (!compr) lrf_r = new LRFaxial(maxr, nodesr);
      else lrf_r = new LRFcAxial(maxr, nodesr, compr[0], compr[1], compr[2]);

      LRFcomposite *mylrf = new LRFcomposite(lrf_r);
      mylrf->add(new LRFxy(minx, maxx, nodesxy, miny, maxy, nodesxy));
      return fitLRF(mylrf);
}

LRF2 *SensorLocalCache::mkLRFaxial3d(int nodesr, int nodesz, double *compr) const
{
//    return fitLRF(new LRFaxial3d(maxr, nodesr, minz, maxz, nodesz));

    LRFaxial3d* lrf = compr ? new LRFcAxial3d(maxr, nodesr, minz, maxz, nodesz, compr[0], compr[1], compr[2]) :
        new LRFaxial3d(maxr, nodesr, minz, maxz, nodesz);
    lrf->SetFlatTop(LRFsettings->fForceZeroDeriv);
    lrf->SetNonNegative(LRFsettings->fForceNonNegative);
    lrf->SetNonIncreasing(LRFsettings->fForceNonIncreasingInR);
    lrf->SetForcedZSlope(LRFsettings->fForceInZ);
    return fitLRF(lrf);
}

LRF2 *SensorLocalCache::mkLRFsliced3D(int nodesr, int nodesz) const
{
    return fitLRF(new LRFsliced3D(minx, maxx, nodesr, miny, maxy, nodesr, minz, maxz, nodesz));
}

LRF2 *SensorLocalCache::mkLRFxyz(int nodesr, int nodesz) const
{
    LRFxyz* lrf = new LRFxyz(minx, maxx, nodesr, miny, maxy, nodesr, minz, maxz, nodesz);
//    lrf->SetNonNegative(LRFsettings->fForceNonNegative);
    return fitLRF(lrf);
}

void SensorLocalCache::expandDomain(double fraction)
{
    double wid = maxx - minx;
    minx -= wid*fraction; maxx += wid*fraction;
    wid = maxy - miny;
    miny -= wid*fraction; maxy += wid*fraction;
    wid = maxz - minz;
    minz -= wid*fraction; maxz += wid*fraction;
}
