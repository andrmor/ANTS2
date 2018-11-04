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
  AParticleTrackingHistoryInterface(EventsDataClass& EventsDataHub);

private:
  EventsDataClass& EventsDataHub;
  const QVector<EventHistoryStructure*>& EventHistory;

public slots:
  int countParticles() {return EventHistory.size();}

  int    getParticleId(int iParticle);
  bool   isSecondary(int iParticle);
  int    getParentIndex(int iParticle);
  double getInitialEnergy(int iParticle);
  QVariantList getInitialPosition(int iParticle);
  QVariantList getDirection(int iParticle);
  int    getTermination(int iParticle);
  //int sernum(int i);

  QVariantList getAllDefinedTerminatorTypes();

  int    countRecords(int iParticle);
  int    getRecordMaterial(int iParticle, int iRecord);
  double getRecordDepositedEnergy(int iParticle, int iRecord);
  double getRecordDistance(int iParticle, int iRecord);

  void saveHistoryToTree(QString fileName);

private:
  bool checkParticle(int i);
  bool checkParticleAndMaterial(int i, int m);
};

#endif // APARTICLETRACKINGHISTORYINTERFACE_H
