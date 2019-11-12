#ifndef ATRACKINGHISTORYCRAWLER_H
#define ATRACKINGHISTORYCRAWLER_H

#include "aeventtrackingrecord.h"

#include <QString>
#include <QSet>
#include <QMap>

#include "TString.h"

class TH1D;
class TH2D;
class TFormula;

// --- Search processors ---

class AHistorySearchProcessor
{
public:
    virtual ~AHistorySearchProcessor(){}

    virtual void beforeSearch(){}
    virtual void afterSearch(){}

    // ---------------

    virtual void onNewEvent(){}
    virtual bool onNewTrack(const AParticleTrackingRecord & ){return false;} // master track control - the bool flag will be returned in the onTrackEnd

    virtual void onLocalStep(const ATrackingStepData & ){} // anything but transportation

    virtual void onTransitionOut(const ATrackingStepData & ){} // "from" step
    virtual void onTransitionIn (const ATrackingStepData & ){} // "from" step
    virtual void onTransition(const ATrackingStepData & , const ATrackingStepData & ){} // "fromfrom" step, "from" step - "Creation" step cannot call this method!

    virtual void onTrackEnd(bool /*bMaster*/){} // flag is the value returned by onNewTrack()
    virtual void onEventEnd(){}

    bool isInlineSecondaryProcessing() const {return bInlineSecondaryProcessing;}
    bool isIgnoreParticleSelectors()   const {return bIgnoreParticleSelectors;}

protected:
    bool bInlineSecondaryProcessing = false;
    bool bIgnoreParticleSelectors   = false;
};

class AHistorySearchProcessor_findParticles : public AHistorySearchProcessor
{
public:
    virtual ~AHistorySearchProcessor_findParticles(){}

    virtual bool onNewTrack(const AParticleTrackingRecord & pr) override;
    virtual void onLocalStep(const ATrackingStepData & tr) override;
    virtual void onTrackEnd(bool) override;

    QString Candidate;
    bool bConfirmed = false;
    QMap<QString, int> FoundParticles;
};

class AHistorySearchProcessor_findProcesses : public AHistorySearchProcessor
{
public:
    virtual ~AHistorySearchProcessor_findProcesses(){}

    virtual void onLocalStep(const ATrackingStepData & tr) override;
    virtual void onTransitionOut(const ATrackingStepData & ) override;
    virtual void onTransitionIn (const ATrackingStepData & ) override;

    QMap<QString, int> FoundProcesses;
};

class AHistorySearchProcessor_findDepositedEnergy : public AHistorySearchProcessor
{
public:
    enum CollectionMode {Individual, WithSecondaries, OverEvent};

    AHistorySearchProcessor_findDepositedEnergy(CollectionMode mode, int bins, double from = 0, double to = 0);
    virtual ~AHistorySearchProcessor_findDepositedEnergy();

    virtual void onNewEvent() override;
    virtual bool onNewTrack(const AParticleTrackingRecord & pr) override;
    virtual void onLocalStep(const ATrackingStepData & tr) override;
    virtual void onTransitionOut(const ATrackingStepData & tr) override; // in Geant4 energy loss can happen on transition
    virtual void onTrackEnd(bool bMaster) override;
    virtual void onEventEnd() override;

    CollectionMode Mode;
    double Depo = 0;
    TH1D * Hist = nullptr;
    bool bSecondaryTrackingStarted = false;
};

struct AParticleDepoStat
{
    AParticleDepoStat(int num, double sum, double sumOfSquares) : num(num), sum(sum), sumOfSquares(sumOfSquares) {}
    AParticleDepoStat(){}

    void append(double val) {num++; sum += val; sumOfSquares += val*val;}

    int num = 0;
    double sum = 0;
    double sumOfSquares = 0;
};

class AHistorySearchProcessor_getDepositionStats : public AHistorySearchProcessor
{
public:
    virtual bool onNewTrack(const AParticleTrackingRecord & pr) override;
    virtual void onLocalStep(const ATrackingStepData & tr) override;
    virtual void onTransitionOut(const ATrackingStepData & tr) override; // in Geant4 energy loss can happen on transition

    const QString Dummy = "___error___";
    const QString * ParticleName = &Dummy;
    bool bAlreadyFound = false;
    QMap<QString, AParticleDepoStat>::iterator itParticle;

    QMap<QString, AParticleDepoStat> DepoData;
};

class AHistorySearchProcessor_findTravelledDistances : public AHistorySearchProcessor
{
public:
    AHistorySearchProcessor_findTravelledDistances(int bins, double from = 0, double to = 0);
    virtual ~AHistorySearchProcessor_findTravelledDistances();

    virtual bool onNewTrack(const AParticleTrackingRecord & pr) override;
    virtual void onLocalStep(const ATrackingStepData & tr) override;
    virtual void onTransitionOut(const ATrackingStepData & tr) override; // "from" step
    virtual void onTransitionIn (const ATrackingStepData & tr) override; // "from" step
    virtual void onTrackEnd(bool) override;

    float Distance = 0;
    float LastPosition[3];
    bool bStarted = false;
    TH1D * Hist = nullptr;
};

class AHistorySearchProcessor_Border : public AHistorySearchProcessor
{
public:
    AHistorySearchProcessor_Border(const QString & what,
                                   const QString & cuts,
                                   int bins, double from, double to);
    AHistorySearchProcessor_Border(const QString & what, const QString & vsWhat,
                                   const QString & cuts,
                                   int bins, double from, double to);
    AHistorySearchProcessor_Border(const QString & what, const QString & vsWhat,
                                   const QString & cuts,
                                   int bins1, double from1, double to1,
                                   int bins2, double from2, double to2);
    AHistorySearchProcessor_Border(const QString & what, const QString & vsWhat, const QString & andVsWhat,
                                   const QString & cuts,
                                   int bins1, double from1, double to1,
                                   int bins2, double from2, double to2);
    virtual ~AHistorySearchProcessor_Border();

    virtual void afterSearch() override;

    // direction info can be [0,0,0] !!!
    virtual void onTransition(const ATrackingStepData & fromfromTr, const ATrackingStepData & fromTr) override; // "from" step

    QString ErrorString;  // after constructor, valid if ErrorString is empty
    bool bRequiresDirections = false;

    TFormula * formulaWhat1 = nullptr;
    TFormula * formulaWhat2 = nullptr;
    TFormula * formulaWhat3 = nullptr;
    TFormula * formulaCuts = nullptr;

    //double  x, y, z, time, energy, vx, vy, vz
    //        0  1  2    3     4      5   6   7
    double par[8];
    TH1D * Hist1D = nullptr;
    TH1D * Hist1Dnum = nullptr;
    TH2D * Hist2D = nullptr;
    TH2D * Hist2Dnum = nullptr;

private:
    TFormula * parse(QString & expr);
};


// --------------------------------------

class AFindRecordSelector
{
public:
  //track level
    bool bParticle = false;
    QString Particle;
    bool bPrimary = false;
    bool bSecondary = false;
    bool bLimitToFirstInteractionOfPrimary = false;

  //transportation
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

    void find(const AFindRecordSelector & criteria, AHistorySearchProcessor & processor) const;

private:
    const std::vector<AEventTrackingRecord*> & History;

    enum ProcessType {Creation, Local, NormalTransportation, ExitingWorld};

    void findRecursive(const AParticleTrackingRecord & pr, const AFindRecordSelector &opt, AHistorySearchProcessor & processor) const;
};

#endif // ATRACKINGHISTORYCRAWLER_H
