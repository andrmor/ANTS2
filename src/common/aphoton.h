#ifndef APHOTON
#define APHOTON

class ASimulationStatistics;

class APhoton
{
public:
    double r[3]; //position
    double v[3]; //direction (must be already normalized to unit vector!!!)
    double time; //time stamp
    //double wavelength;
    int waveIndex; //wavelength is always binned during simulations
    int scint_type; //1 - primary //2 - secondary // 0 - undefined
    bool fSkipThisPhoton; //flag to skip this photon due to e.g. direction check

    ASimulationStatistics* SimStat;

    APhoton() {fSkipThisPhoton = false;}
    void CopyFrom(const APhoton* CopyFrom)
      {
        r[0] = CopyFrom->r[0];
        r[1] = CopyFrom->r[1];
        r[2] = CopyFrom->r[2];

        v[0] = CopyFrom->v[0];
        v[1] = CopyFrom->v[1];
        v[2] = CopyFrom->v[2];

        time = CopyFrom->time;
        //wavelength = CopyFrom->wavelength;
        waveIndex = CopyFrom->waveIndex;
        scint_type = CopyFrom->scint_type;
        fSkipThisPhoton = CopyFrom->fSkipThisPhoton;

        SimStat = CopyFrom->SimStat;
      }
};

#endif // APHOTON

