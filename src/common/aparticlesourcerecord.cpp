#include "aparticlesourcerecord.h"
#include "amaterialparticlecolection.h"
#include "afiletools.h"
#include "ajsontools.h"
#include "aparticle.h" //---
#include "acommonfunctions.h"

#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>

#include "TH1D.h"
#include "TRandom2.h"

GunParticleStruct * GunParticleStruct::clone() const
{
    //shallow copy
    GunParticleStruct* gp = new GunParticleStruct(*this);

    //clear dynamic
    gp->spectrum = 0;

    //deep copy for dynamic properties
    if (spectrum)
        gp->spectrum = new TH1D(*spectrum);

    return gp;
}

double GunParticleStruct::generateEnergy(TRandom2 * RandGen) const
{
    if (bUseFixedEnergy || !spectrum)
        return energy;
    //return spectrum->GetRandom();
    return GetRandomFromHist(spectrum, RandGen);
}

bool GunParticleStruct::loadSpectrum(const QString &fileName)
{
    QVector<double> x, y;
    int error = LoadDoubleVectorsFromFile(fileName, &x, &y);
    if (error > 0) return false;

    delete spectrum; spectrum = 0;
    int size = x.size();
    //double* xx = new double[size];
    //for (int i = 0; i<size; i++) xx[i]=x[i];
    spectrum = new TH1D("","Energy spectrum", size-1, x.data());
    for (int j = 1; j < size+1; j++)
        spectrum->SetBinContent(j, y[j-1]);
    return true;
}

void GunParticleStruct::writeToJson(QJsonObject &json, const AMaterialParticleCollection &MpCollection) const
{
    const AParticle* p = MpCollection.getParticle(ParticleId); // TODO matColl -> partToJson
    QJsonObject jparticle;
        jparticle["name"] = p->ParticleName;
        jparticle["type"] = p->type;
        jparticle["charge"] = p->charge;
        jparticle["mass"] = p->mass;
    json["Particle"] = jparticle;

    json["StatWeight"] = StatWeight;
    json["Individual"] = Individual;
    json["LinkedTo"] = LinkedTo;
    json["LinkingProbability"] = LinkingProbability;
    json["LinkingOppositeDir"] = LinkingOppositeDir;
    json["Energy"] = energy;
    json["UseFixedEnergy"] = bUseFixedEnergy;
    if ( spectrum )
    {
        QJsonArray ja;
            writeTH1DtoJsonArr(spectrum, ja);
        json["EnergySpectrum"] = ja;
    }
    json["PreferredUnits"] = PreferredUnits;
}

bool GunParticleStruct::readFromJson(const QJsonObject &json, AMaterialParticleCollection & MpCollection)
{
    QJsonObject jparticle = json["Particle"].toObject();
    if (jparticle.isEmpty())
    {
        qWarning()<<"Particle data not given in the particle source file";
        return false;
    }
    QString name = jparticle["name"].toString();
    int type = jparticle["type"].toInt();
    int charge = jparticle["charge"].toInt();
    double mass = jparticle["mass"].toDouble();
    //looking for this particle in the collection and create if necessary
    AParticle::ParticleType Type = static_cast<AParticle::ParticleType>(type);
    ParticleId = MpCollection.FindCreateParticle(name, Type, charge, mass);
    //qDebug()<<"Added gun particle with particle Id"<<ParticleId<<ParticleCollection->at(ParticleId)->ParticleName;

    parseJson(json, "StatWeight",  StatWeight );
    parseJson(json, "Individual",  Individual );
    parseJson(json, "LinkedTo",  LinkedTo ); //linked always to previously already defined particles!
    parseJson(json, "LinkingProbability",  LinkingProbability );
    parseJson(json, "LinkingOppositeDir",  LinkingOppositeDir );

    parseJson(json, "Energy",  energy );
    parseJson(json, "PreferredUnits",  PreferredUnits );
    parseJson(json, "UseFixedEnergy",  bUseFixedEnergy );

    QJsonArray ar = json["EnergySpectrum"].toArray();
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
        spectrum = new TH1D("", "", size-1, xx);
        for (int j = 1; j<size+1; j++) spectrum->SetBinContent(j, yy[j-1]);
        delete[] xx;
        delete[] yy;
    }
    return true;
}

GunParticleStruct::~GunParticleStruct()
{
  if (spectrum) delete spectrum;
}

// ---------------------- AParticleSourceRecord ----------------------

AParticleSourceRecord::~AParticleSourceRecord()
{
    clearGunParticles();
}

void AParticleSourceRecord::clearGunParticles()
{
    for (GunParticleStruct* g : GunParticles) delete g;
    GunParticles.clear();
}

AParticleSourceRecord * AParticleSourceRecord::clone() const
{
    //shallow copy
    AParticleSourceRecord * newRec = new AParticleSourceRecord(*this);

    //clear dynamic
    newRec->GunParticles.clear();

    //deep copy of dynamic resources
    for (GunParticleStruct* g : GunParticles)
        newRec->GunParticles << g->clone();

    return newRec;
}

void AParticleSourceRecord::writeToJson(QJsonObject & json, const AMaterialParticleCollection & MpCollection) const
{
    json["Name"] = name;
    json["Type"] = shape;
    json["Activity"] = Activity;
    json["X"] = X0;
    json["Y"] = Y0;
    json["Z"] = Z0;
    json["Size1"] = size1;
    json["Size2"] = size2;
    json["Size3"] = size3;
    json["Phi"] = Phi;
    json["Theta"] = Theta;
    json["Psi"] = Psi;
    json["CollPhi"] = CollPhi;
    json["CollTheta"] = CollTheta;
    json["Spread"] = Spread;

    json["DoMaterialLimited"] = DoMaterialLimited;
    json["LimitedToMaterial"] = LimtedToMatName;

    //particles
    int GunParticleSize = GunParticles.size();
    json["Particles"] = GunParticleSize;
    QJsonArray jParticleEntries;
    for (const GunParticleStruct* gp : GunParticles)
    {
        QJsonObject js;
        gp->writeToJson(js, MpCollection);
        jParticleEntries.append(js);
    }
    json["GunParticles"] = jParticleEntries;
}

bool AParticleSourceRecord::readFromJson(const QJsonObject &json, AMaterialParticleCollection &MpCollection)
{
    parseJson(json, "Name", name);
    parseJson(json, "Type", shape);
    parseJson(json, "Activity", Activity);
    parseJson(json, "X", X0);
    parseJson(json, "Y", Y0);
    parseJson(json, "Z", Z0);
    parseJson(json, "Size1", size1);
    parseJson(json, "Size2", size2);
    parseJson(json, "Size3", size3);
    parseJson(json, "Phi", Phi);
    parseJson(json, "Theta", Theta);
    parseJson(json, "Psi", Psi);
    parseJson(json, "CollPhi", CollPhi);
    parseJson(json, "CollTheta", CollTheta);
    parseJson(json, "Spread", Spread);

    DoMaterialLimited = fLimit = false;
    LimtedToMatName = "";
    if (json.contains("DoMaterialLimited"))
    {
        parseJson(json, "DoMaterialLimited", DoMaterialLimited);
        parseJson(json, "LimitedToMaterial", LimtedToMatName);

        if (DoMaterialLimited)
        {
            bool fFound = false;
            int iMat;
            for (iMat = 0; iMat < MpCollection.countMaterials(); iMat++)  //TODO make a method in MpCol
                if (LimtedToMatName == MpCollection[iMat]->name)
                {
                    fFound = true;
                    break;
                }

            if (fFound) //only in this case limit to material will be used!
            {
                fLimit = true;
                LimitedToMat = iMat;
            }
        }
    }

    //GunParticles
    clearGunParticles();
    QJsonArray jGunPartArr = json["GunParticles"].toArray();
    int numGP = jGunPartArr.size();
    //qDebug()<<"Entries in gunparticles:"<<numGP;
    for (int ip = 0; ip < numGP; ip++)
    {
        QJsonObject jThisGunPart = jGunPartArr[ip].toObject();

        GunParticleStruct* gp = new GunParticleStruct();
        bool bOK = gp->readFromJson(jThisGunPart, MpCollection);
        if (!bOK)
        {
            delete gp;
            return false;
        }
        GunParticles << gp;
    }
    return true;
}

const QString AParticleSourceRecord::getShapeString() const
{
    switch (shape)
    {
    case 0: return "Point";
    case 1: return "Linear";
    case 2: return "Square";
    case 3: return "Round";
    case 4: return "Box";
    case 5: return "Cylinder";
    default: ;
    }
    return "-error-";
}

const QString AParticleSourceRecord::CheckSource(const AMaterialParticleCollection & MpCollection) const
{
    if (shape < 0 || shape > 5) return "unknown source shape";

    int numParts = GunParticles.size();     //11 - no particles defined
    if (numParts == 0) return "no particles defined";

    if (Spread < 0) return "negative spread angle";
    if (Activity < 0) return "negative activity";

    //checking all particles
    int numIndParts = 0;
    double TotPartWeight = 0;
    for (int ip = 0; ip<numParts; ip++)
    {
        GunParticleStruct* gp = GunParticles.at(ip);
        int pid = gp->ParticleId;
        if (pid < 0 || pid >= MpCollection.countParticles()) return QString("uses not valid particle index %1").arg(pid);

        if (gp->Individual)
        {
            //individual
            numIndParts++;
            if (GunParticles.at(ip)->StatWeight < 0) return QString("negative statistical weight for particle #%1").arg(ip);
            TotPartWeight += GunParticles.at(ip)->StatWeight;
        }
        else
        {
            //linked
            if (ip == gp->LinkedTo) return QString("particle #%1 is linked to itself").arg(ip);
            if (ip < gp->LinkedTo) return QString("invalid linking for particle #%1").arg(ip);
        }

        if (gp->energy <= 0) return QString("invalid energy of %1 for particle #%2").arg(gp->energy).arg(ip);
    }

    if (numIndParts == 0) return "no individual particles defined";
    if (TotPartWeight == 0) return "total statistical weight of individual particles is zero";

    return "";
}
