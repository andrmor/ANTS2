#include "aphotonhistorylog.h"
#include "amaterialparticlecolection.h"

APhotonHistoryLog::APhotonHistoryLog(const double *Position, const QString volumeName,
                                     double Time,
                                     int iWave,
                                     APhotonHistoryLog::NodeType node,
                                     int MatIndex, int MatIndexAfter,
                                     int number) :
  process(node), volumeName(volumeName), time(Time),
  matIndex(MatIndex), matIndexAfter(MatIndexAfter), number(number), iWave(iWave)
{
  r[0] = Position[0];
  r[1] = Position[1];
  r[2] = Position[2];
}

QString APhotonHistoryLog::Print(AMaterialParticleCollection *MpCollection) const
{
  QString s;

  s += GetProcessName(process);

  if (process == HitPM || process == Detected || process == NotDetected) s += " PM# " + QString::number(number);
  if (process == HitDummyPM) s += " DummyPM# " + QString::number(number);

  if (process == Fresnel_Reflection || process == Override_Loss || process == Override_Back || process == Fresnel_Transmition || process == Override_Forward)
    s += " [" + MpCollection->getMaterialName(matIndex) +"/"+ MpCollection->getMaterialName(matIndexAfter)+"]";

  s += QString(" at ( ") + QString::number(r[0])+", "+ QString::number(r[1]) + ", "+QString::number(r[2])+" )";
  if (!volumeName.isEmpty()) s += " in " + volumeName;
  if (iWave != -1) s += " iWave="+QString::number(iWave);
  s += ", " + QString::number(time)+" ns";

  return s;
}

// =====================================================================
// WATER'S FUNCTIONS ===================================================
// =====================================================================

bool APhotonHistoryLog::isDetected()
{
	if(process == Detected){
		return true;
	} else { 
		return false;
	}
	
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
    case Grid_Enter: return "Grid_Enter";
    case Grid_Exit: return "Grid_Exit";
    case Grid_ShiftIn: return "Grid_ShiftIn";
    case Grid_ShiftOut: return "Grid_ShiftOut";
    case KilledByMonitor: return "StoppedByMonitor";
    default: return "Error: unknown index!";
    }
}

const QString APhotonHistoryLog::PrintAllProcessTypes()
{
  QString s = "<br>Defined types:<br>";
  for (int i=0; i<__SizeOfNodeTypes__; i++)
    s += QString::number(i) + " -> " + GetProcessName(i) + "<br>";
  return s;
}
