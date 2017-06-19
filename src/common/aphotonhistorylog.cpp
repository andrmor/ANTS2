#include "aphotonhistorylog.h"
#include "amaterialparticlecolection.h"

APhotonHistoryLog::APhotonHistoryLog(const double *Position, const QString volumeName, double Time, APhotonHistoryLog::NodeType node, int MatIndex, int MatIndexAfter, int number) :
  volumeName(volumeName), time(Time), node(node),
  matIndex(MatIndex), matIndexAfter(MatIndexAfter),
  number(number)
{
  r[0] = Position[0];
  r[1] = Position[1];
  r[2] = Position[2];
}

QString APhotonHistoryLog::Print(AMaterialParticleCollection *MpCollection) const
{
  QString s;

  s += GetProcessName(node);

  if (node == HitPM || node == Detected || node == NotDetected) s += " PM# " + QString::number(number);
  if (node == HitDummyPM) s += " DummyPM# " + QString::number(number);

  if (node == Fresnel_Reflection || node == Override_Loss || node == Override_Back || node == Fresnel_Transmition || node == Override_Forward)
    s += " [" + MpCollection->getMaterialName(matIndex) +"/"+ MpCollection->getMaterialName(matIndexAfter)+"]";

  s += QString(" at ( ") + QString::number(r[0])+", "+ QString::number(r[1]) + ", "+QString::number(r[2])+" )";
  if (!volumeName.isEmpty())
    s += " in " + volumeName;

  s += ", " + QString::number(time)+" ns";

  return s;
}

const QString APhotonHistoryLog::GetProcessName(int nodeType)
{
  switch (nodeType)
    {
    case Undefined: return "Undefined";
    case Created: return "Created";
    case HitPM: return "HitPM";
    case HitDummyPM: return "HitDummyPM";
    case Escaped: return "Escaped";
    case Absorbed: return "Absorbed";
    case MaxNumberCyclesReached: return "MaxNumberCyclesReached";
    case Rayleigh: return "Rayleigh";
    case Reemission: return "Reemission";
    case Fresnel_Reflection: return "Fresnel_Reflection";
    case Fresnel_Transmition: return "Fresnel_Transmition";
    case Override_Loss: return "Override_Loss";
    case Override_Forward: return "Override_Forward";
    case Override_Back: return "Override_Back";
    case Detected: return "Detected";
    case NotDetected: return "NotDetected";
    case GeneratedOutsideGeometry: return "GeneratedOutsideGeometry";
    default: return "Error: unknown index!";
    }
}



