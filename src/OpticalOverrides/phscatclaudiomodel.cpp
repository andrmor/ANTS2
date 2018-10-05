#include "phscatclaudiomodel.h"
#include "amaterialparticlecolection.h"
#include "aphoton.h"
#include "asimulationstatistics.h"
#include "atracerstateful.h"
#include "ajsontools.h"

#include <QDebug>
#include <QVector>
#include <QJsonObject>

#include "TRandom2.h"
#include "TVector3.h"

#ifdef GUI
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QFrame>
#include <QDoubleValidator>
#include <QComboBox>
#endif

#define MODEL_VERSION 3

const QString PhScatClaudioModel::getReportLine() const
{
    QString s;
    s += QString("sA %1 / ").arg(sigma_alpha);
    s += QString("sH %1 / ").arg(sigma_h);
    s += QString("Alb %1 / ").arg(albedo);
    s += "HD ";
    switch (HeightDistribution)
    {
    case empirical : s += "em"; break;
    case gaussian : s += "gau"; break;
    case exponential : s += "exp"; break;
    }
    s += " / SD ";
    switch (SlopeDistribution)
    {
    case trowbridgereitz : s += "tr"; break;
    case cooktorrance : s += "cook"; break;
    case bivariatecauchy : s += "biv"; break;
    }
    return s;
}

const QString PhScatClaudioModel::getLongReportLine() const
{
        QString s = "--> Caludio's model v2.2 <--\n";
//        s += "Refractive index of metal:\n";
//        s += QString("  real: %1\n").arg(RealN);
//        s += QString("  imaginary: %1").arg(ImaginaryN);
        return s;
}

void PhScatClaudioModel::writeToJson(QJsonObject &json) const
{
  AOpticalOverride::writeToJson(json);

  json["SigmaAlpha"] = sigma_alpha;
  json["SigmaH"] = sigma_h;
  json["Albedo"] = albedo;
  json["HDmodel"] = HeightDistribution;
  json["SDmodel"] = SlopeDistribution;  
}

bool PhScatClaudioModel::readFromJson(const QJsonObject &json)
{
    if ( !parseJson(json, "SigmaAlpha", sigma_alpha)) return false;
    if ( !parseJson(json, "SigmaH", sigma_h)) return false;
    if ( !parseJson(json, "Albedo", albedo)) return false;

    int ival;
    if ( !parseJson(json, "HDmodel", ival)) return false;
    if (ival<0 || ival>2) return false;
    HeightDistribution = static_cast<HeightDistrEnum>(ival);

    if ( !parseJson(json, "SDmodel", ival)) return false;
    if (ival<0 || ival>2) return false;
    SlopeDistribution = static_cast<SlopeDistrEnum>(ival);

    return true;
}

#ifdef GUI
QWidget *PhScatClaudioModel::getEditWidget(QWidget *, GraphWindowClass *)
{
    QFrame* f = new QFrame();
    f->setFrameStyle(QFrame::Box);

    QHBoxLayout* hl = new QHBoxLayout(f);
        QVBoxLayout* l = new QVBoxLayout();
            QLabel* lab = new QLabel("Sigma alpha:");
        l->addWidget(lab);
            lab = new QLabel("Sigma spike:");
        l->addWidget(lab);
            lab = new QLabel("Albedo:");
        l->addWidget(lab);
            lab = new QLabel("Hight distribution:");
        l->addWidget(lab);
            lab = new QLabel("Slope distribution:");
        l->addWidget(lab);
    hl->addLayout(l);
        l = new QVBoxLayout();
            QLineEdit* le = new QLineEdit(QString::number(sigma_alpha));
            QDoubleValidator* val = new QDoubleValidator(f);
            val->setNotation(QDoubleValidator::StandardNotation);
            val->setBottom(0);
            val->setDecimals(6);
            le->setValidator(val);
            QObject::connect(le, &QLineEdit::editingFinished, [le, this]() { this->sigma_alpha = le->text().toDouble(); } );
        l->addWidget(le);
            le = new QLineEdit(QString::number(sigma_h));
            le->setValidator(val);
            QObject::connect(le, &QLineEdit::editingFinished, [le, this]() { this->sigma_h = le->text().toDouble(); } );
        l->addWidget(le);
            le = new QLineEdit(QString::number(albedo));
            le->setValidator(val);
            QObject::connect(le, &QLineEdit::editingFinished, [le, this]() { this->albedo = le->text().toDouble(); } );
        l->addWidget(le);
            QComboBox* com = new QComboBox();
            com->addItems(QStringList({"empirical", "gaussian", "exponential"}));
            com->setCurrentIndex(HeightDistribution);
            QObject::connect(com, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), [this](int index) { this->HeightDistribution = static_cast<HeightDistrEnum>(index); } );
        l->addWidget(com);
            com = new QComboBox();
            com->addItems(QStringList({"trowbridgereitz", "cooktorrance", "bivariatecauchy"}));
            com->setCurrentIndex(SlopeDistribution);
            QObject::connect(com, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), [this](int index) { this->SlopeDistribution = static_cast<SlopeDistrEnum>(index); } );
        l->addWidget(com);
    hl->addLayout(l);

    return f;
}
#endif

const QString PhScatClaudioModel::checkOverrideData()
{
    if (sigma_alpha < 0) return "sigma alpha should be >= 0";
    if (sigma_h < 0) return "sigma h should be >= 0";
    if (albedo < 0 || albedo > 1.0) return "albedo should be within [0, 1.0] range";
    if (HeightDistribution < 0 || HeightDistribution > 2) return "hight distribution model can be 0, 1 or 2";
    if (SlopeDistribution < 0 || SlopeDistribution > 2) return "slope distribution model can be 0, 1 or 2";
    return "";
}

double PhScatClaudioModel::SpikeIntensity(int iWave, double costi)
{
  if (HeightDistribution == empirical)  // for the empirical function observed by C. Silva
    return exp(-sigma_h*costi);

  double wavelength = 1.0e-9 * MatCollection->convertWaveIndexToWavelength(iWave);
  double wave_number = 2.0*3.1415926535/wavelength;
  double optical_roughness = wave_number * wave_number * costi * costi * sigma_h * sigma_h;

  if (HeightDistribution == gaussian)
    return exp(-optical_roughness);
  if (HeightDistribution == exponential)
    return 1.0 / (  pow((1.0+pow(optical_roughness,2)/4.0), 2)  ); //Confirmar melhor
  return 0;
}

double PhScatClaudioModel::Fresnel(double E1_perp, double E1_parl, double cosinc, double Rinda1, double Rinda2)
{
  double E2_perp, E2_parl;
  double sin2, cos2;

  if (fabs(cosinc) < 1.0 - 1.0e-10)
       sin2 = sqrt(1.0 - cosinc*cosinc)*Rinda1/Rinda2;     // *** Snell's Law ***
  else sin2 = 0.0;

  if(sin2 < 1.0)
    {
      cos2 =  sqrt(1.0 - sin2*sin2);
      E2_perp = E1_perp * (Rinda1*cosinc-Rinda2*cos2)/(Rinda1*cosinc+Rinda2*cos2);
      E2_parl = E1_parl * (Rinda2*cosinc-Rinda1*cos2)/(Rinda2*cosinc+Rinda1*cos2);

      return E2_perp*E2_perp + E2_parl*E2_parl;
    }
  return 1.0;
}

// ============= Model dependent methods ===========

// ------- newer model  - V2
double PhScatClaudioModelV2::SlopeAngle(double random_num)
{
  if (SlopeDistribution == trowbridgereitz)
    return atan(sigma_alpha*sqrt(random_num)/(sqrt(1.0-random_num)));
  if (SlopeDistribution == cooktorrance)
    return atan(sqrt(-sigma_alpha*sigma_alpha*log(1.0-random_num)));
  if (SlopeDistribution == bivariatecauchy)
    return atan(sigma_alpha*sqrt(random_num*(2.0-random_num))/(1.0-random_num));
  return 0;
}

double PhScatClaudioModelV2::GnFunc(double cost)
{
  if(cost>0)
    return 2.0/(1.0+sqrt(1.0+sigma_alpha*sigma_alpha*(1.0-cost*cost)/(cost*cost)));
  else
    return 0;
}

AOpticalOverride::OpticalOverrideResultEnum PhScatClaudioModelV2::calculate(ATracerStateful &Resources, APhoton *Photon, const double* NormalVector)
{
  TVector3 K(Photon->v);                // photon direction
  //qDebug() << "Photon direction (i,j,k):"<<K.x()<<K.y()<<K.z();
  //qDebug() << "iWave:"<<iWave;

  //ANTS2 SPECIFIC: navigator gives normal in the direction of the photon, while Claudio's model assumes the opposite direction
  TVector3 GlobalNormal(-NormalVector[0], -NormalVector[1], -NormalVector[2]);  // normal to the surface
  //qDebug() << "Surface normal (i,j,k):"<<GlobalNormal.x()<<GlobalNormal.y()<<GlobalNormal.z();

  //refractive indexes of materials before and after the interface
  double Rindex1 = (*MatCollection)[MatFrom]->getRefractiveIndex(Photon->waveIndex);
  double Rindex2 = (*MatCollection)[MatTo]->getRefractiveIndex(Photon->waveIndex);
  //qDebug() << "Refractive indices from and to:" << Rindex1 << Rindex2;

  double costi = - GlobalNormal * K;    // global angle of incidence
  //qDebug() << "costi:"<<costi;

  double gni = (sigma_alpha > 0) ? GnFunc(costi) : 1.0; // shadowing fuction
//  qDebug() << "gni:"<<gni;
  double gnr;   // one more shadowing function

  double lambda = SpikeIntensity(Photon->waveIndex, costi); // relative intensity of the specular spike
  //qDebug() << "lambda:"<<lambda;

  TVector3 ScatNormal = GlobalNormal;
  double costl; // local angle of incidence
  double costr; // reflection angle
  double weight_microfacet = 1.5;

  // If the random number is larger than lambda the light will be scattered
  //according to the local normal. If the sigma_alpha is zero there is no specular lobe    
  if (Resources.RandGen->Rndm() < lambda || sigma_alpha == 0)
    {
      Status = SpikeReflection;
      costl = costi;
      costr = costi;     
      weight_microfacet = gni;
      gnr = gni;
    }
  else
    {
      int counter = 1000;
      do
        {
          counter--;
          if (counter == 0)
            {
              Status = Error;
              return _Error_;
            }

          Status = LobeReflection;
          double alpha_WRM = SlopeAngle(Resources.RandGen->Rndm()); // Sampling of the slope angle
          double phi_alpha_WRM = 2.0 * 3.1415926535 * Resources.RandGen->Rndm();

          // Set the Components of the Global Normal
          double CosAlpha_WRM = cos(alpha_WRM);
          double SinAlpha_WRM = sin(alpha_WRM);

          // Calculating the local angle
          ScatNormal.SetX(SinAlpha_WRM * (cos(phi_alpha_WRM)));
          ScatNormal.SetY(SinAlpha_WRM * (sin(phi_alpha_WRM)));
          ScatNormal.SetZ(CosAlpha_WRM);
          ScatNormal.RotateUz(GlobalNormal);

          costl = - K * ScatNormal;

          // Calculating the reflection angle
          costr = 2.0 * costl*CosAlpha_WRM - costi; //angle between the global normal and the viewing direction
          gnr  = GnFunc(costr);

          //if(costl < 0 && costr < 0)  //// Andr: dif!
          if (costl < 0)
            weight_microfacet = 0;
          else
            weight_microfacet = gni * costl / (costi*CosAlpha_WRM);

          // qDebug() << "new weight microfacet:"<< weight_microfacet;
        }
      while ( Resources.RandGen->Rndm()*1.5 > weight_microfacet );
    }


  //In ANTS2 there is no photon polarisation -> assuming 50% / 50%
    //The polarization of the incident photon
    //TVector3 A_trans = OldMomentum.cross(theScatNormal); //Measuring the Polarization
    //A_trans = A_trans.Unit();
  double E1_perp = sqrt(2.0)/2.0;//OldPolarization * A_trans;
    //TVector3 E1pp    = E1_perp * A_trans;
    //TVector3 E1pl    = OldPolarization - E1pp;
  double E1_parl = E1_perp; //E1pl.mag();

  double Amp_perp = Fresnel(E1_perp, 0, costl, Rindex1, Rindex2);
  double Amp_parl = Fresnel(0, E1_parl, costl, Rindex1, Rindex2);
  double Amp_tot = (Amp_perp+Amp_parl)*gnr;
  // qDebug() << "Amp_total"<<Amp_tot<<"gnr"<<gnr<<"perp"<<Amp_perp<<"par"<<Amp_parl;

      //TVector3 A_paral;
  if( Resources.RandGen->Rndm() < Amp_tot)
    {
      //Computation of the new momentum and the new polarization for the reflected photon (specular lobe or specular spike)
      K = 2.0 * costl * ScatNormal + K;
      // qDebug() << "K:"<<K[0]<<K[1]<<K[2];
      // qDebug() << "Kout"<<Kout[0]<<Kout[1]<<Kout[2];
      // qDebug() << "ScatNormal"<<ScatNormal[0]<<ScatNormal[1]<<ScatNormal[2]<<"costl"<<costl;

        //A_paral = NewMomentum.cross(A_trans);
        //A_paral   = A_paral.unit();
        //G4double C_perp = std::sqrt(Amp_perp/Amp_tot);
        //G4double C_parl = std::sqrt(Amp_parl/Amp_tot);
        //NewPolarization = C_parl*A_paral + C_perp*A_trans;
      Photon->SimStat->OverrideClaudioSpec++;
    }
  else
    {
      double gnd;
      double Fres_Term = 0;
      do
        {
          if ( Resources.RandGen->Rndm() > albedo)
            {
              Status = Absorption;
              Photon->SimStat->OverrideClaudioAbs++;
              return Absorbed;
            }

          Status = LambertianReflection;  //The computation of the diffuse direction direction

          Fres_Term = 0; gnd = 0;

          double sintdsqquared = Resources.RandGen->Rndm();
          double sintd = sqrt(sintdsqquared);
          double costd = sqrt(1.0 - sintdsqquared);
          double phid = 2.0 * 3.1415926535 * Resources.RandGen->Rndm();

          double sintinter = Rindex1*sintd/Rindex2;
          double costiter = 0;

          if (sintinter < 1.0)
            {
              costiter = sqrt(1.0 - sintinter*sintinter);
              Fres_Term  = 1.0 - Fresnel(sqrt(2.0)/2.0, sqrt(2.0)/2.0, costiter, Rindex2, Rindex1);

              K.SetX(sintd*(cos(phid)));
              K.SetY(sintd*(sin(phid)));
              K.SetZ(costd);
              K.RotateUz(ScatNormal);
              gnd = GnFunc( GlobalNormal * K );
            }
        }
      while(sigma_alpha>0 && Resources.RandGen->Rndm() > gnd*Fres_Term);

//      //The diffuse lobe will be randomly polarized
//      A_trans = NewMomentum.cross(theScatNormal);
//      A_trans = A_trans.unit();
//      A_paral = NewMomentum.cross(A_trans);
//      A_paral = A_paral.unit();
//      G4double ThetaPolar = twopi*G4UniformRand();
//      NewPolarization = cos(ThetaPolar)*A_paral + sin(ThetaPolar)*A_trans;
      Photon->SimStat->OverrideClaudioLamb++;
    }

  Photon->v[0] = K.X();
  Photon->v[1] = K.Y();
  Photon->v[2] = K.Z();
  return Back;
}

const QString PhScatClaudioModelV2::getReportLine() const
{
  return " v2";
}

AOpticalOverride::OpticalOverrideResultEnum PhScatClaudioModelV2d2::calculate(ATracerStateful &Resources, APhoton *Photon, const double *NormalVector)
{
  TVector3 K(Photon->v);                // photon direction
  //qDebug() << "Photon direction (i,j,k):"<<K.x()<<K.y()<<K.z();
  //qDebug() << "iWave:"<<iWave;

  //ANTS2 SPECIFIC: navigator gives normal in the direction of the photon, while Claudio's model assumes the opposite direction
  TVector3 GlobalNormal(-NormalVector[0], -NormalVector[1], -NormalVector[2]);  // normal to the surface
  //qDebug() << "Surface normal (i,j,k):"<<GlobalNormal.x()<<GlobalNormal.y()<<GlobalNormal.z();

  //refractive indexes of materials before and after the interface
  double Rindex1 = (*MatCollection)[MatFrom]->getRefractiveIndex(Photon->waveIndex);
  double Rindex2 = (*MatCollection)[MatTo]->getRefractiveIndex(Photon->waveIndex);
  //qDebug() << "Refractive indices from and to:" << Rindex1 << Rindex2;

  double costi = - GlobalNormal * K;    // global angle of incidence
  //qDebug() << "costi:"<<costi;

  double gni = (sigma_alpha > 0) ? GnFunc(costi) : 1.0; // shadowing fuction
//  qDebug() << "gni:"<<gni;
  double gnr;   // one more shadowing function

  double lambda = SpikeIntensity(Photon->waveIndex, costi); // relative intensity of the specular spike
  //qDebug() << "lambda:"<<lambda;

  TVector3 ScatNormal = GlobalNormal;
  double costl; // local angle of incidence
  double costr; // reflection angle
  double weight_microfacet = 1.5;

  // If the random number is larger than lambda the light will be scattered
  //according to the local normal. If the sigma_alpha is zero there is no specular lobe
  if (Resources.RandGen->Rndm() < lambda || sigma_alpha == 0)
    {
      Status = SpikeReflection;
      costl = costi;
      costr = costi;
      weight_microfacet = gni;
      gnr = gni;
    }
  else
    {
      int counter = 1000;
      do
        {
          counter--;
          if (counter == 0)
            {
              Status = Error;
              return _Error_;
            }

          Status = LobeReflection;
          double alpha_WRM = SlopeAngle(Resources.RandGen->Rndm()); // Sampling of the slope angle
          double phi_alpha_WRM = 2.0 * 3.1415926535 * Resources.RandGen->Rndm();

          // Set the Components of the Global Normal
          double CosAlpha_WRM = cos(alpha_WRM);
          double SinAlpha_WRM = sin(alpha_WRM);

          // Calculating the local angle
          ScatNormal.SetX(SinAlpha_WRM * (cos(phi_alpha_WRM)));
          ScatNormal.SetY(SinAlpha_WRM * (sin(phi_alpha_WRM)));
          ScatNormal.SetZ(CosAlpha_WRM);
          ScatNormal.RotateUz(GlobalNormal);

          costl = - K * ScatNormal;

          // Calculating the reflection angle
          costr = 2.0 * costl*CosAlpha_WRM - costi; //angle between the global normal and the viewing direction
          gnr  = GnFunc(costr);

          //if(costl < 0 && costr < 0)  //// Andr: dif!
          if (costl < 0)
            weight_microfacet = 0;
          else
            weight_microfacet = gni * costl / (costi*CosAlpha_WRM);

          // qDebug() << "new weight microfacet:"<< weight_microfacet;
        }
      while ( Resources.RandGen->Rndm()*1.5 > weight_microfacet );
    }


  //In ANTS2 there is no photon polarisation -> assuming 50% / 50%
    //The polarization of the incident photon
    //TVector3 A_trans = OldMomentum.cross(theScatNormal); //Measuring the Polarization
    //A_trans = A_trans.Unit();
  double E1_perp = sqrt(2.0)/2.0;//OldPolarization * A_trans;
    //TVector3 E1pp    = E1_perp * A_trans;
    //TVector3 E1pl    = OldPolarization - E1pp;
  double E1_parl = E1_perp; //E1pl.mag();

  double Amp_perp = Fresnel(E1_perp, 0, costl, Rindex1, Rindex2);
  double Amp_parl = Fresnel(0, E1_parl, costl, Rindex1, Rindex2);
  double Amp_tot = (Amp_perp+Amp_parl)*gnr;
  // qDebug() << "Amp_total"<<Amp_tot<<"gnr"<<gnr<<"perp"<<Amp_perp<<"par"<<Amp_parl;

      //TVector3 A_paral;
  if( Resources.RandGen->Rndm() < Amp_tot)
    {
      //Computation of the new momentum and the new polarization for the reflected photon (specular lobe or specular spike)
      K = 2.0 * costl * ScatNormal + K;
      // qDebug() << "K:"<<K[0]<<K[1]<<K[2];
      // qDebug() << "Kout"<<Kout[0]<<Kout[1]<<Kout[2];
      // qDebug() << "ScatNormal"<<ScatNormal[0]<<ScatNormal[1]<<ScatNormal[2]<<"costl"<<costl;

        //A_paral = NewMomentum.cross(A_trans);
        //A_paral   = A_paral.unit();
        //G4double C_perp = std::sqrt(Amp_perp/Amp_tot);
        //G4double C_parl = std::sqrt(Amp_parl/Amp_tot);
        //NewPolarization = C_parl*A_paral + C_perp*A_trans;
      Photon->SimStat->OverrideClaudioSpec++;
    }
  else
    {
      double gnd;
      double Fres_Term = 0;
      do
        {
          if ( Resources.RandGen->Rndm() > albedo)
            {
              Status = Absorption;
              Photon->SimStat->OverrideClaudioAbs++;
              return Absorbed;
            }

          Status = LambertianReflection;  //The computation of the diffuse direction direction

          Fres_Term = 0; gnd = 0;

          double sintdsqquared = Resources.RandGen->Rndm();
          double sintd = sqrt(sintdsqquared);
          double costd = sqrt(1.0 - sintdsqquared);
          double phid = 2.0 * 3.1415926535 * Resources.RandGen->Rndm();

          double sintinter = Rindex2*sintd/Rindex1;  //DIF from V2
          double costiter = 0;

          Fres_Term  = Fresnel(sqrt(2.0)/2.0, sqrt(2.0)/2.0, costd, Rindex2, Rindex1);  //DIF from V2

          if (sintinter < 1.0)
            {
              costiter = sqrt(1.0 - sintinter*sintinter);

              K.SetX(sintinter*(cos(phid)));  //DIF from V2
              K.SetY(sintinter*(sin(phid)));  //DIF from V2
              K.SetZ(costiter);  //DIF from V2
              K.RotateUz(ScatNormal);  //DIF from V2
              gnd = GnFunc( GlobalNormal * K ); //new fix for diff phots leak
            }
        }
      while(sigma_alpha>0 && Resources.RandGen->Rndm()<Fres_Term || Resources.RandGen->Rndm()>gnd );  //DIF from V2 +++ ExtraFix: before gnd*Fres_Term ExtraExtra - leak fix

//      //The diffuse lobe will be randomly polarized
//      A_trans = NewMomentum.cross(theScatNormal);
//      A_trans = A_trans.unit();
//      A_paral = NewMomentum.cross(A_trans);
//      A_paral = A_paral.unit();
//      G4double ThetaPolar = twopi*G4UniformRand();
//      NewPolarization = cos(ThetaPolar)*A_paral + sin(ThetaPolar)*A_trans;
      Photon->SimStat->OverrideClaudioLamb++;
    }

  Photon->v[0] = K.X();
  Photon->v[1] = K.Y();
  Photon->v[2] = K.Z();
  //qDebug() << "---"<<(int)Status<<PhotonDir[0]<<PhotonDir[1]<<PhotonDir[2];
  return Back;
}

const QString PhScatClaudioModelV2d2::getReportLine() const
{
  return " v2.2";
}

AOpticalOverride::OpticalOverrideResultEnum PhScatClaudioModelV2d1::calculate(ATracerStateful &Resources, APhoton *Photon, const double *NormalVector)
{
  TVector3 K(Photon->v);                // photon direction
    //qDebug() << "Photon direction (i,j,k):"<<K.x()<<K.y()<<K.z();
    //qDebug() << "iWave:"<<iWave;

    //ANTS2 SPECIFIC: navigator gives normal in the direction of the photon, while Claudio's model assumes the opposite direction
    TVector3 GlobalNormal(-NormalVector[0], -NormalVector[1], -NormalVector[2]);  // normal to the surface
    //qDebug() << "Surface normal (i,j,k):"<<GlobalNormal.x()<<GlobalNormal.y()<<GlobalNormal.z();

    //refractive indexes of materials before and after the interface
    double Rindex1 = (*MatCollection)[MatFrom]->getRefractiveIndex(Photon->waveIndex);
    double Rindex2 = (*MatCollection)[MatTo]->getRefractiveIndex(Photon->waveIndex);
    //qDebug() << "Refractive indices from and to:" << Rindex1 << Rindex2;

    double costi = - GlobalNormal * K;    // global angle of incidence
    //qDebug() << "costi:"<<costi;

    double gni = (sigma_alpha > 0) ? GnFunc(costi) : 1.0; // shadowing fuction
  //  qDebug() << "gni:"<<gni;
    double gnr;   // one more shadowing function

    double lambda = SpikeIntensity(Photon->waveIndex, costi); // relative intensity of the specular spike
    //qDebug() << "lambda:"<<lambda;

    TVector3 ScatNormal = GlobalNormal;
    double costl; // local angle of incidence
    double costr; // reflection angle
    double weight_microfacet = 1.5;

    // If the random number is larger than lambda the light will be scattered
    //according to the local normal. If the sigma_alpha is zero there is no specular lobe
    if (Resources.RandGen->Rndm() < lambda || sigma_alpha == 0)
      {
        Status = SpikeReflection;
        costl = costi;
        costr = costi;
        weight_microfacet = gni;
        gnr = gni;
      }
    else
      {
        int counter = 1000;
        do
          {
            counter--;
            if (counter == 0)
              {
                Status = Error;
                return _Error_;
              }

            Status = LobeReflection;
            double alpha_WRM = SlopeAngle(Resources.RandGen->Rndm()); // Sampling of the slope angle
            double phi_alpha_WRM = 2.0 * 3.1415926535 * Resources.RandGen->Rndm();

            // Set the Components of the Global Normal
            double CosAlpha_WRM = cos(alpha_WRM);
            double SinAlpha_WRM = sin(alpha_WRM);

            // Calculating the local angle
            ScatNormal.SetX(SinAlpha_WRM * (cos(phi_alpha_WRM)));
            ScatNormal.SetY(SinAlpha_WRM * (sin(phi_alpha_WRM)));
            ScatNormal.SetZ(CosAlpha_WRM);
            ScatNormal.RotateUz(GlobalNormal);

            costl = - K * ScatNormal;

            // Calculating the reflection angle
            costr = 2.0 * costl*CosAlpha_WRM - costi; //angle between the global normal and the viewing direction
            gnr  = GnFunc(costr);

            //if(costl < 0 && costr < 0)  //// Andr: dif!
            if (costl < 0)
              weight_microfacet = 0;
            else
              weight_microfacet = gni * costl / (costi*CosAlpha_WRM);

            // qDebug() << "new weight microfacet:"<< weight_microfacet;
          }
        while ( Resources.RandGen->Rndm()*1.5 > weight_microfacet );
      }


    //In ANTS2 there is no photon polarisation -> assuming 50% / 50%
      //The polarization of the incident photon
      //TVector3 A_trans = OldMomentum.cross(theScatNormal); //Measuring the Polarization
      //A_trans = A_trans.Unit();
    double E1_perp = sqrt(2.0)/2.0;//OldPolarization * A_trans;
      //TVector3 E1pp    = E1_perp * A_trans;
      //TVector3 E1pl    = OldPolarization - E1pp;
    double E1_parl = E1_perp; //E1pl.mag();

    double Amp_perp = Fresnel(E1_perp, 0, costl, Rindex1, Rindex2);
    double Amp_parl = Fresnel(0, E1_parl, costl, Rindex1, Rindex2);
    double Amp_tot = (Amp_perp+Amp_parl)*gnr;
    // qDebug() << "Amp_total"<<Amp_tot<<"gnr"<<gnr<<"perp"<<Amp_perp<<"par"<<Amp_parl;

        //TVector3 A_paral;
    if( Resources.RandGen->Rndm() < Amp_tot)
      {
        //Computation of the new momentum and the new polarization for the reflected photon (specular lobe or specular spike)
        K = 2.0 * costl * ScatNormal + K;
        // qDebug() << "K:"<<K[0]<<K[1]<<K[2];
        // qDebug() << "Kout"<<Kout[0]<<Kout[1]<<Kout[2];
        // qDebug() << "ScatNormal"<<ScatNormal[0]<<ScatNormal[1]<<ScatNormal[2]<<"costl"<<costl;

          //A_paral = NewMomentum.cross(A_trans);
          //A_paral   = A_paral.unit();
          //G4double C_perp = std::sqrt(Amp_perp/Amp_tot);
          //G4double C_parl = std::sqrt(Amp_parl/Amp_tot);
          //NewPolarization = C_parl*A_paral + C_perp*A_trans;
        Photon->SimStat->OverrideClaudioSpec++;
      }
    else
      {
        //double gnd;
        double Fres_Term = 0;
        do
          {
            if ( Resources.RandGen->Rndm() > albedo)
              {
                Status = Absorption;
                Photon->SimStat->OverrideClaudioAbs++;
                return Absorbed;
              }

            Status = LambertianReflection;  //The computation of the diffuse direction direction

            Fres_Term = 0; //gnd = 0;

            double sintdsqquared = Resources.RandGen->Rndm();
            double sintd = sqrt(sintdsqquared);
            double costd = sqrt(1.0 - sintdsqquared);
            double phid = 2.0 * 3.1415926535 * Resources.RandGen->Rndm();

            double sintinter = Rindex2*sintd/Rindex1;  //DIF from V2
            double costiter = 0;

            Fres_Term  = Fresnel(sqrt(2.0)/2.0, sqrt(2.0)/2.0, costd, Rindex2, Rindex1);  //DIF from V2

            if (sintinter < 1.0)
              {
                costiter = sqrt(1.0 - sintinter*sintinter);

                K.SetX(sintinter*(cos(phid)));  //DIF from V2
                K.SetY(sintinter*(sin(phid)));  //DIF from V2
                K.SetZ(costiter);  //DIF from V2
                K.RotateUz(ScatNormal);  //DIF from V2
                //gnd = 1;
              }
          }
        while(sigma_alpha>0 && Resources.RandGen->Rndm() < Fres_Term);  //DIF from V2 +++ ExtraFix: before gnd*Fres_Term

  //      //The diffuse lobe will be randomly polarized
  //      A_trans = NewMomentum.cross(theScatNormal);
  //      A_trans = A_trans.unit();
  //      A_paral = NewMomentum.cross(A_trans);
  //      A_paral = A_paral.unit();
  //      G4double ThetaPolar = twopi*G4UniformRand();
  //      NewPolarization = cos(ThetaPolar)*A_paral + sin(ThetaPolar)*A_trans;
        Photon->SimStat->OverrideClaudioLamb++;
      }

    Photon->v[0] = K.X();
    Photon->v[1] = K.Y();
    Photon->v[2] = K.Z();
    //qDebug() << "---"<<(int)Status<<PhotonDir[0]<<PhotonDir[1]<<PhotonDir[2];
    return Back;
}

const QString PhScatClaudioModelV2d1::getReportLine() const
{
  return " v2.1";
}
