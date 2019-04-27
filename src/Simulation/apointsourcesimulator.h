#ifndef APOINTSOURCESIMULATOR_H
#define APOINTSOURCESIMULATOR_H

#include "asimulator.h"
#include "aphoton.h"

#include <QJsonObject>

#include "TString.h"
#include "TVector3.h"

class ANodeRecord;
class AScanRecord;
class TH1I; // change to TH1D?

class APointSourceSimulator : public ASimulator
{
public:
    explicit APointSourceSimulator(const DetectorClass *detector, ASimulationManager *simMan, int ID);
    ~APointSourceSimulator();

    virtual int getEventCount() const;
    virtual int getTotalEventCount() const { return totalEventCount; }
    virtual int getEventsDone() const { return eventCurrent; }
    virtual bool setup(QJsonObject & json);
    virtual void simulate();
    virtual void appendToDataHub(EventsDataClass * dataHub);
    virtual void mergeData() override {}

    int getNumRuns() const {return NumRuns;}

private:
    bool SimulateSingle();
    bool SimulateRegularGrid();
    bool SimulateFlood();
    bool SimulateCustomNodes();

    //utilities
    int  PhotonsToRun();
    void GenerateTraceNphotons(AScanRecord * scs, double time0 = 0, int iPoint = 0);
    bool FindSecScintBounds(double *r, double & z1, double & z2, double & timeOfDrift, double & driftSpeedInSecScint);
    void OneNode(ANodeRecord & node);
    bool isInsideLimitingObject(const double * r);
    virtual void ReserveSpace(int expectedNumEvents); //no need

    QJsonObject simOptions;
    TH1I * CustomHist = nullptr; //custom photon generation distribution

    APhoton PhotonOnStart; //properties of the photon which are used to initiate Photon_Tracker

    int totalEventCount = 0;

    int PointSimMode;            // 0-Single 1-Scan 2-Flood
    int ScintType;               // 1 - primary, 2 - secondary
    int NumRuns;                 // multiple runs per node

    TString SecScintName = "SecScint";

    //bool fOnlyPrimScint; //do not create event outside of prim scintillator
    bool fLimitNodesToObject;
    TString LimitNodesToObject;

    //photons per node info
    int numPhotMode; // 0-constant, 1-uniform, 2-gauss, 3-custom
    int numPhotsConst;
    int numPhotUniMin, numPhotUniMax;
    double numPhotGaussMean, numPhotGaussSigma;

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
