#ifndef ATRACKINGHISTORYCRAWLER_H
#define ATRACKINGHISTORYCRAWLER_H

#include "aeventtrackingrecord.h"

#include <QString>
#include <QSet>
#include <QMap>

#include "TString.h"

class TH1D;

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

    virtual void onLocalStep(const ATrackingStepData & ){} // anything but transportation
    enum Direction {OUT, IN, OUT_IN};
    virtual void onTransition(const ATrackingStepData & , Direction ){} // "from" step

    virtual void onTrackEnd() = 0;

};

class AHistorySearchProcessor_findParticles : public AHistorySearchProcessor
{
public:
    virtual ~AHistorySearchProcessor_findParticles(){}

    virtual void onParticle(const AParticleTrackingRecord & pr);
    virtual void onLocalStep(const ATrackingStepData & tr) override;
    virtual void onTrackEnd() override;

    QString Candidate;
    bool bConfirmed = false;
    QMap<QString, int> FoundParticles;
};

class AHistorySearchProcessor_findDepositedEnergy : public AHistorySearchProcessor
{
public:
    AHistorySearchProcessor_findDepositedEnergy(int bins, double from = 0, double to = 0);
    virtual ~AHistorySearchProcessor_findDepositedEnergy();

    virtual void onParticle(const AParticleTrackingRecord & pr);
    virtual void onLocalStep(const ATrackingStepData & tr) override;
    virtual void onTrackEnd() override;

    double Depo = 0;
    TH1D * Hist = nullptr;
};

class AHistorySearchProcessor_findTravelledDistances : public AHistorySearchProcessor
{
public:
    AHistorySearchProcessor_findTravelledDistances(int bins, double from = 0, double to = 0);
    virtual ~AHistorySearchProcessor_findTravelledDistances();

    virtual void onParticle(const AParticleTrackingRecord & pr);
    virtual void onLocalStep(const ATrackingStepData & tr) override;
    virtual void onTransition(const ATrackingStepData & tr, Direction direction); // "from" step
    virtual void onTrackEnd() override;

    float Distance = 0;
    float LastPosition[3];
    bool bStarted = false;
    TH1D * Hist = nullptr;
};

class AHistorySearchProcessor_findMaterials : public AHistorySearchProcessor
{
public:
    virtual ~AHistorySearchProcessor_findMaterials(){}

    //virtual void onParticle(const AParticleTrackingRecord & pr);
    virtual void onLocalStep(const ATrackingStepData & tr) override;
    //virtual void onTrackEnd() override;

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
    bool bInOutSeparately = false; // if true, "in" and "out" conditions will be checked independently (both can trigger processor call)
    //from
    bool bFromMat = false;
    bool bFromVolume = false;
    bool bFromVolIndex = false;
    int  FromMat = 0;
    TString FromVolume;
    int  FromVolIndex = 0;
    //to
    bool bToMat = false;
    bool bToVolume = false;
    bool bToVolIndex = false;
    int  ToMat = 0;
    TString ToVolume;
    int  ToVolIndex = 0;

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

    enum ProcessType {Creation, Local, NormalTransportation, ExitingWorld};

    void findRecursive(const AParticleTrackingRecord & pr, const AFindRecordSelector &opt, std::vector<AHistorySearchProcessor*> & processors) const;
};

#endif // ATRACKINGHISTORYCRAWLER_H
