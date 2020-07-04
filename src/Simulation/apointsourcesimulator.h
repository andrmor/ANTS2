#ifndef APOINTSOURCESIMULATOR_H
#define APOINTSOURCESIMULATOR_H

#include "asimulator.h"
#include "aphoton.h"

#include "vector"

#include "TString.h"
#include "TVector3.h"

class APhotonSimSettings;
class ANodeRecord;
class AScanRecord;
class TH1D;

class APointSourceSimulator : public ASimulator
{
public:
    explicit APointSourceSimulator(const ASimSettings & SimSet, const DetectorClass & detector, const std::vector<ANodeRecord*> & Nodes, int threadID, int startSeed);
    ~APointSourceSimulator();

    int  getEventCount() const override;
    int  getTotalEventCount() const override {return TotalEvents;}
    int  getEventsDone() const override {return eventCurrent;}
    bool setup() override;   // json already not needed, wait for fix of the particleSim then remove
    void simulate() override;
    void appendToDataHub(EventsDataClass * dataHub) override;

private:
    bool simulateSingle();
    bool simulateRegularGrid();
    bool simulateFlood();
    bool simulateCustomNodes();
    bool simulatePhotonsFromFile();

    void simulateOneNode(const ANodeRecord & node);

    int  getNumPhotToRun();
    void generateAndTracePhotons(AScanRecord * scs, double time0 = 0, int iPoint = 0);
    bool findSecScintBounds(double *r, double & z1, double & z2, double & timeOfDrift, double & driftSpeedInSecScint);
    bool isInsideLimitingObject(const double * r);
    void applySpatialDist(double * center, APhoton & photon) const;

private:
    const APhotonSimSettings & PhotSimSettings;
    const std::vector<ANodeRecord *> & Nodes;

    TH1D *   CustomHist     = nullptr;
    int      NumRuns        = 1;
    bool     bLimitToVolume = false;
    TString  LimitToVolume;
    bool     bIsotropic     = true;
    TVector3 ConeDir;
    double   CosConeAngle   = 0;
    bool     bCone          = false;
    int      TotalEvents    = 0;
    APhoton  Photon;                    //properties of the photon which are used to initiate Photon_Tracker

    const TString SecScintName = "SecScint";
};

// TODO !*! assureNavigatorPresent() of ASimulator - it is already in simulate(), why navigator is sometimes missing? Maybe in setup due to use of another thread?

#endif // APOINTSOURCESIMULATOR_H
