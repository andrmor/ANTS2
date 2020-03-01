#include "amaterial.h"
#include "aparticle.h"
#include "ajsontools.h"
#include "amaterialparticlecolection.h"
#include "achemicalelement.h"
#include "acommonfunctions.h"
#include "afiletools.h"

#ifdef __USE_ANTS_NCRYSTAL__
#include "NCrystal/NCrystal.hh"
#include "arandomgenncrystal.h"
#endif

#include <QStandardPaths>
#include <QFile>
#include <QDebug>

#include "TH1D.h"
#include "TRandom2.h"

AMaterial::AMaterial()
{
    clear();
}

double AMaterial::getPhotonYield(int iParticle) const
{
    if (iParticle < 0 || iParticle >= MatParticle.size()) return PhotonYieldDefault;

    const double & py = MatParticle.at(iParticle).PhYield;
    if (py == -1) return PhotonYieldDefault;
    return py;
}

double AMaterial::getRefractiveIndex(int iWave) const
{
  if (iWave == -1 || nWaveBinned.isEmpty()) return n;
  return nWaveBinned.at(iWave);
}

double AMaterial::getAbsorptionCoefficient(int iWave) const
{
  //qDebug() << iWave << ( absWaveBinned.size()  >0 ? absWaveBinned.at(iWave) : -1 ) ;
  if (iWave == -1 || absWaveBinned.isEmpty()) return abs;
  return absWaveBinned.at(iWave);
}

double AMaterial::getReemissionProbability(int iWave) const
{
    //qDebug() << "reemis->" << iWave << ( reemissionProbBinned.size() > 0 ? reemissionProbBinned.at(iWave) : reemissionProb );
    if (iWave == -1 || reemissionProbBinned.isEmpty()) return reemissionProb;
    return reemissionProbBinned.at(iWave);
}

void AMaterial::generateTGeoMat()
{
    GeoMat = ChemicalComposition.generateTGeoMaterial(name.toLocal8Bit().data(), density);
}

double AMaterial::FT(double td, double tr, double t) const
{
    return 1.0 - ((tr + td) / td) * exp(- t / td) + (tr / td) * exp(- t * (1.0 / tr + 1.0 / td));
}

double single_exp(double t, double tau2)
{
    return std::exp(-1.0*t/tau2)/tau2;
}
double bi_exp(double t, double tau1,double tau2)
{
    return std::exp(-1.0*t/tau2)*(1-std::exp(-1.0*t/tau1))/tau2/tau2*(tau1+tau2);
}
double AMaterial::GeneratePrimScintTime(TRandom2 * RandGen) const
{
    //select decay time component
    double DecayTime = 0;
    if (PriScint_Decay.size() == 1)
        DecayTime = PriScint_Decay.first().value;
    else
    {
        //selecting decay time component
        const double generatedStatWeight = _PrimScintSumStatWeight_Decay * RandGen->Rndm();
        double cumulativeStatWeight = 0;
        for (int i=0; i<PriScint_Decay.size(); i++)
        {
            cumulativeStatWeight += PriScint_Decay.at(i).statWeight;
            if (generatedStatWeight < cumulativeStatWeight)
            {
                DecayTime = PriScint_Decay.at(i).value;
                break;
            }
        }
    }

    if (DecayTime == 0)
        return 0; // decay time is 0 -> rise time is ignored

    //select rise time component
    double RiseTime = 0;
    if (PriScint_Raise.size() == 1)
        RiseTime = PriScint_Raise.first().value;
    else
    {
        //selecting raise time component
        const double generatedStatWeight = _PrimScintSumStatWeight__Raise * RandGen->Rndm();
        double cumulativeStatWeight = 0;
        for (int i=0; i<PriScint_Raise.size(); i++)
        {
            cumulativeStatWeight += PriScint_Raise.at(i).statWeight;
            if (generatedStatWeight < cumulativeStatWeight)
            {
                RiseTime = PriScint_Raise.at(i).value;
                break;
            }
        }
    }

    if (RiseTime == 0)
        return RandGen->Exp(DecayTime);

    double EmissionTime = 0;
    // From G4Scintillation of Geant4
    double d = (RiseTime + DecayTime) / DecayTime;
    while (true)
    {
        double ran1 = RandGen->Rndm();
        double ran2 = RandGen->Rndm();
        EmissionTime = -1.0 * DecayTime * std::log(1.0 - ran1);
        double gg = d * single_exp(EmissionTime, DecayTime);
        if (ran2 <= bi_exp(EmissionTime, RiseTime, DecayTime) / gg)
            break;
    }

    return EmissionTime;
}

void AMaterial::updateNeutronDataOnCompositionChange(const AMaterialParticleCollection *MPCollection)
{
    //      qDebug() << "Updating neutron data";
    int neutronId = MPCollection->getNeutronIndex();
    if (neutronId == -1) return; //not dfefined in this configuration

    QVector<NeutralTerminatorStructure>& Terminators = MatParticle[neutronId].Terminators;
    Terminators.resize(2);
    Terminators[0].Type = NeutralTerminatorStructure::Absorption;
    Terminators[1].Type = NeutralTerminatorStructure::ElasticScattering;

    for (int iTerm=0; iTerm<2; iTerm++)
    {
        NeutralTerminatorStructure& t = Terminators[iTerm];
        QVector<ANeutronInteractionElement> NewIsotopeRecords;
        for (int iEl=0; iEl<ChemicalComposition.countElements(); iEl++)
        {
            const AChemicalElement* El = ChemicalComposition.getElement(iEl);
            for (int iIso=0; iIso<El->countIsotopes(); iIso++)
            {
                //      qDebug() << "==Creating ANeutronInteractionElement for " << El->Symbol << El->Isotopes.at(iIso).Mass << ":";
                bool bAlreadyExists = false;
                for (ANeutronInteractionElement& absEl : t.IsotopeRecords)
                    if (absEl.Name == El->Symbol && absEl.Mass == El->Isotopes.at(iIso).Mass)
                    {
                        //      qDebug() << "Found in old list, copying";
                        bAlreadyExists = true;
                        absEl.MolarFraction = El->MolarFraction*0.01 * El->Isotopes.at(iIso).Abundancy; //updating molar fraction
                        NewIsotopeRecords << absEl;
                        break;
                    }
                if (!bAlreadyExists)
                {
                    //      qDebug() << "Not found, creating new record";
                    NewIsotopeRecords << ANeutronInteractionElement(El->Isotopes.at(iIso).Symbol, El->Isotopes.at(iIso).Mass, El->MolarFraction*0.01*El->Isotopes.at(iIso).Abundancy);
                }
            }
        }
        t.IsotopeRecords = NewIsotopeRecords;
    }
}

void AMaterial::updateRuntimeProperties(bool bLogLogInterpolation, TRandom2* RandGen, int numThreads)
{
    for (int iP=0; iP<MatParticle.size(); iP++)
    {
       for (int iTerm=0; iTerm<MatParticle[iP].Terminators.size(); iTerm++)
       {
           //qDebug() << "-----"<<name << iP << iTerm;
           MatParticle[iP].Terminators[iTerm].UpdateRunTimeProperties(bLogLogInterpolation, RandGen, numThreads, temperature);
       }
    }

    //updating sum stat weights for primary scintillation time generator
    _PrimScintSumStatWeight_Decay = 0;
    _PrimScintSumStatWeight__Raise = 0;
    for (const APair_ValueAndWeight& pair : PriScint_Decay)
        _PrimScintSumStatWeight_Decay += pair.statWeight;
    for (const APair_ValueAndWeight& pair : PriScint_Raise)
        _PrimScintSumStatWeight__Raise += pair.statWeight;
}

void AMaterial::UpdateRandGen(int ID, TRandom2 *RandGen)
{
    for (int iP=0; iP<MatParticle.size(); iP++)
    {
       for (int iTerm=0; iTerm<MatParticle[iP].Terminators.size(); iTerm++)
           MatParticle[iP].Terminators[iTerm].UpdateRandGen(ID, RandGen);
    }
}

void AMaterial::clear()
{
  name = "Undefined";
  n = 1.0;
  density = p1 = p2 = p3 = abs = rayleighMFP = reemissionProb = 0;
  temperature = 298.0;
  e_driftVelocity = W = SecYield = SecScintDecayTime = e_diffusion_L = e_diffusion_T = 0;
  rayleighWave = 500.0;
  Comments = "";

  PriScint_Decay.clear();
  PriScint_Decay << APair_ValueAndWeight(0, 1.0);
  PriScint_Raise.clear();
  PriScint_Raise << APair_ValueAndWeight(0, 1.0);

  rayleighBinned.clear();

  nWave_lambda.clear();
  nWave.clear();
  nWaveBinned.clear();

  absWave_lambda.clear();
  absWave.clear();
  absWaveBinned.clear();

  reemisProbWave.clear();
  reemisProbWave_lambda.clear();
  reemissionProbBinned.clear();

  PrimarySpectrum_lambda.clear();
  PrimarySpectrum.clear();

  SecondarySpectrum_lambda.clear();
  SecondarySpectrum.clear();

  if(PrimarySpectrumHist)
    {
      delete PrimarySpectrumHist;
      PrimarySpectrumHist = 0;
    }
  if (SecondarySpectrumHist)
    {
      delete SecondarySpectrumHist;
      SecondarySpectrumHist = 0;
    }

  MatParticle.clear();

  GeoMat = 0; //if created, deleted by TGeoManager
  GeoMed = 0; //if created, deleted by TGeoManager

  //Do not touch overrides - handled by loaded (want to keep overrides intact when handling inspector window)
}

void AMaterial::writeToJson(QJsonObject &json, AMaterialParticleCollection* MpCollection) const
{
  json["*MaterialName"] = name;
  json["Density"] = density;
  json["Temperature"] = temperature;
  json["ChemicalComposition"] = ChemicalComposition.writeToJson();
  json["RefractiveIndex"] = n;
  json["BulkAbsorption"] = abs;
  json["RayleighMFP"] = rayleighMFP;
  json["RayleighWave"] = rayleighWave;
  json["ReemissionProb"] = reemissionProb;

  json["PhotonYieldDefault"] = PhotonYieldDefault;

  {
    QJsonArray ar;
    for (const APair_ValueAndWeight& pair : PriScint_Decay)
    {
        QJsonArray el;
        el << pair.value << pair.statWeight;
        ar.append(el);
    }
    json["PrimScintDecay"] = ar;
  }

  {
    QJsonArray ar;
    for (const APair_ValueAndWeight& pair : PriScint_Raise)
    {
        QJsonArray el;
        el << pair.value << pair.statWeight;
        ar.append(el);
    }
    json["PrimScintRaise"] = ar;
  }

  json["W"] = W;
  json["SecScint_PhYield"] = SecYield;
  json["SecScint_Tau"] = SecScintDecayTime;
  json["ElDriftVelo"] = e_driftVelocity;
  json["ElDiffusionL"] = e_diffusion_L;
  json["ElDiffusionT"] = e_diffusion_T;

  {
      json["TGeoP1"] = p1;
      json["TGeoP2"] = p2;
      json["TGeoP3"] = p3;
  }

  json["Comments"] = Comments;

  {
      QJsonArray ar;
      writeTwoQVectorsToJArray(nWave_lambda, nWave, ar);
      json["RefractiveIndexWave"] = ar;
  }

  {
      QJsonArray ar;
      writeTwoQVectorsToJArray(absWave_lambda, absWave, ar);
      json["BulkAbsorptionWave"] = ar;
  }

  {
      QJsonArray ar;
      writeTwoQVectorsToJArray(reemisProbWave_lambda, reemisProbWave, ar);
      json["ReemissionProbabilityWave"] = ar;
  }

  {
      QJsonArray ar;
      writeTwoQVectorsToJArray(PrimarySpectrum_lambda, PrimarySpectrum, ar);
      json["PrimScintSpectrum"] = ar;
  }

  {
      QJsonArray ar;
      writeTwoQVectorsToJArray(SecondarySpectrum_lambda, SecondarySpectrum, ar);
      json["SecScintSpectrum"] = ar;
  }

  {
      QJsonArray ar;
      for (const QString & s : Tags) ar.append(s);
      json["*Tags"] = ar;
  }

  //MatParticle properties
  //if a particle has default configuration (TrackingAllowed and MatIsTransparent), skip its record
  QJsonArray jParticleEntries;
  const QVector<AParticle *> *ParticleCollection = MpCollection->getParticleCollection();
  for (int ip=0; ip<ParticleCollection->size(); ip++)
  {
      QJsonObject jMatParticle;

      QJsonObject jparticle;
      ParticleCollection->at(ip)->writeToJson(jparticle);
      jMatParticle["*Particle"] = jparticle;
      jMatParticle["TrackingAllowed"] = MatParticle[ip].TrackingAllowed;
      jMatParticle["MatIsTransparent"] = MatParticle[ip].MaterialIsTransparent;
      jMatParticle["PrimScintPhYield"] = MatParticle[ip].PhYield;
      jMatParticle["IntrEnergyRes"] = MatParticle[ip].IntrEnergyRes;
      jMatParticle["DataSource"] = MatParticle[ip].DataSource;
      jMatParticle["DataString"] = MatParticle[ip].DataString;

      if ( MpCollection->getParticleType(ip) == AParticle::_neutron_ )
      {
          jMatParticle["CaptureEnabled"] = MatParticle[ip].bCaptureEnabled;
          jMatParticle["ElasticEnabled"] = MatParticle[ip].bElasticEnabled;
          jMatParticle["UseNCrystal"] = MatParticle[ip].bUseNCrystal;
          jMatParticle["AllowAbsentCsData"] = MatParticle[ip].bAllowAbsentCsData;
      }

      QJsonArray iar;
      writeTwoQVectorsToJArray(MatParticle[ip].InteractionDataX, MatParticle[ip].InteractionDataF, iar);
      jMatParticle["TotalInteraction"] = iar;

      if ( MpCollection->getParticleType(ip) != AParticle::_charged_)
      {
          QJsonArray ar;
          for (int iTerm=0; iTerm<MatParticle[ip].Terminators.size(); iTerm++)
          {
              QJsonObject jterm;
              MatParticle[ip].Terminators.at(iTerm).writeToJson(jterm, MpCollection);
              ar << jterm;
          }
          jMatParticle["Terminators"] = ar;
      }

      //appending this particle entry to the json array
      jParticleEntries.append(jMatParticle);
  }
  //if not empty, appending array with Matparticle properties to main json object
  if (!jParticleEntries.isEmpty()) json["MatParticles"] = jParticleEntries;
}

bool AMaterial::readFromJson(QJsonObject &json, AMaterialParticleCollection *MpCollection, QVector<QString> SuppressParticles)
{
  clear(); //clear all settings and set default properties
  //general data
  parseJson(json, "*MaterialName", name);
  parseJson(json, "Density", density);
  temperature = 298.0; //compatibility
  parseJson(json, "Temperature", temperature);
  if (json.contains("ChemicalComposition"))
  {
      QJsonObject ccjson = json["ChemicalComposition"].toObject();
      ChemicalComposition.readFromJson(ccjson);
  }
  else ChemicalComposition.clear();
  parseJson(json, "RefractiveIndex", n);
  parseJson(json, "BulkAbsorption", abs);
  parseJson(json, "RayleighMFP", rayleighMFP);
  parseJson(json, "RayleighWave", rayleighWave);
  parseJson(json, "ReemissionProb", reemissionProb);
  //PhotonYieldDefault for compatibility at the end

  if (json.contains("PrimScint_Tau")) //compatibility
  {
      double tau = json["PrimScint_Tau"].toDouble();
      PriScint_Decay.clear();
      PriScint_Decay << APair_ValueAndWeight(tau, 1.0);
  }
  if (json.contains("PrimScint_Decay")) //compatibility
  {
      PriScint_Decay.clear();
      if (json["PrimScint_Decay"].isArray())
      {
          QJsonArray ar = json["PrimScint_Decay"].toArray();
          for (int i=0; i<ar.size(); i++)
          {
              QJsonArray el = ar.at(i).toArray();
              if (el.size() == 2)
                PriScint_Decay << APair_ValueAndWeight(el.at(1).toDouble(), el.at(0).toDouble());
              else
                  qWarning() << "Bad size of decay time pair, skipping!";
          }
      }
      else
      {
          double tau = json["PrimScint_Decay"].toDouble();
          PriScint_Decay << APair_ValueAndWeight(tau, 1.0);
      }
  }
  if (json.contains("PrimScint_Raise")) //compatibility
  {
      PriScint_Raise.clear();
      if (json["PrimScint_Raise"].isArray())
      {
          QJsonArray ar = json["PrimScint_Raise"].toArray();
          for (int i=0; i<ar.size(); i++)
          {
              QJsonArray el = ar.at(i).toArray();
              if (el.size() == 2)
                PriScint_Raise << APair_ValueAndWeight(el.at(1).toDouble(), el.at(0).toDouble());
              else
                  qWarning() << "Bad size of raise time pair, skipping!";
          }
      }
      else
      {
          //compatibility
          double tau = json["PrimScint_Raise"].toDouble();
          PriScint_Raise << APair_ValueAndWeight(tau, 1.0);
      }
  }
  if (json.contains("PrimScintDecay"))
  {
      PriScint_Decay.clear();
      if (json["PrimScintDecay"].isArray())
      {
          QJsonArray ar = json["PrimScintDecay"].toArray();
          for (int i=0; i<ar.size(); i++)
          {
              QJsonArray el = ar.at(i).toArray();
              if (el.size() == 2)
                PriScint_Decay << APair_ValueAndWeight(el.at(0).toDouble(), el.at(1).toDouble());
              else
                  qWarning() << "Bad size of decay time pair, skipping!";
          }
      }
      else
      {
          double tau = json["PrimScintDecay"].toDouble();
          PriScint_Decay << APair_ValueAndWeight(tau, 1.0);
      }
  }
  if (json.contains("PrimScintRaise"))
  {
      PriScint_Raise.clear();
      if (json["PrimScintRaise"].isArray())
      {
          QJsonArray ar = json["PrimScintRaise"].toArray();
          for (int i=0; i<ar.size(); i++)
          {
              QJsonArray el = ar.at(i).toArray();
              if (el.size() == 2)
                PriScint_Raise << APair_ValueAndWeight(el.at(0).toDouble(), el.at(1).toDouble());
              else
                  qWarning() << "Bad size of raise time pair, skipping!";
          }
      }
      else
      {
          //compatibility
          double tau = json["PrimScintRaise"].toDouble();
          PriScint_Raise << APair_ValueAndWeight(tau, 1.0);
      }
  }

  parseJson(json, "W", W);
  parseJson(json, "SecScint_PhYield", SecYield);
  parseJson(json, "SecScint_Tau", SecScintDecayTime);
  parseJson(json, "ElDriftVelo", e_driftVelocity);
  parseJson(json, "ElDiffusionL", e_diffusion_L);
  parseJson(json, "ElDiffusionT", e_diffusion_T);
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
  if (json.contains("ReemissionProbabilityWave"))
  {
      QJsonArray ar = json["ReemissionProbabilityWave"].toArray();
      readTwoQVectorsFromJArray(ar, reemisProbWave_lambda, reemisProbWave);
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

  Tags.clear();
  if (json.contains("*Tags"))
  {
      QJsonArray ar = json["*Tags"].toArray();
      for (int i=0; i<ar.size(); i++) Tags << ar[i].toString();
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
  for (int index = 0; index < jParticleEntries.size(); index++)
  {
      QJsonObject jMatParticle = jParticleEntries[index].toObject();

      QJsonObject jparticle = jMatParticle["*Particle"].toObject();
      AParticle pa;
      pa.readFromJson(jparticle);
      bool bNewParticle = (MpCollection->findParticle(pa) == -1);
      if (bNewParticle && SuppressParticles.contains(pa.ParticleName))
          continue;

      int ip = MpCollection->findOrAddParticle(jparticle);
      MatParticle.resize(MpCollection->countParticles());
      MatParticleStructure & MP = MatParticle[ip];

      parseJson(jMatParticle, "TrackingAllowed",  MP.TrackingAllowed);
      parseJson(jMatParticle, "MatIsTransparent", MP.MaterialIsTransparent);
      parseJson(jMatParticle, "PrimScintPhYield", MP.PhYield);

      MP.IntrEnergyRes = 0;
      parseJson(jMatParticle, "IntrEnergyRes", MP.IntrEnergyRes);

      MP.DataSource.clear();
      parseJson(jMatParticle, "DataSource", MP.DataSource);
      MP.DataString.clear();
      parseJson(jMatParticle, "DataString", MP.DataString);

      MP.bCaptureEnabled = true; //compatibility
      parseJson(jMatParticle, "CaptureEnabled", MP.bCaptureEnabled);
      MP.bElasticEnabled = false; //compatibility
      parseJson(jMatParticle, "EllasticEnabled", MP.bElasticEnabled); //old configs were with this typo
      parseJson(jMatParticle, "ElasticEnabled",  MP.bElasticEnabled);
      MP.bUseNCrystal = false; //compatibility
      parseJson(jMatParticle, "UseNCrystal", MP.bUseNCrystal);

      MP.bAllowAbsentCsData = false;
      parseJson(jMatParticle, "AllowAbsentCsData", MP.bAllowAbsentCsData);

      if (jMatParticle.contains("TotalInteraction"))
      {
          QJsonArray iar = jMatParticle["TotalInteraction"].toArray();
          readTwoQVectorsFromJArray(iar, MP.InteractionDataX, MP.InteractionDataF);
      }

      QJsonArray arTerm;
      parseJson(jMatParticle, "Terminators", arTerm);
      parseJson(jMatParticle, "GammaTerminators", arTerm); //compatibility
         // old neutron terminators are NOT compatible with new system
      if (!arTerm.isEmpty())
      {
          MP.Terminators.clear();
          for (int iTerm=0; iTerm<arTerm.size(); iTerm++)
            {
              QJsonObject jterm = arTerm[iTerm].toObject();
              NeutralTerminatorStructure newTerm;
              newTerm.readFromJson(jterm, MpCollection);
              MP.Terminators << newTerm;
            }
      }
      else
      {
          if (jMatParticle.contains("NeutronTerminators"))
          {
              qWarning() << "This config file contains old (incompatible) standard for neutron interaction data - material is set to transparent for neutrons!";
              //making material transparent
              MP.MaterialIsTransparent = true;
          }
      }
  }

  if (!parseJson(json, "PhotonYieldDefault", PhotonYieldDefault))
  {
      //qDebug() << "Compatibility mode: no PhotonYieldDefault found";
      PhotonYieldDefault = 0;
      if (numParticles != 0)
      {
          bool bAllSame = true;
          for (int iP = 1; iP < numParticles; iP++)
              if (MatParticle.at(iP).PhYield != MatParticle.at(0).PhYield)
              {
                  bAllSame = false;
                  break;
              }
          if (bAllSame) PhotonYieldDefault = MatParticle.at(0).PhYield;
      }
  }

  return true;
}

bool MatParticleStructure::CalculateTotalForGamma() //true - success, false - mismatch in binning of the data
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

void MatParticleStructure::Clear()
{
    TrackingAllowed = true;
    MaterialIsTransparent = true;
    PhYield = 0;
    IntrEnergyRes = 0;

    bCaptureEnabled = false;
    bElasticEnabled = false;
    bAllowAbsentCsData = false;

    InteractionDataX.clear();
    InteractionDataF.clear();

    Terminators.clear();

    DataSource.clear();
    DataString.clear();
}

ANeutronInteractionElement *NeutralTerminatorStructure::getNeutronInteractionElement(int index)
{
    if (index<0 || index>=IsotopeRecords.size()) return 0;
    return &IsotopeRecords[index];
}

const ANeutronInteractionElement *NeutralTerminatorStructure::getNeutronInteractionElement(int index) const
{
    if (index<0 || index>=IsotopeRecords.size()) return 0;
    return &IsotopeRecords[index];
}

static int fixCounter = 0;
void NeutralTerminatorStructure::UpdateRunTimeProperties(bool bUseLogLog, TRandom2* RandGen, int numThreads, double temp)
{
#ifdef  __USE_ANTS_NCRYSTAL__
    for (const NCrystal::Scatter * sc : NCrystal_scatters) sc->unref();
    NCrystal_scatters.clear();

    if ( !NCrystal_Ncmat.isEmpty() )
    {
        //    qDebug() << "Text in config:"<<NCrystal_Ncmat;
        QString dir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
        QString tmpFileName = dir + QString("/___tmp%1.ncmat").arg(fixCounter++); //NCrystal rememebers configs by name... and replaces with old if the name matches! :)
        //    qDebug() << "tmp file:"<<tmpFileName;
        SaveTextToFile(tmpFileName, NCrystal_Ncmat);

        QString settings = QString("%1;dcutoff=%2Aa;packfact=%3;temp=%4K").arg(tmpFileName).arg(NCrystal_Dcutoff).arg(NCrystal_Packing).arg(temp);
        //    qDebug() << "NCrystal options line:"<<settings;

        try
        {
            if (numThreads < 1) numThreads = 1;
            //  qDebug() << "Num threads:"<<numThreads;
            for (int i=0; i<numThreads; i++)
            {
                NCrystal::disableCaching();
                const NCrystal::Scatter * sc = NCrystal::createScatter( settings.toLatin1().data() );
                sc->ref();

                ARandomGenNCrystal* rnd = new ARandomGenNCrystal(*RandGen);
                const_cast<NCrystal::Scatter *>(sc)->setRandomGenerator(rnd);

                NCrystal_scatters.append(sc);
            }
        }
        catch (...)
        {
            qWarning() << "NCrystal has rejected the provided configuration!";
        }

        QFile f(tmpFileName);
        f.remove();
    }
#endif

    if (Type == ElasticScattering || Type == Absorption)
        if (!IsotopeRecords.isEmpty())
        {
            //      qDebug() << "Updating neutron cross-section data...";
            PartialCrossSectionEnergy.clear();
            PartialCrossSection.clear();
            //      qDebug() << "isotope records defined:"<<IsotopeRecords.size();
            for (int iElement=0; iElement<IsotopeRecords.size(); iElement++)
            {
                ANeutronInteractionElement& nie = IsotopeRecords[iElement];
                //  qDebug() << nie.Name << "-" << nie.Mass << "  with molar fraction:"<<nie.MolarFraction;

                //updating neutron absorption runtime properties (can be custom distr of deposited energy)
                nie.updateRuntimeProperties();

                //  qDebug() << "size of cross-section dataset"<<nie.Energy.size() << nie.CrossSection.size();
                if (nie.Energy.isEmpty()) continue;
                if (PartialCrossSectionEnergy.isEmpty())
                {
                    //this is first non-empty element - energy binning will be according to it
                    PartialCrossSectionEnergy = nie.Energy;
                    //first element cross-section times its molar fraction
                    for (int i=0; i<nie.CrossSection.size(); i++)
                        PartialCrossSection << nie.CrossSection.at(i) * nie.MolarFraction;
                }
                else
                {
                    //using interpolation to match already defined energy bins
                    for (int iEnergy=0; iEnergy<PartialCrossSectionEnergy.size(); iEnergy++)
                    {
                        const double& energy = PartialCrossSectionEnergy.at(iEnergy);
                        const double cs = GetInterpolatedValue(energy, &nie.Energy, &nie.CrossSection, bUseLogLog);
                        PartialCrossSection[iEnergy] += cs * nie.MolarFraction;
                    }
                }
            }
        }
    //      qDebug() << "...done!";
}

void NeutralTerminatorStructure::ClearProperties()
{
#ifdef  __USE_ANTS_NCRYSTAL__
    for (const NCrystal::Scatter * sc : NCrystal_scatters) sc->unref();
    NCrystal_scatters.clear();

    NCrystal_Ncmat.clear();
    NCrystal_Dcutoff = 0;
    NCrystal_Packing = 1.0;
#endif

    PartialCrossSectionEnergy.clear();
    PartialCrossSection.clear();

    for (int iElement=0; iElement<IsotopeRecords.size(); iElement++)
    {
        ANeutronInteractionElement& nie = IsotopeRecords[iElement];
        //  qDebug() << nie.Name << "-" << nie.Mass << "  with molar fraction:"<<nie.MolarFraction;
        //  qDebug() << "size of cross-section dataset"<<nie.Energy.size() << nie.CrossSection.size();
        nie.Energy.clear();
        nie.CrossSection.clear();
    }
}

void NeutralTerminatorStructure::UpdateRandGen(int ID, TRandom2 *RandGen)
{
#ifdef  __USE_ANTS_NCRYSTAL__
    if (NCrystal_scatters.isEmpty()) return; //nothing to update
    if (ID > -1 && ID < NCrystal_scatters.size())
    {
        ARandomGenNCrystal* rnd = new ARandomGenNCrystal(*RandGen);
        const NCrystal::Scatter * sc = NCrystal_scatters.at(ID);
        const_cast<NCrystal::Scatter *>(sc)->setRandomGenerator(rnd);
    }
    else
    {
        qWarning() << "|||---Error: Bad thread index" << ID << "while"<<NCrystal_scatters.size()<<"NCrystal scatters are defined!";
    }
#endif
}

void NeutralTerminatorStructure::writeToJson(QJsonObject &json, AMaterialParticleCollection *MpCollection) const
{
    json["ReactionType"] = Type;
    QJsonArray ar;
    writeTwoQVectorsToJArray( PartialCrossSectionEnergy, PartialCrossSection, ar);
    json["InteractionData"] = ar;

    QJsonArray irAr;
    for (int i=0; i<IsotopeRecords.size(); i++)
        irAr << IsotopeRecords[i].writeToJson(MpCollection);
    json["IsotopeRecords"] = irAr;

    if (Type == ElasticScattering)
    {
        json["NCrystal_Ncmat"] = NCrystal_Ncmat;
        json["NCystal_CutOff"] = NCrystal_Dcutoff;
        json["NCystal_Packing"] = NCrystal_Packing;
    }
}

void NeutralTerminatorStructure::readFromJson(const QJsonObject &json, AMaterialParticleCollection *MpCollection)
{
    int rtype = 5; //undefined
    parseJson(json, "ReactionType", rtype);
    Type = static_cast<NeutralTerminatorStructure::ReactionType>(rtype);

    QJsonArray ar = json["InteractionData"].toArray();
    readTwoQVectorsFromJArray(ar, PartialCrossSectionEnergy, PartialCrossSection);

    if (json.contains("IsotopeRecords"))
    {
        QJsonArray ar = json["IsotopeRecords"].toArray();
        IsotopeRecords.clear();
        for (int i=0; i<ar.size(); i++)
        {
            ANeutronInteractionElement el;
            QJsonObject js = ar[i].toObject();
            el.readFromJson(js, MpCollection);
            IsotopeRecords << el;
        }
    }

    NCrystal_Ncmat.clear();
    NCrystal_Dcutoff = 0;
    NCrystal_Packing = 1.0;
    parseJson(json, "NCrystal_Ncmat", NCrystal_Ncmat);
    parseJson(json, "NCystal_CutOff", NCrystal_Dcutoff);
    parseJson(json, "NCystal_Packing", NCrystal_Packing);
}

bool NeutralTerminatorStructure::isParticleOneOfSecondaries(int iPart) const
{
    if (Type != Absorption) return false;

    for (int ie=0; ie<IsotopeRecords.size(); ie++)
        for (int ir=0; ir<IsotopeRecords.at(ie).DecayScenarios.size(); ir++)
            for (int ig=0; ig<IsotopeRecords.at(ie).DecayScenarios.at(ir).GeneratedParticles.size(); ig++)
                if (IsotopeRecords.at(ie).DecayScenarios.at(ir).GeneratedParticles.at(ig).ParticleId == iPart)
                    return true;
    return false;
}

void NeutralTerminatorStructure::prepareForParticleRemove(int iPart)
{
    if (Type != Absorption) return;

    for (int ie=0; ie<IsotopeRecords.size(); ie++)
        for (int ir=0; ir<IsotopeRecords.at(ie).DecayScenarios.size(); ir++)
            for (int ig=0; ig<IsotopeRecords.at(ie).DecayScenarios.at(ir).GeneratedParticles.size(); ig++)
            {
                int& thisParticle = IsotopeRecords[ie].DecayScenarios[ir].GeneratedParticles[ig].ParticleId;
                if (thisParticle > iPart) thisParticle--;
            }
}

double NeutralTerminatorStructure::getNCrystalCrossSectionBarns(double energy_keV, int threadIndex) const
{
#ifdef  __USE_ANTS_NCRYSTAL__
    if (threadIndex < NCrystal_scatters.size())
    {
        return NCrystal_scatters.at(threadIndex)->crossSectionNonOriented(energy_keV * 1000.0); //energy to eV
    }
    else
    {
        qWarning() << "|||---Error: Bad thread index" << threadIndex << "while"<<NCrystal_scatters.size()<<"NCrystal scatters are defined!";
        return 0;
    }
#else
    return 0;
#endif
}

void NeutralTerminatorStructure::generateScatteringNonOriented(double energy_keV, double &angle, double &delta_ekin_keV, int threadIndex) const
{
#ifdef  __USE_ANTS_NCRYSTAL__
    if (threadIndex < NCrystal_scatters.size())
    {
        NCrystal_scatters.at(threadIndex)->generateScatteringNonOriented(energy_keV * 1000.0, angle, delta_ekin_keV); //input energy to eV, output is actually in eV !
        delta_ekin_keV *= 0.001; //from eV to keV
    }
    else
    {
        qWarning() << "|||---Error: Bad thread index" << threadIndex << "while"<<NCrystal_scatters.size()<<"NCrystal scatters are defined!";
        angle = 0;
        delta_ekin_keV = 0;
    }
#else
    angle = 0;
    delta_ekin_keV = 0;
#endif
}

const QString AMaterial::CheckMaterial(int iPart, const AMaterialParticleCollection* MpCollection) const
{
  const QString errInComposition = ChemicalComposition.checkForErrors();
  if (!errInComposition.isEmpty())
      return name + ": " + errInComposition;

  if (iPart<0 || iPart>=MpCollection->countParticles())
      return QString("Wrong particle index: ") + QString::number(iPart);

  const MatParticleStructure* mp = &MatParticle.at(iPart);
  //shortcuts - if particle tracking is disabled or material is transparent - no checks
  if ( !mp->TrackingAllowed ) return "";
  if (  mp->MaterialIsTransparent ) return "";

  const AParticle::ParticleType& pt = MpCollection->getParticleType(iPart);

  int interSize = mp->InteractionDataX.size();

  if (pt == AParticle::_charged_)
  {
      if (interSize == 0 || mp->InteractionDataF.size() == 0) return "Empty stopping power data";
      if (interSize != mp->InteractionDataF.size()) return "Mismatch in total interaction (X and F) length";
      if (interSize < 2) return "Stopping power data size is less than 2 points";
      for (int i=1; i<interSize; i++)
          if (mp->InteractionDataX[i-1] > mp->InteractionDataX[i])
              return "Stopping power data: total energy binning is not non-decreasing";
      return ""; //nothing else is needed to be checked for charged
  }

  if (pt == AParticle::_gamma_)
    {
      if (interSize == 0 || mp->InteractionDataF.size() == 0) return "Empty total interaction cross-section data";
      if (interSize != mp->InteractionDataF.size()) return "Mismatch in total interaction (X and F) length";
      if (interSize < 2) return "Total interaction cross-section data size is less than 2 points";
      for (int i=1; i<interSize; i++)
          if (mp->InteractionDataX[i-1] > mp->InteractionDataX[i])
              return "Total interaction cross-section data: total energy binning is not non-decreasing";

      const int terms = mp->Terminators.size();
      if (mp->Terminators.isEmpty()) return "No terminators defined for gamma";
      if (terms<2 || terms>3) return "Invalid number of terminators for gamma";

      if (mp->Terminators[0].Type != NeutralTerminatorStructure::Photoelectric)
          return "First terminator has to be photoelectric!";
      if (mp->Terminators[1].Type != NeutralTerminatorStructure::ComptonScattering)
          return "Second terminator has to be compton!";
      if (terms == 3)
        if (mp->Terminators[2].Type != NeutralTerminatorStructure::PairProduction)
            return "Third terminator has to be pair production!";

      //checking partial crossection for terminators
      for (int iTerm=0; iTerm<terms; iTerm++)
        {
          const QVector<double>*  e = &mp->Terminators[iTerm].PartialCrossSectionEnergy;
          const QVector<double>* cs = &mp->Terminators[iTerm].PartialCrossSection;
          if (e->size() != interSize || cs->size() != interSize)
              return "Mismatch between total and partial interaction data size";
          for (int i=0; i<interSize; i++)
              if (mp->InteractionDataX[i] != e->at(i))
                  return "Mismatch in energy of total and partial cross interaction datasets";
        }

      // confirminf that TotalInteraction is indeed the sum of the partial cross-sections
      for (int i=0; i<interSize; i++)
        {
          double sum = 0;
          for (int ite=0; ite<terms; ite++)
              sum += mp->Terminators[ite].PartialCrossSection[i];
          double delta =  sum - mp->InteractionDataF[i];
          if ( fabs(delta) > 0.0001*mp->InteractionDataF[i] )
              return "Total interaction in not equal to sum of partials";
        }

      //properties for gamma are properly defined!
      return "";
    }

  if (pt == AParticle::_neutron_)
    {
      if (!mp->bCaptureEnabled && !mp->bElasticEnabled) return "";

      int numTerm = mp->Terminators.size();
      if (numTerm != 2 ) return "Wrong number of terminators for neutrons";

      //confirming terminators type is "capture" or "elastic"
      if (mp->Terminators.at(0).Type != NeutralTerminatorStructure::Absorption)
          return "First terminator has to be absorption";
      if (mp->Terminators.at(1).Type != NeutralTerminatorStructure::ElasticScattering)
          return "Second terminator has to be scattering";

      //checking all terminators one by one
      if (mp->bElasticEnabled)
      {
          const NeutralTerminatorStructure& term = mp->Terminators.at(1);

          if (mp->bUseNCrystal)
          {
#ifdef __USE_ANTS_NCRYSTAL__
              if (term.NCrystal_Ncmat.isEmpty())
                  return "Configuration of NCrystal-related properties (ncmat) was not provided";
              if (term.NCrystal_scatters.isEmpty())
                return "Invalid NCrystal ncmat was provided!";
#else
              return "Scattering is configured to use NCrystal, but ANTS2 was compiled without NCrystal support!";
#endif
          }
          else
          {
              if (term.IsotopeRecords.isEmpty())
                  return QString("No elements defined for neutron elastic scattering");

              if (term.PartialCrossSection.isEmpty())
                  return QString("Total elastic scaterring cross-section is not defined");

              for (int i=0; i<term.IsotopeRecords.size(); i++)
                {
                    const ANeutronInteractionElement& se = term.IsotopeRecords.at(i);

                    QString isoName = se.Name+"-"+QString::number(se.Mass);

                    if (!mp->bAllowAbsentCsData)
                        if (se.Energy.isEmpty())
                            return isoName + " has no cross-section data for elastic scaterring";

                    if (se.Energy.size() != se.CrossSection.size())
                        return isoName + " - mismatch in cross-section data for elastic scattering";
                }
          }
      }

      if (mp->bCaptureEnabled)
      {
          for (int iTerm=0; iTerm<numTerm; iTerm++)
          {
              const NeutralTerminatorStructure& term = mp->Terminators.at(0);

              if (term.IsotopeRecords.isEmpty())
                  return QString("No elements defined for neutron absorption");

              if (term.PartialCrossSection.isEmpty())
                  return QString("Total absorption cross-section is not defined");

              for (int i=0; i<term.IsotopeRecords.size(); i++)
                {
                    const ANeutronInteractionElement& ae = term.IsotopeRecords.at(i);

                    QString isoName = ae.Name+"-"+QString::number(ae.Mass);

                    if (!mp->bAllowAbsentCsData)
                        if (ae.Energy.isEmpty())
                            return isoName + " has no cross-section data for absorption";

                    if (ae.Energy.size() != ae.CrossSection.size())
                        return isoName + " - mismatch in cross-section data for absorption";

                    double branching = 0;
                    for (int iDS=0; iDS<ae.DecayScenarios.size(); iDS++)
                    {
                        const ADecayScenario& ds = ae.DecayScenarios.at(iDS);
                        branching += ds.Branching;
                        for (int ip=0; ip<ds.GeneratedParticles.size(); ip++)
                        {
                            int partId = ds.GeneratedParticles.at(ip).ParticleId;
                            if (partId<0 || partId>=MpCollection->countParticles())
                                return "Unknown generated particle after capture for " + isoName;
                        }
                    }
                    if ( !ae.DecayScenarios.isEmpty() && fabs(branching - 1.0) > 0.001)
                    {
                        return "Sum of branching factors does not sum to 100% for " + isoName;
                    }
                }
          }
      }
    }

  return ""; //passed all tests
}

bool AMaterial::isNCrystalInUse() const
{
    for (const MatParticleStructure& mp : MatParticle)
        if (mp.TrackingAllowed && !mp.MaterialIsTransparent)
            if (mp.bElasticEnabled && mp.bUseNCrystal)
                return true;

    return false;
}
