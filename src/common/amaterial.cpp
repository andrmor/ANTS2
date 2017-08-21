#include "amaterial.h"
#include "aparticle.h"
#include "ajsontools.h"
#include "amaterialparticlecolection.h"
#include "acommonfunctions.h"

#include "TH1D.h"

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
  return nWaveBinned[iWave];
}

double AMaterial::getAbsorptionCoefficient(int iWave) const
{
  if (iWave == -1 || absWaveBinned.isEmpty()) return abs;
  return absWaveBinned[iWave];
}

void AMaterial::updateNeutronDataOnCompositionChange(const AMaterialParticleCollection *MPCollection)
{
    int neutronId = MPCollection->getNeutronIndex();
    if (neutronId == -1) return; //not dfefined in thsi configuration

    QVector<NeutralTerminatorStructure>& Terminators = MatParticle[neutronId].Terminators;
    Terminators.resize(2);
    //1 - capture
    NeutralTerminatorStructure& ct = Terminators.first();
    ct.Type = NeutralTerminatorStructure::Capture;
    QVector<ACaptureElement> CaptureElementsNew;
    for (int iEl=0; iEl<ChemicalComposition.countElements(); iEl++)
    {
        AChemicalElement* El = ChemicalComposition.getElement(iEl);
        for (int iIso=0; iIso<El->countIsotopes(); iIso++)
        {
            qDebug() << "capture" << El->Symbol << El->Isotopes.at(iIso).Mass << ":";
            bool bAlreadyExists = false;
            for (ACaptureElement& capEl : ct.CaptureElements)
                if (capEl.Name == El->Symbol && capEl.Mass == El->Isotopes.at(iIso).Mass)
                {
                    qDebug() << "Found in old list, copying";
                    bAlreadyExists = true;
                    CaptureElementsNew << capEl;
                    break;
                }
            if (!bAlreadyExists)
            {
                qDebug() << "Not found, creating new record";
                CaptureElementsNew << ACaptureElement(El->Isotopes.at(iIso).Symbol, El->Isotopes.at(iIso).Mass, El->MolarFraction*0.01*El->Isotopes.at(iIso).Abundancy);
            }
        }
    }
    ct.CaptureElements = CaptureElementsNew;

    //2 - elastic scatter
    NeutralTerminatorStructure& st = Terminators.last();
    st.Type = NeutralTerminatorStructure::ElasticScattering;
    QVector<AElasticScatterElement> ScatterElementsNew;
    for (int iEl=0; iEl<ChemicalComposition.countElements(); iEl++)
    {
        AChemicalElement* El = ChemicalComposition.getElement(iEl);
        for (int iIso=0; iIso<El->countIsotopes(); iIso++)
        {
            qDebug() << "scatter" << El->Symbol << El->Isotopes.at(iIso).Mass << ":";
            bool bAlreadyExists = false;
            for (AElasticScatterElement& scatEl : ct.ScatterElements)
                if (scatEl.Name == El->Symbol && scatEl.Mass == El->Isotopes.at(iIso).Mass)
                {
                    qDebug() << "Found in old list, copying";
                    bAlreadyExists = true;
                    ScatterElementsNew << scatEl;
                    break;
                }
            if (!bAlreadyExists)
            {
                qDebug() << "Not found, creating new record";
                ScatterElementsNew << AElasticScatterElement(El->Isotopes.at(iIso).Symbol, El->Isotopes.at(iIso).Mass, El->MolarFraction*0.01*El->Isotopes.at(iIso).Abundancy);
            }
        }
    }
    st.ScatterElements = ScatterElementsNew;




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

void AMaterial::writeToJson(QJsonObject &json, AMaterialParticleCollection* MpCollection) //QVector<AParticle *> *ParticleCollection)
{
  //general data
  json["*MaterialName"] = name;
  json["Density"] = density;
  json["ChemicalComposition"] = ChemicalComposition.writeToJson();
  json["IsotopeDensity"] = atomicDensity;
  json["RefractiveIndex"] = n;
  json["BulkAbsorption"] = abs;
  json["RayleighMFP"] = rayleighMFP;
  json["RayleighWave"] = rayleighWave;
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
              jterm["ReactionType"] = (int)MatParticle[ip].Terminators[iTerm].Type;
              if (MatParticle[ip].Terminators[iTerm].Type == NeutralTerminatorStructure::ElasticScattering)
              {
                  QJsonArray ellAr;
                  for (int i=0; i<MatParticle[ip].Terminators[iTerm].ScatterElements.size(); i++)
                      ellAr << MatParticle[ip].Terminators[iTerm].ScatterElements[i].writeToJson();
                  jterm["ScatterElements"] = ellAr;
              }

              if (MatParticle[ip].Terminators[iTerm].Type == NeutralTerminatorStructure::Capture)
              {
                  QJsonArray capAr;
                  for (int i=0; i<MatParticle[ip].Terminators[iTerm].CaptureElements.size(); i++)
                      capAr << MatParticle[ip].Terminators[iTerm].CaptureElements[i].writeToJson(MpCollection);
                  jterm["CaptureElements"] = capAr;
              }

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
  if (json.contains("ChemicalComposition"))
  {
      QJsonObject ccjson = json["ChemicalComposition"].toObject();
      ChemicalComposition.readFromJson(ccjson);
  }
  else ChemicalComposition.clear();
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

      MatParticle[ip].bCaptureEnabled = true; //compatibility
      MatParticle[ip].bEllasticEnabled = false; //compatibility
      parseJson(jMatParticle, "CaptureEnabled", MatParticle[ip].bCaptureEnabled);
      parseJson(jMatParticle, "EllasticEnabled", MatParticle[ip].bEllasticEnabled);

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
              MatParticle[ip].Terminators[iTerm].Type = static_cast<NeutralTerminatorStructure::ReactionType>(jterm["ReactionType"].toInt());
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

              if (MatParticle[ip].Terminators[iTerm].Type == NeutralTerminatorStructure::Capture)
                {
                  //using branchings to calculate partical cross sections (can be changed in the future!)
                  MatParticle[ip].Terminators[iTerm].PartialCrossSectionEnergy = MatParticle[ip].InteractionDataX;
                  MatParticle[ip].Terminators[iTerm].PartialCrossSection.resize(0);
                  for (int i=0; i<MatParticle[ip].InteractionDataF.size(); i++)
                    MatParticle[ip].Terminators[iTerm].PartialCrossSection.append(MatParticle[ip].Terminators[iTerm].branching * MatParticle[ip].InteractionDataF[i]);
                }

              if (jterm.contains("ScatterElements"))
              {
                  QJsonArray ellAr = jterm["ScatterElements"].toArray();
                  MatParticle[ip].Terminators[iTerm].ScatterElements.clear();
                  for (int i=0; i<ellAr.size(); i++)
                  {
                      AElasticScatterElement el;
                      QJsonObject js = ellAr[i].toObject();
                      el.readFromJson(js);
                      MatParticle[ip].Terminators[iTerm].ScatterElements << el;
                  }
              }

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
  bCaptureEnabled = true;
  bEllasticEnabled = false;
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

ACaptureElement *NeutralTerminatorStructure::getCaptureElement(int index)
{
    if (index<0 || index>=CaptureElements.size()) return 0;
    return &CaptureElements[index];
}

AElasticScatterElement *NeutralTerminatorStructure::getElasticScatterElement(int index)
{
    if (index<0 || index>=ScatterElements.size()) return 0;
    return &ScatterElements[index];
}

void NeutralTerminatorStructure::UpdateRuntimePropertiesForNeutrons(bool bUseLogLog)
{    
    if (Type == Capture)
    {
        //nothing to do here
    }
    else if (Type == ElasticScattering && !ScatterElements.isEmpty())
    {
        qDebug() << "Updaring neutron elastic scattering terminator";
        MeanElementMass = 0;
        for (const AElasticScatterElement& el : ScatterElements)
            MeanElementMass += el.Mass * el.MolarFraction;
        qDebug() << "Mean elements mass:" << MeanElementMass;

        PartialCrossSectionEnergy.clear();
        PartialCrossSection.clear();
        qDebug() << "Scatter elements defined:"<<ScatterElements.size();
        for (int iElement=0; iElement<ScatterElements.size(); iElement++)
        {
            AElasticScatterElement& se = ScatterElements[iElement];
            qDebug() << se.Name << "-" << se.Mass;
            qDebug() << se.Energy.size() << se.CrossSection.size();
            if (se.Energy.isEmpty()) continue;
            if (PartialCrossSectionEnergy.isEmpty())
            {
                //this is first non-empty element - energy binning will be according to it
                PartialCrossSectionEnergy = se.Energy;
                //first element cross-section times its molar fraction
                for (int i=0; i<se.CrossSection.size(); i++)
                    PartialCrossSection << se.CrossSection.at(i) * se.MolarFraction;
            }
            else
            {
                //using interpolation to match already defined energy bins
                for (int iEnergy=0; iEnergy<PartialCrossSectionEnergy.size(); iEnergy++)
                {
                    const double& energy = PartialCrossSectionEnergy.at(iEnergy);
                    const double cs = GetInterpolatedValue(energy, &se.Energy, &se.CrossSection, bUseLogLog);
                    PartialCrossSection[iEnergy] += cs * se.MolarFraction;
                }
            }
        }
        qDebug() << "Energy-->" <<PartialCrossSectionEnergy;
        qDebug() << "Cross-section-->" <<PartialCrossSection;
    }
}
