#ifndef APARTICLETRACKINGHISTORYINTERFACE_H
#define APARTICLETRACKINGHISTORYINTERFACE_H

#include "ascriptinterface.h"

#include <QString>
#include <QVector>
#include <QVariantList>

class EventsDataClass;
struct EventHistoryStructure;

class AParticleTrackingHistoryInterface : public AScriptInterface
{
  Q_OBJECT
public:
  AParticleTrackingHistoryInterface(QVector<EventHistoryStructure*>& EventHistory);

private:
  QVector<EventHistoryStructure*>& EventHistory;

public slots:
  int countParticles() {return EventHistory.size();}

  int    getParticleId(int iParticle);
  bool   isSecondary(int iParticle);
  int    getParent(int iParticle);
  double getInitialEnergy(int iParticle);
  int    getTermination(int iParticle);

  QVariantList getAllDefinedTerminatorTypes();
  //QVariantList getDirection(int i);
  //int sernum(int i);

  int    countRecords(int iParticle);
  int    getRecordMaterial(int iParticle, int iRecord);
  double getRecordDepositedEnergy(int iParticle, int iRecord);
  double getRecordDistance(int iParticle, int iRecord);

  //bool   saveHistoryToTTree(QString fileName);

private:
  bool checkParticle(int i);
  bool checkParticleAndMaterial(int i, int m);
};

#endif // APARTICLETRACKINGHISTORYINTERFACE_H
