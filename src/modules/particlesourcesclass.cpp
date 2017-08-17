#include "particlesourcesclass.h"
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



ParticleSourcesClass::ParticleSourcesClass(const DetectorClass *Detector, TString nameID)
    : Detector(Detector), MpCollection(Detector->MpCollection), RandGen(Detector->RandGen), NameID(nameID), TotalActivity(0) {}

ParticleSourcesClass::~ParticleSourcesClass()
{
    ParticleSourcesClass::clear();
}

void ParticleSourcesClass::clear()
{
    for (int i=0; i<ParticleSourcesData.size(); i++) delete ParticleSourcesData[i];
    ParticleSourcesData.clear();
    TotalActivity = 0;
}

void ParticleSourcesClass::Init()
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
}

QVector<GeneratedParticleStructure>* ParticleSourcesClass::GenerateEvent()
{
  //after any operation with sources (add, remove), init should be called before first use!

  QVector<GeneratedParticleStructure>* GeneratedParticles = new QVector<GeneratedParticleStructure>;

  //selecting the source
  int isource = 0;
  int NumSources = ParticleSourcesData.size();
  if (NumSources>1)
    {
      double rnd = RandGen->Rndm()*TotalActivity;
      for (isource; isource<NumSources-1; isource++)
        {
          if (ParticleSourcesData[isource]->Activity >= rnd) break; //this source selected
          rnd -= ParticleSourcesData[isource]->Activity;
        }
    }

  //position
  double R[3];
  if (ParticleSourcesData[isource]->fLimit)
  {
      QElapsedTimer timer;
      timer.start();
      do
      {
          if (timer.elapsed()>500) return GeneratedParticles;
            //qDebug() << "Time passed" << timer.elapsed() << "milliseconds";
          GeneratePosition(isource, R);
      }
      while ( Detector->GeoManager->FindNode(R[0], R[1], R[2])->GetVolume()->GetMaterial()->GetIndex() != ParticleSourcesData[isource]->LimitedToMat );
  }
  else GeneratePosition(isource, R);

  //selecting the particle
  double rnd = RandGen->Rndm()*TotalParticleWeight[isource];
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
      GeneratedParticles->last().Position[0] = R[0];
      GeneratedParticles->last().Position[1] = R[1];
      GeneratedParticles->last().Position[2] = R[2];
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

                  GeneratedParticleStructure ps;
                  ps.ParticleId = ParticleSourcesData[isource]->GunParticles[thisParticle]->ParticleId;
                  if (ParticleSourcesData[isource]->GunParticles[thisParticle]->spectrum == 0)
                    ps.Energy = ParticleSourcesData[isource]->GunParticles[thisParticle]->energy;
                  else ps.Energy = ParticleSourcesData[isource]->GunParticles[thisParticle]->spectrum->GetRandom();
                  ps.Direction[0] = -GeneratedParticles->at(index).Direction[0];
                  ps.Direction[1] = -GeneratedParticles->at(index).Direction[1];
                  ps.Direction[2] = -GeneratedParticles->at(index).Direction[2];
                  GeneratedParticles->append(ps);
                }

              GeneratedParticles->last().Position[0] = R[0];
              GeneratedParticles->last().Position[1] = R[1];
              GeneratedParticles->last().Position[2] = R[2];
            }
          //qDebug()<<"---No Event:"<<NoEvent;
        }
      while (NoEvent);
    }

  return GeneratedParticles;
}

void ParticleSourcesClass::GeneratePosition(int isource, double *R)
{
  int index = ParticleSourcesData[isource]->index;
  double X0 = ParticleSourcesData[isource]->X0;
  double Y0 = ParticleSourcesData[isource]->Y0;
  double Z0 = ParticleSourcesData[isource]->Z0;
  double Phi = ParticleSourcesData[isource]->Phi*3.1415926535/180.0;
  double Theta = ParticleSourcesData[isource]->Theta*3.1415926535/180.0;
  double Psi = ParticleSourcesData[isource]->Psi*3.1415926535/180.0;
  double size1 = ParticleSourcesData[isource]->size1;
  double size2 = ParticleSourcesData[isource]->size2;
  double size3 = ParticleSourcesData[isource]->size3;

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

void ParticleSourcesClass::AddParticleInCone(int isource, int iparticle, QVector<GeneratedParticleStructure> *GeneratedParticles)
{
  GeneratedParticleStructure ps;

  ps.ParticleId = ParticleSourcesData[isource]->GunParticles[iparticle]->ParticleId;

    //energy
  if (ParticleSourcesData[isource]->GunParticles[iparticle]->spectrum == 0)
    ps.Energy = ParticleSourcesData[isource]->GunParticles[iparticle]->energy;
  else ps.Energy = ParticleSourcesData[isource]->GunParticles[iparticle]->spectrum->GetRandom();
    //generating random direction inside the collimation cone
  double spread = ParticleSourcesData[isource]->Spread*3.1415926535/180.0; //max angle away from generation diretion
  double cosTheta = cos(spread);
  double z = cosTheta + RandGen->Rndm() * (1.0 - cosTheta);
  double tmp = TMath::Sqrt(1.0 - z*z);
  double phi = RandGen->Rndm()*3.1415926535*2.0;
  TVector3 K1(tmp*cos(phi), tmp*sin(phi), z);
  TVector3 Coll(CollimationDirection[isource]);
  K1.RotateUz(Coll);
  ps.Direction[0] = K1[0];
  ps.Direction[1] = K1[1];
  ps.Direction[2] = K1[2];

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

void ParticleSourcesClass::onIsParticleInUse(int particleId, bool& fAnswer, QString *SourceName)
{
    for (int isource=0; isource<ParticleSourcesData.size(); isource++ )
      {
        ParticleSourceStructure* ps = ParticleSourcesData[isource];
        for (int ip = 0; ip<ps->GunParticles.size(); ip++)
          {
            if ( particleId == ps->GunParticles[ip]->ParticleId )
              {
                fAnswer = true;
                if (SourceName) *SourceName = ps->name;
                return;
              }
          }
      }
    fAnswer = false;
    if (SourceName) *SourceName = "";
}

void ParticleSourcesClass::onRequestRegisterParticleRemove(int particleId)
{
    for (int isource=0; isource<ParticleSourcesData.size(); isource++ )
      {
        ParticleSourceStructure* ps = ParticleSourcesData[isource];
        for (int ip = 0; ip<ps->GunParticles.size(); ip++)
            if ( ps->GunParticles[ip]->ParticleId > particleId)
                ps->GunParticles[ip]->ParticleId--;
      }

    QJsonObject json;
    writeToJson(json);
    emit RequestUpdateSourcesInConfig(json);
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

bool ParticleSourcesClass::writeSourceToJson(int iSource, QJsonObject &json)
{
   if (iSource<0 || iSource>ParticleSourcesData.size()-1) return false;

   ParticleSourceStructure* s = ParticleSourcesData[iSource];

   //general
   json["Name"] = s->name;
   json["Type"] = s->index;
   json["Activity"] = s->Activity;
   json["X"] = s->X0;
   json["Y"] = s->Y0;
   json["Z"] = s->Z0;
   json["Size1"] = s->size1;
   json["Size2"] = s->size2;
   json["Size3"] = s->size3;
   json["Phi"] = s->Phi;
   json["Theta"] = s->Theta;
   json["Psi"] = s->Psi;
   json["CollPhi"] = s->CollPhi;
   json["CollTheta"] = s->CollTheta;
   json["Spread"] = s->Spread;

   json["DoMaterialLimited"] = s->DoMaterialLimited;
   json["LimitedToMaterial"] = s->LimtedToMatName;

   //particles
   int GunParticleSize = s->GunParticles.size();
   json["Particles"] = GunParticleSize;

   QJsonArray jParticleEntries;
   for (int ip=0; ip<GunParticleSize; ip++)
         {
            QJsonObject jGunParticle;

            QJsonObject jparticle;
            int ParticleId = s->GunParticles[ip]->ParticleId;
            const AParticle* p = MpCollection->getParticle(ParticleId);
            jparticle["name"] = p->ParticleName;
            jparticle["type"] = p->type;
            jparticle["charge"] = p->charge;
            jparticle["mass"] = p->mass;
              //adding to top level
            jGunParticle["Particle"] = jparticle;

            jGunParticle["StatWeight"] = s->GunParticles[ip]->StatWeight;
            bool individual = s->GunParticles[ip]->Individual;
            jGunParticle["Individual"] = individual;
            //if (!individual)
            //  {
                jGunParticle["LinkedTo"] = s->GunParticles[ip]->LinkedTo;
                jGunParticle["LinkingProbability"] = s->GunParticles[ip]->LinkingProbability;
                jGunParticle["LinkingOppositeDir"] = s->GunParticles[ip]->LinkingOppositeDir;
            //  }
            if ( s->GunParticles[ip]->spectrum == 0 )
              jGunParticle["Energy"] = s->GunParticles[ip]->energy;
            else
              {
                //saving spectrum               
                QJsonArray ja;
                writeTH1DtoJsonArr(s->GunParticles[ip]->spectrum, ja);
                jGunParticle["EnergySpectrum"] = ja;
              }
            jParticleEntries.append(jGunParticle);
         }
   json["GunParticles"] = jParticleEntries;
   return true;
}

bool ParticleSourcesClass::writeToJson(QJsonObject &json)
{
  QJsonArray ja;
  for (int iSource=0; iSource<ParticleSourcesData.size(); iSource++)
    {
      QJsonObject js1;
      bool fOK = writeSourceToJson(iSource, js1);
      if (!fOK) return false;
      ja.append(js1);
    }
  json["ParticleSources"] = ja;
  return true;
}

bool ParticleSourcesClass::SaveSourceToJsonFile(QString fileName, int iSource)
{
  if (iSource<0 || iSource>ParticleSourcesData.size()-1)
    {
      qWarning("Particle source was NOT saved - wrong source index");
      return false;
    }

  QJsonObject json;
  bool fOK = writeSourceToJson(iSource, json);
  if (!fOK) return false;

  //saving json file
  QFile saveFile(fileName);
  if (!saveFile.open(QIODevice::WriteOnly))
    {
          qWarning()<<"Cannot save to file "+fileName;
          return false;
    }
  QJsonDocument saveDoc(json);
  saveFile.write(saveDoc.toJson());
  return true;
}

bool ParticleSourcesClass::readSourceFromJson(int iSource, QJsonObject &json)
{
    //qDebug() << "Read!";
  if (iSource<-1 || iSource>ParticleSourcesData.size()-1)
    {
      qWarning("Particle source was NOT loaded - wrong source index");
      return false;
    }

  if (iSource == -1)
    {
      //append new!
      ParticleSourceStructure* ns = new ParticleSourceStructure();
      ParticleSourcesData.append(ns);
      iSource = ParticleSourcesData.size()-1;
    }

  delete ParticleSourcesData[iSource]; //have to delete here, or histograms might have not unique names
  ParticleSourceStructure* s = new ParticleSourceStructure();
  ParticleSourcesData[iSource] = s;
  //reading general info
  parseJson(json, "Name", s->name);
  //qDebug() << s->name;
  parseJson(json, "Type", s->index);
  parseJson(json, "Activity", s->Activity);
  parseJson(json, "X", s->X0);
  parseJson(json, "Y", s->Y0);
  parseJson(json, "Z", s->Z0);
  parseJson(json, "Size1", s->size1);
  parseJson(json, "Size2", s->size2);
  parseJson(json, "Size3", s->size3);
  parseJson(json, "Phi", s->Phi);
  parseJson(json, "Theta", s->Theta);
  parseJson(json, "Psi", s->Psi);
  parseJson(json, "CollPhi", s->CollPhi);
  parseJson(json, "CollTheta", s->CollTheta);
  parseJson(json, "Spread", s->Spread);

  s->DoMaterialLimited = s->fLimit = false;
  s->LimtedToMatName = "";
  if (json.contains("DoMaterialLimited"))
  {
      parseJson(json, "DoMaterialLimited", s->DoMaterialLimited);
      parseJson(json, "LimitedToMaterial", s->LimtedToMatName);

      if (s->DoMaterialLimited) checkLimitedToMaterial(s);
  }

  //GunParticles
  //int declPart; //declared number of particles
  //parseJson(json, "Particles", declPart);  //obsolete
  QJsonArray jGunPartArr = json["GunParticles"].toArray();
  int numGP = jGunPartArr.size();
  //qDebug()<<"Entries in gunparticles:"<<numGP;
  for (int ip = 0; ip<numGP; ip++)
    {
      QJsonObject jThisGunPart = jGunPartArr[ip].toObject();

      //extracting the particle
      QJsonObject jparticle = jThisGunPart["Particle"].toObject();
      //JsonParser GunParticleParser(jThisGunPart);
      if (jparticle.isEmpty())
        {
          qWarning()<<"Cannot extract particle in the material file";
          return false;
        }
      QString name = jparticle["name"].toString();
      int type = jparticle["type"].toInt();
      int charge = jparticle["charge"].toInt();
      double mass = jparticle["mass"].toDouble();
      //looking for this particle in the collection and create if necessary
      AParticle::ParticleType Type = static_cast<AParticle::ParticleType>(type);
      int ParticleId = MpCollection->FindCreateParticle(name, Type, charge, mass);

      s->GunParticles.append(new GunParticleStruct());
      s->GunParticles[ip]->ParticleId = ParticleId;
      //qDebug()<<"Added gun particle with particle Id"<<ParticleId<<ParticleCollection->at(ParticleId)->ParticleName;

      //going through gunparticle general properties
      parseJson(jThisGunPart, "StatWeight",  s->GunParticles[ip]->StatWeight );
      parseJson(jThisGunPart, "Individual",  s->GunParticles[ip]->Individual );
      parseJson(jThisGunPart, "LinkedTo",  s->GunParticles[ip]->LinkedTo ); //linked always to previously already defined particles!
      parseJson(jThisGunPart, "LinkingProbability",  s->GunParticles[ip]->LinkingProbability );
      parseJson(jThisGunPart, "LinkingOppositeDir",  s->GunParticles[ip]->LinkingOppositeDir );

      parseJson(jThisGunPart, "Energy",  s->GunParticles[ip]->energy );
      QJsonArray ar = jThisGunPart["EnergySpectrum"].toArray();
      if (!ar.isEmpty())
        {
          int size = ar.size();
          double* xx = new double [size];
          double* yy = new double [size];
          for (int i=0; i<size; i++)
            {
              xx[i] = ar[i].toArray()[0].toDouble();
              yy[i] = ar[i].toArray()[1].toDouble();
            }
          //generating unique name for the histogram
          QString str = "EnSp" + QString::number(iSource) + "s" + QString::number(ip) + "p"+NameID;
          //qDebug() << "name:" <<str;
          QByteArray ba = str.toLocal8Bit();
          const char *c_str = ba.data();
          s->GunParticles[ip]->spectrum = new TH1D(c_str,"Energy spectrum", size-1, xx);
          for (int j = 1; j<size+1; j++) s->GunParticles[ip]->spectrum->SetBinContent(j, yy[j-1]);
          delete[] xx;
          delete[] yy;
        }
      // TO DO !!! *** add error if no energy or spectrum
    }
  return true;
}

void ParticleSourcesClass::checkLimitedToMaterial(ParticleSourceStructure* s)
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

bool ParticleSourcesClass::readFromJson(QJsonObject &json)
{
  if (!json.contains("ParticleSources"))
    {
      qWarning() << "--- Json does not contain config for particle sources!";
      return false;
    }

  QJsonArray ar = json["ParticleSources"].toArray();
  if (ar.isEmpty())
    {
      //qDebug() << "No sources defined in the json object!";
      return false;
    }

  //        qDebug() << "    Sources in json:"<< ar.size();
  ParticleSourcesClass::clear();

  for (int iSource =0; iSource<ar.size(); iSource++)
    {
      QJsonObject json = ar[iSource].toObject();
      bool fOK = readSourceFromJson(-1, json);
      if (!fOK)
        {
          qWarning() << "||| Load particles from json failed!";
          ParticleSourcesClass::clear();
          return false;
        }
    }

  return true;
}

bool ParticleSourcesClass::LoadSourceFromJsonFile(QString fileName, int iSource)
{
  if (iSource<0 || iSource>ParticleSourcesData.size()-1)
    {
      qWarning("||| Particle source was NOT loaded - wrong source index");
      return false;
    }

  QFile file(fileName);
  if(!file.open(QIODevice::ReadOnly | QFile::Text))
        {
          qCritical()<<"||| Failed to open file" + fileName + "!";
          return false;
        }
  QFileInfo QF(fileName);
  QByteArray saveData = file.readAll();

  QJsonParseError error;
  QJsonDocument loadDoc(QJsonDocument::fromJson(saveData, &error));
  QJsonObject json = loadDoc.object();

  if (error.error != 0)
    {
      qWarning()<<"||| Load failed: JSON parser reports error "<<error.errorString();
      return false;
    }
  //parsing successful

  //reading general info
  JsonParser parser(json);
  bool ok = true;
  ParticleSourceStructure* s = ParticleSourcesData[iSource];
  ok &= parser.ParseObject("Name", s->name);
  ok &= parser.ParseObject("Type", s->index);
  ok &= parser.ParseObject("Activity", s->Activity);
  ok &= parser.ParseObject("X", s->X0);
  ok &= parser.ParseObject("Y", s->Y0);
  ok &= parser.ParseObject("Z", s->Z0);
  ok &= parser.ParseObject("Size1", s->size1);
  ok &= parser.ParseObject("Size2", s->size2);
  ok &= parser.ParseObject("Size3", s->size3);
  ok &= parser.ParseObject("Phi", s->Phi);
  ok &= parser.ParseObject("Theta", s->Theta);
  ok &= parser.ParseObject("Psi", s->Psi);
  ok &= parser.ParseObject("CollPhi", s->CollPhi);
  ok &= parser.ParseObject("CollTheta", s->CollTheta);
  ok &= parser.ParseObject("Spread", s->Spread);

  //GunParticles
  int declPart; //declared number of particles
  ok &= parser.ParseObject("Particles", declPart);

  QJsonArray jParticleEntries;
  if (parser.ParseObject("GunParticles", jParticleEntries))
    {
      int numGP = jParticleEntries.size();
      if (declPart != numGP)
        {
          qWarning()<<"Error: number of particles in json array is not equal to declared one!";
          return false;
        }
//      qDebug()<<"Entries in gunparticles:"<<numGP;

      for (int ip = 0; ip<numGP; ip++)
        {
          QJsonObject jGunParticle = jParticleEntries[ip].toObject();

          //extracting the particle
          QJsonObject jparticle;
          JsonParser GunParticleParser(jGunParticle);
          bool ok = GunParticleParser.ParseObject("Particle", jparticle);
          if (!ok)
            {
              qWarning()<<"Cannot extract particle in the material file";
              return false;
            }
          QString name;
          int type;
          int charge;
          double mass;
          JsonParser ParticleParser(jparticle);
          ok &= ParticleParser.ParseObject("name", name );
          ok &= ParticleParser.ParseObject("type", type );
          ok &= ParticleParser.ParseObject("charge", charge );
          ok &= ParticleParser.ParseObject("mass", mass );
          if (!ok)
            {
              qWarning()<<"Cannot extract particle properties in the material file";
              return false;
            }
            //looking for this particle in the collection and create if necessary
          AParticle::ParticleType Type = static_cast<AParticle::ParticleType>(type);
          int ParticleId = MpCollection->FindCreateParticle(name, Type, charge, mass);

          s->GunParticles.append(new GunParticleStruct());
          s->GunParticles[ip]->ParticleId = ParticleId;
//          qDebug()<<"--------"<<ParticleId;

            //going through gunparticle general properties
          ok &= GunParticleParser.ParseObject("StatWeight",  s->GunParticles[ip]->StatWeight );
          GunParticleParser.ParseObject("Individual",  s->GunParticles[ip]->Individual );
          GunParticleParser.ParseObject("LinkedTo",  s->GunParticles[ip]->LinkedTo ); //linked always to previously already defined particles!
          GunParticleParser.ParseObject("LinkingProbability",  s->GunParticles[ip]->LinkingProbability );
          GunParticleParser.ParseObject("LinkingOppositeDir",  s->GunParticles[ip]->LinkingOppositeDir );

          GunParticleParser.ParseObject("Energy",  s->GunParticles[ip]->energy );
//qDebug()<<"--------";

          //old
          QString SpectrumFile;
          if (GunParticleParser.ParseObject("EnergySpectrumFile", SpectrumFile ) )
            {
              bool ok = LoadGunEnergySpectrum(iSource, ip, QF.path() + "/" +SpectrumFile);
              if (!ok)
                {
                  qWarning()<<"Load energy spectrum failed! file: "+SpectrumFile;
                  return false;
                }
            }
          //new
          QJsonArray ar;
          if (GunParticleParser.ParseObject("EnergySpectrum", ar) )
            {
              if (s->GunParticles[ip]->spectrum) delete s->GunParticles[ip]->spectrum;
              int size = ar.size();
              double* xx = new double [size];
              double* yy = new double [size];
              for (int i=0; i<size; i++)
                {
                  xx[i] = ar[i].toArray()[0].toDouble();
                  yy[i] = ar[i].toArray()[1].toDouble();
                }
              //generating unique name for the histogram
              QString str = "EnSp" + QString::number(iSource) + "s" + QString::number(ip) + "p";
              QByteArray ba = str.toLocal8Bit();
              const char *c_str = ba.data();
              s->GunParticles[ip]->spectrum = new TH1D(c_str,"Energy spectrum", size-1, xx);
              for (int j = 1; j<size+1; j++) s->GunParticles[ip]->spectrum->SetBinContent(j, yy[j-1]);
              delete[] xx;
              delete[] yy;
            }
           // TO DO !!! *** add error if no energy or spectrum
       }
    }

  return true;
}

bool ParticleSourcesClass::LoadGunEnergySpectrum(int iSource, int iParticle, QString fileName)
{
  if (iSource<0 || iSource>ParticleSourcesData.size()-1)
    {
      qWarning("Energy spectrum was NOT loaded - wrong source index");
      return false;
    }
  if (iParticle<0 || iParticle>ParticleSourcesData[iSource]->GunParticles.size()-1)
    {
      qWarning("Energy spectrum was NOT loaded - wrong particle index");
      return false;
    }

  QVector<double> x, y;
  int error = LoadDoubleVectorsFromFile(fileName, &x, &y);
  if (error>0) return false;

  if (ParticleSourcesData[iSource]->GunParticles[iParticle]->spectrum) delete ParticleSourcesData[iSource]->GunParticles[iParticle]->spectrum;
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

int ParticleSourcesClass::CheckSource(int isource)
{
  if (isource<0 || isource>size()-1) return 1; // 1 - wrong isource

  // obsolete: 2 - particle collection not connected
  if (MpCollection == 0) return 3; // 3 - material collection not connected
  if (getTotalActivity() == 0) return 4; // 4 - total activity = 0

  ParticleSourceStructure* ps = ParticleSourcesData[isource];
  if (ps->index <0 || ps->index>5) return 10; //10 - unknown source shape

  int numParts = ps->GunParticles.size();     //11 - no particles defined
  if (numParts == 0) return 11;

  //checking all particles
  int numIndParts = 0;
  double TotPartWeight = 0;
  for (int ip = 0; ip<numParts; ip++)
    {
      GunParticleStruct* gp = ps->GunParticles[ip];

      int id = gp->ParticleId;
      //if (id<0 || id>ParticleCollection->size()-1) return 12; // 12 - use of non-defined particle
      if (id<0 || id>MpCollection->countParticles()-1) return 12; // 12 - use of non-defined particle

      if (gp->Individual)
        {
          //individual
          numIndParts++;

          TotPartWeight += ParticleSourcesData[isource]->GunParticles[ip]->StatWeight;
        }
      else
        {
          //linked
          if (ip == gp->LinkedTo) return 13; // 13 - particle is linked to itself
        }
    }

  if (numIndParts == 0) return 21; //21 - no individual particles defined
  if (TotPartWeight == 0) return 22;  //total weight of individual particles = 0

  return 0; //all is OK
}

QString ParticleSourcesClass::getErrorString(int error)
{
  switch (error)
    {
    case 0: return "No errors";
    case 1: return "Wrong source number";
    case 2: return "Particle collection not connected";
    case 3: return "Materia collection not connected";
    case 4: return "Total activity of all sources = 0";
    case 10: return "Unknown source shape";
    case 11: return "No particles defined";
    case 12: return "Use of non-defined particle";
    case 13: return "Particle is linked to itself";
    case 21: return "No individual particles defined";
    case 22: return "Notal probability to emit a particle = 0";
    }

  return "undefined error";
}

void ParticleSourcesClass::append(ParticleSourceStructure *gunParticle)
{
  ParticleSourcesData.append(gunParticle);
  CalculateTotalActivity();
}

void ParticleSourcesClass::remove(int iSource)
{
  if (ParticleSourcesData.isEmpty()) return;
  if (iSource >= ParticleSourcesData.size()) return;

  delete ParticleSourcesData[iSource];
  ParticleSourcesData.remove(iSource);
  CalculateTotalActivity();
}

GunParticleStruct::~GunParticleStruct()
{
  if (spectrum) delete spectrum;
}


ParticleSourceStructure::ParticleSourceStructure()
{
  name = "Underfined_name";
  Activity = 1;
  index = 0;
  X0 = 0; Y0 = 0; Z0 = 0;
  size1 = 10; size2 = 10; size3 = 10;
  Phi = 0; Theta = 0; Psi = 0;
  CollPhi = 0; CollTheta = 0;
  Spread = 45;
  DoMaterialLimited = fLimit = false;

  GunParticles.resize(0);
}

ParticleSourceStructure::~ParticleSourceStructure()
{
  for (int i=0; i<GunParticles.size(); i++) delete GunParticles[i];
}
