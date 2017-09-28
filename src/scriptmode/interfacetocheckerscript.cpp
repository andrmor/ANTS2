#include "interfacetocheckerscript.h"
#include "mainwindow.h"
#include "eventsdataclass.h"
#include "aenergydepositioncell.h"
#include "aparticleonstack.h"

#include <QDebug>

InterfaceToInteractionScript::InterfaceToInteractionScript(MainWindow* MW, EventsDataClass* EventsDataHub)
  : MW(MW), EventsDataHub(EventsDataHub)
{  
  H["ClearStack"] = "Clear particle stack";
  H["AddParticleToStack"] = "Add a particle (or several identical particles) to the stack";
  H["TrackStack"] = "Track all particles from the stack";
  H["ClearAllData"] = "Removes all data completely. Use it if the module i used in iterations or for minimizer after analysis is completed";

  H["count"] = "Returns the number of available particle records";

  H["termination"] = "Returns termination type (int) for the given particle record";
  //1 - escaped 2 - all energy dissipated 3 - photoeffect 4 - compton 5 - capture
  //6 - error - undefined termination 7 - created outside defined geometry 8 - found untrackable material
  H["terminationStr"] = "Returns termination type (string) for the given pareticle record";
  H["dX"] = "Returns x-component of the particle direction vector";
  H["dY"] = "Returns x-component of the particle direction vector";
  H["dZ"] = "Returns x-component of the particle direction vector";
  H["particleId"] = "Returns the ID of the particle, used by the material collection";
  H["sernum"] = "Returns serial number of the particle";
  H["isSecondary"] = "Returns true if the particle is secondary (created during tracking of the particle stack)";

  // MaterialCrossed - info from DepositionHistory
  H["MaterialsCrossed_count"] = "Returns the number of materials crossed by the particle during tracking";
  H["MaterialsCrossed_matId"] = "Returns material ID (used by material collection) for the given material index";
  H["MaterialsCrossed_energy"] = "Returns the total deposited energy inside the given (by index) material";
  H["MaterialsCrossed_distance"] = "Returns the total distance travelled inside the given (by index) material";

  // Deposition - info from EnergyVector
  H["Deposition_countMaterials"] = "Returns the number of material records for the given particle";
  H["Deposition_matId"] = "Returns material ID for the given particle and material index";
  H["Deposition_startX"] = "Returns the start X position for the given particle inside the material with the given index";
  H["Deposition_startY"] = "Returns the start Y position for the given particle inside the material with the given index";
  H["Deposition_startZ"] = "Returns the start Z position for the given particle inside the material with the given index";
    // +subindex - node index
  H["Deposition_countNodes"] = "Returns the number of deposition nodes for the given particle inside the material with the given index";
  H["Deposition_X"] = "Returns the X position for the given particle, material index and node index";
  H["Deposition_Y"] = "Returns the Y position for the given particle, material index and node index";
  H["Deposition_Z"] = "Returns the Z position for the given particle, material index and node index";
  H["Deposition_dE"] = "Returns the deposited energy for the given particle, material index and node index";
  H["Deposition_dL"] = "Returns the length of the node with the given particle, material index and node index";
}

void InterfaceToInteractionScript::ClearStack()
{
  //qDebug() << "Clear stack triggered";
  MW->on_pbClearAllStack_clicked();
  //qDebug() << "Done";
}

void InterfaceToInteractionScript::AddParticleToStack(int particleID, double X, double Y, double Z,
                                                      double dX, double dY, double dZ,
                                                      double Time, double Energy,
                                                      int numCopies)
{
  MW->ParticleStack.reserve(MW->ParticleStack.size() + numCopies);
  for (int i=0; i<numCopies; i++)
  {
     AParticleOnStack *tmp = new AParticleOnStack(particleID,   X, Y, Z,   dX, dY, dZ,  Time, Energy);
     MW->ParticleStack.append(tmp);
  }
  MW->on_pbRefreshStack_clicked();
}

void InterfaceToInteractionScript::TrackStack()
{
  //qDebug() << "->Track stack triggered";
  ClearExtractedData();
  //qDebug() << "--->PR cleared";
  MW->on_pbTrackStack_clicked();
  //qDebug() << "--->Stack tracked";
  populateParticleRecords();
  //qDebug() << "--->Particle records populated";
}

void InterfaceToInteractionScript::ClearExtractedData()
{
    PR.clear();
}

void InterfaceToInteractionScript::populateParticleRecords()
{ 
  ClearExtractedData();
  if (EventsDataHub->EventHistory.isEmpty())
    {
      abort("EventHistory is empty!");
      return;
    }
  if (MW->EnergyVector.isEmpty())
    {
      abort("EnergyVector is empty!");
      return;
    }

  int indexEV = 0;
  for (int i=0; i<EventsDataHub->EventHistory.size(); i++)
    {
      ParticleRecord pr;

      // migrating EventHistory
      pr.History = EventsDataHub->EventHistory.at(i); //only address!

      // adding info from EnergyVector
      int sernum = pr.History->index;
        //needs at least one entry in EnergyVector with the same serial number
      while (indexEV<MW->EnergyVector.size() && MW->EnergyVector.at(indexEV)->index == sernum)
        {          
          MaterialRecord mr;
          // Material ID
          mr.MatId = MW->EnergyVector.at(indexEV)->MaterialId;
          // Start position
          double* r = MW->EnergyVector.at(indexEV)->r;
          double  l = MW->EnergyVector.at(indexEV)->cellLength;
          mr.StartPosition[0] = r[0] - l * pr.History->dx;
          mr.StartPosition[1] = r[1] - l * pr.History->dy;
          mr.StartPosition[2] = r[2] - l * pr.History->dz;
          // Deposition info
          do
            {
              DepoNode depo;
              // position
              depo.R[0] = MW->EnergyVector.at(indexEV)->r[0];
              depo.R[1] = MW->EnergyVector.at(indexEV)->r[1];
              depo.R[2] = MW->EnergyVector.at(indexEV)->r[2];
              // energy
              depo.Energy = MW->EnergyVector.at(indexEV)->dE;
              // length
              depo.CellLength = MW->EnergyVector.at(indexEV)->cellLength;
              // adding to the record
              mr.ByMaterial.append(depo);
              indexEV++;              
            }
          while(indexEV<MW->EnergyVector.size() && MW->EnergyVector.at(indexEV)->index==sernum && MW->EnergyVector.at(indexEV)->MaterialId==mr.MatId);
          pr.Deposition.append(mr);
        }
      PR.append(pr);
    }
}

int InterfaceToInteractionScript::termination(int i)
{
  if (i<0 || i>PR.size()-1)
    {
      abort("Attempt to address non-existent particle");
      return -1;
    }
  return PR.at(i).History->Termination;
}

QString InterfaceToInteractionScript::terminationStr(int i)
{
  if (i<0 || i>PR.size()-1)
    {
      abort("Attempt to address non-existent particle");
      return "Attempt to address non-existent particle";
    }
  switch (PR.at(i).History->Termination)
    {
    case 0:  return "error - particle tracking was never stopped";
    case 1:  return "escaped";
    case 2:  return "all energy dissipated";
    case 3:  return "photoeffect";
    case 4:  return "compton";
    case 5:  return "neutron capture";
    case 6:  return "error reported";
    case 7:  return "was created outside of the world";
    case 8:  return "entered material with tracking forbidden";
    case 9:  return "pair production";
    default: return "unknown termination";
    }
}

double InterfaceToInteractionScript::dX(int i)
{
  if (i<0 || i>PR.size()-1)
    {
      abort("Attempt to address non-existent particle");
      return std::numeric_limits<double>::quiet_NaN();
    }
  return PR.at(i).History->dx;
}

double InterfaceToInteractionScript::dY(int i)
{
  if (i<0 || i>PR.size()-1)
    {
      abort("Attempt to address non-existent particle");
      return std::numeric_limits<double>::quiet_NaN();
    }
  return PR.at(i).History->dy;
}

double InterfaceToInteractionScript::dZ(int i)
{
  if (i<0 || i>PR.size()-1)
    {
      abort("Attempt to address non-existent particle");
      return std::numeric_limits<double>::quiet_NaN();
    }
  return PR.at(i).History->dz;
}

int InterfaceToInteractionScript::particleId(int i)
{
  if (i<0 || i>PR.size()-1)
    {
      abort("Attempt to address non-existent particle");
      return -1;
    }
  return PR.at(i).History->ParticleId;
}

int InterfaceToInteractionScript::sernum(int i)
{
  if (i<0 || i>PR.size()-1)
    {
      abort("Attempt to address non-existent particle");
      return -1;
    }
  return PR.at(i).History->index;
}

int InterfaceToInteractionScript::isSecondary(int i)
{
  if (i<0 || i>PR.size()-1)
    {
      abort("Attempt to address non-existent particle");
      return false;
    }
  return PR.at(i).History->isSecondary();
}

int InterfaceToInteractionScript::getParent(int i)
{
  if (i<0 || i>PR.size()-1)
    {
      abort("Attempt to address non-existent particle");
      return -1;
    }
  return PR.at(i).History->SecondaryOf;
}

int InterfaceToInteractionScript::MaterialsCrossed_count(int i)
{
  if (i<0 || i>PR.size()-1)
    {
      abort("Attempt to address non-existent particle");
      return -1;
    }
  return PR.at(i).History->Deposition.size();
}

int InterfaceToInteractionScript::MaterialsCrossed_matId(int i, int m)
{
  if (i<0 || i>PR.size()-1)
    {
      abort("Attempt to address non-existent particle");
      return -1;
    }
  if (m<0 || m>PR.at(i).History->Deposition.size()-1)
    {
      abort("Attempt to address non-existent material");
      return -1;
    }
  return PR.at(i).History->Deposition.at(m).MaterialId;
}

int InterfaceToInteractionScript::MaterialsCrossed_energy(int i, int m)
{
  if (i<0 || i>PR.size()-1)
    {
      abort("Attempt to address non-existent particle");
      return -1;
    }
  if (m<0 || m>PR.at(i).History->Deposition.size()-1)
    {
      abort("Attempt to address non-existent material");
      return -1;
    }
  return PR.at(i).History->Deposition.at(m).DepositedEnergy;
}

int InterfaceToInteractionScript::MaterialsCrossed_distance(int i, int m)
{
  if (i<0 || i>PR.size()-1)
    {
      abort("Attempt to address non-existent particle");
      return -1;
    }
  if (m<0 || m>PR.at(i).History->Deposition.size()-1)
    {
      abort("Attempt to address non-existent material");
      return -1;
    }
  return PR.at(i).History->Deposition.at(m).Distance;
}

int InterfaceToInteractionScript::Deposition_countMaterials(int i)
{
  if (i<0 || i>PR.size()-1)
    {
      abort("Attempt to address non-existent particle");
      return -1;
    }
  return PR.at(i).Deposition.size();
}

int InterfaceToInteractionScript::Deposition_matId(int i, int m)
{
  if (i<0 || i>PR.size()-1)
    {
      abort("Attempt to address non-existent particle");
      return -1;
    }
  if (m<0 || m>PR.at(i).Deposition.size()-1)
    {
      abort("Attempt to address non-existent material in deposition");
      return -1;
    }

  return PR.at(i).Deposition.at(m).MatId;
}

double InterfaceToInteractionScript::Deposition_startX(int i, int m)
{
  if (i<0 || i>PR.size()-1)
    {
      abort("Attempt to address non-existent particle");
      return std::numeric_limits<double>::quiet_NaN();
    }
  if (m<0 || m>PR.at(i).Deposition.size()-1)
    {
      abort("Attempt to address non-existent material in deposition");
      return std::numeric_limits<double>::quiet_NaN();
    }

  return PR.at(i).Deposition.at(m).StartPosition[0];
}

double InterfaceToInteractionScript::Deposition_startY(int i, int m)
{
  if (i<0 || i>PR.size()-1)
    {
      abort("Attempt to address non-existent particle");
      return std::numeric_limits<double>::quiet_NaN();
    }
  if (m<0 || m>PR.at(i).Deposition.size()-1)
    {
      abort("Attempt to address non-existent material in deposition");
      return std::numeric_limits<double>::quiet_NaN();
    }

  return PR.at(i).Deposition.at(m).StartPosition[1];
}

double InterfaceToInteractionScript::Deposition_startZ(int i, int m)
{
  if (i<0 || i>PR.size()-1)
    {
      abort("Attempt to address non-existent particle");
      return std::numeric_limits<double>::quiet_NaN();
    }
  if (m<0 || m>PR.at(i).Deposition.size()-1)
    {
      abort("Attempt to address non-existent material in deposition");
      return std::numeric_limits<double>::quiet_NaN();
    }

  return PR.at(i).Deposition.at(m).StartPosition[2];
}

#include "TGeoManager.h"
#include "TGeoVolume.h"
#include "TGeoNode.h"
#include "detectorclass.h"
QString InterfaceToInteractionScript::Deposition_volumeName(int i, int m)
{
  if (i<0 || i>PR.size()-1)
    {
      abort("Attempt to address non-existent particle");
      return "";
    }
  if (m<0 || m>PR.at(i).Deposition.size()-1)
    {
      abort("Attempt to address non-existent material in deposition");
      return "";
    }

  if (PR.at(i).Deposition.at(m).ByMaterial.isEmpty()) return "";
  TGeoManager* GeoManager = MW->Detector->GeoManager;
  double* R = (double*)PR.at(i).Deposition.at(m).ByMaterial.first().R;
  TGeoNode* node = GeoManager->FindNode(R[0], R[1], R[2]);
  if (node) return QString(node->GetName());
  else return "";
}

int InterfaceToInteractionScript::Deposition_volumeIndex(int i, int m)
{
  if (i<0 || i>PR.size()-1)
    {
      abort("Attempt to address non-existent particle");
      return -1;
    }
  if (m<0 || m>PR.at(i).Deposition.size()-1)
    {
      abort("Attempt to address non-existent material in deposition");
      return -1;
    }

  if (PR.at(i).Deposition.at(m).ByMaterial.isEmpty()) return -1;
  TGeoManager* GeoManager = MW->Detector->GeoManager;
  double* R = (double*)PR.at(i).Deposition.at(m).ByMaterial.first().R;
  TGeoNode* node = GeoManager->FindNode(R[0], R[1], R[2]);
  if (node) return node->GetNumber();
  else return -1;
}

double InterfaceToInteractionScript::Deposition_energy(int i, int m)
{
  if (i<0 || i>PR.size()-1)
    {
      abort("Attempt to address non-existent particle");
      return 0;
    }
  if (m<0 || m>PR.at(i).Deposition.size()-1)
    {
      abort("Attempt to address non-existent material in deposition");
      return 0;
    }

  double energy = 0;
  for (int ien=0; ien<PR.at(i).Deposition.at(m).ByMaterial.size(); ien++)
    energy += PR.at(i).Deposition.at(m).ByMaterial.at(ien).Energy;
  return energy;
}

int InterfaceToInteractionScript::Deposition_countNodes(int i, int m)
{
  if (i<0 || i>PR.size()-1)
    {
      abort("Attempt to address non-existent particle");
      return -1;
    }
  if (m<0 || m>PR.at(i).Deposition.size()-1)
    {
      abort("Attempt to address non-existent material in deposition");
      return -1;
    }

  return PR.at(i).Deposition.at(m).ByMaterial.size();
}

double InterfaceToInteractionScript::Deposition_X(int i, int m, int n)
{
  if (i<0 || i>PR.size()-1)
    {
      abort("Attempt to address non-existent particle");
      return std::numeric_limits<double>::quiet_NaN();
    }
  if (m<0 || m>PR.at(i).Deposition.size()-1)
    {
      abort("Attempt to address non-existent material in deposition");
      return std::numeric_limits<double>::quiet_NaN();
    }
  if (n<0 || n>PR.at(i).Deposition.at(m).ByMaterial.size()-1)
    {
      abort("Attempt to address non-existent deposition node");
      return std::numeric_limits<double>::quiet_NaN();
    }
  return PR.at(i).Deposition.at(m).ByMaterial.at(n).R[0];
}

double InterfaceToInteractionScript::Deposition_Y(int i, int m, int n)
{
  if (i<0 || i>PR.size()-1)
    {
      abort("Attempt to address non-existent particle");
      return std::numeric_limits<double>::quiet_NaN();
    }
  if (m<0 || m>PR.at(i).Deposition.size()-1)
    {
      abort("Attempt to address non-existent material in deposition");
      return std::numeric_limits<double>::quiet_NaN();
    }
  if (n<0 || n>PR.at(i).Deposition.at(m).ByMaterial.size()-1)
    {
      abort("Attempt to address non-existent deposition node");
      return std::numeric_limits<double>::quiet_NaN();
    }
  return PR.at(i).Deposition.at(m).ByMaterial.at(n).R[1];
}

double InterfaceToInteractionScript::Deposition_Z(int i, int m, int n)
{
  if (i<0 || i>PR.size()-1)
    {
      abort("Attempt to address non-existent particle");
      return std::numeric_limits<double>::quiet_NaN();
    }
  if (m<0 || m>PR.at(i).Deposition.size()-1)
    {
      abort("Attempt to address non-existent material in deposition");
      return std::numeric_limits<double>::quiet_NaN();
    }
  if (n<0 || n>PR.at(i).Deposition.at(m).ByMaterial.size()-1)
    {
      abort("Attempt to address non-existent deposition node");
      return std::numeric_limits<double>::quiet_NaN();
    }
  return PR.at(i).Deposition.at(m).ByMaterial.at(n).R[2];
}

double InterfaceToInteractionScript::Deposition_dE(int i, int m, int n)
{
  if (i<0 || i>PR.size()-1)
    {
      abort("Attempt to address non-existent particle");
      return std::numeric_limits<double>::quiet_NaN();
    }
  if (m<0 || m>PR.at(i).Deposition.size()-1)
    {
      abort("Attempt to address non-existent material in deposition");
      return std::numeric_limits<double>::quiet_NaN();
    }
  if (n<0 || n>PR.at(i).Deposition.at(m).ByMaterial.size()-1)
    {
      abort("Attempt to address non-existent deposition node");
      return std::numeric_limits<double>::quiet_NaN();
    }
  return PR.at(i).Deposition.at(m).ByMaterial.at(n).Energy;
}

double InterfaceToInteractionScript::Deposition_dL(int i, int m, int n)
{
  if (i<0 || i>PR.size()-1)
    {
      abort("Attempt to address non-existent particle");
      return std::numeric_limits<double>::quiet_NaN();
    }
  if (m<0 || m>PR.at(i).Deposition.size()-1)
    {
      abort("Attempt to address non-existent material in deposition");
      return std::numeric_limits<double>::quiet_NaN();
    }
  if (n<0 || n>PR.at(i).Deposition.at(m).ByMaterial.size()-1)
    {
      abort("Attempt to address non-existent deposition node");
      return std::numeric_limits<double>::quiet_NaN();
    }
  return PR.at(i).Deposition.at(m).ByMaterial.at(n).CellLength;
}
