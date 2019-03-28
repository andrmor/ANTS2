#ifndef AEVENTTRACKINGRECORD_H
#define AEVENTTRACKINGRECORD_H

#include <QString>
#include <vector>

class AParticleTrackingRecord;

class ATrackingStepData
{
public:
    double  Position[3];
    double  DepositedEnergy;
    QString Process;                         //step defining process
    std::vector<AParticleTrackingRecord *> Secondaries; //secondaries created in this step
};

class AParticleTrackingRecord
{
public:
    static AParticleTrackingRecord* create(int ParticleId, double StartEnergy, double * StartPosition, AParticleTrackingRecord * SecondaryOf = nullptr);

    void addStep(ATrackingStepData * step);

    // prevent creation on the stack and copy/move
private:
    AParticleTrackingRecord(int ParticleId, double StartEnergy, double * StartPosition, AParticleTrackingRecord * SecondaryOf = nullptr);

    AParticleTrackingRecord(const AParticleTrackingRecord &) = delete;
    AParticleTrackingRecord & operator=(const AParticleTrackingRecord &) = delete;
    AParticleTrackingRecord(AParticleTrackingRecord &&) = delete;
    AParticleTrackingRecord & operator=(AParticleTrackingRecord &&) = delete;

public:
    ~AParticleTrackingRecord();

    int     ParticleId;                       // ants ID of the particle
    double  StartEnergy;                      // initial kinetic energy
    double  StartPosition[3];                 // initial position
    std::vector<ATrackingStepData *> Steps;   // tracking steps
    AParticleTrackingRecord * SecondaryOf = nullptr; //0 means primary

};

class AEventTrackingRecord
{
public:
    static AEventTrackingRecord* create();
    void   addParticleTrackingRecord(AParticleTrackingRecord * rec);
    bool   isEmpty() {return PrimaryParticleRecords.empty();}
    size_t size() { return PrimaryParticleRecords.size(); }

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
