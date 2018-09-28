#ifndef APHOTONTRACER_H
#define APHOTONTRACER_H

#include "aphotonhistorylog.h"
#include <QVector>
#include "TMathBase.h"

class APhoton;
class TGeoManager;
class AMaterial;
class APmHub;
class AMaterialParticleCollection;
class AOneEvent;
class GeneralSimSettings;
class TGeoNavigator;
class TrackHolderClass;
class TRandom2;
class TGeoVolume;
class AGridElementRecord;

class APhotonTracer
{
public:
    explicit APhotonTracer(TGeoManager* geoManager,
                           TRandom2* RandomGenerator,
                           AMaterialParticleCollection* materialCollection,
                           APmHub* Pms,
                           const QList<AGridElementRecord*>* Grids);
    ~APhotonTracer();

    void UpdateGeoManager(TGeoManager* NewGeoManager) {GeoManager = NewGeoManager;}//will be obsolete with new simulation system soon
    void configure(const GeneralSimSettings *simSet, AOneEvent* oneEvent, bool fBuildTracks, QVector<TrackHolderClass *> *tracks);

    void TracePhoton(const APhoton* Photon);

    AOneEvent* getEvent() {return OneEvent;}  //only used in LRF-based sim

    void setMaxTracks(int maxTracks) {MaxTracks = maxTracks;}

private:    
    TRandom2* RandGen;
    TGeoManager* GeoManager;
    TGeoNavigator *navigator;    
    AMaterialParticleCollection* MaterialCollection;
    APmHub* PMs;
    const QList<AGridElementRecord*>* grids;
    AOneEvent* OneEvent; //PM signals for this event are collected here
    QVector<TrackHolderClass*>* Tracks;
    TrackHolderClass* track;
    QVector<APhotonHistoryLog> PhLog;

    int MaxTracks = 10;
    int PhotonTracksAdded = 0;

    int Counter; //number of photon transitions - there is a limit on this set by user
    APhoton* p; //the photon which is traced
    double rnd; //pre-generated random number for accelerated mode
    double Step;
    Double_t* N; //normal vector to the surface
    bool fHaveNormal;
    bool fMissPM;
    int MatIndexFrom; //material index of the current medium or medium before the interface
    int MatIndexTo;   //material index of the medium after interface
    AMaterial* MaterialFrom; //material before the interface
    AMaterial* MaterialTo;   //material after the interface
    double RefrIndexFrom, RefrIndexTo; //refractive indexes n1 and n2
    bool fDoFresnel; //flag - to perform or not the fresnel calculation on the interface
    bool fBuildTracks;
    const GeneralSimSettings* SimSet;
    bool fGridShiftOn;
    Double_t FromGridCorrection[3]; //add to xyz of the current point in glob coordinates of the grid element to obtain true global point coordinates
    Double_t FromGridElementToGridBulk[3]; //add to xyz of current point for gridnavigator to obtain normal navigator current point coordinates
    TGeoVolume* GridVolume; // the grid bulk

    QString nameFrom;
    QString nameTo;

    enum AbsRayEnum {AbsRayNotTriggered=0, AbsTriggered, RayTriggered, WaveShifted};
    inline AbsRayEnum AbsorptionAndRayleigh();
    inline double CalculateReflectionCoefficient();
    inline void PMwasHit(int PMnumber);
    inline bool PerformRefraction(Double_t nn);
    inline void PerformReflection();
    inline void RandomDir();
    inline bool GridWasHit(int GridNumber);
    inline void ReturnFromGridShift();
    inline void AppendTrack();
    inline void AppendHistoryRecord();
};
#endif // APHOTONTRACER_H
