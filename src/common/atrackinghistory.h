#ifndef ATRACKINGHISTORY_H
#define ATRACKINGHISTORY_H

#include <QString>
#include <vector>

class ATrackingHistory;

class ATrackingStepData
{
public:
    double  Position[3];
    double  DepositedEnergy;
    QString Process;                         //step defining process
    std::vector<ATrackingHistory *> Secondaries; //secondaries created in this step
};

class ATrackingHistory
{
public:
    static ATrackingHistory* create(int ParticleId, double StartEnergy, double * StartPosition, ATrackingHistory * SecondaryOf = nullptr);

    void addStep(ATrackingStepData * step);

    // prevent creation on the stack due to the use of pointers to the secondaries
private:
    ATrackingHistory(int ParticleId, double StartEnergy, double * StartPosition, ATrackingHistory * SecondaryOf = nullptr);

    ATrackingHistory(const ATrackingHistory &) = delete;
    ATrackingHistory & operator=(const ATrackingHistory &) = delete;
    ATrackingHistory(ATrackingHistory &&) = delete;
    ATrackingHistory & operator=(ATrackingHistory &&) = delete;

public:
    ~ATrackingHistory();

    int     ParticleId;                       // ants ID of the particle
    double  StartEnergy;                      // initial kinetic energy
    double  StartPosition[0];                 // initial position
    std::vector<ATrackingStepData *> Steps;   // tracking steps
    ATrackingHistory * SecondaryOf = nullptr; //0 means primary

};

#endif // ATRACKINGHISTORY_H
