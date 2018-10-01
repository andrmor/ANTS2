#include "aopticaloverride.h"
#include "amaterialparticlecolection.h"
#include "aphoton.h"
#include "acommonfunctions.h"
#include "ajsontools.h"
#include "afiletools.h"
#include "asimulationstatistics.h"

#ifdef SIM
#include "phscatclaudiomodel.h"
#include "scatteronmetal.h"
#include "ascriptopticaloverride.h"
#endif

#include <QDebug>
#include <QJsonObject>

#include "TMath.h"
#include "TRandom2.h"
#include "TH1I.h"
#include "TH1.h"

#ifdef GUI
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QFrame>
#include <QDoubleValidator>
#include <QComboBox>
#include <QPushButton>
#include <QFileDialog>
#include "amessage.h"
#include "TGraph.h"
#include "graphwindowclass.h"
#endif

void AOpticalOverride::writeToJson(QJsonObject &json)
{
    json["Model"] = getType();
    json["MatFrom"] = MatFrom;
    json["MatTo"] = MatTo;
}

bool AOpticalOverride::readFromJson(QJsonObject &json)
{
    QString type = json["Model"].toString();
    if (type != getType()) return false; //file for wrong model!
    return true;
}

#ifdef GUI
#include <QFrame>
QWidget *AOpticalOverride::getEditWidget(QWidget *, GraphWindowClass *)
{
    QFrame* f = new QFrame();
    f->setFrameStyle(QFrame::Box);
    f->setMinimumHeight(100);

    return f;
}
#endif

void AOpticalOverride::RandomDir(TRandom2 *RandGen, APhoton *Photon)
{
  //Sphere function of Root:
  double a=0, b=0, r2=1;
  while (r2 > 0.25)
    {
      a  = RandGen->Rndm() - 0.5;
      b  = RandGen->Rndm() - 0.5;
      r2 =  a*a + b*b;
    }
  Photon->v[2] = ( -1.0 + 8.0 * r2 );
  double scale = 8.0 * TMath::Sqrt(0.25 - r2);
  Photon->v[0] = a*scale;
  Photon->v[1] = b*scale;
}

BasicOpticalOverride::BasicOpticalOverride(AMaterialParticleCollection *MatCollection, int MatFrom, int MatTo, double probLoss, double probRef, double probDiff, int scatterModel)
    : AOpticalOverride(MatCollection, MatFrom, MatTo),
      probLoss(probLoss), probRef(probRef), probDiff(probDiff), scatterModel(scatterModel) {}

BasicOpticalOverride::BasicOpticalOverride(AMaterialParticleCollection *MatCollection, int MatFrom, int MatTo)
    : AOpticalOverride(MatCollection, MatFrom, MatTo) {}

AOpticalOverride::OpticalOverrideResultEnum BasicOpticalOverride::calculate(TRandom2 *RandGen, APhoton *Photon, const double *NormalVector)
{
    double rnd = RandGen->Rndm();

    // surface loss?
    rnd -= probLoss;
    if (rnd<0)
    {
        // qDebug()<<"Override: surface loss!";
        Photon->SimStat->OverrideSimplisticAbsorption++;
        Status = Absorption;
        return Absorbed;
    }

  // specular reflection?
  rnd -= probRef;
  if (rnd<0)
    {
      // qDebug()<<"Override: specular reflection!";
        //rotating the vector: K = K - 2*(NK)*N
      double NK = NormalVector[0]*Photon->v[0]; NK += NormalVector[1]*Photon->v[1];  NK += NormalVector[2]*Photon->v[2];
      Photon->v[0] -= 2.0*NK*NormalVector[0]; Photon->v[1] -= 2.0*NK*NormalVector[1]; Photon->v[2] -= 2.0*NK*NormalVector[2];

      Photon->SimStat->OverrideSimplisticReflection++;
      Status = SpikeReflection;
      return Back;
    }

  // scattering?
  rnd -= probDiff;
  if (rnd<0)
    {
      // qDebug()<<"scattering triggered";
      Photon->SimStat->OverrideSimplisticScatter++;

      switch (scatterModel)
        {
        case 0: //4Pi scattering
          // qDebug()<<"4Pi scatter";
          RandomDir(RandGen, Photon);
          // qDebug()<<"New direction:"<<K[0]<<K[1]<<K[2];

          //enering new volume or backscattering?
          //normal is in the positive direction in respect to the original direction!
          if (Photon->v[0]*NormalVector[0] + Photon->v[1]*NormalVector[1] + Photon->v[2]*NormalVector[2] < 0)
            {
              // qDebug()<<"   scattering back";
              Status = LambertianReflection;
              return Back;
            }
          // qDebug()<<"   continuing to the next volume";
          Status = Transmission;
          return Forward;

        case 1: //2Pi lambertian, remaining in the same volume (back scattering)
          {
            // qDebug()<<"2Pi lambertian scattering backward";
            double norm2;
            do
              {
                RandomDir(RandGen, Photon);
                Photon->v[0] -= NormalVector[0]; Photon->v[1] -= NormalVector[1]; Photon->v[2] -= NormalVector[2];
                norm2 = Photon->v[0]*Photon->v[0] + Photon->v[1]*Photon->v[1] + Photon->v[2]*Photon->v[2];
              }
            while (norm2 < 0.000001);

            double normInverted = 1.0/TMath::Sqrt(norm2);
            Photon->v[0] *= normInverted; Photon->v[1] *= normInverted; Photon->v[2] *= normInverted;
            Status = LambertianReflection;
            return Back;
          }
        case 2: //2Pi lambertian, scattering to the next volume
          {
            // qDebug()<<"2Pi lambertian scattering forward";
            double norm2;
            do
              {
                RandomDir(RandGen, Photon);
                Photon->v[0] += NormalVector[0]; Photon->v[1] += NormalVector[1]; Photon->v[2] += NormalVector[2];
                norm2 = Photon->v[0]*Photon->v[0] + Photon->v[1]*Photon->v[1] + Photon->v[2]*Photon->v[2];
              }
            while (norm2 < 0.000001);

            double normInverted = 1.0/TMath::Sqrt(norm2);
            Photon->v[0] *= normInverted; Photon->v[1] *= normInverted; Photon->v[2] *= normInverted;
            Status = Transmission;
            return Forward;
          }
        }
    }

  // overrides NOT triggered - what is left is covered by Fresnel in the tracker code
  // qDebug()<<"Overrides did not trigger, using fresnel";
  Status = Transmission;
  return NotTriggered;
}

void BasicOpticalOverride::printConfiguration(int /*iWave*/)
{
  qDebug() << "-------Configuration:-------";
  qDebug() << "Absorption fraction:"<<probLoss;
  qDebug() << "Specular fraction:"<<probRef;
  qDebug() << "Scatter fraction:"<<probDiff;
  qDebug() << "Scatter model (4Pi/LambBack/LambForward):"<<scatterModel;
  qDebug() << "----------------------------";
}

QString BasicOpticalOverride::getReportLine()
{
  QString s = "to " + (*MatCollection)[MatTo]->name;
  QString s1;
  s += "->";

  double prob = probRef+probDiff+probLoss;
  if (prob == 0) return s + " To be defined"; //not defined - shown during configuration phase only
  double probFresnel = 1.0 - prob;  

  if (probLoss>0)
  {
      s1.setNum(probLoss);
      s += " Loss: "+s1+";";
  }
  if (probRef>0)
  {
      s1.setNum(probRef);
      s += " Spec: "+s1+";";
  }
  if (probDiff>0)
  {
      s1.setNum(probDiff);
      switch( scatterModel )
        {
        case 0:
          s += " Scat(4Pi): ";
          break;
        case 1:
          s += " Scat(Lamb_B): ";
          break;
        case 2:
          s += " Scat(Lamb_F): ";
          break;
        }
      s += s1;
  }

  if (probFresnel>1e-10)
  {
      s1.setNum(probFresnel);
      s += " Fresnel: "+s1;
  }

  return s;
}

void BasicOpticalOverride::writeToJson(QJsonObject &json)
{
  AOpticalOverride::writeToJson(json);

  json["Abs"]  = probLoss;
  json["Spec"] = probRef;
  json["Scat"] = probDiff;
  json["ScatMode"] = scatterModel;  
}

bool BasicOpticalOverride::readFromJson(QJsonObject &json)
{
  QString type = json["Model"].toString();
  if (type != getType()) return false; //file for wrong model!

  probLoss = json["Abs"].toDouble();
  probRef =  json["Spec"].toDouble();
  probDiff = json["Scat"].toDouble();
  scatterModel = json["ScatMode"].toInt();
  return true;
}

#ifdef GUI
QWidget *BasicOpticalOverride::getEditWidget(QWidget*, GraphWindowClass *)
{
    QFrame* f = new QFrame();
    f->setFrameStyle(QFrame::Box);

    QHBoxLayout* hl = new QHBoxLayout(f);
        QVBoxLayout* l = new QVBoxLayout();
            QLabel* lab = new QLabel("Absorption:");
        l->addWidget(lab);
            lab = new QLabel("Specular reflection:");
        l->addWidget(lab);
            lab = new QLabel("Scattering:");
        l->addWidget(lab);
    hl->addLayout(l);
        l = new QVBoxLayout();
            QLineEdit* le = new QLineEdit(QString::number(probLoss));
            QDoubleValidator* val = new QDoubleValidator(f);
            val->setNotation(QDoubleValidator::StandardNotation);
            val->setBottom(0);
            //val->setTop(1.0); //Qt(5.8.0) BUG: check does not work
            val->setDecimals(6);
            le->setValidator(val);
            QObject::connect(le, &QLineEdit::editingFinished, [le, this]() { this->probLoss = le->text().toDouble(); } );
        l->addWidget(le);
            le = new QLineEdit(QString::number(probRef));
            le->setValidator(val);
            QObject::connect(le, &QLineEdit::editingFinished, [le, this]() { this->probRef = le->text().toDouble(); } );
        l->addWidget(le);
            le = new QLineEdit(QString::number(probDiff));
            le->setValidator(val);
            QObject::connect(le, &QLineEdit::editingFinished, [le, this]() { this->probDiff = le->text().toDouble(); } );
        l->addWidget(le);
    hl->addLayout(l);
        l = new QVBoxLayout();
            lab = new QLabel("");
        l->addWidget(lab);
            lab = new QLabel("");
        l->addWidget(lab);
            QComboBox* com = new QComboBox();
            com->addItem("Isotropic (4Pi)"); com->addItem("Lambertian, 2Pi back"); com->addItem("Lambertian, 2Pi forward");
            com->setCurrentIndex(scatterModel);
            QObject::connect(com, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), [this](int index) { this->scatterModel = index; } );
        l->addWidget(com);
    hl->addLayout(l);

    return f;
}
#endif

const QString BasicOpticalOverride::checkOverrideData()
{
    if (probLoss<0 || probLoss>1.0) return "Absorption probability should be within [0, 1.0]";
    if (probRef <0 || probRef >1.0) return "Reflection probability should be within [0, 1.0]";
    if (probDiff<0 || probDiff>1.0) return "Scattering probability should be within [0, 1.0]";

    if (probLoss + probRef + probDiff > 1.0) return "Sum of all process probabilities cannot exceed 1.0";

    return "";
}

AOpticalOverride *OpticalOverrideFactory(QString model, AMaterialParticleCollection *MatCollection, int MatFrom, int MatTo)
{
   if (model == "Simplistic_model")
     return new BasicOpticalOverride(MatCollection, MatFrom, MatTo);
   if (model == "SimplisticSpectral_model")
     return new SpectralBasicOpticalOverride(MatCollection, MatFrom, MatTo);
   else if (model == "Claudio_Model_V2")
     return new PhScatClaudioModelV2(MatCollection, MatFrom, MatTo);
   else if (model == "Claudio_Model_V2d1")
     return new PhScatClaudioModelV2d1(MatCollection, MatFrom, MatTo);
   else if (model == "Claudio_Model_V2d2")
     return new PhScatClaudioModelV2d2(MatCollection, MatFrom, MatTo);
   else if (model == "Claudio_Model") //compatibility
     return new PhScatClaudioModelV2(MatCollection, MatFrom, MatTo);
   else if (model == "DielectricToMetal")
     return new ScatterOnMetal(MatCollection, MatFrom, MatTo);
   else if (model == "FS_NP" || model=="Neves_model")
     return new FSNPOpticalOverride(MatCollection, MatFrom, MatTo);
   else if (model == "SurfaceWLS")
     return new AWaveshifterOverride(MatCollection, MatFrom, MatTo);
   else if (model == "CustomScript")
     return new AScriptOpticalOverride(MatCollection, MatFrom, MatTo);

   return NULL; //undefined override type!
}

const QStringList ListOvAllOpticalOverrideTypes()
{
    QStringList l;

    l << "Simplistic_model"
      << "SimplisticSpectral_model"
      << "Claudio_Model_V2d2"
      << "DielectricToMetal"
      << "FS_NP"
      << "SurfaceWLS"
      << "CustomScript";

    return l;
}

AOpticalOverride::OpticalOverrideResultEnum FSNPOpticalOverride::calculate(TRandom2 *RandGen, APhoton *Photon, const double *NormalVector)
{
  // Angular reflectance: fraction of light reflected at the interface bewteen
  // medium 1 and medium 2 assuming non-polarized incident light:
  // ==> Incident light goes from medium 1 (n1) into medium 2 (n2).
  // ==> cos(0): perpendicular to the surface.
  // ==> cos(Pi/2): grazing/parallel to the surface.
  // NOTE that even if the incident light is not polarized (equal amounts of s-
  // and p-), the reflected light may be polarized because different percentages
  // of s-(perpendicular) and p-(parallel) polarized waves are/may be reflected.

  //refractive indexes of materials before and after the interface
  double n1 = (*MatCollection)[MatFrom]->getRefractiveIndex(Photon->waveIndex);
  double n2 = (*MatCollection)[MatTo]->getRefractiveIndex(Photon->waveIndex);

  //angle of incidence
  double cos1 = 0;
  for (int i=0; i<3; i++)
     cos1 += NormalVector[i]*Photon->v[i];

  //Calculating reflection probability
  double fresnelUnpolarR;
  double sin1sqr = 1.0 - cos1*cos1;
  double nsqr = n2/n1; nsqr *= nsqr;
  if (sin1sqr > nsqr)
    fresnelUnpolarR = 1.0;
  else
    {
      double f1 = nsqr * cos1;
      double f2 = TMath::Sqrt( nsqr - sin1sqr );
      // p-polarized
      double Rp12 = (f1-f2) / (f1+f2); Rp12 *= Rp12;
      // s-polarized
      double Rs12 = (cos1-f2) / (cos1+f2); Rs12 *= Rs12;

      fresnelUnpolarR = 0.5 * (Rp12+Rs12);
    }

//  if random[0,1]<fresnelUnpolarR do specular reflection
  if (RandGen->Rndm() < fresnelUnpolarR)
    {
      //qDebug()<<"Override: specular reflection";
        //rotating the vector: K = K - 2*(NK)*N
      Photon->v[0] -= 2.0*cos1*NormalVector[0]; Photon->v[1] -= 2.0*cos1*NormalVector[1]; Photon->v[2] -= 2.0*cos1*NormalVector[2];
      Status = SpikeReflection;
      Photon->SimStat->OverrideFSNPspecular++;
      return Back;
    }

// if random[0,1]>albedo kill photon else do diffuse reflection
  if (RandGen->Rndm() > Albedo)
    {
      //qDebug()<<"Override: absorption";
      Status = Absorption;
      Photon->SimStat->OverrideFSNPabs++;
      return Absorbed;
    }

  //qDebug() << "Override: Lambertian scattering";
  double norm2;
  do
    {
      RandomDir(RandGen, Photon);
      Photon->v[0] -= NormalVector[0]; Photon->v[1] -= NormalVector[1]; Photon->v[2] -= NormalVector[2];
      norm2 = Photon->v[0]*Photon->v[0] + Photon->v[1]*Photon->v[1] + Photon->v[2]*Photon->v[2];
    }
  while (norm2 < 0.000001);
  double normInverted = 1.0/TMath::Sqrt(norm2);
  Photon->v[0] *= normInverted; Photon->v[1] *= normInverted; Photon->v[2] *= normInverted;
  Status = LambertianReflection;
  Photon->SimStat->OverrideFSNlambert++;
  return Back;

// OLD MODEL
//first absorption check is made agains albedo, if failed - photon lost
//if angle of incidence is smaller than critical angle -> diffuse scattering
// else -> specular reflection
//  if (RandGen->Rndm() > Albedo)
//    { //absorption!
//      Status = Absorption;
//      return Absorbed;
//    }

//  //critical angle check
//  double NK = 0;
//  for (int i=0; i<3; i++)
//    NK += NormalVector[i]*PhotonDir[i]; // NK = cos of the angle of incidence = cos1
//  double sin1 = sqrt(1.0 - NK*NK);
//  double sin2 = n1/n2 * sin1;

//  if (fabs(sin2)>1.0)
//    { //total internal!
//      // qDebug()<<"Override: specular reflection!";
//        //rotating the vector: K = K - 2*(NK)*N
//      PhotonDir[0] -= 2.0*NK*NormalVector[0]; PhotonDir[1] -= 2.0*NK*NormalVector[1]; PhotonDir[2] -= 2.0*NK*NormalVector[2];
//      Status = SpikeReflection;
//      return Back;
//    }

//  //else diffuse
//  // qDebug()<<"2Pi lambertian scattering backward";
//  double norm2;
//  do
//    {
//      RandomDir(RandGen, PhotonDir);
//      PhotonDir[0] -= NormalVector[0]; PhotonDir[1] -= NormalVector[1]; PhotonDir[2] -= NormalVector[2];
//      norm2 = PhotonDir[0]*PhotonDir[0] + PhotonDir[1]*PhotonDir[1] + PhotonDir[2]*PhotonDir[2];
//    }
//  while (norm2 < 0.000001);

//  double normInverted = 1.0/TMath::Sqrt(norm2);
//  PhotonDir[0] *= normInverted; PhotonDir[1] *= normInverted; PhotonDir[2] *= normInverted;
//  // qDebug()<<"Photon dir after scatter:"<<K[0]<<K[1]<<K[2];
//  Status = LambertianReflection;
//  return Back;
}

void FSNPOpticalOverride::printConfiguration(int /*iWave*/)
{
  qDebug() << "-------Configuration:-------";
  qDebug() << "FS_NP model";
  qDebug() << "Albedo:"<<Albedo;
  qDebug() << "----------------------------";
}

QString FSNPOpticalOverride::getReportLine()
{
  QString s = "to " + (*MatCollection)[MatTo]->name;
  s += "->FS_NP model, albedo="+QString::number(Albedo);
  return s;
}

void FSNPOpticalOverride::writeToJson(QJsonObject &json)
{
  AOpticalOverride::writeToJson(json);

  json["Albedo"] = Albedo;
}

bool FSNPOpticalOverride::readFromJson(QJsonObject &json)
{
  QString type = json["Model"].toString();
  if (type != getType()) return false; //file for wrong model!

  if (json.contains("Albedo"))
    {
      Albedo = json["Albedo"].toDouble();
      return true;
    }
  else
      return false;
}

#ifdef GUI
QWidget *FSNPOpticalOverride::getEditWidget(QWidget *, GraphWindowClass *)
{
    QFrame* f = new QFrame();
    f->setFrameStyle(QFrame::Box);

    QHBoxLayout* l = new QHBoxLayout(f);
        QLabel* lab = new QLabel("Albedo:");
    l->addWidget(lab);
        QLineEdit* le = new QLineEdit(QString::number(Albedo));
        QDoubleValidator* val = new QDoubleValidator(f);
        val->setNotation(QDoubleValidator::StandardNotation);
        val->setBottom(0);
        //val->setTop(1.0); //Qt(5.8.0) BUG: check does not work
        val->setDecimals(6);
        le->setValidator(val);
        QObject::connect(le, &QLineEdit::editingFinished, [le, this]() { this->Albedo = le->text().toDouble(); } );
    l->addWidget(le);

    return f;
}
#endif

const QString FSNPOpticalOverride::checkOverrideData()
{
    if (Albedo<0 || Albedo>1.0) return "Albedo should be within [0, 1.0]";
    return "";
}

AWaveshifterOverride::AWaveshifterOverride(AMaterialParticleCollection *MatCollection, int MatFrom, int MatTo, int ReemissionModel)
    : AOpticalOverride(MatCollection, MatFrom, MatTo), ReemissionModel(ReemissionModel)
{
    Spectrum = 0;
}

AWaveshifterOverride::~AWaveshifterOverride()
{
    if (Spectrum) delete Spectrum;
}

void AWaveshifterOverride::initializeWaveResolved(bool bWaveResolved, double waveFrom, double waveStep, int waveNodes)
{
    if (bWaveResolved)
    {
        WaveFrom = waveFrom;
        WaveStep = waveStep;
        WaveNodes = waveNodes;

        ConvertToStandardWavelengthes(&ReemissionProbability_lambda, &ReemissionProbability, WaveFrom, WaveStep, WaveNodes, &ReemissionProbabilityBinned);

        QVector<double> y;
        ConvertToStandardWavelengthes(&EmissionSpectrum_lambda, &EmissionSpectrum, WaveFrom, WaveStep, WaveNodes, &y);
        TString name = "WLSEmSpec";
        name += MatFrom;
        name += "to";
        name += MatTo;
        if (Spectrum) delete Spectrum;
        Spectrum = new TH1D(name,"", WaveNodes, WaveFrom, WaveFrom+WaveStep*WaveNodes);
        for (int j = 1; j<WaveNodes+1; j++)  Spectrum->SetBinContent(j, y[j-1]);
        Spectrum->GetIntegral(); //to make thread safe
    }
    else
    {
        ReemissionProbabilityBinned.clear();
        delete Spectrum; Spectrum = 0;
    }
}

AOpticalOverride::OpticalOverrideResultEnum AWaveshifterOverride::calculate(TRandom2 *RandGen, APhoton *Photon, const double *NormalVector)
{
    //currently assuming there is no scattering on original wavelength - only reemission or absorption

    if ( !Spectrum ||                               //emission spectrum not defined
         Photon->waveIndex == -1 ||                 //or photon without wavelength
         ReemissionProbabilityBinned.isEmpty() )    //or probability not defined
    {
        Status = Absorption;
        Photon->SimStat->OverrideWLSabs++;
        return Absorbed;
    }

    double prob = ReemissionProbabilityBinned.at(Photon->waveIndex); // probability of reemission
    if (RandGen->Rndm() < prob)
    {
        //triggered!

        //generating new wavelength and waveindex
        double wavelength;
        int waveIndex;
        int attempts = -1;
        do
        {
            attempts++;
            if (attempts > 9)
              {
                Status = Absorption;
                Photon->SimStat->OverrideWLSabs++;
                return Absorbed;
              }
            wavelength = Spectrum->GetRandom();
            waveIndex = (wavelength - WaveFrom)/WaveStep;
        }
        while (waveIndex < Photon->waveIndex); //conserving energy

        Photon->waveIndex = waveIndex;
        Photon->SimStat->OverrideWLSshift++;

        if (ReemissionModel == 0)
        {
            RandomDir(RandGen, Photon);
            //enering new volume or backscattering?
            //normal is in the positive direction in respect to the original direction!
            if (Photon->v[0]*NormalVector[0] + Photon->v[1]*NormalVector[1] + Photon->v[2]*NormalVector[2] < 0)
              {
                // qDebug()<<"   scattering back";
                Status = LambertianReflection;
                return Back;
              }
            // qDebug()<<"   continuing to the next volume";
            Status = Transmission;
            return Forward;
        }

        double norm2 = 0;
        if (ReemissionModel == 1)
        {
            // qDebug()<<"2Pi lambertian scattering backward";
            do
            {
                RandomDir(RandGen, Photon);
                Photon->v[0] -= NormalVector[0]; Photon->v[1] -= NormalVector[1]; Photon->v[2] -= NormalVector[2];
                norm2 = Photon->v[0]*Photon->v[0] + Photon->v[1]*Photon->v[1] + Photon->v[2]*Photon->v[2];
            }
            while (norm2 < 0.000001);

            double normInverted = 1.0/TMath::Sqrt(norm2);
            Photon->v[0] *= normInverted; Photon->v[1] *= normInverted; Photon->v[2] *= normInverted;
            Status = LambertianReflection;

            return Back;
        }

        // qDebug()<<"2Pi lambertian scattering forward";
        do
          {
            RandomDir(RandGen, Photon);
            Photon->v[0] += NormalVector[0]; Photon->v[1] += NormalVector[1]; Photon->v[2] += NormalVector[2];
            norm2 = Photon->v[0]*Photon->v[0] + Photon->v[1]*Photon->v[1] + Photon->v[2]*Photon->v[2];
          }
        while (norm2 < 0.000001);

        double normInverted = 1.0/TMath::Sqrt(norm2);
        Photon->v[0] *= normInverted; Photon->v[1] *= normInverted; Photon->v[2] *= normInverted;
        Status = Transmission;
        return Forward;
    }

    // else absorption
    Status = Absorption;
    Photon->SimStat->OverrideWLSabs++;
    return Absorbed;
}

void AWaveshifterOverride::printConfiguration(int /*iWave*/)
{
    qDebug() << "-------Configuration:-------";
    qDebug() << "Surface wavelengthshifter override";
    QString str = "Reemission probabililty: ";
    str += (ReemissionProbability_lambda.isEmpty() ? "not defined" : "defined");
    qDebug() << str;
    str = "Emission spectrum: ";
    str += (EmissionSpectrum_lambda.isEmpty() ? "not defined" : "defined");
    qDebug() << str;
    qDebug() << "----------------------------";
}

QString AWaveshifterOverride::getReportLine()
{
    QString s = "to " + (*MatCollection)[MatTo]->name;
    s += "->SurfaceWLS";
    if (ReemissionProbability_lambda.isEmpty() ) s += "; Prob: not set!";
    if (EmissionSpectrum_lambda.isEmpty() ) s += "; Spectrum not set!";
    return s;
}

void AWaveshifterOverride::writeToJson(QJsonObject &json)
{
    AOpticalOverride::writeToJson(json);

    QJsonArray arRP;
    writeTwoQVectorsToJArray(ReemissionProbability_lambda, ReemissionProbability, arRP);
    json["ReemissionProbability"] = arRP;
    QJsonArray arEm;
    writeTwoQVectorsToJArray(EmissionSpectrum_lambda, EmissionSpectrum, arEm);
    json["EmissionSpectrum"] = arEm;
    json["ReemissionModel"] = ReemissionModel;
}

bool AWaveshifterOverride::readFromJson(QJsonObject &json)
{
    //QString type = json["Model"].toString();
    //if (type != getType()) return false; //file for wrong model!
    if (!AOpticalOverride::readFromJson(json)) return false;

    QJsonArray arRP = json["ReemissionProbability"].toArray();
    readTwoQVectorsFromJArray(arRP, ReemissionProbability_lambda, ReemissionProbability);
    QJsonArray arEm = json["EmissionSpectrum"].toArray();
    readTwoQVectorsFromJArray(arEm, EmissionSpectrum_lambda, EmissionSpectrum);

    ReemissionModel = 1;
    parseJson(json, "ReemissionModel", ReemissionModel);

    return true;
}

#ifdef GUI
QWidget *AWaveshifterOverride::getEditWidget(QWidget *caller, GraphWindowClass *GraphWindow)
{
    QFrame* f = new QFrame();
    f->setFrameStyle(QFrame::Box);

    QVBoxLayout* vl = new QVBoxLayout(f);
        QHBoxLayout* l = new QHBoxLayout();
            QLabel* lab = new QLabel("Reemission generation:");
        l->addWidget(lab);
            QComboBox* com = new QComboBox();
            com->addItem("Isotropic (4Pi)"); com->addItem("Lambertian, 2Pi back"); com->addItem("Lambertian, 2Pi forward");
            com->setCurrentIndex(ReemissionModel);
            QObject::connect(com, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), [this](int index) { this->ReemissionModel = index; } );
        l->addWidget(com);
    vl->addLayout(l);
        l = new QHBoxLayout();
            QVBoxLayout* vv = new QVBoxLayout();
                lab = new QLabel("Reemission probability:");
            vv->addWidget(lab);
                lab = new QLabel("Emission spectrum:");
            vv->addWidget(lab);
        l->addLayout(vv);
            vv = new QVBoxLayout();
                QPushButton* pb = new QPushButton("Load");
                pb->setToolTip("Every line of the file should contain 2 numbers:\nwavelength[nm] reemission_probability[0..1]");
                QObject::connect(pb, &QPushButton::clicked, [caller, this] {loadReemissionProbability(caller);});
            vv->addWidget(pb);
                pb = new QPushButton("Load");
                pb->setToolTip("Every line of the file should contain 2 numbers:\nwavelength[nm] relative_intencity[>=0]");
                QObject::connect(pb, &QPushButton::clicked, [caller, this] {loadEmissionSpectrum(caller);});
            vv->addWidget(pb);
        l->addLayout(vv);
            vv = new QVBoxLayout();
                pb = new QPushButton("Show");
                QObject::connect(pb, &QPushButton::clicked, [GraphWindow, caller, this] {showReemissionProbability(GraphWindow, caller);});
            vv->addWidget(pb);
                pb = new QPushButton("Show");
                QObject::connect(pb, &QPushButton::clicked, [GraphWindow, caller, this] {showEmissionSpectrum(GraphWindow, caller);});
            vv->addWidget(pb);
        l->addLayout(vv);
            vv = new QVBoxLayout();
                pb = new QPushButton("Binned");
                QObject::connect(pb, &QPushButton::clicked, [GraphWindow, caller, this] {showBinnedReemissionProbability(GraphWindow, caller);});
            vv->addWidget(pb);
                pb = new QPushButton("Binned");
                QObject::connect(pb, &QPushButton::clicked, [GraphWindow, caller, this] {showBinnedEmissionSpectrum(GraphWindow, caller);});
            vv->addWidget(pb);
        l->addLayout(vv);
    vl->addLayout(l);
        lab = new QLabel("If simulation is NOT wavelength-resolved, this override does nothing!");
        lab->setAlignment(Qt::AlignCenter);
    vl->addWidget(lab);

    return f;
}
#endif

#ifdef GUI
void AWaveshifterOverride::loadReemissionProbability(QWidget* caller)
{
    QString fileName = QFileDialog::getOpenFileName(caller, "Load reemission probability", "", "Data files (*.dat *.txt);;All files (*)");
    if (fileName.isEmpty()) return;
    //GlobSet->LastOpenDir = QFileInfo(fileName).absolutePath();
    QVector<double> X, Y;
    int ret = LoadDoubleVectorsFromFile(fileName, &X, &Y);
    if (ret == 0)
    {
        ReemissionProbability_lambda = X;
        ReemissionProbability = Y;
    }
}

void AWaveshifterOverride::loadEmissionSpectrum(QWidget *caller)
{
    QString fileName = QFileDialog::getOpenFileName(caller, "Load emission spectrum", "", "Data files (*.dat *.txt);;All files (*)");
    if (fileName.isEmpty()) return;
    //GlobSet->LastOpenDir = QFileInfo(fileName).absolutePath();
    QVector<double> X, Y;
    int ret = LoadDoubleVectorsFromFile(fileName, &X, &Y);
    if (ret == 0)
    {
        EmissionSpectrum_lambda = X;
        EmissionSpectrum = Y;
    }
}

void AWaveshifterOverride::showReemissionProbability(GraphWindowClass *GraphWindow, QWidget* caller)
{
    if (ReemissionProbability_lambda.isEmpty())
    {
        message("No data were loaded", caller);
        return;
    }
    TGraph* gr = GraphWindow->ConstructTGraph(ReemissionProbability_lambda, ReemissionProbability, "Reemission probability", "Wavelength, nm", "Reemission probability, a.u.", 2, 20, 1, 2, 2);
    gr->SetMinimum(0);
    GraphWindow->Draw(gr, "apl");
}

void AWaveshifterOverride::showEmissionSpectrum(GraphWindowClass *GraphWindow, QWidget *caller)
{
    if (EmissionSpectrum_lambda.isEmpty())
    {
        message("No data were loaded", caller);
        return;
    }
    TGraph* gr = GraphWindow->ConstructTGraph(EmissionSpectrum_lambda, EmissionSpectrum,
                                              "Emission spectrum", "Wavelength, nm", "Relative intensity, a.u.",
                                              4, 20, 1,
                                              4, 2);
    gr->SetMinimum(0);
    GraphWindow->Draw(gr, "apl");
}

void AWaveshifterOverride::showBinnedReemissionProbability(GraphWindowClass *GraphWindow, QWidget *caller)
{
    bool bWR;
    double WaveFrom, WaveTo, WaveStep;
    int WaveNodes;
    MatCollection->GetWave(bWR, WaveFrom, WaveTo, WaveStep, WaveNodes);
    initializeWaveResolved(bWR, WaveFrom, WaveStep, WaveNodes);

    //TODO run checker

    if (!bWR)
    {
        message("Simulation is NOT wavelength resolved, override is inactive!", caller);
        return;
    }

    QVector<double> waveIndex;
    for (int i=0; i<WaveNodes; i++) waveIndex << i;
    TGraph* gr = GraphWindow->ConstructTGraph(waveIndex, ReemissionProbabilityBinned,
                                              "Reemission probability (binned)", "Wave index", "Reemission probability, a.u.",
                                              2, 20, 1,
                                              2, 2);
    gr->SetMinimum(0);
    GraphWindow->Draw(gr, "apl");
}

void AWaveshifterOverride::showBinnedEmissionSpectrum(GraphWindowClass *GraphWindow, QWidget *caller)
{
    bool bWR;
    double WaveFrom, WaveTo, WaveStep;
    int WaveNodes;
    MatCollection->GetWave(bWR, WaveFrom, WaveTo, WaveStep, WaveNodes);
    initializeWaveResolved(bWR, WaveFrom, WaveStep, WaveNodes);

    //TODO run checker

    if (!bWR)
    {
        message("Simulation is NOT wavelength resolved, override is inactive!", caller);
        return;
    }

    double integral = Spectrum->ComputeIntegral();
    if (integral <= 0)
    {
        message("Binned emission spectrum: integral <=0, override will report an error!", caller);
        return;
    }

    TH1D* SpectrumCopy = new TH1D(*Spectrum);
    SpectrumCopy->SetTitle("Binned emission spectrum");
    SpectrumCopy->GetXaxis()->SetTitle("Wavelength, nm");
    SpectrumCopy->GetYaxis()->SetTitle("Relative intensity, a.u.");
    GraphWindow->Draw(SpectrumCopy, "hist"); //gets ownership of the copy
}
#endif

const QString AWaveshifterOverride::checkOverrideData()
{
    if (ReemissionProbability_lambda.isEmpty())
        return "Reemission probability not loaded";
    if (ReemissionProbability_lambda.size() != ReemissionProbability.size())
        return "Mismatch in reemission probability data";

    if (EmissionSpectrum_lambda.isEmpty())
        return "Emission spectrum not loaded";
    if (EmissionSpectrum_lambda.size() != EmissionSpectrum.size())
        return "Mismatch in emission spectrum data";

    bool bWR;
    double WaveFrom, WaveTo, WaveStep;
    int WaveNodes;
    MatCollection->GetWave(bWR, WaveFrom, WaveTo, WaveStep, WaveNodes);
    initializeWaveResolved(bWR, WaveFrom, WaveStep, WaveNodes);

    if (bWR && Spectrum->ComputeIntegral() <= 0)
            return "Binned emission spectrum: integral should be > 0";
    return "";
}

// ---------------- SpectralSimplistic -------------------
SpectralBasicOpticalOverride::SpectralBasicOpticalOverride(AMaterialParticleCollection *MatCollection, int MatFrom, int MatTo)
    : BasicOpticalOverride(MatCollection, MatFrom, MatTo, 0,0,0, 1), effectiveWavelength(500)
{
    Wave << 500;
    ProbLoss << 0;
    ProbRef << 0;
    ProbDiff << 0;
}

SpectralBasicOpticalOverride::SpectralBasicOpticalOverride(AMaterialParticleCollection *MatCollection, int MatFrom, int MatTo, int ScatterModel, double EffWave)
    : BasicOpticalOverride(MatCollection, MatFrom, MatTo, 0,0,0, ScatterModel), effectiveWavelength(EffWave)
{
    Wave << 500;
    ProbLoss << 0;
    ProbRef << 0;
    ProbDiff << 0;
}

AOpticalOverride::OpticalOverrideResultEnum SpectralBasicOpticalOverride::calculate(TRandom2 *RandGen, APhoton *Photon, const double *NormalVector)
{
    int waveIndex = Photon->waveIndex;
    if (waveIndex == -1) waveIndex = effectiveWaveIndex;

    probLoss = ProbLossBinned.at(waveIndex);
    probDiff = ProbDiffBinned.at(waveIndex);
    probRef  = ProbRefBinned.at(waveIndex);

    return BasicOpticalOverride::calculate(RandGen, Photon, NormalVector);
}

void SpectralBasicOpticalOverride::printConfiguration(int /*iWave*/)
{
    qDebug() << "-------Configuration:-------";
    qDebug() << "Wavelength:"<<Wave;
    qDebug() << "Absorption fraction:"<<ProbLoss;
    qDebug() << "Specular fraction:"<<ProbRef;
    qDebug() << "Scatter fraction:"<<ProbDiff;
    qDebug() << "Scatter model (4Pi/LambBack/LambForward):"<<scatterModel;
    qDebug() << "----------------------------";
}

QString SpectralBasicOpticalOverride::getReportLine()
{
    QString s = "to " + (*MatCollection)[MatTo]->name;
    s += "->";

    if (Wave.isEmpty()) return s + " To be defined"; //not defined - shown during configuration phase only

    s += " Spectral data with " + QString::number(Wave.size()) + " points";
    return s;
}

void SpectralBasicOpticalOverride::writeToJson(QJsonObject &json)
{
    AOpticalOverride::writeToJson(json);

    json["ScatMode"] = scatterModel;
    json["EffWavelength"] = effectiveWavelength;

    if (Wave.size() != ProbLoss.size() || Wave.size() != ProbRef.size() || Wave.size() != ProbDiff.size())
    {
        qWarning() << "Mismatch in data size for SpectralBasicOverride! skipping data!";
        return;
    }
    QJsonArray sp;
    for (int i=0; i<Wave.size(); i++)
    {
        QJsonArray ar;
        ar << Wave.at(i) << ProbLoss.at(i) << ProbRef.at(i) << ProbDiff.at(i);
        sp << ar;
    }
    json["Data"] = sp;
}

bool SpectralBasicOpticalOverride::readFromJson(QJsonObject &json)
{
    if (!AOpticalOverride::readFromJson(json)) return false;

    parseJson(json, "ScatMode", scatterModel);
    parseJson(json, "EffWavelength", effectiveWavelength);

    //after constructor vectors are not empty!
    Wave.clear();
    ProbLoss.clear();
    ProbRef.clear();
    ProbDiff.clear();

    QJsonArray sp;
    parseJson(json, "Data", sp);
    for (int i=0; i<sp.size(); i++)
    {
        QJsonArray ar = sp.at(i).toArray();
        Wave     << ar.at(0).toDouble();
        ProbLoss << ar.at(1).toDouble();
        ProbRef  << ar.at(2).toDouble();
        ProbDiff << ar.at(3).toDouble();
    }

    return true;
}

void SpectralBasicOpticalOverride::initializeWaveResolved(bool bWaveResolved, double waveFrom, double waveStep, int waveNodes)
{
    if (bWaveResolved)
    {
        ConvertToStandardWavelengthes(&Wave, &ProbLoss, waveFrom, waveStep, waveNodes, &ProbLossBinned);
        ConvertToStandardWavelengthes(&Wave, &ProbRef, waveFrom, waveStep, waveNodes, &ProbRefBinned);
        ConvertToStandardWavelengthes(&Wave, &ProbDiff, waveFrom, waveStep, waveNodes, &ProbDiffBinned);

        effectiveWaveIndex = (effectiveWavelength - waveFrom) / waveStep;
        if (effectiveWaveIndex < 0) effectiveWaveIndex = 0;
        else if (effectiveWaveIndex >= waveNodes ) effectiveWaveIndex = waveNodes - 1;
        //qDebug() << "Eff wave"<<effectiveWavelength << "assigned index:"<<effectiveWaveIndex;
    }
    else
    {
        int isize = Wave.size();
        int i = 0;
        if (isize != 1)  //esle use i = 0
        {
            for (; i < isize; i++)
                if (Wave.at(i) > effectiveWavelength) break;

            //closest is i-1 or i
            if (i != 0)
                if ( fabs(Wave.at(i-1) - effectiveWavelength) < fabs(Wave.at(i) - effectiveWavelength) ) i--;
        }

        //qDebug() << "Selected i = "<< i << "with wave"<<Wave.at(i) << Wave;
        effectiveWaveIndex = 0;
        ProbLossBinned = QVector<double>(1, ProbLoss.at(i));
        ProbRefBinned = QVector<double>(1, ProbRef.at(i));
        ProbDiffBinned = QVector<double>(1, ProbDiff.at(i));
        //qDebug() << "LossRefDiff"<<ProbLossBinned.at(effectiveWaveIndex)<<ProbRefBinned.at(effectiveWaveIndex)<<ProbDiffBinned.at(effectiveWaveIndex);
    }
}

const QString SpectralBasicOpticalOverride::loadData(const QString &fileName)
{
    QVector< QVector<double>* > vec;
    vec << &Wave << &ProbLoss << &ProbRef << &ProbDiff;
    QString err = LoadDoubleVectorsFromFile(fileName, vec);
    if (!err.isEmpty()) return err;

    for (int i=0; i<Wave.size(); i++)
    {
        double sum = ProbLoss.at(i) + ProbRef.at(i) + ProbDiff.at(i);
        if (sum > 1.0) return QString("Sum of probabilities is larger than 1.0 for wavelength of ") + QString::number(Wave.at(i)) + " nm";
    }

    if (Wave.isEmpty())
        return "No data were read from the file!";

    return "";
}

#ifdef GUI
void SpectralBasicOpticalOverride::loadSpectralData(QWidget* caller)
{
    QString fileName = QFileDialog::getOpenFileName(caller, "Load spectral data (Wavelength, Absorption, Reflection, Scattering)", "", "Data files (*.dat *.txt);;All files (*)");
    if (fileName.isEmpty()) return;

    QVector< QVector<double>* > vec;
    vec << &Wave << &ProbLoss << &ProbRef << &ProbDiff;
    QString err = LoadDoubleVectorsFromFile(fileName, vec);
    if (!err.isEmpty()) message(err, caller);
}

#include "TMultiGraph.h"
#include "TGraph.h"
#include "graphwindowclass.h"
void SpectralBasicOpticalOverride::showLoaded(GraphWindowClass* GraphWindow)
{
    QVector<double> Fr;
    for (int i=0; i<Wave.size(); i++)
        Fr << (1.0 - ProbLoss.at(i) - ProbRef.at(i) - ProbDiff.at(i));

    TMultiGraph* mg = new TMultiGraph();
    TGraph* gLoss = GraphWindow->ConstructTGraph(Wave, ProbLoss, "Absorption", "Wavelength, nm", "", 2, 20, 1, 2);
    mg->Add(gLoss, "LP");
    TGraph* gRef = GraphWindow->ConstructTGraph(Wave, ProbRef, "Specular reflection", "Wavelength, nm", "", 4, 21, 1, 4);
    mg->Add(gRef, "LP");
    TGraph* gDiff = GraphWindow->ConstructTGraph(Wave, ProbDiff, "Diffuse scattering", "Wavelength, nm", "", 7, 22, 1, 7);
    mg->Add(gDiff, "LP");
    TGraph* gFr = GraphWindow->ConstructTGraph(Wave, Fr, "Fresnel", "Wavelength, nm", "", 1, 24, 1, 1, 1, 1);
    mg->Add(gFr, "LP");

    mg->SetMinimum(0);
    GraphWindow->Draw(mg, "apl");
    mg->GetXaxis()->SetTitle("Wavelength, nm");
    mg->GetYaxis()->SetTitle("Probability");
    GraphWindow->AddLegend(0.7,0.8, 0.95,0.95, "");
}

void SpectralBasicOpticalOverride::showBinned(QWidget *widget, GraphWindowClass *GraphWindow)
{
    bool bWR;
    double WaveFrom, WaveTo, WaveStep;
    int WaveNodes;
    MatCollection->GetWave(bWR, WaveFrom, WaveTo, WaveStep, WaveNodes);
    initializeWaveResolved(bWR, WaveFrom, WaveStep, WaveNodes);

    //TODO run checker

    if (!bWR)
    {
        QString s = "Simulation is configured as not wavelength-resolved\n";
        s +=        "All photons will have the same properties:\n";
        s +=QString("Absorption: %1").arg(probLoss) + "\n";
        s +=QString("Specular reflection: %1").arg(probRef) + "\n";
        s +=QString("Scattering: %1").arg(probDiff);
        message(s, widget);
        return;
    }

    QVector<double> waveIndex;
    for (int i=0; i<WaveNodes; i++) waveIndex << i;

    QVector<double> Fr;
    for (int i=0; i<waveIndex.size(); i++)
        Fr << (1.0 - ProbLossBinned.at(i) - ProbRefBinned.at(i) - ProbDiffBinned.at(i));

    TMultiGraph* mg = new TMultiGraph();
    TGraph* gLoss = GraphWindow->ConstructTGraph(waveIndex, ProbLossBinned, "Loss", "Wave index", "Loss", 2, 20, 1, 2);
    mg->Add(gLoss, "LP");
    TGraph* gRef = GraphWindow->ConstructTGraph(waveIndex, ProbRefBinned, "Specular reflection", "Wave index", "Reflection", 4, 21, 1, 4);
    mg->Add(gRef, "LP");
    TGraph* gDiff = GraphWindow->ConstructTGraph(waveIndex, ProbDiffBinned, "Diffuse scattering", "Wave index", "Scatter", 7, 22, 1, 7);
    mg->Add(gDiff, "LP");
    TGraph* gFr = GraphWindow->ConstructTGraph(waveIndex, Fr, "Fresnel", "Wave index", "", 1, 24, 1, 1, 1, 1);
    mg->Add(gFr, "LP");

    mg->SetMinimum(0);
    GraphWindow->Draw(mg, "apl");
    mg->GetXaxis()->SetTitle("Wave index");
    mg->GetYaxis()->SetTitle("Probability");
    GraphWindow->AddLegend(0.7,0.8, 0.95,0.95, "");
}
#endif

#ifdef GUI
QWidget *SpectralBasicOpticalOverride::getEditWidget(QWidget *caller, GraphWindowClass *GraphWindow)
{
    QFrame* f = new QFrame();
    f->setFrameStyle(QFrame::Box);

    QVBoxLayout* vl = new QVBoxLayout(f);
        QHBoxLayout* l = new QHBoxLayout();
            QLabel* lab = new QLabel("Absorption, reflection and scattering:");
        l->addWidget(lab);
            QPushButton* pb = new QPushButton("Load");
            pb->setToolTip("Every line of the file should contain 4 numbers:\nwavelength[nm] absorption_prob[0..1] reflection_prob[0..1] scattering_prob[0..1]");
            QObject::connect(pb, &QPushButton::clicked, [caller, this] {loadSpectralData(caller);});
        l->addWidget(pb);
            pb = new QPushButton("Show");
            QObject::connect(pb, &QPushButton::clicked, [GraphWindow, this] {showLoaded(GraphWindow);});
        l->addWidget(pb);
            pb = new QPushButton("Binned");
            QObject::connect(pb, &QPushButton::clicked, [caller, GraphWindow, this] {showBinned(caller, GraphWindow);});
        l->addWidget(pb);
    vl->addLayout(l);
        l = new QHBoxLayout();
            lab = new QLabel("Scattering model:");
        l->addWidget(lab);
            QComboBox* com = new QComboBox();
            com->addItem("Isotropic (4Pi)"); com->addItem("Lambertian, 2Pi back"); com->addItem("Lambertian, 2Pi forward");
            com->setCurrentIndex(scatterModel);
            QObject::connect(com, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), [this](int index) { this->scatterModel = index; } );
        l->addWidget(com);
    vl->addLayout(l);
        l = new QHBoxLayout();
            lab = new QLabel("For photons with WaveIndex=-1, assume wavelength of:");
        l->addWidget(lab);
            QLineEdit* le = new QLineEdit(QString::number(effectiveWavelength));
            QDoubleValidator* val = new QDoubleValidator(f);
            val->setNotation(QDoubleValidator::StandardNotation);
            val->setBottom(0);
            val->setDecimals(6);
            le->setValidator(val);
            QObject::connect(le, &QLineEdit::editingFinished, [le, this]() { this->effectiveWavelength = le->text().toDouble(); } );
        l->addWidget(le);
            lab = new QLabel("nm");
        l->addWidget(lab);
    vl->addLayout(l);

    return f;
}
#endif

const QString SpectralBasicOpticalOverride::checkOverrideData()
{
    //checking spectrum
    if (Wave.size() == 0) return "Spectral data are not defined";
    if (Wave.size() != ProbLoss.size() || Wave.size() != ProbRef.size() || Wave.size() != ProbDiff.size()) return "Spectral data do not match in size";
    for (int i=0; i<Wave.size(); i++)
    {
        if (Wave.at(i) < 0) return "negative wavelength are not allowed";
        if (ProbLoss.at(i) < 0 || ProbLoss.at(i) > 1.0) return "absorption probability has to be in the range of [0, 1.0]";
        if (ProbDiff.at(i) < 0 || ProbDiff.at(i) > 1.0) return "scattering probability has to be in the range of [0, 1.0]";
        if (ProbRef.at(i) < 0 || ProbRef.at(i) > 1.0) return "scattering probability has to be in the range of [0, 1.0]";
        double sum = ProbLoss.at(i) + ProbRef.at(i) + ProbDiff.at(i);
        if (sum > 1.0) return QString("Sum of probabilities is larger than 1.0 for wavelength of %1 nm").arg(Wave.at(i));
    }
    if (scatterModel < 0 || scatterModel > 2) return "unknown scattering model";

    //TODO check effective wavelength
    //TODO check binned

    return "";
}
