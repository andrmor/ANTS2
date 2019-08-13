#include "anoderecord.h"

#include <QDebug>

ANodeRecord::ANodeRecord(double x, double y, double z, double time, int numPhot, ANodeRecord * rec) :
    Time(time), NumPhot(numPhot), LinkedNode(rec)
{
    R[0] = x;
    R[1] = y;
    R[2] = z;
}

ANodeRecord::ANodeRecord(double * r, double time, int numPhot, ANodeRecord *rec) :
    Time(time), NumPhot(numPhot), LinkedNode(rec)
{
    for (int i=0; i<3; i++) R[i] = r[i];
}

ANodeRecord::~ANodeRecord()
{
    delete LinkedNode;
}

ANodeRecord *ANodeRecord::createS(double x, double y, double z, double time, int numPhot, ANodeRecord * rec)
{
    return new ANodeRecord(x, y, z, time, numPhot, rec);
}

ANodeRecord *ANodeRecord::createV(double *r, double time, int numPhot, ANodeRecord *rec)
{
    return new ANodeRecord(r, time, numPhot, rec);
}

void ANodeRecord::addLinkedNode(ANodeRecord * node)
{
    ANodeRecord * n = this;
    while (n->LinkedNode) n = n->LinkedNode;
    n->LinkedNode = node;
}

int ANodeRecord::getNumberOfLinkedNodes()
{
    int counter = 0;
    ANodeRecord * node = this;
    while (node->LinkedNode)
    {
        counter++;
        node = node->LinkedNode;
    }
    return counter;
}
