#ifndef SENSORLRFS_H
#define SENSORLRFS_H

//ANTS2
#include "pmsensor.h"
#include "pmsensorgroup.h"
#include "alrffitsettings.h"

//Qt
#include <QObject>
#include <QVector>
#include <QString>
#include <QJsonObject>

class EventsDataClass;

class IterationGroups
{
public:
    explicit IterationGroups(){}
    IterationGroups(int pmCount, const QVector<PMsensorGroup> &groups);
    IterationGroups(const IterationGroups &copy);
    ~IterationGroups(){}

    IterationGroups &operator=(const IterationGroups &copy);

    void clear() { for(int i = 0; i < PMsensorGroups.size(); i++) PMsensorGroups[i].clear(); }

    int countPMs() const { return sensors.size(); }
    int countGroups() { return PMsensorGroups.size(); }
    PMsensor *sensor(int ipm) { return sensors[ipm]; }
    PMsensorGroup* getGroup(int igroup) { return &PMsensorGroups[igroup]; }
    LRF2 *lrf(int ipm) { return sensors[ipm] ? sensors[ipm]->GetLRF() : 0; }
    QVector <PMsensorGroup> *getGroups() { return &PMsensorGroups; }

    QJsonObject makeJson;

    void writeJSON(QJsonObject &json);
    bool readJSON(QJsonObject &json);

    void setName(QString name) {Name = name;}
    void setSerialNumber(int number) {SerialNumber = number;}
    void setGrouppingOption(QString option) {GrouppingOption = option;}
    QString getName() const {return Name;}
    int getSerialNumber() const {return SerialNumber;}
    QString getGrouppingOption() const {return GrouppingOption;}

    bool isIterationValid(); //main function to check validity of IterationGroup
      bool isAllLRFsDefined();
      bool isAllGainsDefined();

private:
    void fillRefs(int pmCount = 0);

    QVector <PMsensor*> sensors;
    QVector <PMsensorGroup> PMsensorGroups;

    QString Name;
    int SerialNumber;
    QString GrouppingOption;

    friend class SensorLRFs;
};

class SensorLRFs : public QObject
{
  Q_OBJECT
public:
    explicit SensorLRFs(int nPMs, QObject *parent = 0);
    ~SensorLRFs();

    QJsonObject LRFmakeJson;

    //void clear() { clear(this->nPMs); }
    void clear(int numPMs); //also used to communicate the new number of PMs in the detector!

    //make LRFs
    bool makeLRFs(QJsonObject &json, EventsDataClass *EventsDataHub, pms *PMs);
    bool makeAxialLRFsFromRfiles(QJsonObject &json, QString FileNamePattern, pms *PMs);
    bool onlyGains(QJsonObject &json, EventsDataClass *EventsDataHub, pms *PMs);

    double getFunction(double *r, double *p);
    double getFunction2D(double *r, double *p);
    double getInvertedFunction2D(double *r, double *p);
    double getFunction2Dplus(double *r, double *p);
    double getRadial(double *r, double *p);
    double getRadialPlus(double *r, double *p);
    double getError(double *r, double *p);
    double getError2D(double *r, double *p);
    double getErrorRadial(double *r, double *p);
    double getLRF(int pmt, double *r) {return currentIter->sensors[pmt]->eval(r);}
    double getLRF(int pmt, const double *r) {return currentIter->sensors[pmt]->eval(r);}
    double getLRF(int iter, int pmt, double *r) {return iterations[iter].sensors[pmt]->eval(r);}
    double getLRFlocal(int pmt, double *r_local) {return currentIter->sensors[pmt]->evalLocal(r_local);}
    double getLRFlocal(int iter, int pmt, double *r_local) {return iterations[iter].sensors[pmt]->evalLocal(r_local);}
    double getLRF(int pmt, double x, double y, double z);
    double getLRF(int iter, int pmt, double x, double y, double z);
    double getLRFErr(int pmt, double *r) {return currentIter->sensors[pmt]->evalErr(r);}
    double getLRFErr(int pmt, const double *r) {return currentIter->sensors[pmt]->evalErr(r);}
    double getLRFErrLocal(int pmt, double *r_local) {return currentIter->sensors[pmt]->evalErrLocal(r_local);}
       double getLRFErr(int pmt, double x, double y, double z);
    double getLRFDrvX(int pmt, double *r) {return currentIter->sensors[pmt]->evalDrvX(r);}
       double getLRFDrvX(int pmt, double x, double y, double z);
    double getLRFDrvY(int pmt, double *r) {return currentIter->sensors[pmt]->evalDrvY(r);}
       double getLRFDrvY(int pmt, double x, double y, double z);

    //compatibility
    const LRF2* operator[] (int pmt) {return currentIter->sensors[pmt]->GetLRF();}

    // add/delete iterations
      // Raimundo:
    void addIteration(int pmCount, const QVector<PMsensorGroup> &groups );
      // Andr:
    bool addNewIteration(IterationGroups &iteration);
    bool addNewIteration(int pmCount, const QVector<PMsensorGroup> &groups , QString GrouppingOption);
    void deleteIteration(int iter);

    //currentIter
    bool isCurrentIterDefined() {return currentIter;}
    PMsensor *getSensor(int ipm) { return currentIter->sensors[ipm]; }
    PMsensorGroup *getSensorGroup(int igrp) { return &currentIter->PMsensorGroups[igrp]; }
    int countGroups() {return currentIter->PMsensorGroups.size();}
    int getCurrentIterIndex() const;
    void setCurrentIter(int index);
    void setCurrentIterName(QString name) {if (currentIter) currentIter->setName(name);}
    bool isAllLRFsDefined() const;

    //currentIter and iterations vector
    int countIterations() {return iterations.size();}
    IterationGroups* getIteration(int iter = -1); //-1 - currentIter
    int getIterationSerialNumer(int iter = -1);   //-1 - currentIter
    QString getIterationName(int iter = -1);      //-1 - currentIter
    void clearHistory(bool onlyTmp = false);

    //save-load
    bool saveAllIterations(QJsonObject &json); // *** to be changed
    bool saveIteration(int iter, QJsonObject &json);
    bool loadAll(QJsonObject &json);
    QString getLastError() {return error_string;}

    void requestStop() {fStopRequest = true;}
signals:
    void SensorLRFsReadySignal(bool);
    void ProgressReport(int);

private:
    int nPMs;
 //   QVector<LRF2*> LRFs;
    IterationGroups *currentIter;
    QVector<IterationGroups> iterations;

    QString error_string;
    int nextSerialNumber;

    ALrfFitSettings LRFsettings;

    bool fStopRequest;

    bool makeGroupLRF(int igrp, ALrfFitSettings &LRFsettings, QVector<PMsensorGroup> *groups, SensorLocalCache *lrfmaker);
};

#endif // SENSORLRFS_H
