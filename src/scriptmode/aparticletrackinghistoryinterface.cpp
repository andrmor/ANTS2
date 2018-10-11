#include "aparticletrackinghistoryinterface.h"
#include "eventsdataclass.h"
#include "ahistoryrecords.h"

AParticleTrackingHistoryInterface::AParticleTrackingHistoryInterface(EventsDataClass &EventsDataHub) :
    EventsDataHub(EventsDataHub), EventHistory(EventsDataHub.EventHistory) {}

QVariantList AParticleTrackingHistoryInterface::getAllDefinedTerminatorTypes()
{
    const QStringList defined = EventHistoryStructure::getAllDefinedTerminationTypes();
    QVariantList l;
    for (int i=0; i<defined.size(); ++i)
        l << QString::number(i)+ " = " + defined.at(i);
    return l;
}

int AParticleTrackingHistoryInterface::getTermination(int iParticle)
{
    if (!checkParticle(iParticle)) return -1;
    return EventHistory.at(iParticle)->Termination;
}

QVariantList AParticleTrackingHistoryInterface::getDirection(int iParticle)
{
    QVariantList vl;
    if (checkParticle(iParticle))
       vl << EventHistory.at(iParticle)->dx << EventHistory.at(iParticle)->dy << EventHistory.at(iParticle)->dz;
    return vl;
}

QVariantList AParticleTrackingHistoryInterface::getInitialPosition(int iParticle)
{
    QVariantList vl;
    if (checkParticle(iParticle))
       vl << EventHistory.at(iParticle)->x << EventHistory.at(iParticle)->y << EventHistory.at(iParticle)->z;
    return vl;
}

int AParticleTrackingHistoryInterface::getParticleId(int iParticle)
{
    if (!checkParticle(iParticle)) return -1;
    return EventHistory.at(iParticle)->ParticleId;
}

//int AParticleTrackingHistoryInterface::sernum(int i)
//{
//    if (!checkParticle(i)) return -1;
//    return EventHistory.at(i)->index;
//}

bool AParticleTrackingHistoryInterface::isSecondary(int iParticle)
{
    if (!checkParticle(iParticle)) return false;
    return EventHistory.at(iParticle)->isSecondary();
}

int AParticleTrackingHistoryInterface::getParentIndex(int iParticle)
{
    if (!checkParticle(iParticle)) return -1;
    return EventHistory.at(iParticle)->SecondaryOf;
}

double AParticleTrackingHistoryInterface::getInitialEnergy(int iParticle)
{
    if (!checkParticle(iParticle)) return -1;
    return EventHistory.at(iParticle)->initialEnergy;
}

int AParticleTrackingHistoryInterface::countRecords(int iParticle)
{
    if (!checkParticle(iParticle)) return 0;
    return EventHistory.at(iParticle)->Deposition.size();
}

int AParticleTrackingHistoryInterface::getRecordMaterial(int iParticle, int iRecord)
{
    if (!checkParticleAndMaterial(iParticle, iRecord)) return -1;
    return EventHistory.at(iParticle)->Deposition.at(iRecord).MaterialId;
}

double AParticleTrackingHistoryInterface::getRecordDepositedEnergy(int iParticle, int iRecord)
{
    if (!checkParticleAndMaterial(iParticle, iRecord)) return -1;
    return EventHistory.at(iParticle)->Deposition.at(iRecord).DepositedEnergy;
}

double AParticleTrackingHistoryInterface::getRecordDistance(int iParticle, int iRecord)
{
    if (!checkParticleAndMaterial(iParticle, iRecord)) return -1;
    return EventHistory.at(iParticle)->Deposition.at(iRecord).Distance;
}

bool AParticleTrackingHistoryInterface::checkParticle(int i)
{
    if (i<0 || i >= EventHistory.size())
    {
        abort("Attempt to address non-existent particle number in history");
        return false;
    }
    return true;
}

bool AParticleTrackingHistoryInterface::checkParticleAndMaterial(int i, int m)
{
    if (i<0 || i >= EventHistory.size())
    {
        abort("Attempt to address non-existent particle number in history");
        return false;
    }
    if (m<0 || m >= EventHistory.at(i)->Deposition.size())
    {
        abort("Attempt to address non-existent material record in history");
        return false;
    }
    return true;
}

void AParticleTrackingHistoryInterface::saveHistoryToTree(QString fileName)
{
    EventsDataHub.saveEventHistoryToTree(fileName.toLatin1().data());
}

