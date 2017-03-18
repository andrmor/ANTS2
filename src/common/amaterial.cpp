#include "amaterial.h"
#include "aparticle.h"
#include "ajsontools.h"
#include "amaterialparticlecolection.h"

#include "TH1D.h"

AMaterial::AMaterial()
{
  PrimarySpectrumHist = 0;
  SecondarySpectrumHist=0;
  GeoMat=0;
  GeoMed=0;

  clear();
}

double AMaterial::getRefractiveIndex(int iWave) const
{
  if (iWave == -1 || nWaveBinned.isEmpty()) return n;
  return nWaveBinned[iWave];
}

double AMaterial::getAbsorptionCoefficient(int iWave) const
{
  if (iWave == -1 || absWaveBinned.isEmpty()) return abs;
  return absWaveBinned[iWave];
}

void AMaterial::clear()
{
  name = "Undefined";
  n = 1;
  density = atomicDensity = p1 = p2 = p3 = abs = rayleighMFP = reemissionProb = 0;
  e_driftVelocity = W = SecYield = PriScintDecayTime = SecScintDecayTime = 0;
  rayleighWave = 500.0;
  Comments = "";

  rayleighBinned.clear();
  nWave_lambda.clear();
  nWave.clear();
  nWaveBinned.clear();
  absWave_lambda.clear();
  absWave.clear();
  absWaveBinned.clear();
  PrimarySpectrum_lambda.clear();
  PrimarySpectrum.clear();
  SecondarySpectrum_lambda.clear();
  SecondarySpectrum.clear();

  if(PrimarySpectrumHist)
    {
      delete PrimarySpectrumHist;
      PrimarySpectrumHist=0;
    }
  if (SecondarySpectrumHist)
    {
      delete SecondarySpectrumHist;
      SecondarySpectrumHist = 0;
    }

  MatParticle.clear();

  GeoMat = 0; //if created, deleted by TGeomanager
  GeoMed = 0; //if created, deleted by TGeomanager

  //Do not touch overrides - handled by loaded (want to keep overrides intact when handling inspector window)
}

void AMaterial::writeToJson(QJsonObject &json, QVector<AParticle *> *ParticleCollection)
{
  //general data
  json["*MaterialName"] = name;
  json["Density"] = density;
  //if (atomicDensity>0)
    json["IsotopeDensity"] = atomicDensity;
  json["RefractiveIndex"] = n;
  json["BulkAbsorption"] = abs;
  //if (rayleighMFP > 0)
    {
      json["RayleighMFP"] = rayleighMFP;
      json["RayleighWave"] = rayleighWave;
    }
  json["ReemissionProb"] = reemissionProb;
  json["PrimScint_Tau"] = PriScintDecayTime;
  json["W"] = W;
  json["SecScint_PhYield"] = SecYield;
  json["SecScint_Tau"] = SecScintDecayTime;
  json["ElDriftVelo"] = e_driftVelocity;
  if (p1!=0 || p2!=0 || p3!=0)
    {
      json["TGeoP1"] = p1;
      json["TGeoP2"] = p2;
      json["TGeoP3"] = p3;
    }
  //if (!Comments.isEmpty())
    json["Comments"] = Comments;

  //wavelength-resolved data
  //if (nWave_lambda.size() > 0)
    {
      QJsonArray ar;
      writeTwoQVectorsToJArray(nWave_lambda, nWave, ar);
      json["RefractiveIndexWave"] = ar;
    }
  //if (absWave_lambda.size() >0)
    {
      QJsonArray ar;
      writeTwoQVectorsToJArray(absWave_lambda, absWave, ar);
      json["BulkAbsorptionWave"] = ar;
    }
  //if (PrimarySpectrum_lambda.size() >0)
    {
      QJsonArray ar;
      writeTwoQVectorsToJArray(PrimarySpectrum_lambda, PrimarySpectrum, ar);
      json["PrimScintSpectrum"] = ar;
    }
  //if (SecondarySpectrum_lambda.size() >0)
    {
      QJsonArray ar;
      writeTwoQVectorsToJArray(SecondarySpectrum_lambda, SecondarySpectrum, ar);
      json["SecScintSpectrum"] = ar;
    }

  //MatParticle properties
  //if a particle has default configuration (TrackingAllowed and MatIsTransparent), skip its record
  QJsonArray jParticleEntries;
  for (int ip=0; ip<ParticleCollection->size(); ip++)
    {
      //default properties?
      //if (MatParticle[ip].TrackingAllowed && MatParticle[ip].MaterialIsTransparent && MatParticle[ip].InteractionDataF.isEmpty()) continue; //skip
      //writing info on this particle
      QJsonObject jMatParticle;

      QJsonObject jparticle;
      ParticleCollection->at(ip)->writeToJson(jparticle);
      jMatParticle["*Particle"] = jparticle;
      jMatParticle["TrackingAllowed"] = MatParticle[ip].TrackingAllowed;
      //if (MatParticle[ip].TrackingAllowed) //if not, no need for anything else
      {
        jMatParticle["MatIsTransparent"] = MatParticle[ip].MaterialIsTransparent;
        jMatParticle["PrimScintPhYield"] = MatParticle[ip].PhYield;
        jMatParticle["IntrEnergyRes"] = MatParticle[ip].IntrEnergyRes;
        jMatParticle["DataSource"] = MatParticle[ip].DataSource;
        jMatParticle["DataString"] = MatParticle[ip].DataString;
        if (MatParticle[ip].InteractionDataF.size() > 0)
          {
            QJsonArray iar;
            writeTwoQVectorsToJArray(MatParticle[ip].InteractionDataX, MatParticle[ip].InteractionDataF, iar);
            jMatParticle["TotalInteraction"] = iar;
            //gamma-specific data
            if ((*ParticleCollection)[ip]->type == AParticle::_gamma_)
              {
                QJsonArray jgamma;
                int iTerminators = MatParticle[ip].Terminators.size();
                for (int iTerm=0; iTerm<iTerminators; iTerm++)
                  {
                    QJsonObject jterm;
                    jterm["ReactionType"] = MatParticle[ip].Terminators[iTerm].Type;
                    QJsonArray ar;
                    writeTwoQVectorsToJArray(MatParticle[ip].Terminators[iTerm].PartialCrossSectionEnergy, MatParticle[ip].Terminators[iTerm].PartialCrossSection, ar);
                    jterm["InteractionData"] = ar;
                    jgamma.append(jterm);
                  }
                jMatParticle["GammaTerminators"] = jgamma;
              }
            //neutron-specific data
            if ((*ParticleCollection)[ip]->type == AParticle::_neutron_)
              {
                QJsonArray jneutron;
                int iTerminators = MatParticle[ip].Terminators.size();
                for (int iTerm=0; iTerm<iTerminators; iTerm++)
                  {
                    QJsonObject jterm;
                    jterm["Branching"] = MatParticle[ip].Terminators[iTerm].branching;
                    jterm["ReactionType"] = 2;  //can be only capture
                    //going through secondary particles
                    QJsonArray jsecondaries;
                    for (int is=0; is<MatParticle[ip].Terminators[iTerm].GeneratedParticles.size();is++ )
                      {
                        QJsonObject jsecpart;
                        int pa = MatParticle[ip].Terminators[iTerm].GeneratedParticles[is];
                        QJsonObject jj;
                        (*ParticleCollection)[pa]->writeToJson(jj);
                        jsecpart["SecParticle"] = jj;
                        jsecpart["energy"] = MatParticle[ip].Terminators[iTerm].GeneratedParticleEnergies[is];
                        jsecondaries.append(jsecpart);
                      }
                    jterm["Secondaries"] = jsecondaries;
                    jneutron.append(jterm);
                  }
                jMatParticle["NeutronTerminators"] = jneutron;
              }
          }
        else jMatParticle["MatIsTransparent"] = true; //if tracking allowed, but total interaction is NOT defined, make material transparent
      }
      //appending this particle entry to the json array
      jParticleEntries.append(jMatParticle);
    }
  //if not empty, appending array with Matparticle properties to main json object
  if (!jParticleEntries.isEmpty()) json["MatParticles"] = jParticleEntries;
}

bool AMaterial::readFromJson(QJsonObject &json, AMaterialParticleCollection *MpCollection)
{
  clear(); //clear all settings and set default properties

  //general data
  parseJson(json, "*MaterialName", name);
  parseJson(json, "Density", density);
  parseJson(json, "IsotopeDensity", atomicDensity);
  parseJson(json, "RefractiveIndex", n);
  parseJson(json, "BulkAbsorption", abs);
  parseJson(json, "RayleighMFP", rayleighMFP);
  parseJson(json, "RayleighWave", rayleighWave);
  parseJson(json, "ReemissionProb", reemissionProb);
  parseJson(json, "PrimScint_Tau", PriScintDecayTime);
  parseJson(json, "W", W);
  parseJson(json, "SecScint_PhYield", SecYield);
  parseJson(json, "SecScint_Tau", SecScintDecayTime);
  parseJson(json, "ElDriftVelo", e_driftVelocity);
  parseJson(json, "TGeoP1", p1);
  parseJson(json, "TGeoP2", p2);
  parseJson(json, "TGeoP3", p3);
  parseJson(json, "Comments", Comments);

  //wavelength-resolved data
  if (json.contains("RefractiveIndexWave"))
    {
      QJsonArray ar = json["RefractiveIndexWave"].toArray();
      readTwoQVectorsFromJArray(ar, nWave_lambda, nWave);
    }
  if (json.contains("BulkAbsorptionWave"))
    {
      QJsonArray ar = json["BulkAbsorptionWave"].toArray();
      readTwoQVectorsFromJArray(ar, absWave_lambda, absWave);
    }
  if (json.contains("PrimScintSpectrum"))
    {
      QJsonArray ar = json["PrimScintSpectrum"].toArray();
      readTwoQVectorsFromJArray(ar, PrimarySpectrum_lambda, PrimarySpectrum);
    }
  if (json.contains("SecScintSpectrum"))
    {
      QJsonArray ar = json["SecScintSpectrum"].toArray();
      readTwoQVectorsFromJArray(ar, SecondarySpectrum_lambda, SecondarySpectrum);
    }

  //MatParticle properties
  //filling default properties for all particles
  int numParticles = MpCollection->countParticles();
  MatParticle.resize(numParticles);
  for (int ip=0; ip<numParticles; ip++)
    {
      MatParticle[ip].TrackingAllowed = true;
      MatParticle[ip].MaterialIsTransparent = true;
    }
  // reading defined properties
  QJsonArray jParticleEntries = json["MatParticles"].toArray();
  for (int index=0; index<jParticleEntries.size(); index++)
    {
      QJsonObject jMatParticle = jParticleEntries[index].toObject();

      QJsonObject jparticle = jMatParticle["*Particle"].toObject();
      int ip = MpCollection->findOrCreateParticle(jparticle);

      parseJson(jMatParticle, "TrackingAllowed", MatParticle[ip].TrackingAllowed);
      parseJson(jMatParticle, "MatIsTransparent", MatParticle[ip].MaterialIsTransparent);
      parseJson(jMatParticle, "PrimScintPhYield", MatParticle[ip].PhYield);

      MatParticle[ip].IntrEnergyRes = 0;
      parseJson(jMatParticle, "IntrEnergyRes", MatParticle[ip].IntrEnergyRes);

      MatParticle[ip].DataSource.clear();
      MatParticle[ip].DataString.clear();
      parseJson(jMatParticle, "DataSource", MatParticle[ip].DataSource);
      parseJson(jMatParticle, "DataString", MatParticle[ip].DataString);

      if (jMatParticle.contains("TotalInteraction"))
        {
          QJsonArray iar = jMatParticle["TotalInteraction"].toArray();
          readTwoQVectorsFromJArray(iar, MatParticle[ip].InteractionDataX, MatParticle[ip].InteractionDataF);
        }
      if (jMatParticle.contains("GammaTerminators"))
        { //gamma-specific data
          QJsonArray jgamma = jMatParticle["GammaTerminators"].toArray();
          int numTerms = jgamma.size();
          MatParticle[ip].Terminators.resize(numTerms);
          for (int iTerm=0; iTerm<numTerms; iTerm++)
            {
              QJsonObject jterm = jgamma[iTerm].toObject();
              //parseJson(jterm, "ReactionType", MatParticle[ip].Terminators[iTerm].Type);
              if (jterm.contains("ReactionType"))
                  MatParticle[ip].Terminators[iTerm].Type = static_cast<NeutralTerminatorStructure::ReactionType>(jterm["ReactionType"].toInt());
              QJsonArray ar = jterm["InteractionData"].toArray();
              readTwoQVectorsFromJArray(ar, MatParticle[ip].Terminators[iTerm].PartialCrossSectionEnergy, MatParticle[ip].Terminators[iTerm].PartialCrossSection);
            }
        }
      if (jMatParticle.contains("NeutronTerminators"))
        { //neutron-specific data
          QJsonArray jneutron = jMatParticle["NeutronTerminators"].toArray();
          int numTerms = jneutron.size();
          MatParticle[ip].Terminators.resize(numTerms);
          //qDebug() << "Terminators:"<<numTerms;
          for (int iTerm=0; iTerm<numTerms; iTerm++)
            {
              QJsonObject jterm = jneutron[iTerm].toObject();
              parseJson(jterm, "Branching", MatParticle[ip].Terminators[iTerm].branching);
              MatParticle[ip].Terminators[iTerm].Type = NeutralTerminatorStructure::Capture; //2, can only be capture
              //going through secondary particles
              QJsonArray jsecondaries = jterm["Secondaries"].toArray();
              int numSecondaries = jsecondaries.size();
              //qDebug() << "Secondaries"<<numSecondaries;
              MatParticle[ip].Terminators[iTerm].GeneratedParticles.resize(numSecondaries);
              MatParticle[ip].Terminators[iTerm].GeneratedParticleEnergies.resize(numSecondaries);
              for (int is=0; is<numSecondaries;is++ )
                {
                  QJsonObject jsecpart = jsecondaries[is].toObject();

                  QJsonObject jsp = jsecpart["SecParticle"].toObject();
                  int isp = MpCollection->findOrCreateParticle(jsp);
                  //qDebug() << "Secondary:"<<isp;
                  MatParticle[ip].Terminators[iTerm].GeneratedParticles[is] = isp;
                  parseJson(jsecpart, "energy", MatParticle[ip].Terminators[iTerm].GeneratedParticleEnergies[is]);
                }
              //using branchings to calculate partical cross sections (can be changed in the future!)
              MatParticle[ip].Terminators[iTerm].PartialCrossSectionEnergy = MatParticle[ip].InteractionDataX;
              MatParticle[ip].Terminators[iTerm].PartialCrossSection.resize(0);
              for (int i=0; i<MatParticle[ip].InteractionDataF.size(); i++)
                MatParticle[ip].Terminators[iTerm].PartialCrossSection.append(MatParticle[ip].Terminators[iTerm].branching * MatParticle[ip].InteractionDataF[i]);
            }
        }
    }
  return true;
}

MatParticleStructure::MatParticleStructure()
{
  PhYield = 0;
  IntrEnergyRes = 0;
  TrackingAllowed = false;
  MaterialIsTransparent = true;
}

bool MatParticleStructure::CalculateTotal() //true - success, false - mismatch in binning of the data
{
  InteractionDataX = Terminators[0].PartialCrossSectionEnergy;
  InteractionDataF = Terminators[0].PartialCrossSection;
  for (int i=1; i<Terminators.size(); i++)
      for (int j=0; j<Terminators[0].PartialCrossSectionEnergy.size(); j++)
        {
          if (Terminators[i].PartialCrossSectionEnergy[j] != Terminators[0].PartialCrossSectionEnergy[j]) return false;// mismatch!
          InteractionDataF[j] += Terminators[i].PartialCrossSection[j];
        }
  return true;
}
