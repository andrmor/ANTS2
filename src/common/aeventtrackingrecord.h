#ifndef AEVENTTRACKINGRECORD_H
#define AEVENTTRACKINGRECORD_H

#include <QString>
#include <QStringList>
#include <vector>

class AParticleTrackingRecord;
class TrackHolderClass;
class ATrackBuildOptions;
class TGeoNode;
class TrackHolderClass;

class ATrackingStepData
{
public:
    ATrackingStepData(float * position, float time, float energy, float depositedEnergy, const QString & process);
    ATrackingStepData(double * position, double time, double energy, double depositedEnergy, const QString & process);
    ATrackingStepData(float x, float y, float z, float time, float energy, float depositedEnergy, const QString & process);

    virtual ~ATrackingStepData();

    virtual void logToString(QString & str, int offset) const;

public:
    float   Position[3];
    float   Time;
    float   Energy;
    float   DepositedEnergy;
    QString Process;              //step defining process
    std::vector<int> Secondaries; //secondaries created in this step - indexes in the parent record
};

class ATransportationStepData : public ATrackingStepData
{
public:
    ATransportationStepData(float x, float y, float z, float time, float energy, float depositedEnergy, const QString & process);
    ATransportationStepData(double * position, double time, double energy, double depositedEnergy, const QString & process);

    void setVolumeInfo(const QString & volName, int volIndex, int matIndex);

public:
    // for "T" step it is for the next volume, for "C" step it is for the current
    QString VolName;
    int VolIndex;
    int iMaterial;

    virtual void logToString(QString & str, int offset) const override;
};

class AParticleTrackingRecord
{
public:
    static AParticleTrackingRecord* create(const QString & Particle);
    static AParticleTrackingRecord* create(); // try to avoid this

    void updatePromisedSecondary(const QString & particle, float startEnergy, float startX, float startY, float startZ, float startTime, const QString& volName, int volIndex, int matIndex);
    void addStep(ATrackingStepData * step);

    void addSecondary(AParticleTrackingRecord * sec);
    int  countSecondaries() const;

    bool isPrimary() const {return !SecondaryOf;}
    bool isSecondary() const {return (bool)SecondaryOf;}
    const std::vector<ATrackingStepData *> & getSteps() const {return Steps;}
    const AParticleTrackingRecord * getSecondaryOf() const {return SecondaryOf;}
    const std::vector<AParticleTrackingRecord *> & getSecondaries() const {return Secondaries;}

    bool isHaveProcesses(const QStringList & Proc, bool bOnlyPrimary);

    void logToString(QString & str, int offset, bool bExpandSecondaries) const;
    void makeTrack(std::vector<TrackHolderClass *> & Tracks, const QStringList & ParticleNames, const ATrackBuildOptions & TrackBuildOptions, bool bWithSecondaries) const;
    void fillELDD(ATrackingStepData * IdByStep, std::vector<float> & dist, std::vector<float> & ELDD) const;

    virtual ~AParticleTrackingRecord();

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

    bool   isHaveProcesses(const QStringList & Proc, bool bOnlyPrimary) const;

    void   makeTracks(std::vector<TrackHolderClass *> & Tracks, const QStringList & ParticleNames, const ATrackBuildOptions & TrackBuildOptions, bool bWithSecondaries) const;

    const std::vector<AParticleTrackingRecord *> & getPrimaryParticleRecords() const {return PrimaryParticleRecords;}

    virtual ~AEventTrackingRecord();

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
