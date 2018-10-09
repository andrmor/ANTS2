#include "aparticletrackinghistoryinterface.h"
#include "ahistoryrecords.h"

AParticleTrackingHistoryInterface::AParticleTrackingHistoryInterface(QVector<EventHistoryStructure *> &EventHistory) :
    EventHistory(EventHistory) {}

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

//QVariantList AParticleTrackingHistoryInterface::getDirection(int i)
//{
//    QVariantList vl;
//    if (checkParticle(i))
//       vl << EventHistory.at(i)->dx << EventHistory.at(i)->dy << EventHistory.at(i)->dz;
//    return vl;
//}

int AParticleTrackingHistoryInterface::getParticleId(int iParticle)
{
    if (!checkParticle(iParticle)) return -1;
    return EventHistory.at(iParticle)->ParticleId;
}

//int AParticleTrackingHistoryInterface::sernum(int i)
//{
//  if (!checkParticle(i)) return -1;
//  return EventHistory.at(i)->index;
//}

bool AParticleTrackingHistoryInterface::isSecondary(int iParticle)
{
    if (!checkParticle(iParticle)) return false;
    return EventHistory.at(iParticle)->isSecondary();
}

int AParticleTrackingHistoryInterface::getParent(int iParticle)
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

/*
#include "TTree.h"
#include "TFile.h"
bool AParticleTrackingHistoryInterface::saveHistoryToTTree(QString fileName)
{
    TFile* f = new TFile(fileName,"RECREATE");

    TTree *t = new TTree("","Particle tracking history");

    std::vector <double> x; //Can be multiple point reconstruction!
    double chi2;
    double ssum;
    int ievent; //event number
    int good, recOK;

    t->Branch("i",&ievent, "i/I");
    t->Branch("ssum",&ssum, "ssum/D");
    t->Branch("x", &x);

    int ParticleId;
    int index;
    int SecondaryOf;
    float dx, dy, dz;
    float initialEnergy;
    int Termination;

    <
    int MaterialId;
    double DepositedEnergy;
    double Distance;
    >
}
*/
