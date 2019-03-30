#ifndef AEVENTTRACKINGRECORD_H
#define AEVENTTRACKINGRECORD_H

#include <QString>
#include <vector>

class AParticleTrackingRecord;
class TrackHolderClass;
class ATrackBuildOptions;

class ATrackingStepData
{
public:
    ATrackingStepData(float * position, float time, float depositedEnergy, const QString & process);
    ATrackingStepData(float x, float y, float z, float time, float depositedEnergy, const QString & process);

    void  addSecondary(AParticleTrackingRecord * sec);
    const std::vector<AParticleTrackingRecord *> & getSecondaries() {return Secondaries;}

    int   countSecondaries() const;
    void  logToString(QString & str, int offset) const;

public:
    float   Position[3];
    float   Time;
    float   DepositedEnergy;
    QString Process;          //step defining process
private:
    std::vector<AParticleTrackingRecord *> Secondaries; //secondaries created in this step - does not own

};

class AParticleTrackingRecord
{
public:
    static AParticleTrackingRecord* create(int ParticleId, double StartEnergy, double * StartPosition, double Time);
    static AParticleTrackingRecord* create(int ParticleId, float  StartEnergy, float  * StartPosition, float  Time);
    static AParticleTrackingRecord* create(int    ParticleId,
                                           float  StartEnergy,
                                           float  StartX,
                                           float  StartY,
                                           float  StartZ,
                                           float  Time);
    static AParticleTrackingRecord* create(); // avoid if possible: empty record -> ParticleId will be set to -1

    void update(int particleId, float  startEnergy, float  startX, float  startY, float  startZ, float  time);
    void addStep(ATrackingStepData * step);

    void addSecondary(AParticleTrackingRecord * sec);
    int  countSecondaries() const;

    const std::vector<ATrackingStepData *> & getSteps() const {return Steps;}
    const AParticleTrackingRecord * getSecondaryOf() const {return SecondaryOf;}
    const std::vector<AParticleTrackingRecord *> & getSecondaries() const {return Secondaries;}

    void logToString(QString & str, int offset, const QStringList & ParticleNames, bool bExpandSecondaries) const;
    void makeTrack(std::vector<TrackHolderClass *> & Tracks, const ATrackBuildOptions & TrackBuildOptions, bool bWithSecondaries) const;

    ~AParticleTrackingRecord();

    // prevent creation on the stack and copy/move
private:
    AParticleTrackingRecord(int   particleId, float startEnergy, float * startPosition, float time);
    AParticleTrackingRecord(int   particleId,
                            float startEnergy,
                            float startX,
                            float startY,
                            float startZ,
                            float time);

    AParticleTrackingRecord(const AParticleTrackingRecord &) = delete;
    AParticleTrackingRecord & operator=(const AParticleTrackingRecord &) = delete;
    AParticleTrackingRecord(AParticleTrackingRecord &&) = delete;
    AParticleTrackingRecord & operator=(AParticleTrackingRecord &&) = delete;

public:
    int     ParticleId;                       // ants ID of the particle
    float   StartEnergy;                      // initial kinetic energy
    float   StartPosition[3];                 // initial position
    float   StartTime;                        // time of creation

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

    const std::vector<AParticleTrackingRecord *> getPrimaryParticleRecords() const {return PrimaryParticleRecords;}

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
