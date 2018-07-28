#include "amaterial.h"
#include "aparticle.h"
#include "ajsontools.h"
#include "amaterialparticlecolection.h"
#include "acommonfunctions.h"
#include "afiletools.h"

#ifdef __USE_ANTS_NCRYSTAL__
#include "NCrystal/NCrystal.hh"
#endif

#include "TH1D.h"
#include "TRandom2.h"

#include <QFile>
#include <QDebug>

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

//double AMaterial::ft(double td, double tr, double t) const
//{
//    return exp(-t / td) * ( 1.0 - exp(-t / tr) );
//}

//double AMaterial::ft(double td, double tr, double t) const
//{
//    return 1.0 - ((tr + td) / td) * exp(- t / td) + (tr / td) * exp(- t * (1.0 / tr + 1.0 / td));
//}

double AMaterial::ft(double td, double t) const
{
    return 1.0 - ((PriScintRaiseTime + td) / td) * exp(- t / td) + (PriScintRaiseTime / td) * exp(- t * (1.0 / PriScintRaiseTime + 1.0 / td));
}

double AMaterial::GeneratePrimScintTime(TRandom2 *RandGen) const
{
    if (PriScintModel == 1) //Shao
    {
        if ( PriScintRaiseTime == 0 || PriScintDecayTimeVector.isEmpty() || (PriScintDecayTimeVector.size()==1 && PriScintDecayTimeVector.at(0).second == 0) )
        {
            //then use "Sum" model
        }
        else
        {
            //Shao model, upgraded to have several decay components
            double td;

            if (PriScintDecayTimeVector.size() == 1)
            {
                td =  PriScintDecayTimeVector.at(0).second;
            }
            else
            {
                //selecting component
                const double generatedStatWeight = _PrimScintSumStatWeight * RandGen->Rndm();
                double statWeight = 0;
                for (int i=0; i<PriScintDecayTimeVector.size(); i++)
                {
                    statWeight += PriScintDecayTimeVector.at(i).first;
                    if (generatedStatWeight < statWeight)
                    {
                        td = PriScintDecayTimeVector.at(i).second;
                        break;
                    }
                }
            }

            //            double x = RandGen->Rndm();
            //            double z = RandGen->Rndm();
            //            while (z * td * pow((td + PriScintRaiseTime) / PriScintRaiseTime, - (PriScintRaiseTime / td)) / (td + PriScintRaiseTime) > ft(td, PriScintRaiseTime, x * (100.0 * (PriScintRaiseTime + td))))
            //            {
            //                x = RandGen->Rndm();
            //                z = RandGen->Rndm();
            //            }
            //            return x * (100.0 * (PriScintRaiseTime + td));
            double	g = RandGen->Rndm(); //cannot be 0 or 1.0
            double  t = 0;
            //double  dt = 100.0 * (PriScintRaiseTime + td) / 2.0;
            double  dt = 50.0 * (PriScintRaiseTime + td);

            //while ( ((dt / 2.0) / (t + (dt / 2.0))) > pow(10.0, -5) )
            while (  0.5*dt / (t + 0.5*dt) > 0.00001 )
            {
                if ( (ft(td, t) - g) * (ft(td, t + dt) - g) > 0 )
                    t += dt;
                if ( (ft(td, t) - g) * (ft(td, t + dt) - g) < 0 )
                    //dt = dt / 2.0;
                    dt *= 0.5;
            }
            //t += dt / 2.0;
            t += 0.5 * dt;
            return t;
        }
    }

    //"Sum" model
    double t = (PriScintRaiseTime == 0 ? 0 : -PriScintRaiseTime * log(1.0 - RandGen->Rndm()) ); //delay due to raise time
    if ( !PriScintDecayTimeVector.isEmpty() )
    {
        double tau = 0;
        if (PriScintDecayTimeVector.size() == 1) tau = PriScintDecayTimeVector.at(0).second;
        else
        {
            //selecting component
            const double generatedStatWeight = _PrimScintSumStatWeight * RandGen->Rndm();
            double statWeight = 0;
            for (int i=0; i<PriScintDecayTimeVector.size(); i++)
            {
                statWeight += PriScintDecayTimeVector.at(i).first;
                if (generatedStatWeight < statWeight)
                {
                    tau = PriScintDecayTimeVector.at(i).second;
                    break;
                }
            }
        }

        //adding delay due to decay
        t += RandGen->Exp(tau);
    }

    return t;
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
    _PrimScintSumStatWeight = 0;
    for (const QPair<double,double>& pair : PriScintDecayTimeVector)
        _PrimScintSumStatWeight += pair.first;
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
  e_driftVelocity = W = SecYield = PriScintRaiseTime = PriScintModel = SecScintDecayTime = 0;
  PriScintDecayTimeVector.clear();
  rayleighWave = 500.0;
  Comments = "";

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

  GeoMat = 0; //if created, deleted by TGeomanager
  GeoMed = 0; //if created, deleted by TGeomanager

  //Do not touch overrides - handled by loaded (want to keep overrides intact when handling inspector window)
}

void AMaterial::writeToJson(QJsonObject &json, AMaterialParticleCollection* MpCollection) //QVector<AParticle *> *ParticleCollection)
{
  //general data
  json["*MaterialName"] = name;
  json["Density"] = density;
  json["Temperature"] = temperature;
  json["ChemicalComposition"] = ChemicalComposition.writeToJson();
  json["RefractiveIndex"] = n;
  json["BulkAbsorption"] = abs;
  json["RayleighMFP"] = rayleighMFP;
  json["RayleighWave"] = rayleighWave;
  json["ReemissionProb"] = reemissionProb;
  json["PrimScint_Raise"] = PriScintRaiseTime;
  json["PrimScint_Model"] = PriScintModel;
  {
    QJsonArray ar;
    for (const QPair<double, double>& pair : PriScintDecayTimeVector)
    {
        QJsonArray el;
        el << pair.first << pair.second;
        ar.append(el);
    }
    json["PrimScint_Decay"] = ar;
  }
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
  json["Comments"] = Comments;

  //wavelength-resolved data
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
  //if (reemisProbWave_lambda.size() >0)
    {
      QJsonArray ar;
      writeTwoQVectorsToJArray(reemisProbWave_lambda, reemisProbWave, ar);
      json["ReemissionProbabilityWave"] = ar;
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
      jMatParticle["CaptureEnabled"] = MatParticle[ip].bCaptureEnabled;
      jMatParticle["EllasticEnabled"] = MatParticle[ip].bEllasticEnabled;
      jMatParticle["UseNCrystal"] = MatParticle[ip].bUseNCrystal;
      jMatParticle["AllowAbsentCsData"] = MatParticle[ip].bAllowAbsentCsData;

      QJsonArray iar;
      writeTwoQVectorsToJArray(MatParticle[ip].InteractionDataX, MatParticle[ip].InteractionDataF, iar);
      jMatParticle["TotalInteraction"] = iar;

      //if ( ((*ParticleCollection)[ip]->type != AParticle::_charged_))
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

//          //gamma-specific data
//          if ((*ParticleCollection)[ip]->type == AParticle::_gamma_)
//          {
//              QJsonArray jgamma;
//              int iTerminators = MatParticle[ip].Terminators.size();
//              for (int iTerm=0; iTerm<iTerminators; iTerm++)
//              {
//                  QJsonObject jterm;
//                  jterm["ReactionType"] = MatParticle[ip].Terminators[iTerm].Type;
//                  QJsonArray ar;
//                  writeTwoQVectorsToJArray(MatParticle[ip].Terminators[iTerm].PartialCrossSectionEnergy, MatParticle[ip].Terminators[iTerm].PartialCrossSection, ar);
//                  jterm["InteractionData"] = ar;
//                  jgamma.append(jterm);
//              }
//              jMatParticle["GammaTerminators"] = jgamma;
//          }

//          //neutron-specific data
//          if ((*ParticleCollection)[ip]->type == AParticle::_neutron_)
//          {
//              QJsonArray jneutron;
//              int iTerminators = MatParticle[ip].Terminators.size();
//              for (int iTerm=0; iTerm<iTerminators; iTerm++)
//              {
//                  QJsonObject jterm;
//                jterm["Branching"] = MatParticle[ip].Terminators[iTerm].branching;
//                  jterm["ReactionType"] = (int)MatParticle[ip].Terminators[iTerm].Type;
//                  if (MatParticle[ip].Terminators[iTerm].Type == NeutralTerminatorStructure::ElasticScattering)
//                  {
//                      QJsonArray ellAr;
//                      for (int i=0; i<MatParticle[ip].Terminators[iTerm].ScatterElements.size(); i++)
//                          ellAr << MatParticle[ip].Terminators[iTerm].ScatterElements[i].writeToJson();
//                      jterm["ScatterElements"] = ellAr;
//                  }

//                  if (MatParticle[ip].Terminators[iTerm].Type == NeutralTerminatorStructure::Capture)
//                  {
//                      QJsonArray capAr;
//                      for (int i=0; i<MatParticle[ip].Terminators[iTerm].AbsorptionElements.size(); i++)
//                          capAr << MatParticle[ip].Terminators[iTerm].AbsorptionElements[i].writeToJson(MpCollection);
//                      jterm["CaptureElements"] = capAr;
//                  }

//                  //going through secondary particles
//                  QJsonArray jsecondaries;
//                  for (int is=0; is<MatParticle[ip].Terminators[iTerm].GeneratedParticles.size();is++ )
//                  {
//                      QJsonObject jsecpart;
//                      int pa = MatParticle[ip].Terminators[iTerm].GeneratedParticles[is];
//                      QJsonObject jj;
//                      (*ParticleCollection)[pa]->writeToJson(jj);
//                      jsecpart["SecParticle"] = jj;
//                      jsecpart["energy"] = MatParticle[ip].Terminators[iTerm].GeneratedParticleEnergies[is];
//                      jsecondaries.append(jsecpart);
//                  }
//                  jterm["Secondaries"] = jsecondaries;
//                  jneutron.append(jterm);
//              }
//              jMatParticle["NeutronTerminators"] = jneutron;
//          }
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
  parseJson(json, "PrimScint_Raise", PriScintRaiseTime);
  parseJson(json, "PrimScint_Model", PriScintModel);
  if (json.contains("PrimScint_Tau")) //compatibility
  {
      double tau = json["PrimScint_Tau"].toDouble();
      PriScintDecayTimeVector << QPair<double,double>(1.0, tau);
  }
  if (json.contains("PrimScint_Decay"))
  {
      PriScintDecayTimeVector.clear();
      QJsonArray ar = json["PrimScint_Decay"].toArray();
      for (int i=0; i<ar.size(); i++)
      {
          QJsonArray el = ar.at(i).toArray();
          if (el.size() == 2)
            PriScintDecayTimeVector << QPair<double,double>(el.at(0).toDouble(), el.at(1).toDouble());
          else
              qWarning() << "Bad size of decay time pair, skipping!";
      }
  }
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

      MatParticle[ip].bCaptureEnabled = true; //compatibility
      MatParticle[ip].bEllasticEnabled = false; //compatibility
      parseJson(jMatParticle, "CaptureEnabled", MatParticle[ip].bCaptureEnabled);
      parseJson(jMatParticle, "EllasticEnabled", MatParticle[ip].bEllasticEnabled);
      MatParticle[ip].bUseNCrystal = false; //compatibility
      parseJson(jMatParticle, "UseNCrystal", MatParticle[ip].bUseNCrystal);

      MatParticle[ip].bAllowAbsentCsData = false;
      parseJson(jMatParticle, "AllowAbsentCsData", MatParticle[ip].bAllowAbsentCsData);

      if (jMatParticle.contains("TotalInteraction"))
        {
          QJsonArray iar = jMatParticle["TotalInteraction"].toArray();
          readTwoQVectorsFromJArray(iar, MatParticle[ip].InteractionDataX, MatParticle[ip].InteractionDataF);
        }

      QJsonArray arTerm;
      parseJson(jMatParticle, "Terminators", arTerm);
      parseJson(jMatParticle, "GammaTerminators", arTerm); //conpatibility
         // old neutron terminators are NOT compatible with new system
      if (!arTerm.isEmpty())
      {
          MatParticle[ip].Terminators.clear();
          for (int iTerm=0; iTerm<arTerm.size(); iTerm++)
            {
              QJsonObject jterm = arTerm[iTerm].toObject();
              NeutralTerminatorStructure newTerm;
              newTerm.readFromJson(jterm, MpCollection);
              MatParticle[ip].Terminators << newTerm;
            }
      }
      else
      {
          if (jMatParticle.contains("NeutronTerminators"))
          {
              qWarning() << "This config file contains old (incompatible) standard for neutron interaction data - material is seto to transparent for neutrons!";
              //making material transparent
              MatParticle[ip].MaterialIsTransparent = true;
          }
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
    bEllasticEnabled = false;
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

#include "arandomgenncrystal.h"
void NeutralTerminatorStructure::UpdateRunTimeProperties(bool bUseLogLog, TRandom2* RandGen, int numThreads, double temp)
{
#ifdef  __USE_ANTS_NCRYSTAL__
    for (const NCrystal::Scatter * sc : NCrystal_scatters) sc->unref();
    NCrystal_scatters.clear();

    if ( !NCrystal_Ncmat.isEmpty() )
    {
        QString tmpFileName = "___tmp.ncmat";
        SaveTextToFile(tmpFileName, NCrystal_Ncmat);

        QString settings = QString("%1;dcutoff=%2Aa;packfact=%3;temp=%4K").arg(tmpFileName).arg(NCrystal_Dcutoff).arg(NCrystal_Packing).arg(temp);
        //  qDebug() << "NCrystal options line:"<<settings;

        if (numThreads < 1) numThreads = 1;
        for (int i=0; i<numThreads; i++)
        {
            const NCrystal::Scatter * sc = NCrystal::createScatter( settings.toLatin1().data() );
            ARandomGenNCrystal* rnd = new ARandomGenNCrystal(*RandGen);
            //rnd->ref(); //need?
            const_cast<NCrystal::Scatter *>(sc)->setRandomGenerator(rnd);
            sc->ref();

            NCrystal_scatters.append(sc);
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


void NeutralTerminatorStructure::UpdateRandGen(int ID, TRandom2 *RandGen)
{
#ifdef  __USE_ANTS_NCRYSTAL__
    if (NCrystal_scatters.isEmpty()) return; //nothing to update
    if (ID > -1 && ID < NCrystal_scatters.size())
    {
        const NCrystal::Scatter * sc = NCrystal_scatters.at(ID);
        ARandomGenNCrystal* rnd = new ARandomGenNCrystal(*RandGen);
        //rnd->ref(); //need?
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

QString AMaterial::CheckMaterial(int iPart, const AMaterialParticleCollection* MpCollection) const
{
  const QString errInComposition = ChemicalComposition.checkForErrors();
  if (!errInComposition.isEmpty())
      return name + ": " + errInComposition;

  if (iPart<0 || iPart>=MpCollection->countParticles())
      return QString("Wrong particle index: ") + QString::number(iPart);

  const MatParticleStructure* mp = &MatParticle.at(iPart);
  //shortcuts - if particle tracking is disabled or material is transparent - no checks
  if ( !mp->TrackingAllowed) return "";
  if (  mp->MaterialIsTransparent) return "";

  const AParticle::ParticleType& pt = MpCollection->getParticleType(iPart);

  //check TotalInteraction
  int interSize = mp->InteractionDataX.size();
  //check needed only for charged particles - they used it (stopping power is provided there)
  if (pt == AParticle::_charged_)
  {
      if (interSize == 0 || mp->InteractionDataF.size() == 0) return "Empty stopping power data";
      if (interSize<2 || mp->InteractionDataF.size()<2) return "Stopping power data size is less than 2 points";
      if (interSize != mp->InteractionDataF.size()) return "Mismatch in total interaction (X and F) length";
      for (int i=1; i<interSize; i++) //protected against length = 0 or 1
          if (mp->InteractionDataX[i-1] > mp->InteractionDataX[i]) return "Stopping power data: total energy binning is not non-decreasing";
      return ""; //nothing else is needed to be checked
  }

  if (pt == AParticle::_gamma_)
    {
      const int terms = mp->Terminators.size();
      if (mp->Terminators.isEmpty()) return "No terminator defined for gamma";
      if (terms<2 || terms>3) return "Invalid number of terminators for gamma";

      if (terms == 2) //compatibility - no pair production
        if (
             mp->Terminators[0].Type != NeutralTerminatorStructure::Photoelectric ||
             mp->Terminators[1].Type != NeutralTerminatorStructure::ComptonScattering
            ) return "Invalid terminators for gamma";

      if (terms == 3)
        if (
             mp->Terminators[0].Type != NeutralTerminatorStructure::Photoelectric ||
             mp->Terminators[1].Type != NeutralTerminatorStructure::ComptonScattering ||
             mp->Terminators[2].Type != NeutralTerminatorStructure::PairProduction
            ) return "Invalid terminators for gamma";

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
      if (!mp->bCaptureEnabled && !mp->bEllasticEnabled) return "";

      int numTerm = mp->Terminators.size();
      if (numTerm != 2 ) return "Wrong number of terminators for neutrons";

      //confirming terminators type is "capture" or "ellastic"
      if (mp->Terminators[0].Type != NeutralTerminatorStructure::Absorption ||
          mp->Terminators[1].Type != NeutralTerminatorStructure::ElasticScattering)
          return "Wrong terminator types for neutrons";

      //checking all terminator one by one
      if (mp->bEllasticEnabled)
      {
          const NeutralTerminatorStructure& term = mp->Terminators[1];
          if (term.IsotopeRecords.isEmpty())
              return QString("No elements defined for neutron ellastic scattering");

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

      if (mp->bCaptureEnabled)
      {
          for (int iTerm=0; iTerm<numTerm; iTerm++)
          {
              const NeutralTerminatorStructure& term = mp->Terminators[0];

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
