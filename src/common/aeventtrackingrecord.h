#ifndef AEVENTTRACKINGRECORD_H
#define AEVENTTRACKINGRECORD_H

#include <QString>
#include <vector>

class AParticleTrackingRecord;
class TrackHolderClass;
class ATrackBuildOptions;
class TGeoNode;

class ATrackingStepData
{
public:
    ATrackingStepData(float * position, float time, float energy, float depositedEnergy, const QString & process);
    ATrackingStepData(float x, float y, float z, float time, float energy, float depositedEnergy, const QString & process);

    void  logToString(QString & str, int offset) const;

public:
    float   Position[3];
    float   Time;
    float   Energy;
    float   DepositedEnergy;
    QString Process;              //step defining process
    TGeoNode * GeoNode = nullptr;
    std::vector<int> Secondaries; //secondaries created in this step - indexes in the parent record

};

class AParticleTrackingRecord
{
public:
    static AParticleTrackingRecord* create(const QString & Particle);
    static AParticleTrackingRecord* create(); // try to avoid this

    void updatePromisedSecondary(const QString & particle, float startEnergy, float startX, float startY, float startZ, float startTime);
    void addStep(ATrackingStepData * step);

    void addSecondary(AParticleTrackingRecord * sec);
    int  countSecondaries() const;

    const std::vector<ATrackingStepData *> & getSteps() const {return Steps;}
    const AParticleTrackingRecord * getSecondaryOf() const {return SecondaryOf;}
    const std::vector<AParticleTrackingRecord *> & getSecondaries() const {return Secondaries;}

    void logToString(QString & str, int offset, bool bExpandSecondaries) const;
    void makeTrack(std::vector<TrackHolderClass *> & Tracks, const QStringList & ParticleNames, const ATrackBuildOptions & TrackBuildOptions, bool bWithSecondaries) const;

    void updateGeoNodes();

    ~AParticleTrackingRecord();

    // prevent creation on the stack and copy/move
private:
    AParticleTrackingRecord(const QString & particle) : ParticleName(particle) {}

    AParticleTrackingRecord(const AParticleTrackingRecord &) = delete;
    AParticleTrackingRecord & operator=(const AParticleTrackingRecord &) = delete;
    AParticleTrackingRecord(AParticleTrackingRecord &&) = delete;
    AParticleTrackingRecord & operator=(AParticleTrackingRecord &&) = delete;

public:
    //int     ParticleId;                       // ants ID of the particle
    QString ParticleName;

private:
    std::vector<ATrackingStepData *> Steps;   // tracking steps
    AParticleTrackingRecord * SecondaryOf = nullptr;    // 0 means primary
    std::vector<AParticleTrackingRecord *> Secondaries; // vector of secondaries

};

class AEventTrackingRecord
{
public:
    static AEventTrackingRecord* create();
    void   addPrimaryRecord(AParticleTrackingRecord * rec);

    bool   isEmpty() const {return PrimaryParticleRecords.empty();}
    int    countPrimaries() const;

    const std::vector<AParticleTrackingRecord *> & getPrimaryParticleRecords() const {return PrimaryParticleRecords;}

    ~AEventTrackingRecord();

    // prevent creation on the stack and copy/move
private:
    AEventTrackingRecord();

    AEventTrackingRecord(const AEventTrackingRecord &) = delete;
    AEventTrackingRecord & operator=(const AEventTrackingRecord &) = delete;
    AEventTrackingRecord(AEventTrackingRecord &&) = delete;
    AEventTrackingRecord & operator=(AEventTrackingRecord &&) = delete;

private:
    std::vector<AParticleTrackingRecord *> PrimaryParticleRecords;

};


#endif // AEVENTTRACKINGRECORD_H
