#ifndef APHOTONHISTORYLOG_H
#define APHOTONHISTORYLOG_H

#include <QString>

class APhotonHistoryLog
{
public:
    enum NodeType {Undefined = 0, Created, HitPM, HitDummyPM, Escaped, Absorbed, MaxNumberCyclesReacehed, Rayleigh,
                   Reemitted, FresnelReflected, FresnelTransmitted, LossOnOverride, OverrideForward, OverrideBack};

public:
    APhotonHistoryLog(const double* Position, double Time, NodeType node, int MatIndex, int MatIndexAfter = -1) :
      node(node), time(Time),
      matIndex(MatIndex), matIndexAfter(MatIndexAfter)
    {r[0]=Position[0]; r[1]=Position[1]; r[2]=Position[2];}
    APhotonHistoryLog() : node(Undefined) {}

    double r[3];        //position
    double time;
    int matIndex;       //material index of the medium
    int matIndexAfter;  //material index of the medium after interface (if applicable)

    NodeType node;

    QString Print() const
    {
      QString s = QString("XYZ=") + QString::number(r[0]) +","+ QString::number(r[1]) + ","+QString::number(r[2])+", ";
      s += QString("Time=")+QString::number(time)+", ";
      s += QString("MatIndex=")+QString::number(matIndex)+", ";
      s += (matIndexAfter==-1 ? "" : "MatIndexAfter="+QString::number(matIndexAfter)+", ");
      s += GetProcessName(node);
      return s;
    }

    static const QString GetProcessName(int nodeType)
    {
      switch (nodeType)
        {
        case Undefined: return "Undefined";
        case Created: return "Created";
        case HitPM: return "HitPM";
        case HitDummyPM: return "HitDummyPM";
        case Escaped: return "Escaped";
        case Absorbed: return "Absorbed";
        case MaxNumberCyclesReacehed: return "MaxNumberCyclesReacehed";
        case Rayleigh: return "Rayleigh";
        case Reemitted: return "Reemitted";
        case FresnelReflected: return "FresnelReflected";
        case FresnelTransmitted: return "FresnelTransmitted";
        case LossOnOverride: return "LossOnOverride";
        case OverrideForward: return "OverrideForward";
        case OverrideBack: return "OverrideBack";
        default: return "Error: unknown index!";
        }
    }
};

#endif // APHOTONHISTORYLOG_H
