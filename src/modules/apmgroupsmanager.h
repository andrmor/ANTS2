#ifndef APMGROUPSMANAGER_H
#define APMGROUPSMANAGER_H

#include <QObject>
#include <QVector>

class pms;
class QJsonObject;

class APmRecordNew
{
public:
    bool member;
    double gain;
    double cutOffMin, cutOffMax;

    APmRecordNew() : member(false), gain(1.0), cutOffMin(-1.0e10), cutOffMax(1.0e10) {}
    APmRecordNew(bool fIsMember) : member(fIsMember), gain(1.0), cutOffMin(-1.0e10), cutOffMax(1.0e10) {}
    APmRecordNew(bool fIsMember, double Gain, double Min, double Max) : member(fIsMember), gain(Gain), cutOffMin(Min), cutOffMax(Max) {}
};

class APmGroup
{
public:
    APmGroup(QString name) : Name(name) {}
    APmGroup() : Name("Undefined") {}

    QString Name;
    QVector<APmRecordNew> PMS;

    void  writeToJson(QJsonObject& json); // all excluding Filter and Rec jsons
    bool readFromJson(QJsonObject& json, int numPMs); // all excluding Filter and Rec jsons

    bool isPmMember(int ipm) const;
    void onNumberPMsChanged(int numPMs);
};

class APmGroupsManager : public QObject
{
    Q_OBJECT
public:
    explicit APmGroupsManager(pms* PMs, QJsonObject* ConfigJSON);
    ~APmGroupsManager();

    QVector<APmGroup*> Groups;
    inline int getCurrentGroup() const {return CurrentGroup;}
    void setCurrentGroup(int iGroup);

    //basic info requests
    inline int countPMgroups() const {return Groups.size();}
    const QString getGroupName(int iGroup) const;    // empty string if bad group
    //APmGroup const getCurrentGroup() const;
    int getGroupByName(QString Name) const;          //-1 if not found
    bool isPmBelongsToGroup(int ipm, int igroup) const;
    inline bool isPmBelongsToGroupFast(int ipm, int igroup) const {return Groups.at(igroup)->PMS.at(ipm).member;}
    inline bool isPmInCurrentGroupFast(int ipm) const {return Groups.at(CurrentGroup)->PMS.at(ipm).member;}
    int countPMsWithoutGroup(QVector<int>* unassigned=0) const;
    int countStaticPassives() const;

    // manipulating groups
    void clearPMgroups(bool fSkipJsonUpdate=false);  //leave one default PM group
    void deleteAllPMgroups();                        //not safe! deletes all groups, NOT updates Config->JSON
    int definePMgroup(QString Name, bool fSkipJsonUpdate=false); // if Name already exists, adds "1" at the end, returns the new group index
    bool removeGroup(int igroup);
    bool renameGroup(int igroup, QString name);
    void autogroup(int Regularity);                  //creates groups and assign PMs

    // json: Config
    void writeSensorGroupsToJson(QJsonObject& json);
    bool readGroupsFromJson(QJsonObject& json);
    void fixSensorGroupsCompatibility(QJsonObject& json);
       // direct updates of Config->JSON
    void updateGroupsInGlobalConfig();
    void synchronizeJsonArraysWithNumberGroups();    //if number of groups was changed by user, need to synchronize filt and rec arrays in JSON

    // tmp-passive control
    void selectActiveSensorGroup(int igroup);
    void clearActiveSensorGroups();

    // PM composition in a group
    void addPMtoGroup(int ipm, int iGroup, APmRecordNew record = APmRecordNew(true));
    void removeAllPmRecords(int iGroup);
    void removePMfromGroup(int ipm, int iGroup);
    int countPMsInGroup(int iGroup) const;

    // updates requested from GUI controls
    void setCutOffs(int ipm, int iGroup, double min, double max, bool fSkipJsonUpdate=false);
    void setGain(int ipm, int iGroup, double gain, bool fSkipJsonUpdate=false);
    void setAllGainsToUnity();

    // gains and cutoff - slow, but safe
    double getGain(int ipm, int iGroup) const;      //safe, returns 0 to indicate problem
    inline double getGainFast(int ipm, int iGroup) const {return Groups.at(iGroup)->PMS.at(ipm).gain;}
    double getCutOffMin(int ipm, int iGroup) const; //safe, returns 0 if problem
    double getCutOffMinFast(int ipm, int iGroup) const {return Groups.at(iGroup)->PMS.at(ipm).cutOffMin;}
    double getCutOffMax(int ipm, int iGroup) const; //safe, returns 0 if problem
    double getCutOffMaxFast(int ipm, int iGroup) const {return Groups.at(iGroup)->PMS.at(ipm).cutOffMax;}

    //triggered by detector on rebuild if number of PMs changed
    void onNumberOfPMsChanged();

    //passive PMs
    inline bool isPassive(int ipm) const { return PassiveStatus.at(ipm) != 0; }
    inline bool isActive(int ipm) const { return PassiveStatus.at(ipm) == 0; }
    inline bool isStaticPassive(int ipm) const { return PassiveStatus.at(ipm) & 1; }
    inline void setStaticPassive(int ipm) {PassiveStatus[ipm] |= 1;}
    inline void clearStaticPassive(int ipm) {PassiveStatus[ipm] &= ~(1);}
    inline bool isTMPPassive(int ipm) const { return PassiveStatus.at(ipm) & 4; }
    inline void setTMPpassive(int ipm) { PassiveStatus[ipm] |= 4;}
    inline void clearTMPpassive(int ipm) { PassiveStatus[ipm] &= ~(4);}
    int countActives(); //returns number of active PMs

    QVector<unsigned char> PassiveStatus; //Active if PassiveStatus==0; bit 0 -> static; bit 1 -> for dynamic; bit 2 -> group (TMPpassive)

private:
    int CurrentGroup;

    //aliases
    pms* PMs;
    QJsonObject* ConfigJSON;

signals:
    void CurrentSensorGroupChanged();
};

#endif // APMGROUPSMANAGER_H
