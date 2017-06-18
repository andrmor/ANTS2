#ifndef APHOTONHISTORYLOG_H
#define APHOTONHISTORYLOG_H

class APhotonHistoryLog
{
public:
    enum NodeType {Undefined = 0, Created, HitPM, HitDummyPM, Escaped, Absorbed, MaxNumberCyclesReacehed, Rayleigh,
                   Reemitted, FresnelReflected, FresnelTransmitted, LossOnOverride, OverrideForward, OverrideBack};

public:
    APhotonHistoryLog(double* Position, NodeType node) : Node(node) {r[0]=Position[0]; r[1]=Position[1]; r[2]=Position[2];}
    APhotonHistoryLog() : Node(Undefined) {}
    double r[3]; //position

    NodeType Node;
};

#endif // APHOTONHISTORYLOG_H
