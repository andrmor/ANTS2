#ifndef AEVENTTRACKINGRECORD_H
#define AEVENTTRACKINGRECORD_H

#include <QString>
#include <vector>

class AParticleTrackingRecord;

class ATrackingStepData
{
public:
    float  Position[3];
    float  Time;
    float  DepositedEnergy;
    QString Process;                                    //step defining process
    std::vector<AParticleTrackingRecord *> Secondaries; //secondaries created in this step - does not own

    int   countSecondaries() const;
    const QString toString(int offset) const;
};

class AParticleTrackingRecord
{
public:
    static AParticleTrackingRecord* create(int ParticleId, double StartEnergy, double * StartPosition, double Time, AParticleTrackingRecord * SecondaryOf = nullptr);
    static AParticleTrackingRecord* create(int ParticleId, float  StartEnergy, float  * StartPosition, float  Time, AParticleTrackingRecord * SecondaryOf = nullptr);

    void addStep(ATrackingStepData * step);

    void addSecondary(AParticleTrackingRecord * sec);
    int  countSecondaries() const;

    const QString toString(int offset, const QStringList & ParticleNames, bool bExpandSecondaries) const;

    // prevent creation on the stack and copy/move
private:
    AParticleTrackingRecord(int ParticleId, float StartEnergy, float * StartPosition, float Time, AParticleTrackingRecord * SecondaryOf = nullptr);

    AParticleTrackingRecord(const AParticleTrackingRecord &) = delete;
    AParticleTrackingRecord & operator=(const AParticleTrackingRecord &) = delete;
    AParticleTrackingRecord(AParticleTrackingRecord &&) = delete;
    AParticleTrackingRecord & operator=(AParticleTrackingRecord &&) = delete;

public:
    ~AParticleTrackingRecord();

    int     ParticleId;                       // ants ID of the particle
    float   StartEnergy;                      // initial kinetic energy
    float   StartPosition[3];                 // initial position
    float   StartTime;                        // time of creation
    std::vector<ATrackingStepData *> Steps;   // tracking steps

    AParticleTrackingRecord * SecondaryOf = nullptr;    // 0 means primary
    std::vector<AParticleTrackingRecord *> Secondaries; // vector of secondaries

};

class AEventTrackingRecord
{
public:
    static AEventTrackingRecord* create();
    void   addPrimaryRecord(AParticleTrackingRecord * rec);

    bool   isEmpty() {return PrimaryParticleRecords.empty();}
    int    countPrimaries();

    // prevent creation on the stack and copy/move
private:
    AEventTrackingRecord();

    AEventTrackingRecord(const AEventTrackingRecord &) = delete;
    AEventTrackingRecord & operator=(const AEventTrackingRecord &) = delete;
    AEventTrackingRecord(AEventTrackingRecord &&) = delete;
    AEventTrackingRecord & operator=(AEventTrackingRecord &&) = delete;

public:
    ~AEventTrackingRecord();

    std::vector<AParticleTrackingRecord *> PrimaryParticleRecords;

};


#endif // AEVENTTRACKINGRECORD_H
