#include "ainterfacetodeposcript.h"
#include "eventsdataclass.h"
#include "aenergydepositioncell.h"
#include "aparticleonstack.h"
#include "simulationmanager.h"
#include "asandwich.h"
#include "globalsettingsclass.h"
#include "atrackrecords.h"

#include <QDebug>

#include "TRandom2.h"
#include "TGeoTrack.h"
#include "TGeoManager.h"

AInterfaceToDepoScript::AInterfaceToDepoScript(DetectorClass *Detector, GlobalSettingsClass *GlobSet, EventsDataClass* EventsDataHub)
  : Detector(Detector), GlobSet(GlobSet), EventsDataHub(EventsDataHub)
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

  H["getAllDefinedTerminatorTypes"] = "Return array with all defined terminator types. Format of array element is 'index=type'";
}

AInterfaceToDepoScript::~AInterfaceToDepoScript()
{
    ClearExtractedData();
    ClearStack();
    clearEnergyVector();
}

void AInterfaceToDepoScript::ClearStack()
{
  //qDebug() << "Clear stack triggered";

  //MW->on_pbClearAllStack_clicked();

  for (int i=0; i<ParticleStack.size(); i++)
      delete ParticleStack[i];
  ParticleStack.clear();

  //qDebug() << "Done";
}

void AInterfaceToDepoScript::AddParticleToStack(int particleID, double X, double Y, double Z,
                                                      double dX, double dY, double dZ,
                                                      double Time, double Energy,
                                                      int numCopies)
{
//  MW->ParticleStack.reserve(MW->ParticleStack.size() + numCopies);
//  for (int i=0; i<numCopies; i++)
//  {
//     AParticleOnStack *tmp = new AParticleOnStack(particleID,   X, Y, Z,   dX, dY, dZ,  Time, Energy);
//     MW->ParticleStack.append(tmp);
//  }
//  MW->on_pbRefreshStack_clicked();

    ParticleStack.reserve( ParticleStack.size() + numCopies );
    for (int i=0; i<numCopies; i++)
    {
        AParticleOnStack* tmp = new AParticleOnStack(particleID,   X, Y, Z,   dX, dY, dZ,  Time, Energy);
        ParticleStack.append(tmp);
    }
}

void AInterfaceToDepoScript::TrackStack(bool bDoTracks)
{
  //qDebug() << "->Track stack triggered";
  ClearExtractedData();
  //qDebug() << "--->PR cleared";

  //MW->on_pbTrackStack_clicked();
  bool bOK = doTracking(bDoTracks);
  //qDebug() << "--->Stack tracked" << bOK;
  if (!bOK) return;

  populateParticleRecords();
  //qDebug() << "--->Particle records populated";
}

void AInterfaceToDepoScript::ClearExtractedData()
{
    PR.clear();
}

void AInterfaceToDepoScript::populateParticleRecords()
{
  ClearExtractedData();
  if (EventsDataHub->EventHistory.isEmpty())
    {
      abort("EventHistory is empty!");
      return;
    }
  if (EnergyVector.isEmpty())
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
      while (indexEV < EnergyVector.size() && EnergyVector.at(indexEV)->index == sernum)
        {
          MaterialRecord mr;
          // Material ID
          mr.MatId = EnergyVector.at(indexEV)->MaterialId;
          // Start position
          double* r = EnergyVector.at(indexEV)->r;
          double  l = EnergyVector.at(indexEV)->cellLength;
          mr.StartPosition[0] = r[0] - l * pr.History->dx;
          mr.StartPosition[1] = r[1] - l * pr.History->dy;
          mr.StartPosition[2] = r[2] - l * pr.History->dz;
          // Deposition info
          do
            {
              DepoNode depo;
              // position
              depo.R[0] = EnergyVector.at(indexEV)->r[0];
              depo.R[1] = EnergyVector.at(indexEV)->r[1];
              depo.R[2] = EnergyVector.at(indexEV)->r[2];
              // energy
              depo.Energy = EnergyVector.at(indexEV)->dE;
              // length
              depo.CellLength = EnergyVector.at(indexEV)->cellLength;
              // adding to the record
              mr.ByMaterial.append(depo);
              indexEV++;
            }
          while(indexEV < EnergyVector.size() &&
                EnergyVector.at(indexEV)->index == sernum &&
                EnergyVector.at(indexEV)->MaterialId == mr.MatId);
          pr.Deposition.append(mr);
        }
      PR.append(pr);
  }
}

#include "aconfiguration.h"
bool AInterfaceToDepoScript::doTracking(bool bDoTracks)
{
    clearEnergyVector();
    EventsDataHub->clear();

    if ( !Detector->Config->JSON.contains("SimulationConfig"))
    {
        abort("Configuration does not contain simulation settings");
        return false;
    }
    QJsonObject json = Detector->Config->JSON["SimulationConfig"].toObject();

    //overrides
    json["Mode"] = "StackSim";
    json["DoGuiUpdate"] = false;

    if (!json.contains("ParticleSourcesConfig"))
    {
        abort("Json sent to simulator does not contain particle sim config data!");
        return false;
    }
        QJsonObject js = json["ParticleSourcesConfig"].toObject();
        if ( !js.contains("SourceControlOptions"))
        {
            abort("Json sent to simulator does not contain proper sim config data!");
            return false;
        }
        //control options
            QJsonObject cjs = js["SourceControlOptions"].toObject();

            cjs["AllowMultipleParticles"] = false;
            cjs["DoS1"] = false;
            cjs["DoS2"] = false;
            cjs["ParticleTracks"] = bDoTracks;
            cjs["IgnoreNoHitsEvents"] = false;
            cjs["IgnoreNoDepoEvents"] = false;
        js["SourceControlOptions"] = cjs;
    json["ParticleSourcesConfig"] = js;
    //SaveJsonToFile(json, "ThisSimConfig.json");
    //qDebug() << js;

    GeneralSimSettings simSettings;
    simSettings.readFromJson(json);
    simSettings.fLogsStat = true; //force to populate logs

    //========== prepare simulator ==========
    ParticleSourceSimulator *pss = new ParticleSourceSimulator(Detector, "TestSimulator");
    pss->setSimSettings(&simSettings);
    pss->setup(json);
    pss->initSimStat();
    pss->setRngSeed(Detector->RandGen->Rndm()*1000000);

    EventsDataHub->SimStat->initialize(Detector->Sandwich->MonitorsRecords);

    bool fOK = pss->standaloneTrackStack(&ParticleStack);
    if (!fOK)
    {
        abort("Error in tracker: " + pss->getErrorString());
        return false;
    }
    //--- Retrieve results ---

    clearEnergyVector(); // just in case clear procedures change
    EnergyVector = pss->getEnergyVector();
    pss->ClearEnergyVectorButKeepObjects(); //disconnected this copy so delete of the simulator does not kill the vector
    //  qDebug() << "-------------En vector size:"<<EnergyVector.size();

    //track handling
    if (bDoTracks)
    {
        Detector->GeoManager->ClearTracks();

        int numTracks = 0;
        //qDebug() << "Tracks collected:"<<pss->tracks.size();
        for (int iTr=0; iTr<pss->tracks.size(); iTr++)
        {
            TrackHolderClass* th = pss->tracks[iTr];

            if (numTracks < GlobSet->MaxNumberOfTracks)
            {
                TGeoTrack* track = new TGeoTrack(1, th->UserIndex);
                track->SetLineColor(th->Color);
                track->SetLineWidth(th->Width);
                for (int iNode=0; iNode<th->Nodes.size(); iNode++)
                    track->AddPoint(th->Nodes[iNode].R[0], th->Nodes[iNode].R[1], th->Nodes[iNode].R[2], th->Nodes[iNode].Time);

                if (track->GetNpoints()>1)
                {
                    numTracks++;
                    Detector->GeoManager->AddTrack(track);
                }
                else delete track;
            }
            delete th;
        }
        pss->tracks.clear();
    }

    //report data saved in history
    pss->appendToDataHub(EventsDataHub);

    delete pss;
    return true;
}

void AInterfaceToDepoScript::clearEnergyVector()
{
    for (int i=0; i<EnergyVector.size(); i++) delete EnergyVector[i];
    EnergyVector.clear();
}

int AInterfaceToDepoScript::termination(int i)
{
  if (i<0 || i>PR.size()-1)
    {
      abort("Attempt to address non-existent particle");
      return -1;
    }
  return PR.at(i).History->Termination;
}

QString AInterfaceToDepoScript::terminationStr(int i)
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
    case 10: return "ellastic";
    case 11: return "stopped by monitor";
    default: return "unknown termination";
    }
}

double AInterfaceToDepoScript::dX(int i)
{
  if (i<0 || i>PR.size()-1)
    {
      abort("Attempt to address non-existent particle");
      return std::numeric_limits<double>::quiet_NaN();
    }
  return PR.at(i).History->dx;
}

double AInterfaceToDepoScript::dY(int i)
{
  if (i<0 || i>PR.size()-1)
    {
      abort("Attempt to address non-existent particle");
      return std::numeric_limits<double>::quiet_NaN();
    }
  return PR.at(i).History->dy;
}

double AInterfaceToDepoScript::dZ(int i)
{
  if (i<0 || i>PR.size()-1)
    {
      abort("Attempt to address non-existent particle");
      return std::numeric_limits<double>::quiet_NaN();
    }
  return PR.at(i).History->dz;
}

int AInterfaceToDepoScript::particleId(int i)
{
  if (i<0 || i>PR.size()-1)
    {
      abort("Attempt to address non-existent particle");
      return -1;
    }
  return PR.at(i).History->ParticleId;
}

int AInterfaceToDepoScript::sernum(int i)
{
  if (i<0 || i>PR.size()-1)
    {
      abort("Attempt to address non-existent particle");
      return -1;
    }
  return PR.at(i).History->index;
}

int AInterfaceToDepoScript::isSecondary(int i)
{
  if (i<0 || i>PR.size()-1)
    {
      abort("Attempt to address non-existent particle");
      return false;
    }
  return PR.at(i).History->isSecondary();
}

int AInterfaceToDepoScript::getParent(int i)
{
  if (i<0 || i>PR.size()-1)
    {
      abort("Attempt to address non-existent particle");
      return -1;
    }
  return PR.at(i).History->SecondaryOf;
}

int AInterfaceToDepoScript::MaterialsCrossed_count(int i)
{
  if (i<0 || i>PR.size()-1)
    {
      abort("Attempt to address non-existent particle");
      return -1;
    }
  return PR.at(i).History->Deposition.size();
}

int AInterfaceToDepoScript::MaterialsCrossed_matId(int i, int m)
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

int AInterfaceToDepoScript::MaterialsCrossed_energy(int i, int m)
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

int AInterfaceToDepoScript::MaterialsCrossed_distance(int i, int m)
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

int AInterfaceToDepoScript::Deposition_countMaterials(int i)
{
  if (i<0 || i>PR.size()-1)
    {
      abort("Attempt to address non-existent particle");
      return -1;
    }
  return PR.at(i).Deposition.size();
}

int AInterfaceToDepoScript::Deposition_matId(int i, int m)
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

double AInterfaceToDepoScript::Deposition_startX(int i, int m)
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

double AInterfaceToDepoScript::Deposition_startY(int i, int m)
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

double AInterfaceToDepoScript::Deposition_startZ(int i, int m)
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
QString AInterfaceToDepoScript::Deposition_volumeName(int i, int m)
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
  TGeoManager* GeoManager = Detector->GeoManager;
  double* R = (double*)PR.at(i).Deposition.at(m).ByMaterial.first().R;
  TGeoNode* node = GeoManager->FindNode(R[0], R[1], R[2]);
  if (node) return QString(node->GetName());
  else return "";
}

int AInterfaceToDepoScript::Deposition_volumeIndex(int i, int m)
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
  TGeoManager* GeoManager = Detector->GeoManager;
  double* R = (double*)PR.at(i).Deposition.at(m).ByMaterial.first().R;
  TGeoNode* node = GeoManager->FindNode(R[0], R[1], R[2]);
  if (node) return node->GetNumber();
  else return -1;
}

double AInterfaceToDepoScript::Deposition_energy(int i, int m)
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

int AInterfaceToDepoScript::Deposition_countNodes(int i, int m)
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

double AInterfaceToDepoScript::Deposition_X(int i, int m, int n)
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

double AInterfaceToDepoScript::Deposition_Y(int i, int m, int n)
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

double AInterfaceToDepoScript::Deposition_Z(int i, int m, int n)
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

double AInterfaceToDepoScript::Deposition_dE(int i, int m, int n)
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

double AInterfaceToDepoScript::Deposition_dL(int i, int m, int n)
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

QVariantList AInterfaceToDepoScript::getAllDefinedTerminatorTypes()
{
    const QStringList defined = EventHistoryStructure::getAllDefinedTerminationTypes();
    QVariantList l;
    for (int i=0; i<defined.size(); ++i) l << QString::number(i)+"="+defined.at(i);
    return l;
}

