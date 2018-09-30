#include "amaterialparticlecolection.h"
#include "jsonparser.h"
#include "generalsimsettings.h"
#include "aopticaloverride.h"
#include "ajsontools.h"
#include "acommonfunctions.h"
#include <QtDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QFileInfo>
#include <QDir>

#include "TROOT.h"
#include "TGeoMedium.h"
#include "TGeoMaterial.h"
#include "TH1D.h"
#include "TString.h"

AMaterialParticleCollection::AMaterialParticleCollection()
{
  WaveFrom = 200;
  WaveTo = 800;
  WaveStep = 5;
  WaveNodes = 121;
  WavelengthResolved = false;
  fLogLogInterpolation = false;

  ParticleCollection << new AParticle("gamma", AParticle::_gamma_, 0, 0)
                     << new AParticle("alpha", AParticle::_charged_, 2, 4)
                     << new AParticle("neutron", AParticle::_neutron_, 0, 1);
}

AMaterialParticleCollection::~AMaterialParticleCollection()
{
  clearMaterialCollection();
  clearParticleCollection();
}

void AMaterialParticleCollection::SetWave(bool wavelengthResolved, double waveFrom, double waveTo, double waveStep, int waveNodes)
{
  WavelengthResolved = wavelengthResolved;
  WaveFrom = waveFrom;
  WaveTo = waveTo;
  WaveStep = waveStep;
  WaveNodes = waveNodes;
}

void AMaterialParticleCollection::GetWave(bool &wavelengthResolved, double &waveFrom, double &waveTo, double &waveStep, int &waveNodes) const
{
    wavelengthResolved = WavelengthResolved;
    waveFrom = WaveFrom;
    waveTo = WaveTo;
    waveStep = WaveStep;
    waveNodes = WaveNodes;
}

bool AMaterialParticleCollection::IsWaveResolved() const
{
    return WavelengthResolved;
}

void AMaterialParticleCollection::UpdateRuntimePropertiesAndWavelengthBinning(GeneralSimSettings *SimSet, TRandom2* RandGen, int numThreads)
{
  AMaterialParticleCollection::SetWave(SimSet->fWaveResolved, SimSet->WaveFrom, SimSet->WaveTo, SimSet->WaveStep, SimSet->WaveNodes);
  for (int imat = 0; imat < MaterialCollectionData.size(); imat++)
  {
    UpdateWaveResolvedProperties(imat);
    MaterialCollectionData[imat]->updateRuntimeProperties(fLogLogInterpolation, RandGen, numThreads);
  }
}

void AMaterialParticleCollection::updateRandomGenForThread(int ID, TRandom2* RandGen)
{
    for (int imat = 0; imat < MaterialCollectionData.size(); imat++)
        MaterialCollectionData[imat]->UpdateRandGen(ID, RandGen);
}

void AMaterialParticleCollection::getFirstOverridenMaterial(int &ifrom, int &ito)
{
  for (ifrom=0; ifrom<MaterialCollectionData.size(); ifrom++)
    for (ito=0; ito<MaterialCollectionData.size(); ito++)
      if (MaterialCollectionData[ifrom]->OpticalOverrides[ito]) return;
  ifrom = 0;
  ito = 0;
}

QString AMaterialParticleCollection::getMaterialName(int matIndex)
{
  if (matIndex<0 || matIndex>MaterialCollectionData.size()-1) return "";
  return MaterialCollectionData.at(matIndex)->name;
}

const QStringList AMaterialParticleCollection::getListOfMaterialNames() const
{
    QStringList l;
    for (AMaterial* m : MaterialCollectionData)
        l << m->name;
    return l;
}

int AMaterialParticleCollection::getParticleId(QString name) const
{
  for (int i=0; i<ParticleCollection.size(); i++)
    if (ParticleCollection.at(i)->ParticleName == name) return i;
  return -1;
}

const QString AMaterialParticleCollection::getParticleName(int particleIndex) const
{
  return ParticleCollection.at(particleIndex)->ParticleName;
}

AParticle::ParticleType AMaterialParticleCollection::getParticleType(int particleIndex) const
{
  return ParticleCollection.at(particleIndex)->type;
}

int AMaterialParticleCollection::getParticleCharge(int particleIndex) const
{
  return ParticleCollection.at(particleIndex)->charge;
}

double AMaterialParticleCollection::getParticleMass(int particleIndex) const
{
  return ParticleCollection.at(particleIndex)->mass;
}

const AParticle *AMaterialParticleCollection::getParticle(int particleIndex) const
{
  return ParticleCollection.at(particleIndex);
}

void AMaterialParticleCollection::clearMaterialCollection()
{
  for (int i=0; i<MaterialCollectionData.size(); i++)
    {
      if (MaterialCollectionData[i]->PrimarySpectrumHist)
        {
          delete MaterialCollectionData[i]->PrimarySpectrumHist;
          MaterialCollectionData[i]->PrimarySpectrumHist = 0;
        }
      if (MaterialCollectionData[i]->SecondarySpectrumHist)
        {
          delete MaterialCollectionData[i]->SecondarySpectrumHist;
          MaterialCollectionData[i]->SecondarySpectrumHist = 0;
        }

      for (int im=0; im<MaterialCollectionData[i]->OpticalOverrides.size(); im++)
        if (MaterialCollectionData[i]->OpticalOverrides[im])
          delete MaterialCollectionData[i]->OpticalOverrides[im];
      MaterialCollectionData[i]->OpticalOverrides.clear();

      delete MaterialCollectionData[i];
    }
  MaterialCollectionData.clear();

  AMaterialParticleCollection::ClearTmpMaterial();
}

void AMaterialParticleCollection::AddNewMaterial(bool fSuppressChangedSignal)
{
  AMaterial *m = new AMaterial;

  int thisMat = MaterialCollectionData.size(); //index of this material (after it is added)
  int numMats = thisMat+1; //collection size after it is added

  //initialize empty optical overrides for all materials
  m->OpticalOverrides.resize(numMats);
  for (int i=0; i<numMats; i++) m->OpticalOverrides[i] = 0;

  //all other materials have to be resized and set to 0 overrides on this material border:
  for (int i=0; i<numMats-1; i++) //over old materials
    {
      MaterialCollectionData[i]->OpticalOverrides.resize(numMats);
      MaterialCollectionData[i]->OpticalOverrides[thisMat] = 0;
    }

  //inicialize empty MatParticle vector for all defined particles
  int numParticles = ParticleCollection.size();
  m->MatParticle.resize(numParticles);

  //appending to the material collection
  MaterialCollectionData.append(m);

  //tmpMaterial object has to be updated too!
  //tmpMaterial "knew" one less material, have to set overrides to itself to nothing
  tmpMaterial.OpticalOverrides.resize(numMats);
  for (int i=0; i<numMats; i++) tmpMaterial.OpticalOverrides[thisMat] = 0;

  if (!fSuppressChangedSignal) generateMaterialsChangedSignal();
}

void AMaterialParticleCollection::AddNewMaterial(QString name, bool fSuppressChangedSignal)
{
    AddNewMaterial(true);
    MaterialCollectionData.last()->name = name;
    ensureMatNameIsUnique(MaterialCollectionData.last());

    if (!fSuppressChangedSignal) generateMaterialsChangedSignal();
}

void AMaterialParticleCollection::ClearTmpMaterial()
{
  tmpMaterial.name = "";
  tmpMaterial.density = 0;
  tmpMaterial.temperature = 298.0;
  tmpMaterial.p1 = 0;
  tmpMaterial.p2 = 0;
  tmpMaterial.p3 = 0;
  tmpMaterial.n = 1;
  tmpMaterial.abs = 0;
  tmpMaterial.reemissionProb = 0;
  tmpMaterial.rayleighMFP = 0;
  tmpMaterial.e_driftVelocity = 0;
  tmpMaterial.W = 0;
  tmpMaterial.SecYield = 0;
  tmpMaterial.SecScintDecayTime = 0;
  tmpMaterial.Comments = "";

  tmpMaterial.PriScintModel = 0;
  tmpMaterial.PriScint_Decay.clear();
  tmpMaterial.PriScint_Decay << APair_ValueAndWeight(0, 1.0);
  tmpMaterial.PriScint_Raise.clear();
  tmpMaterial.PriScint_Raise << APair_ValueAndWeight(0, 1.0);

  int particles = ParticleCollection.size();
  tmpMaterial.MatParticle.resize(particles);
  for (int i=0; i<particles; i++)
    {
      tmpMaterial.MatParticle[i].InteractionDataF.resize(0);
      tmpMaterial.MatParticle[i].InteractionDataX.resize(0);
      tmpMaterial.MatParticle[i].Terminators.resize(0);
      tmpMaterial.MatParticle[i].PhYield = 0;
      tmpMaterial.MatParticle[i].IntrEnergyRes = 0;
      tmpMaterial.MatParticle[i].TrackingAllowed = false;
    }

  int materials = MaterialCollectionData.size();

  tmpMaterial.OpticalOverrides.resize(materials);
  for (int i=0; i<materials; i++)
    {
      if (tmpMaterial.OpticalOverrides[i])
        {
          delete tmpMaterial.OpticalOverrides[i];
          tmpMaterial.OpticalOverrides[i] = 0;
        }
    }

  tmpMaterial.nWave_lambda.clear();
  tmpMaterial.nWave.clear();
  tmpMaterial.nWaveBinned.clear();
  tmpMaterial.absWave_lambda.clear();
  tmpMaterial.absWave.clear();
  tmpMaterial.absWaveBinned.clear();
  tmpMaterial.reemisProbWave.clear();
  tmpMaterial.reemisProbWave_lambda.clear();
  tmpMaterial.reemissionProbBinned.clear();

  tmpMaterial.PrimarySpectrum_lambda.clear();
  tmpMaterial.PrimarySpectrum.clear();
  tmpMaterial.SecondarySpectrum_lambda.clear();
  tmpMaterial.SecondarySpectrum.clear();

  //---POINTERS---
  if (tmpMaterial.PrimarySpectrumHist)
    {
      delete tmpMaterial.PrimarySpectrumHist;
      tmpMaterial.PrimarySpectrumHist = 0;
    }
  if (tmpMaterial.SecondarySpectrumHist)
    {
      delete tmpMaterial.SecondarySpectrumHist;
      tmpMaterial.SecondarySpectrumHist = 0;
    }
  if (tmpMaterial.GeoMat)
    {
      delete tmpMaterial.GeoMat;
      tmpMaterial.GeoMat = 0;
    }
  if (tmpMaterial.GeoMed)
    {
      delete tmpMaterial.GeoMed;
      tmpMaterial.GeoMed = 0;
    }
}

void AMaterialParticleCollection::CopyTmpToMaterialCollection()
{
  QString name = tmpMaterial.name;
  int index = AMaterialParticleCollection::FindMaterial(name);

  if (index == -1)
    {
      //      qDebug()<<"MaterialCollection--> New material: "<<name;
      AMaterialParticleCollection::AddNewMaterial();
      index = MaterialCollectionData.size()-1;
    }
  else
    {
      //      qDebug()<<"MaterialCollection--> Material "+name+" already defined; index = "<<index;
    }
  //*MaterialCollectionData[index] = tmpMaterial; //updating material properties
  QJsonObject js;
  tmpMaterial.writeToJson(js, this);
  //SaveJsonToFile(js,"TMPjson.json");
  MaterialCollectionData[index]->readFromJson(js, this);

  //now update pointers!
  AMaterialParticleCollection::UpdateWaveResolvedProperties(index); //updating effective properties (hists, Binned), remaking hist objects (Pointers are safe - they objects are recreated on each copy)

  generateMaterialsChangedSignal();
  return;
}

int AMaterialParticleCollection::FindMaterial(QString name)
{
  int size = MaterialCollectionData.size();

  for (int index = 0; index < size; index++)
    if (name == MaterialCollectionData[index]->name) return index;

  return -1;
}

void AMaterialParticleCollection::UpdateWaveResolvedProperties(int imat)
{
  //qDebug()<<"Wavelength-resolved?"<<WavelengthResolved;
  //qDebug()<<"--updating wavelength-resolved properties for material index"<<imat;
  if (WavelengthResolved)
  {
      //calculating histograms and "-Binned" for effective data
      MaterialCollectionData[imat]->nWaveBinned.clear();
      if (MaterialCollectionData[imat]->nWave_lambda.size() > 0)
        ConvertToStandardWavelengthes(&MaterialCollectionData[imat]->nWave_lambda, &MaterialCollectionData[imat]->nWave, &MaterialCollectionData[imat]->nWaveBinned);

      MaterialCollectionData[imat]->absWaveBinned.clear();
      if (MaterialCollectionData[imat]->absWave_lambda.size() > 0)
        ConvertToStandardWavelengthes(&MaterialCollectionData[imat]->absWave_lambda, &MaterialCollectionData[imat]->absWave, &MaterialCollectionData[imat]->absWaveBinned);

      MaterialCollectionData[imat]->reemissionProbBinned.clear();
      if (MaterialCollectionData[imat]->reemisProbWave_lambda.size() > 0)
        ConvertToStandardWavelengthes(&MaterialCollectionData[imat]->reemisProbWave_lambda, &MaterialCollectionData[imat]->reemisProbWave, &MaterialCollectionData[imat]->reemissionProbBinned);

      if (MaterialCollectionData[imat]->rayleighMFP != 0)
        {
          MaterialCollectionData[imat]->rayleighBinned.clear();
          double baseWave4 = MaterialCollectionData[imat]->rayleighWave * MaterialCollectionData[imat]->rayleighWave * MaterialCollectionData[imat]->rayleighWave * MaterialCollectionData[imat]->rayleighWave;
          double base = MaterialCollectionData[imat]->rayleighMFP / baseWave4;
          for (int i=0; i<WaveNodes; i++)
            {
              double wave = WaveFrom + WaveStep*i;
              double wave4 = wave*wave*wave*wave;
              MaterialCollectionData[imat]->rayleighBinned.append(base * wave4);
            }
        }

      if (MaterialCollectionData[imat]->PrimarySpectrumHist)
        {
          delete MaterialCollectionData[imat]->PrimarySpectrumHist;
          MaterialCollectionData[imat]->PrimarySpectrumHist = 0;
        }
      if (MaterialCollectionData[imat]->PrimarySpectrum_lambda.size() > 0)
        {
          QVector<double> y;
          ConvertToStandardWavelengthes(&MaterialCollectionData[imat]->PrimarySpectrum_lambda, &MaterialCollectionData[imat]->PrimarySpectrum, &y);
          TString name = "PrimScSp";
          name += imat;
          MaterialCollectionData[imat]->PrimarySpectrumHist = new TH1D(name,"Primary scintillation", WaveNodes, WaveFrom, WaveTo);
          for (int j = 1; j<WaveNodes+1; j++)  MaterialCollectionData[imat]->PrimarySpectrumHist->SetBinContent(j, y[j-1]);
          MaterialCollectionData[imat]->PrimarySpectrumHist->GetIntegral(); //to make thread safe
        }

      if (MaterialCollectionData[imat]->SecondarySpectrumHist)
        {
          delete MaterialCollectionData[imat]->SecondarySpectrumHist;
          MaterialCollectionData[imat]->SecondarySpectrumHist = 0;
        }
      if (MaterialCollectionData[imat]->SecondarySpectrum_lambda.size() > 0)
        {
          QVector<double> y;
          ConvertToStandardWavelengthes(&MaterialCollectionData[imat]->SecondarySpectrum_lambda, &MaterialCollectionData[imat]->SecondarySpectrum, &y);
          TString name = "SecScSp";
          name += imat;
          MaterialCollectionData[imat]->SecondarySpectrumHist = new TH1D(name,"Secondary scintillation", WaveNodes, WaveFrom, WaveTo);
          for (int j = 1; j<WaveNodes+1; j++)  MaterialCollectionData[imat]->SecondarySpectrumHist->SetBinContent(j, y[j-1]);
          MaterialCollectionData[imat]->SecondarySpectrumHist->GetIntegral(); //to make thread safe
        }

      for (int ior=0; ior<MaterialCollectionData[imat]->OpticalOverrides.size(); ior++)
        if (MaterialCollectionData[imat]->OpticalOverrides[ior])
            MaterialCollectionData[imat]->OpticalOverrides[ior]->initializeWaveResolved(true, WaveFrom, WaveStep, WaveNodes);
  }
  else
  {
      //making empty histograms and "-Binned" for effective data
      MaterialCollectionData[imat]->nWaveBinned.clear();
      MaterialCollectionData[imat]->absWaveBinned.clear();
      MaterialCollectionData[imat]->reemissionProbBinned.clear();
      MaterialCollectionData[imat]->rayleighBinned.clear();
      if (MaterialCollectionData[imat]->PrimarySpectrumHist)
      {
          delete MaterialCollectionData[imat]->PrimarySpectrumHist;
          MaterialCollectionData[imat]->PrimarySpectrumHist = 0;
      }
      if (MaterialCollectionData[imat]->SecondarySpectrumHist)
      {
          delete MaterialCollectionData[imat]->SecondarySpectrumHist;
          MaterialCollectionData[imat]->SecondarySpectrumHist = 0;
      }

      for (int ior=0; ior<MaterialCollectionData[imat]->OpticalOverrides.size(); ior++)
        if (MaterialCollectionData[imat]->OpticalOverrides[ior])
            MaterialCollectionData[imat]->OpticalOverrides[ior]->initializeWaveResolved(false, 0, 1, 1);
  }
}

bool AMaterialParticleCollection::isNCrystalInUse() const
{
    for (const AMaterial* m : MaterialCollectionData)
        if (m->isNCrystalInUse()) return true;
    return false;
}

bool AMaterialParticleCollection::AddParticle(QString name, AParticle::ParticleType type, int charge, double mass)
{
  if (getParticleId(name) != -1) return false;

  AParticle* p = new AParticle(name, type, charge, mass);
  ParticleCollection.append(p);
  registerNewParticle();
  emit ParticleCollectionChanged();
  return true;
}

bool AMaterialParticleCollection::UpdateParticle(int particleId, QString name, AParticle::ParticleType type, int charge, double mass)
{
  if (particleId<0 || particleId>ParticleCollection.size()-1) return false;

  AParticle* p = ParticleCollection[particleId];
  p->ParticleName = name;
  p->type = type;
  p->charge = charge;
  p->mass = mass;
  emit ParticleCollectionChanged();
  return true;
}

int AMaterialParticleCollection::getNeutronIndex() const
{
    for (int i=0; i<ParticleCollection.size(); i++)
        if (  getParticleType(i) == AParticle::_neutron_) return i;
    return -1;
}

void AMaterialParticleCollection::CopyMaterialToTmp(int imat)
{
  if (imat<0 || imat>MaterialCollectionData.size()-1)
    {
      qWarning()<<"Error: attempting to copy non-existent material #"<<imat<< " to tmpMaterial!";
      return;
    }
  QJsonObject js;
  MaterialCollectionData[imat]->writeToJson(js, this);
  tmpMaterial.readFromJson(js, this);
}

void AMaterialParticleCollection::ConvertToStandardWavelengthes(QVector<double>* sp_x, QVector<double>* sp_y, QVector<double>* y)
{
  y->resize(0);

  //qDebug()<<"Data range:"<<sp_x->at(0)<<sp_x->at(sp_x->size()-1);
  double xx, yy;
  for (int i=0; i<WaveNodes; i++)
    {
      xx = WaveFrom + WaveStep*i;
      if (xx <= sp_x->at(0)) yy = sp_y->at(0);
      else
        {
          if (xx >= sp_x->at(sp_x->size()-1)) yy = sp_y->at(sp_x->size()-1);
          else
            {
              //general case
              yy = GetInterpolatedValue(xx, sp_x, sp_y); //reusing interpolation function from functions.h
              if (yy<0) yy = 0; //!!! protection against negative
            }
        }
      //      qDebug()<<xx<<yy;
      y->append(yy);
    }
}

int AMaterialParticleCollection::FindCreateParticle(QString Name, AParticle::ParticleType Type, int Charge, double Mass, bool fOnlyFind)
{
  //  qDebug()<<"-----Looking for:"<<Name<<Type<<Charge<<Mass;
  int ParticleId;
  bool flagFound = false;
  //is this particle already in the list of defined ons?
  for (int Id=0; Id<ParticleCollection.size(); Id++)
    if (Name == ParticleCollection[Id]->ParticleName && Type==ParticleCollection[Id]->type && Charge==ParticleCollection[Id]->charge && Mass==ParticleCollection[Id]->mass)
      {
        ParticleId = Id;
        flagFound= true;
        //            qDebug()<<"found:"<<ParticleId;
        break;
      }
  //  qDebug()<<"------Found?"<<flagFound<<ParticleId;
  if (fOnlyFind)
    {
      if (flagFound) return ParticleId;
      else return -1;
    }
  if (!flagFound)
    {
      //have to define a new particle
      AParticle* tmpP = new AParticle(Name, Type, Charge, Mass);
      ParticleId = ParticleCollection.size();
      ParticleCollection.append(tmpP);
      registerNewParticle(); //resize particle-indexed properties of all materials
      emit ParticleCollectionChanged();
      //      qDebug()<<"-------created new particle: "<<ParticleId;
    }
  return ParticleId;
}

int AMaterialParticleCollection::findOrCreateParticle(QJsonObject &json)
{
  AParticle p;
  p.readFromJson(json);
  return FindCreateParticle(p.ParticleName, p.type, p.charge, p.mass, false);
}

const QString AMaterialParticleCollection::CheckMaterial(const AMaterial* mat, int iPart) const
{
  return mat->CheckMaterial(iPart, this);
}

const QString AMaterialParticleCollection::CheckMaterial(int iMat, int iPart) const
{
  if (iMat<0 || iMat>MaterialCollectionData.size()-1) return "Wrong material index: " + QString::number(iMat);
  return CheckMaterial(MaterialCollectionData[iMat], iPart);
}

const QString AMaterialParticleCollection::CheckMaterial(int iMat) const
{
  for (int iPart=0; iPart<ParticleCollection.size(); iPart++)
    {
      QString err = CheckMaterial(iMat, iPart);
      if (!err.isEmpty()) return err;
    }
  return "";
}

const QString AMaterialParticleCollection::CheckTmpMaterial() const
{
  for (int iPart=0; iPart<ParticleCollection.size(); iPart++)
    {
      QString err = tmpMaterial.CheckMaterial(iPart, this);
      if (!err.isEmpty()) return err;
    }
  return "";
}

void AMaterialParticleCollection::registerNewParticle()
{
  if (MaterialCollectionData.size()>0)
    if (MaterialCollectionData[0]->MatParticle.size() !=  ParticleCollection.size()-1 )
      qWarning()<<"MaterialCollection--> RegisterNewParticle reports mismatch!";

  int index = ParticleCollection.size()-1;
  //define empty MatParticle properties for the new particle
  for (int imat=0; imat<MaterialCollectionData.size();imat++)
    {
      MaterialCollectionData[imat]->MatParticle.resize(index+1);
      MaterialCollectionData[imat]->MatParticle[index].Clear();
    }

  tmpMaterial.MatParticle.resize(index+1);
  tmpMaterial.MatParticle[index].Clear();
}

bool AMaterialParticleCollection::DeleteMaterial(int imat)
{
  int size = MaterialCollectionData.size();
  if (imat<0 || imat >= size)
    {
      qWarning()<<"Attempt to remove material with invalid index!";
      return false;
    }

  //clear dynamic properties of this material
  if (MaterialCollectionData[imat]->PrimarySpectrumHist)
    delete MaterialCollectionData[imat]->PrimarySpectrumHist;
  if (MaterialCollectionData[imat]->SecondarySpectrumHist)
    delete MaterialCollectionData[imat]->SecondarySpectrumHist;
  for (int iOther=0; iOther<size; iOther++)  //overrides from this materials to other materials
    {
      if ( MaterialCollectionData[imat]->OpticalOverrides[iOther] )
        {
          delete MaterialCollectionData[imat]->OpticalOverrides[iOther];
          MaterialCollectionData[imat]->OpticalOverrides[iOther] = 0;
        }
    }

  //clear overrides from other materials to this one
  for (int iOther=0; iOther<size; iOther++)
    {
      if ( MaterialCollectionData[iOther]->OpticalOverrides[imat] ) delete MaterialCollectionData[iOther]->OpticalOverrides[imat];
      MaterialCollectionData[iOther]->OpticalOverrides.remove(imat);
    }

  //delete this material
  delete MaterialCollectionData[imat];
  MaterialCollectionData.remove(imat);

  //update indices of override materials
  for (int i=0; i<size-1; i++)
    for (int j=0; j<size-1; j++)
      if (MaterialCollectionData[i]->OpticalOverrides[j])
        MaterialCollectionData[i]->OpticalOverrides[j]->updateMatIndices(i, j);

  //tmpmaterial - does not receive overrides!

  generateMaterialsChangedSignal();

  return true;
}

void AMaterialParticleCollection::writeToJson(QJsonObject &json)
{
  writeParticleCollectionToJson(json);

  QJsonObject js;

  QJsonArray ar;
  for (int i=0; i<MaterialCollectionData.size(); i++)
    {
      QJsonObject jj;
      MaterialCollectionData[i]->writeToJson(jj, this);
      ar.append(jj);
    }
  js["Materials"] = ar;

  QJsonArray oar;
  for (int iFrom=0; iFrom<MaterialCollectionData.size(); iFrom++)
    for (int iTo=0; iTo<MaterialCollectionData.size(); iTo++)
      {
        if ( !MaterialCollectionData.at(iFrom)->OpticalOverrides.at(iTo) ) continue;
        QJsonObject js;
        MaterialCollectionData.at(iFrom)->OpticalOverrides[iTo]->writeToJson(js);
        oar.append(js);
      }
  if (!oar.isEmpty()) js["Overrides"] = oar;
  js["LogLog"] = fLogLogInterpolation;

  json["MaterialCollection"] = js;
}

void AMaterialParticleCollection::writeMaterialToJson(int imat, QJsonObject &json)
{
  if (imat<0 || imat>MaterialCollectionData.size()-1)
    {
      qWarning() << "Attempt to save non-existent material!";
      return;
    }
  MaterialCollectionData[imat]->writeToJson(json, this);
}

bool AMaterialParticleCollection::readFromJson(QJsonObject &json)
{
  readParticleCollectionFromJson(json);

  if (!json.contains("MaterialCollection"))
    {
      qCritical() << "Material collection not found in json";
      exit(-2);
    }
  QJsonObject js = json["MaterialCollection"].toObject();

  //reading materials
  QJsonArray ar = js["Materials"].toArray();
  if (ar.isEmpty())
    {
      qCritical() << "No materials in json";
      exit(-2);
    }
  AMaterialParticleCollection::clearMaterialCollection();
  for (int i=0; i<ar.size(); i++)
    {
      QJsonObject jj = ar[i].toObject();
      AMaterialParticleCollection::AddNewMaterial(true); //also initialize overrides
      MaterialCollectionData.last()->readFromJson(jj, this);
    }
  int numMats = countMaterials();
  //qDebug() << "--> Loaded material collection with"<<numMats<<"materials";

  //reading overrides if present
  QJsonArray oar = js["Overrides"].toArray();
  for (int i=0; i<oar.size(); i++)
    {
      QJsonObject jj = oar[i].toObject();
      if (jj.contains("Model"))
        {
          //new format
          QString model = jj["Model"].toString();
          int MatFrom = jj["MatFrom"].toInt();
          int MatTo = jj["MatTo"].toInt();
          if (MatFrom>numMats-1 || MatTo>numMats-1)
            {
              qWarning()<<"Attempt to override for non-existent material skipped";
              continue;
            }
          AOpticalOverride* ov = OpticalOverrideFactory(model, this, MatFrom, MatTo);
          if (!ov || !ov->readFromJson(jj))
            qWarning() << MaterialCollectionData[MatFrom]->name  << ": optical override load failed!";
          else MaterialCollectionData[MatFrom]->OpticalOverrides[MatTo] = ov;
        }
      //compatibility with old format:
      else if (jj.contains("MatFrom") && jj.contains("ScatModel"))
        {
          int MatFrom, MatTo;
          parseJson(jj, "MatFrom", MatFrom);
          parseJson(jj, "MatTo", MatTo);
          if (MatFrom>numMats-1 || MatTo>numMats-1)
            {
              qWarning()<<"Attempt to override for non-existent material skipped";
              continue;
            }
          double Abs, Scat, Spec;
          int ScatMode;
          parseJson(jj, "Loss", Abs);
          parseJson(jj, "Ref", Spec);
          parseJson(jj, "Scat", Scat);
          parseJson(jj, "ScatModel", ScatMode);
          BasicOpticalOverride* ov = new BasicOpticalOverride(this, MatFrom, MatTo, Abs, Spec, Scat, ScatMode );
          if (!ov)
            qWarning() << MaterialCollectionData[MatFrom]->name << ": optical override load failed!";
          else MaterialCollectionData[MatFrom]->OpticalOverrides[MatTo] = ov;
        }
    }
  fLogLogInterpolation = false;
  parseJson(js, "LogLog", fLogLogInterpolation);
  //qDebug() << "--> Loaded material border overrides from array with"<<oar.size()<<"entries";

  generateMaterialsChangedSignal();

  return true;
}

bool AMaterialParticleCollection::readParticleCollectionFromJson(QJsonObject &json)
{
  if (!json.contains("ParticleCollection"))
    {
      qCritical() << "Json does not contain particle collections!";
      exit(-1);
    }

  clearParticleCollection();
  QJsonArray ar = json["ParticleCollection"].toArray();
  if (ar.isEmpty())
    {
      qCritical() << "Particle collections in json is empty!";
      exit(-1);
    }
  for (int i=0; i<ar.size(); i++)
    {
      QJsonObject js = ar[i].toObject();
      ParticleCollection.append(new AParticle());
      ParticleCollection.last()->readFromJson(js);
    }
  //qDebug() << "--> Loaded particle collection with"<<ParticleCollection.size()<<"particles";
  return true;
}

void AMaterialParticleCollection::AddNewMaterial(QJsonObject &json) //have to be sure json is indeed material properties!
{
  AddNewMaterial();
  AMaterial* mat = MaterialCollectionData.last();
  mat->readFromJson(json, this);

  ensureMatNameIsUnique(mat);

  generateMaterialsChangedSignal();
}

void AMaterialParticleCollection::ensureMatNameIsUnique(AMaterial* mat)
{
    QString name = mat->name;
    bool fFound;
    do
      {
        fFound = false;
        for (int i=0; i<MaterialCollectionData.size(); i++)  //-1 -1 - excluding itself
        {
          if (mat == MaterialCollectionData[i]) continue;
          if (MaterialCollectionData[i]->name == name)
            {
              fFound = true;
              name += "*";
              break;
            }
        }
      }
    while (fFound);
    mat->name = name;
}

int AMaterialParticleCollection::CheckParticleEnergyInRange(int iPart, double Energy)
{
  if (iPart<0 || iPart>ParticleCollection.size()-1) return -1; //not-existent particle

  for (int iMat=0; iMat<MaterialCollectionData.size(); iMat++)
    {
      MatParticleStructure* mp = &MaterialCollectionData[iMat]->MatParticle[iPart];

      if (!mp->TrackingAllowed) continue;
      if (mp->MaterialIsTransparent) continue;

      //we skip empty energy data because this type of check is done in CheckMaterial()
      if(mp->InteractionDataX.isEmpty()) continue;

      //checking is energy in range
      double min = mp->InteractionDataX.first();
      double max = mp->InteractionDataX.last();
      if (Energy<min) return iMat;
      if (Energy>max) return iMat;
    }

  return -1; //no errors
}

void AMaterialParticleCollection::clearParticleCollection()
{
  for (int i=0; i<ParticleCollection.size(); i++)
    delete ParticleCollection[i];
  ParticleCollection.clear();
}

void AMaterialParticleCollection::writeParticleCollectionToJson(QJsonObject &json)
{
  QJsonArray ar;
  for (int i=0; i<ParticleCollection.size(); i++)
    {
      QJsonObject js;
      ParticleCollection[i]->writeToJson(js);
      ar.append(js);
    }
  json["ParticleCollection"] = ar;
}

void AMaterialParticleCollection::generateMaterialsChangedSignal()
{
  QStringList ml;
  for (int i=0; i<MaterialCollectionData.size(); i++)
    ml << MaterialCollectionData.at(i)->name;
  emit MaterialsChanged(ml);
}

void AMaterialParticleCollection::OnRequestListOfParticles(QStringList &definedParticles)
{
    definedParticles.clear();
    for (int i=0; i<ParticleCollection.size(); i++)
      definedParticles << ParticleCollection.at(i)->ParticleName;
}

void AMaterialParticleCollection::IsParticleInUse(int particleId, bool &bInUse, QString& MaterialNames)
{
  bInUse = false;
  MaterialNames.clear();

  for (int m=0; m<MaterialCollectionData.size(); m++)
      for (int p=0; p<ParticleCollection.size(); p++)
      {
          if ( MaterialCollectionData[m]->MatParticle.size() == 0) continue;
          for (int iterm=0; iterm<MaterialCollectionData[m]->MatParticle[p].Terminators.size(); iterm++)
              if (MaterialCollectionData[m]->MatParticle[p].Terminators.at(iterm).isParticleOneOfSecondaries(particleId))
              {
                  bInUse = true;
                  if (!MaterialNames.isEmpty()) MaterialNames += ", ";
                  MaterialNames += MaterialCollectionData[m]->name;
              }
      }
}

void AMaterialParticleCollection::RemoveParticle(int particleId)
{
    //shifting down all affected particle indexes
    for (int m=0; m<MaterialCollectionData.size(); m++)
    {
        for (int p=0; p<ParticleCollection.size(); p++)
        {
            //replacing generated particles
            for (int s=0; s<MaterialCollectionData[m]->MatParticle[p].Terminators.size(); s++)
                MaterialCollectionData[m]->MatParticle[p].Terminators[s].prepareForParticleRemove(particleId);

            //shifting down all particles above Id
            if (p > particleId) MaterialCollectionData[m]->MatParticle[p-1] = MaterialCollectionData[m]->MatParticle[p];
        }
        MaterialCollectionData[m]->MatParticle.resize(ParticleCollection.size()-1);
    }
    //same with the tmpMaterial
    for (int p=0; p<ParticleCollection.size(); p++)
    {
        //replacing generated particles
        for (int s=0; s<tmpMaterial.MatParticle[p].Terminators.size(); s++)
            tmpMaterial.MatParticle[p].Terminators[s].prepareForParticleRemove(particleId);

        //shifting down all particles above Id
        if (p > particleId) tmpMaterial.MatParticle[p-1] = tmpMaterial.MatParticle[p];
    }

    //removing
    delete ParticleCollection[particleId];
    ParticleCollection.remove(particleId);
}
