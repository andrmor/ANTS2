#ifndef APOINTSOURCESIMULATOR_H
#define APOINTSOURCESIMULATOR_H

#include "asimulator.h"
#include "aphoton.h"

#include "TString.h"
#include "TVector3.h"

class APhotonSimSettings;
class QJsonObject;
class ANodeRecord;
class AScanRecord;
class TH1D;

class APointSourceSimulator : public ASimulator
{
public:
    explicit APointSourceSimulator(ASimulationManager & simMan, int ID);
    ~APointSourceSimulator();

    int  getEventCount() const override;
    int  getTotalEventCount() const override {return totalEventCount;}
    int  getEventsDone() const override {return eventCurrent;}
    bool setup(QJsonObject & json) override;
    void simulate() override;
    void appendToDataHub(EventsDataClass * dataHub) override;
    void mergeData() override {}

    int  getNumRuns() const {return NumRuns;}   // !*! to remove

private:
    bool simulateSingle();
    bool simulateRegularGrid();
    bool simulateFlood();
    bool simulateCustomNodes();
    bool simulatePhotonsFromFile();

    void simulateOneNode(ANodeRecord & node);
    int  getNumPhotToRun();
    void generateAndTracePhotons(AScanRecord * scs, double time0 = 0, int iPoint = 0);
    bool findSecScintBounds(double *r, double & z1, double & z2, double & timeOfDrift, double & driftSpeedInSecScint);
    bool isInsideLimitingObject(const double * r);

private:
    const APhotonSimSettings & PhotSimSettings;
    TH1D * CustomHist = nullptr; //custom photon generation distribution

    APhoton PhotonOnStart; //properties of the photon which are used to initiate Photon_Tracker

    int totalEventCount = 0;

    int PointSimMode;            // 0-Single 1-Scan 2-Flood
    int ScintType;               // 1 - primary, 2 - secondary
    int NumRuns;                 // multiple runs per node

    const TString SecScintName = "SecScint";

    //bool fOnlyPrimScint; //do not create event outside of prim scintillator
    bool fLimitNodesToObject;
    TString LimitNodesToObject;

    //photons per node info
    //int numPhotMode; // 0-constant, 1-uniform, 2-gauss, 3-custom
    //int numPhotsConst;
    //int numPhotUniMin, numPhotUniMax;
    //double numPhotGaussMean, numPhotGaussSigma;

    //photon direction option
    bool fRandomDirection;
    //direction vector is in PhotonOnStart
    TVector3 ConeDir;
    double CosConeAngle;
    bool fCone;

    //wavelength and time options
    bool fUseGivenWaveIndex;
    double iFixedWaveIndex;
};

#endif // APOINTSOURCESIMULATOR_H
