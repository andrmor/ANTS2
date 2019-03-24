#include "atracklog_si.h"
#include "eventsdataclass.h"
#include "ahistoryrecords.h"

ATrackLog_SI::ATrackLog_SI(EventsDataClass &EventsDataHub) :
    EventsDataHub(EventsDataHub), EventHistory(EventsDataHub.EventHistory)
{
    H["countParticles"] = "Number of entries in the log";

    H["getParticleId"] = "Particle id of this particle";
    H["isSecondary"] = "Returns true if this is the primary particle.";
    H["getParentIndex"] = "If it is primary particle, returns -1. Otherwise returns index (entry number) of the parent";
    H["getInitialEnergy"] = "Returns energy which the particle had when created.";
    H["getInitialPosition"] = "Returns array of XYZ of the position where the particle was created";
    H["getDirection"] = "Return unit vector (XYZ components) of the particle's direction";
    H["getTermination"] = "Returns int number corresponding to how the tracking of the particle has ended.\n"
                          "Use getAllDefinedTerminatorTypes() method to get the list of all possible terminator values";

    H["getAllDefinedTerminatorTypes"] = "Get the list of all possible values of the termination";

    H["countRecords"] = "Returns the number of volumes the particle passed during tracking";
    H["getRecordMaterial"] = "Returns the list of material indexes of the crossed volumes";
    H["getRecordDepositedEnergy"] = "Returns energies depositied in all crossed volumes";
    H["getRecordDistance"] = "Returns distances travelled by the particle in each volume";

    H["saveHistoryToTree"] = "Save particle tracking log to a CERN root tree";
}

QVariantList ATrackLog_SI::getAllDefinedTerminatorTypes()
{
    const QStringList defined = EventHistoryStructure::getAllDefinedTerminationTypes();
    QVariantList l;
    for (int i=0; i<defined.size(); ++i)
        l << QString::number(i)+ " = " + defined.at(i);
    return l;
}

int ATrackLog_SI::getTermination(int iParticle)
{
    if (!checkParticle(iParticle)) return -1;
    return EventHistory.at(iParticle)->Termination;
}

QVariantList ATrackLog_SI::getDirection(int iParticle)
{
    QVariantList vl;
    if (checkParticle(iParticle))
       vl << EventHistory.at(iParticle)->dx << EventHistory.at(iParticle)->dy << EventHistory.at(iParticle)->dz;
    return vl;
}

QVariantList ATrackLog_SI::getInitialPosition(int iParticle)
{
    QVariantList vl;
    if (checkParticle(iParticle))
       vl << EventHistory.at(iParticle)->x << EventHistory.at(iParticle)->y << EventHistory.at(iParticle)->z;
    return vl;
}

int ATrackLog_SI::getParticleId(int iParticle)
{
    if (!checkParticle(iParticle)) return -1;
    return EventHistory.at(iParticle)->ParticleId;
}

//int AParticleTrackingHistoryInterface::sernum(int i)
//{
//    if (!checkParticle(i)) return -1;
//    return EventHistory.at(i)->index;
//}

bool ATrackLog_SI::isSecondary(int iParticle)
{
    if (!checkParticle(iParticle)) return false;
    return EventHistory.at(iParticle)->isSecondary();
}

int ATrackLog_SI::getParentIndex(int iParticle)
{
    if (!checkParticle(iParticle)) return -1;
    return EventHistory.at(iParticle)->SecondaryOf;
}

double ATrackLog_SI::getInitialEnergy(int iParticle)
{
    if (!checkParticle(iParticle)) return -1;
    return EventHistory.at(iParticle)->initialEnergy;
}

int ATrackLog_SI::countRecords(int iParticle)
{
    if (!checkParticle(iParticle)) return 0;
    return EventHistory.at(iParticle)->Deposition.size();
}

int ATrackLog_SI::getRecordMaterial(int iParticle, int iRecord)
{
    if (!checkParticleAndMaterial(iParticle, iRecord)) return -1;
    return EventHistory.at(iParticle)->Deposition.at(iRecord).MaterialId;
}

double ATrackLog_SI::getRecordDepositedEnergy(int iParticle, int iRecord)
{
    if (!checkParticleAndMaterial(iParticle, iRecord)) return -1;
    return EventHistory.at(iParticle)->Deposition.at(iRecord).DepositedEnergy;
}

double ATrackLog_SI::getRecordDistance(int iParticle, int iRecord)
{
    if (!checkParticleAndMaterial(iParticle, iRecord)) return -1;
    return EventHistory.at(iParticle)->Deposition.at(iRecord).Distance;
}

bool ATrackLog_SI::checkParticle(int i)
{
    if (i<0 || i >= EventHistory.size())
    {
        abort("Attempt to address non-existent particle number in history");
        return false;
    }
    return true;
}

bool ATrackLog_SI::checkParticleAndMaterial(int i, int m)
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

void ATrackLog_SI::saveHistoryToTree(QString fileName)
{
    EventsDataHub.saveEventHistoryToTree(fileName.toLatin1().data());
}

