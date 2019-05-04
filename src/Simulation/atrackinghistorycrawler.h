#ifndef ATRACKINGHISTORYCRAWLER_H
#define ATRACKINGHISTORYCRAWLER_H

#include "aeventtrackingrecord.h"

#include <QString>
#include <QSet>
#include <QMap>

#include "TString.h"

/*
// --- Search conditions ---
class AHistorySearchCondition
{
public:
    virtual ~AHistorySearchCondition(){}

   // false = validation failed
    virtual bool validateParticle(const AParticleTrackingRecord & pr) {return true;}
    virtual bool validateStep(const ATrackingStepData & step) {return true;}
    virtual bool validateTransition(const ATrackingStepData & from, const ATrackingStepData & to) {return true;}
};

class AHistorySearchCondition_particle : public AHistorySearchCondition
{
public:
    virtual ~AHistorySearchCondition(){}

   // false = validation failed
    virtual bool validateParticle(const AParticleTrackingRecord & pr) = 0;
};
*/

// --- Search processors ---

class AHistorySearchProcessor
{
public:
    virtual ~AHistorySearchProcessor(){}

    virtual void onParticle(const AParticleTrackingRecord & ){}
    virtual void onStep(const ATrackingStepData & ){}
    virtual void onTrackEnd() = 0;
    virtual void report(){}
};

class AHistorySearchProcessor_findParticles : public AHistorySearchProcessor
{
public:
    virtual ~AHistorySearchProcessor_findParticles(){}

    virtual void onParticle(const AParticleTrackingRecord & pr);
    virtual void onStep(const ATrackingStepData & tr) override;
    virtual void onTrackEnd() override;
    virtual void report() override;

    QString Candidate;
    bool bConfirmed = false;
    QMap<QString, int> FoundParticles;
};

class AHistorySearchProcessor_findMaterials : public AHistorySearchProcessor
{
public:
    virtual ~AHistorySearchProcessor_findMaterials(){}

    //virtual void onParticle(const AParticleTrackingRecord & pr);
    virtual void onStep(const ATrackingStepData & tr) override;
    //virtual void onTrackEnd() override;
    virtual void report() override;

private:
    QSet<int> FoundMaterials;
};

class AFindRecordSelector
{
public:
  //track level
    bool bParticle = false;
    QString Particle;
    bool bPrimary = false;
    bool bSecondary = false;

  //transportation
    bool bFromMat = false;
    int FromMat = 0;
    bool bToMat = false;
    int ToMat = 0;
    bool bFromVolume = false;
    TString FromVolume;
    bool bToVolume = false;
    TString ToVolume;
    bool bFromVolIndex = false;
    int FromVolIndex = 0;
    bool bToVolIndex = false;
    int ToVolIndex = 0;

  //step level
    bool bMaterial = false;
    int Material = 0;

    bool bVolume = false;
    TString Volume;

    bool bVolumeIndex = false;
    int VolumeIndex = 0;

};

class ATrackingHistoryCrawler
{
public:
    ATrackingHistoryCrawler(const std::vector<AEventTrackingRecord*> & History);

    void find(const AFindRecordSelector & criteria, std::vector<AHistorySearchProcessor*> & processors) const;

private:
    const std::vector<AEventTrackingRecord*> & History;

    void findRecursive(const AParticleTrackingRecord & pr, const AFindRecordSelector &opt, std::vector<AHistorySearchProcessor*> & processors) const;
};

#endif // ATRACKINGHISTORYCRAWLER_H
