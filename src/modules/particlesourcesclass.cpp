#include "particlesourcesclass.h"
#include "aparticlesourcerecord.h"
#include "detectorclass.h"
#include "jsonparser.h"
#include "amaterialparticlecolection.h"
#include "ajsontools.h"
#include "afiletools.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QFileInfo>
#include <QDebug>
#include <QElapsedTimer>

#include "TH1D.h"
#include "TMath.h"
#include "TRandom2.h"
#include "TGeoManager.h"

ParticleSourcesClass::ParticleSourcesClass(const DetectorClass *Detector, TRandom2 *RandGen)
    : Detector(Detector), MpCollection(Detector->MpCollection), RandGen(RandGen) {}

ParticleSourcesClass::~ParticleSourcesClass()
{
    clear();
}

void ParticleSourcesClass::clear()
{
    for (int i=0; i<ParticleSourcesData.size(); i++) delete ParticleSourcesData[i];
    ParticleSourcesData.clear();
    TotalActivity = 0;
}

bool ParticleSourcesClass::Init()
{  
    int NumSources = ParticleSourcesData.size();
    CalculateTotalActivity();
    //qDebug()<<"Tot activity:"<<TotalActivity;

  TotalParticleWeight.fill(0, NumSources);
  for (int isource = 0; isource<NumSources; isource++)
    {      
      for (int i=0; i<ParticleSourcesData[isource]->GunParticles.size(); i++)
        if (ParticleSourcesData[isource]->GunParticles[i]->Individual)
           TotalParticleWeight[isource] += ParticleSourcesData[isource]->GunParticles[i]->StatWeight;
    }

   //creating lists of linked particles
  LinkedPartiles.resize(NumSources);
  for (int isource=0; isource<NumSources; isource++)
    {
      int numParts = ParticleSourcesData[isource]->GunParticles.size();
      LinkedPartiles[isource].resize(numParts);
      for (int iparticle=0; iparticle<numParts; iparticle++)
        {          
          LinkedPartiles[isource][iparticle].resize(0);
          if (!ParticleSourcesData[isource]->GunParticles[iparticle]->Individual) continue; //nothing to do for non-individual particles

          //every individual particles defines an "event generation chain" containing the particle iteslf and all linked (and linked to linked to linked etc) particles
          LinkedPartiles[isource][iparticle].append(LinkedParticleStructure(iparticle)); //list always contains the particle itself - simplifies the generation algorithm
          //only particles with larger indexes can be linked to this particle
          for (int ip=iparticle+1; ip<numParts; ip++)
            if (!ParticleSourcesData[isource]->GunParticles[ip]->Individual) //only looking for non-individuals
              {
                //for iparticle, checking if it is linked to any particle in the list of the LinkedParticles
                for (int idef=0; idef<LinkedPartiles[isource][iparticle].size(); idef++)
                  {
                    int compareWith = LinkedPartiles[isource][iparticle][idef].iParticle;
                    int linkedTo = ParticleSourcesData[isource]->GunParticles[ip]->LinkedTo;
                    if ( linkedTo == compareWith)
                      {
                        LinkedPartiles[isource][iparticle].append(LinkedParticleStructure(ip, linkedTo));
                        break;
                      }
                  }
              }
        }
    }

  /*
  //debug
  for (int isource=0; isource<NumSources; isource++)
    for (int iparticle=0; iparticle<ParticleSourcesData[isource]->GunParticles.size(); iparticle++)
      if (LinkedPartiles[isource][iparticle].size()>0)
        {
          QString str;
          for (int ip= 0; ip<LinkedPartiles[isource][iparticle].size(); ip++) str += QString::number(LinkedPartiles[isource][iparticle][ip].iParticle)+" ";
          qDebug()<<isource<<iparticle<<str;
        }
 */


    //vectors related to collmation direction
  CollimationDirection.resize(NumSources);
  CollimationProbability.resize(NumSources);
  //CollimationSpreadProduct.resize(NumSources);
  for (int isource=0; isource<NumSources; isource++)
    {
      double CollPhi = ParticleSourcesData[isource]->CollPhi*3.1415926535/180.0;
      double CollTheta = ParticleSourcesData[isource]->CollTheta*3.1415926535/180.0;
      double Spread = ParticleSourcesData[isource]->Spread*3.1415926535/180.0;

      CollimationDirection[isource] = TVector3(sin(CollTheta)*sin(CollPhi), sin(CollTheta)*cos(CollPhi), cos(CollTheta));
      CollimationProbability[isource] = 0.5 * (1.0 - cos(Spread));
      //CollimationSpreadProduct[isource] = cos(Spread); //scalar product of coll direction and max spread unit vectors
    }
  return true; //TODO  check for fails
}

QVector<AParticleRecord> *ParticleSourcesClass::GenerateEvent()
{
    //after any operation with sources (add, remove), init should be called before first use!

    QVector<AParticleRecord>* GeneratedParticles = new QVector<AParticleRecord>;

    //selecting the source
    int isource = 0;
    int NumSources = ParticleSourcesData.size();
    if (NumSources>1)
    {
        double rnd = RandGen->Rndm()*TotalActivity;
        for (; isource<NumSources-1; isource++)
        {
            if (ParticleSourcesData[isource]->Activity >= rnd) break; //this source selected
            rnd -= ParticleSourcesData[isource]->Activity;
        }
    }

    //position
    double R[3];
    if (ParticleSourcesData[isource]->fLimit)
    {
        QElapsedTimer timer; //TODO make dynamic member
        timer.start();
        do
        {
            if (timer.elapsed()>500) return GeneratedParticles;
            //qDebug() << "Time passed" << timer.elapsed() << "milliseconds";
            GeneratePosition(isource, R);
        }
        while ( Detector->GeoManager->FindNode(R[0], R[1], R[2])->GetVolume()->GetMaterial()->GetIndex() != ParticleSourcesData[isource]->LimitedToMat ); //gGeoManager is Thread-aware
    }
    else GeneratePosition(isource, R);

    //selecting the particle
    double rnd = RandGen->Rndm() * TotalParticleWeight.at(isource);
    int iparticle;
    for (iparticle = 0; iparticle<ParticleSourcesData[isource]->GunParticles.size()-1; iparticle++)
    {
        if (ParticleSourcesData[isource]->GunParticles[iparticle]->Individual)
        {
            if (ParticleSourcesData[isource]->GunParticles[iparticle]->StatWeight >= rnd) break; //this one
            rnd -= ParticleSourcesData[isource]->GunParticles[iparticle]->StatWeight;
        }
    }
    //iparticle is the particle number in the list
    //qDebug()<<"----Particle"<<iparticle<<"selected";

  //algorithm of generation depends on whether there are particles linked to this one
  if (LinkedPartiles[isource][iparticle].size() == 1) //1 - only the particle itself
    {
      //there are no linked particles     
      //qDebug()<<"Generating individual particle"<<iparticle;
      ParticleSourcesClass::AddParticleInCone(isource, iparticle, GeneratedParticles);
      GeneratedParticles->last().r[0] = R[0];
      GeneratedParticles->last().r[1] = R[1];
      GeneratedParticles->last().r[2] = R[2];
    }
  else
    {
      //there are linked particles
      //qDebug()<<"Generating linked particles (no direction correlation)!";

      bool NoEvent = true;
      QVector<bool> WasGenerated(LinkedPartiles[isource][iparticle].size(), false);
      do
        {
          for (int ip = 0; ip<LinkedPartiles[isource][iparticle].size(); ip++)  //ip - index in the list of linked particles
            {
              //qDebug()<<"---Testing particle:"<<ip;
              int thisParticle = LinkedPartiles[isource][iparticle][ip].iParticle;
              int linkedTo = LinkedPartiles[isource][iparticle][ip].LinkedTo;
              bool fOpposite = ParticleSourcesData[isource]->GunParticles[thisParticle]->LinkingOppositeDir;

              if (ip != 0)
                {
                  //paticle this one is linked to was generated?                  
                  if (!WasGenerated[linkedTo])
                    {
                      //qDebug()<<"skip: parent not generated";
                      continue;
                    }
                  //probability test
                  double LinkingProb = ParticleSourcesData[isource]->GunParticles[thisParticle]->LinkingProbability;
                  if (RandGen->Rndm() > LinkingProb)
                    {
                      //qDebug()<<"skip: linking prob fail";
                      continue;
                    }
                  if (!fOpposite)
                  {
                      //checking the cone
                      if (RandGen->Rndm() > CollimationProbability[isource])
                        {
                          //qDebug()<<"skip: cone test fail";
                          continue;
                        }
                  } //else it will be generated in opposite direction and ignore collimation cone
                }
              else fOpposite = false; //(paranoic) protection

              //generating particle
              //qDebug()<<"success, adding his particle!";
              NoEvent = false;
              WasGenerated[ip] = true;

              if (!fOpposite)
                ParticleSourcesClass::AddParticleInCone(isource, thisParticle, GeneratedParticles);
              else
                {
                  //find index in the GeneratedParticles
                  int index = -1;
                  for (int i=0; i<linkedTo+1; i++) if (WasGenerated.at(i)) index++;
                  //qDebug() << "making this particle opposite to:"<<linkedTo<<"index in GeneratedParticles:"<<index;

                  AParticleRecord ps;
                  ps.Id = ParticleSourcesData[isource]->GunParticles[thisParticle]->ParticleId;
                  ps.energy = ParticleSourcesData[isource]->GunParticles[thisParticle]->generateEnergy();
                  ps.v[0] = -GeneratedParticles->at(index).v[0];
                  ps.v[1] = -GeneratedParticles->at(index).v[1];
                  ps.v[2] = -GeneratedParticles->at(index).v[2];
                  GeneratedParticles->append(ps);
                }

              GeneratedParticles->last().r[0] = R[0];
              GeneratedParticles->last().r[1] = R[1];
              GeneratedParticles->last().r[2] = R[2];
            }
          //qDebug()<<"---No Event:"<<NoEvent;
        }
      while (NoEvent);
    }

  return GeneratedParticles;
}

void ParticleSourcesClass::GeneratePosition(int isource, double *R) const
{
  const int& index = ParticleSourcesData[isource]->shape;
  const double& X0 = ParticleSourcesData[isource]->X0;
  const double& Y0 = ParticleSourcesData[isource]->Y0;
  const double& Z0 = ParticleSourcesData[isource]->Z0;
  const double& Phi = ParticleSourcesData[isource]->Phi*3.1415926535/180.0;
  const double& Theta = ParticleSourcesData[isource]->Theta*3.1415926535/180.0;
  const double& Psi = ParticleSourcesData[isource]->Psi*3.1415926535/180.0;
  const double& size1 = ParticleSourcesData[isource]->size1;
  const double& size2 = ParticleSourcesData[isource]->size2;
  const double& size3 = ParticleSourcesData[isource]->size3;

  switch (index) //source geometry type
    {
     case (0):
      { //point source
        R[0] = X0; R[1] = Y0; R[2] = Z0;
        return;
      }
     case (1):
      {
        //line source
        TVector3 VV(sin(Theta)*sin(Phi), sin(Theta)*cos(Phi), cos(Theta));
        double off = -1.0 + 2.0 * RandGen->Rndm();
        off *= size1;
        R[0] = X0+VV[0]*off;
        R[1] = Y0+VV[1]*off;
        R[2] = Z0+VV[2]*off;
        return;
      }
     case (2):
      {
        //surface square source
        TVector3 V[3];
        V[0].SetXYZ(size1, 0, 0);
        V[1].SetXYZ(0, size2, 0);
        V[2].SetXYZ(0, 0, size3);
        for (int i=0; i<3; i++)
         {
          V[i].RotateX(Phi);
          V[i].RotateY(Theta);
          V[i].RotateZ(Psi);
         }

        double off1 = -1.0 + 2.0 * RandGen->Rndm();
        double off2 = -1.0 + 2.0 * RandGen->Rndm();
        R[0] = X0+V[0][0]*off1+V[1][0]*off2;
        R[1] = Y0+V[0][1]*off1+V[1][1]*off2;
        R[2] = Z0+V[0][2]*off1+V[1][2]*off2;
        return;
      }
    case (3):
      {
        //surface round source
        TVector3 Circ;
        double angle = RandGen->Rndm()*3.1415926535*2.0;
        double r = RandGen->Rndm() + RandGen->Rndm();
        if (r > 1.0) r = (2.0-r)*size1;
        else r *=  size1;
        double x = r*cos(angle);
        double y = r*sin(angle);

        Circ.SetXYZ(x,y,0);
        Circ.RotateX(Phi);
        Circ.RotateY(Theta);
        Circ.RotateZ(Psi);
        R[0] = X0+Circ[0];
        R[1] = Y0+Circ[1];
        R[2] = Z0+Circ[2];
        return;
      }
    case (4):
      {
        //cube source
        TVector3 V[3];
        V[0].SetXYZ(size1, 0, 0);
        V[1].SetXYZ(0, size2, 0);
        V[2].SetXYZ(0, 0, size3);
        for (int i=0; i<3; i++)
         {
          V[i].RotateX(Phi);
          V[i].RotateY(Theta);
          V[i].RotateZ(Psi);
         }

        double off1 = -1.0 + 2.0 * RandGen->Rndm();
        double off2 = -1.0 + 2.0 * RandGen->Rndm();
        double off3 = -1.0 + 2.0 * RandGen->Rndm();
        R[0] = X0+V[0][0]*off1+V[1][0]*off2+V[2][0]*off3;
        R[1] = Y0+V[0][1]*off1+V[1][1]*off2+V[2][1]*off3;
        R[2] = Z0+V[0][2]*off1+V[1][2]*off2+V[2][2]*off3;
        return;
      }
    case (5):
      {
        //cylinder source
        TVector3 Circ;
        double off = (-1.0 + 2.0 * RandGen->Rndm())*size3;
        double angle = RandGen->Rndm()*3.1415926535*2.0;
        double r = RandGen->Rndm() + RandGen->Rndm();
        if (r > 1.0) r = (2.0-r)*size1;
        else r *=  size1;
        double x = r*cos(angle);
        double y = r*sin(angle);
        Circ.SetXYZ(x,y,off);
        Circ.RotateX(Phi);
        Circ.RotateY(Theta);
        Circ.RotateZ(Psi);
        R[0] = X0+Circ[0];
        R[1] = Y0+Circ[1];
        R[2] = Z0+Circ[2];
        return;
      }
    default:
      qWarning()<<"Unknown source geometry!";
      R[0] = 0;
      R[1] = 0;
      R[2] = 0;
    }
  return;
}

void ParticleSourcesClass::AddParticleInCone(int isource, int iparticle, QVector<AParticleRecord> *GeneratedParticles) const
{
  AParticleRecord ps;

  ps.Id = ParticleSourcesData[isource]->GunParticles[iparticle]->ParticleId;

    //energy
  //if (ParticleSourcesData[isource]->GunParticles[iparticle]->spectrum == 0)
  //  ps.Energy = ParticleSourcesData[isource]->GunParticles[iparticle]->energy;
  //else ps.Energy = ParticleSourcesData[isource]->GunParticles[iparticle]->spectrum->GetRandom();
  ps.energy = ParticleSourcesData[isource]->GunParticles[iparticle]->generateEnergy();
    //generating random direction inside the collimation cone
  double spread = ParticleSourcesData[isource]->Spread*3.1415926535/180.0; //max angle away from generation diretion
  double cosTheta = cos(spread);
  double z = cosTheta + RandGen->Rndm() * (1.0 - cosTheta);
  double tmp = TMath::Sqrt(1.0 - z*z);
  double phi = RandGen->Rndm()*3.1415926535*2.0;
  TVector3 K1(tmp*cos(phi), tmp*sin(phi), z);
  TVector3 Coll(CollimationDirection[isource]);
  K1.RotateUz(Coll);
  ps.v[0] = K1[0];
  ps.v[1] = K1[1];
  ps.v[2] = K1[2];

  GeneratedParticles->append(ps);
}

TVector3 ParticleSourcesClass::GenerateRandomDirection()
{
  //Sphere function of Root:
  double a=0,b=0,r2=1;
  while (r2 > 0.25)
    {
      a  = RandGen->Rndm() - 0.5;
      b  = RandGen->Rndm() - 0.5;
      r2 =  a*a + b*b;
    }
  double scale = 8.0 * TMath::Sqrt(0.25 - r2);

  return TVector3(a*scale, b*scale, -1.0 + 8.0 * r2 );
}

#include <QSet>
bool ParticleSourcesClass::IsParticleInUse(int particleId, QString &SourceNames) const
{
    SourceNames.clear();
    QSet<QString> sources;

    for (const AParticleSourceRecord* ps : ParticleSourcesData)
    {
        for (int ip = 0; ip < ps->GunParticles.size(); ip++)
        {
            if ( particleId == ps->GunParticles[ip]->ParticleId )
                sources << ps->name;
        }
    }

    if (sources.isEmpty()) return false;
    else
    {
        for (const QString& s : sources)
        {
            if (!SourceNames.isEmpty()) SourceNames += ", ";
            SourceNames += s;
        }
        return true;
    }
}

void ParticleSourcesClass::RemoveParticle(int particleId)
{
  for (int isource=0; isource<ParticleSourcesData.size(); isource++ )
    {
      AParticleSourceRecord* ps = ParticleSourcesData[isource];
      for (int ip = 0; ip<ps->GunParticles.size(); ip++)
          if ( ps->GunParticles[ip]->ParticleId > particleId)
              ps->GunParticles[ip]->ParticleId--;
  }
}

const QString ParticleSourcesClass::CheckConfiguration() const
{
    if (ParticleSourcesData.isEmpty()) return  "No sources are defined";
    for (AParticleSourceRecord* ps : ParticleSourcesData)
    {
        QString err = ps->CheckSource(*MpCollection);
        if (!err.isEmpty()) return QString("Error in source %1:\n%2").arg(ps->name).arg(err);
    }
    if (TotalActivity == 0) return "Total activity is zero";

    return "";
}

double ParticleSourcesClass::getTotalActivity()
{
    CalculateTotalActivity();
    return TotalActivity;
}

void ParticleSourcesClass::CalculateTotalActivity()
{
    TotalActivity = 0;
    for (int i=0; i<ParticleSourcesData.size(); i++)
        TotalActivity += ParticleSourcesData[i]->Activity;
}

void ParticleSourcesClass::writeToJson(QJsonObject &json) const
{
    QJsonArray ja;
    for (const AParticleSourceRecord* ps : ParticleSourcesData)
    {
        QJsonObject js;
        ps->writeToJson(js, *MpCollection);
        ja.append(js);
    }
    json["ParticleSources"] = ja;
}

bool ParticleSourcesClass::readFromJson(const QJsonObject &json)
{
    clear();

    if (!json.contains("ParticleSources"))
    {
        qWarning() << "--- Json does not contain config for particle sources!";
        return false;
    }

    QJsonArray ar = json["ParticleSources"].toArray();
    if (ar.isEmpty()) return true;

    for (int iSource = 0; iSource < ar.size(); iSource++)
    {
        QJsonObject json = ar.at(iSource).toObject();
        AParticleSourceRecord* ps = new AParticleSourceRecord();
        bool bOK = ps->readFromJson(json, *MpCollection);
        if (!bOK)
        {
            qWarning() << "||| Load particle source #" << iSource << "from json failed!";
            delete ps;
        }
        else ParticleSourcesData << ps;
    }

    return true;
}

void ParticleSourcesClass::checkLimitedToMaterial(AParticleSourceRecord* s)
{
    bool fFound = false;
    int iMat;
    for (iMat=0; iMat<MpCollection->countMaterials(); iMat++)
        if (s->LimtedToMatName == (*MpCollection)[iMat]->name)
        {
            fFound = true;
            break;
        }

    if (fFound)
    { //only in this case limit to material will be used!
        s->fLimit = true;
        s->LimitedToMat = iMat;
    }
    else s->fLimit = false;
}

bool ParticleSourcesClass::LoadGunEnergySpectrum(int iSource, int iParticle, QString fileName)
{
  if (iSource < 0 || iSource >= ParticleSourcesData.size())
    {
      qWarning("Energy spectrum was NOT loaded - wrong source index");
      return false;
    }
  if (iParticle < 0 || iParticle >= ParticleSourcesData[iSource]->GunParticles.size())
    {
      qWarning("Energy spectrum was NOT loaded - wrong particle index");
      return false;
    }

  QVector<double> x, y;
  int error = LoadDoubleVectorsFromFile(fileName, &x, &y);
  if (error>0) return false;

  if (ParticleSourcesData[iSource]->GunParticles[iParticle]->spectrum)
      delete ParticleSourcesData[iSource]->GunParticles[iParticle]->spectrum;
  int size = x.size();
  double* xx = new double [size];
  for (int i = 0; i<size; i++) xx[i]=x[i];

  //generating unique name for the histogram
  QString str = "EnSp" + QString::number(iSource) + "s" + QString::number(iParticle) + "p";
  QByteArray ba = str.toLocal8Bit();
  const char *c_str = ba.data();
  ParticleSourcesData[iSource]->GunParticles[iParticle]->spectrum = new TH1D(c_str,"Energy spectrum", size-1, xx);
  for (int j = 1; j<size+1; j++)  ParticleSourcesData[iSource]->GunParticles[iParticle]->spectrum->SetBinContent(j, y[j-1]);

  return true;
}

void ParticleSourcesClass::append(AParticleSourceRecord *gunParticle)
{
    ParticleSourcesData.append(gunParticle);
    CalculateTotalActivity();
}

void ParticleSourcesClass::forget(AParticleSourceRecord *gunParticle)
{
    ParticleSourcesData.removeAll(gunParticle);
    CalculateTotalActivity();
}

bool ParticleSourcesClass::replace(int iSource, AParticleSourceRecord *gunParticle)
{
    if (iSource < 0 || iSource >= ParticleSourcesData.size()) return false;

    delete ParticleSourcesData[iSource];
    ParticleSourcesData[iSource] = gunParticle;
    return true;
}

void ParticleSourcesClass::remove(int iSource)
{
    if (ParticleSourcesData.isEmpty()) return;
    if (iSource < 0 || iSource >= ParticleSourcesData.size()) return;

    delete ParticleSourcesData[iSource];
    ParticleSourcesData.remove(iSource);
    CalculateTotalActivity();
}
