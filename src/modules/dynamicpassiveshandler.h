#ifndef DYNAMICPASSIVESHANDLER_H
#define DYNAMICPASSIVESHANDLER_H

#include <QVector>

class APmHub;
class EventsDataClass;
struct AReconRecord;
class ReconstructionSettings;
class APmGroupsManager;

class DynamicPassivesHandler
{
public:
    DynamicPassivesHandler(APmHub *Pms, APmGroupsManager* PMgroups, EventsDataClass *eventsDataHub);
    void init(ReconstructionSettings *RecSet, int ThisPmGroup); //configure, clean passives, copy passives from PMgroups
    void calculateDynamicPassives(int ievent, const AReconRecord *rec); //RecSet.fuseDynamicPassives knows to do it or not

    inline bool isActive(int ipm)  const {return StatePMs[ipm] == 0;}
    inline bool isPassive(int ipm) const {return StatePMs[ipm] != 0;}
    inline bool isStaticActive(int ipm)  const {return !(StatePMs[ipm] & 1);}
    inline bool isStaticPassive(int ipm) const {return   StatePMs[ipm] & 1;}
    inline bool isDynamicActive(int ipm)  const {return !(StatePMs[ipm] & 2);}
    inline bool isDynamicPassive(int ipm) const {return   StatePMs[ipm] & 2;}

    int countActives() const;

    inline void setDynamicPassive(int ipm) {StatePMs[ipm] |= 2;}
    inline void clearDynamicPassive(int ipm) {StatePMs[ipm] &= ~(2);}
    void clearDynamicPassives();
private:
    APmHub* PMs;
    APmGroupsManager* PMgroups;
    int ThisPmGroup;
    EventsDataClass* EventsDataHub;
    QVector<char> StatePMs;
    int numPMs;

    bool fByThreshold;
    bool fByDistance;
    double ThresholdLow, ThresholdHigh;
    double MaxDistance;
    double MaxDistance2;

    int CenterOption; //for PassiveType by distance: 0 - CoG, 1 - maximum signal PM, 2 - given XY, 3-scan
    double StartX, StartY;
};

#endif // DYNAMICPASSIVESHANDLER_H
