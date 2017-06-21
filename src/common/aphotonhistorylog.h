#ifndef APHOTONHISTORYLOG_H
#define APHOTONHISTORYLOG_H

#include <QString>

class AMaterialParticleCollection;

class APhotonHistoryLog
{
public:
    enum NodeType {
                   Undefined = 0,//keep the first one zero!!!

                   Created, HitPM, HitDummyPM, Detected, NotDetected, Escaped, Absorbed, MaxNumberCyclesReached, Rayleigh,
                   Reemission, Fresnel_Reflection, Fresnel_Transmition, Override_Loss, Override_Forward, Override_Back, GeneratedOutsideGeometry,

                   __SizeOfNodeTypes__//keep it last!!!
                  };

public:
    APhotonHistoryLog(const double* Position, const QString volumeName, double Time, NodeType process, int MatIndex = -1, int MatIndexAfter = -1, int number = -1);
    APhotonHistoryLog() : process(Undefined) {}

    NodeType process;
    double r[3];        //position xyz
    QString volumeName;
    double time;
    int matIndex;       //material index of the medium
    int matIndexAfter;  //material index of the medium after interface (if applicable)
    int number;         //multipurpose (-1 value by default), if node HitPM/Detected/NotDetected -> contains PM number

    QString Print(AMaterialParticleCollection* MpCollection) const;

    static const QString GetProcessName(int nodeType);
    static const QString PrintAllProcessTypes();
};

#endif // APHOTONHISTORYLOG_H
