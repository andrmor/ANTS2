#ifndef APHOTON
#define APHOTON

class ASimulationStatistics;

class APhoton
{
public:
    APhoton() : fSkipThisPhoton(false), SimStat(0) {}
    APhoton(double* xyz, double* Vxyz, int waveIndex=0, double time=0) :
        time(time), waveIndex(waveIndex), scint_type(0), fSkipThisPhoton(false), SimStat(0)
    {
      r[0]=xyz[0];  r[1]=xyz[1];  r[2]=xyz[2];
      v[0]=Vxyz[0]; v[1]=Vxyz[1]; v[2]=Vxyz[2];
    }

    double r[3]; //position
    double v[3]; //direction (must be already normalized to unit vector!!!)
    double time; //time stamp
    int waveIndex; //wavelength is always binned during simulations
    int scint_type; //1 - primary //2 - secondary // 0 - undefined
    bool fSkipThisPhoton; //flag to skip this photon due to e.g. direction check

    ASimulationStatistics* SimStat;

    void CopyFrom(const APhoton* CopyFrom)
      {
        r[0] = CopyFrom->r[0];
        r[1] = CopyFrom->r[1];
        r[2] = CopyFrom->r[2];

        v[0] = CopyFrom->v[0];
        v[1] = CopyFrom->v[1];
        v[2] = CopyFrom->v[2];

        time = CopyFrom->time;
        waveIndex = CopyFrom->waveIndex;
        scint_type = CopyFrom->scint_type;
        fSkipThisPhoton = CopyFrom->fSkipThisPhoton;

        SimStat = CopyFrom->SimStat;
      }
};

#endif // APHOTON

