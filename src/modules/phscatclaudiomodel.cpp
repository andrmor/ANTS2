#include "phscatclaudiomodel.h"
#include "amaterialparticlecolection.h"
#include "aphoton.h"
#include "asimulationstatistics.h"

#include <QDebug>
#include <QVector>
#include <QJsonObject>

#include "TRandom2.h"
#include "TVector3.h"

#define MODEL_VERSION 3

void PhScatClaudioModel::printConfiguration(int iWave)
{
  double Rindex1 = (*MatCollection)[MatFrom]->getRefractiveIndex(iWave);
  double Rindex2 = (*MatCollection)[MatTo]->getRefractiveIndex(iWave);

  qDebug() << "-------Configuration:-------";  
  qDebug() << "Model:"<<getType();
  qDebug() << "Sigma_alpha:"<<sigma_alpha<<"  Sigma_h:"<<sigma_h;
  qDebug() << "HeightDistrModel(empirical, gaussian, exponential):"<<HeightDistribution;
  qDebug() << "SlopeDistrModel(trowbridgereitz, cooktorrance, bivariatecauchy):"<<SlopeDistribution;
  qDebug() << "Albedo:"<<albedo;
  qDebug() << "Refractive indexes:"<<Rindex1 <<" to "<<Rindex2;
  qDebug() << "Wavelength:" << 1.0e-9 * MatCollection->convertWaveIndexToWavelength(iWave)<< "m";
  qDebug() << "----------------------------";
}

QString PhScatClaudioModel::getReportLine()
{
  QString s = "to " + (*MatCollection)[MatTo]->name;
  QString s1;
  s1.setNum(MatTo);
  s += " ("+s1+") --> Claudio's model";
  return s;
}

void PhScatClaudioModel::writeToJson(QJsonObject &json)
{
  AOpticalOverride::writeToJson(json);

  json["SigmaAlpha"] = sigma_alpha;
  json["SigmaH"] = sigma_h;
  json["Albedo"] = albedo;
  json["HDmodel"] = HeightDistribution;
  json["SDmodel"] = SlopeDistribution;  
}

bool PhScatClaudioModel::readFromJson(QJsonObject &json)
{
  QString type = json["Model"].toString();
  if (!type.startsWith("Claudio_Model"))
    {
      qCritical() << "Attempt to load json file for wrong override model!";
      return false; //file for wrong model!
    }

  sigma_alpha = json["SigmaAlpha"].toDouble();
  sigma_h = json["SigmaH"].toDouble();
  albedo = json["Albedo"].toDouble();
  HeightDistribution = static_cast<HeightDistrEnum>(json["HDmodel"].toInt());
  SlopeDistribution  = static_cast<SlopeDistrEnum>(json["SDmodel"].toInt());
  return true;
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

AOpticalOverride::OpticalOverrideResultEnum PhScatClaudioModelV2::calculate(TRandom2 *RandGen, APhoton *Photon, const double* NormalVector)
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
  if (RandGen->Rndm() < lambda || sigma_alpha == 0)
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
              Status = ErrorDetected;
              return _Error_;
            }

          Status = LobeReflection;
          double alpha_WRM = SlopeAngle(RandGen->Rndm()); // Sampling of the slope angle
          double phi_alpha_WRM = 2.0 * 3.1415926535 * RandGen->Rndm();

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
      while ( RandGen->Rndm()*1.5 > weight_microfacet );
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
  if( RandGen->Rndm() < Amp_tot)
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
          if ( RandGen->Rndm() > albedo)
            {
              Status = Absorption;
              Photon->SimStat->OverrideClaudioAbs++;
              return Absorbed;
            }

          Status = LambertianReflection;  //The computation of the diffuse direction direction

          Fres_Term = 0; gnd = 0;

          double sintdsqquared = RandGen->Rndm();
          double sintd = sqrt(sintdsqquared);
          double costd = sqrt(1.0 - sintdsqquared);
          double phid = 2.0 * 3.1415926535 * RandGen->Rndm();

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
      while(sigma_alpha>0 && RandGen->Rndm() > gnd*Fres_Term);

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

QString PhScatClaudioModelV2::getReportLine()
{
  return PhScatClaudioModel::getReportLine()+"_v2";
}

AOpticalOverride::OpticalOverrideResultEnum PhScatClaudioModelV2d2::calculate(TRandom2 *RandGen, APhoton *Photon, const double *NormalVector)
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
  if (RandGen->Rndm() < lambda || sigma_alpha == 0)
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
              Status = ErrorDetected;
              return _Error_;
            }

          Status = LobeReflection;
          double alpha_WRM = SlopeAngle(RandGen->Rndm()); // Sampling of the slope angle
          double phi_alpha_WRM = 2.0 * 3.1415926535 * RandGen->Rndm();

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
      while ( RandGen->Rndm()*1.5 > weight_microfacet );
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
  if( RandGen->Rndm() < Amp_tot)
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
          if ( RandGen->Rndm() > albedo)
            {
              Status = Absorption;
              Photon->SimStat->OverrideClaudioAbs++;
              return Absorbed;
            }

          Status = LambertianReflection;  //The computation of the diffuse direction direction

          Fres_Term = 0; gnd = 0;

          double sintdsqquared = RandGen->Rndm();
          double sintd = sqrt(sintdsqquared);
          double costd = sqrt(1.0 - sintdsqquared);
          double phid = 2.0 * 3.1415926535 * RandGen->Rndm();

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
      while(sigma_alpha>0 && RandGen->Rndm()<Fres_Term || RandGen->Rndm()>gnd );  //DIF from V2 +++ ExtraFix: before gnd*Fres_Term ExtraExtra - leak fix

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

QString PhScatClaudioModelV2d2::getReportLine()
{
  return PhScatClaudioModel::getReportLine()+"_v2.2";
}

AOpticalOverride::OpticalOverrideResultEnum PhScatClaudioModelV2d1::calculate(TRandom2 *RandGen, APhoton *Photon, const double *NormalVector)
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
    if (RandGen->Rndm() < lambda || sigma_alpha == 0)
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
                Status = ErrorDetected;
                return _Error_;
              }

            Status = LobeReflection;
            double alpha_WRM = SlopeAngle(RandGen->Rndm()); // Sampling of the slope angle
            double phi_alpha_WRM = 2.0 * 3.1415926535 * RandGen->Rndm();

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
        while ( RandGen->Rndm()*1.5 > weight_microfacet );
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
    if( RandGen->Rndm() < Amp_tot)
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
            if ( RandGen->Rndm() > albedo)
              {
                Status = Absorption;
                Photon->SimStat->OverrideClaudioAbs++;
                return Absorbed;
              }

            Status = LambertianReflection;  //The computation of the diffuse direction direction

            Fres_Term = 0; //gnd = 0;

            double sintdsqquared = RandGen->Rndm();
            double sintd = sqrt(sintdsqquared);
            double costd = sqrt(1.0 - sintdsqquared);
            double phid = 2.0 * 3.1415926535 * RandGen->Rndm();

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
        while(sigma_alpha>0 && RandGen->Rndm() < Fres_Term);  //DIF from V2 +++ ExtraFix: before gnd*Fres_Term

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

QString PhScatClaudioModelV2d1::getReportLine()
{
  return PhScatClaudioModel::getReportLine()+"_v2.1";
}
