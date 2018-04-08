//ANTS
#include "apmhub.h"
#include "amaterialparticlecolection.h"
#include "generalsimsettings.h"
#include "apmposangtyperecord.h"
#include "agammarandomgenerator.h"
#include "ajsontools.h"
#include "acommonfunctions.h"
#include "acustomrandomsampling.h"

//Qt
#include <QVector>
#include <QDebug>
#include <QFile>
#include <QTextStream>

//Root
#include "TRandom2.h"
#include "TMath.h"
#include "TH1D.h"

APmHub::APmHub(AMaterialParticleCollection *materialCollection, TRandom2 *randGen) :
   RandGen(randGen), MaterialCollection(materialCollection)
{    
    APmHub::clear();
    GammaRandomGen = new AGammaRandomGenerator();
    PMtypes.append(new APmType("Type1"));
}

APmHub::~APmHub()
{
  //    qDebug() << "  --- PMs destructor";
  delete GammaRandomGen;
  clear();
  clearPMtypes();
}

void APmHub::writeInividualOverridesToJson(QJsonObject &json)
{
  QJsonObject js;

  //if (isPDEeffectiveOverriden())
    writePDEeffectiveToJson(js);  // scalar PDEs
  //if (isPDEwaveOverriden())
    writePDEwaveToJson(js);       // wave-resolved PDE
  //if (isAngularOverriden())
    writeAngularToJson(js);       // angular response
  //if (isAreaOverriden())
    writeAreaToJson(js);          // area response
  writeRelQE_PDE(js);             // relative factors used to adjust gains

  json["IndividualPMoverrides"] = js;
}

bool APmHub::readInividualOverridesFromJson(QJsonObject &json)
{
  if (!json.contains("IndividualPMoverrides"))
    {
      qWarning() << "Json does not contain individual PMs override data!";
      return false;
    }
  QJsonObject js = json["IndividualPMoverrides"].toObject();
  APmHub::resetOverrides();
  readPDEeffectiveFromJson(js);
  readPDEwaveFromJson(js);
  readAngularFromJson(js);
  readAreaFromJson(js);
  //relative QE / ElStr if at least one is not 1
  readRelQE_PDE(js);
  return true;
}

void APmHub::writeElectronicsToJson(QJsonObject &json)
{
  QJsonObject js;

  QJsonObject pj;
      pj["Active"] = fDoPHS;
      QJsonArray arr;
      for (int ipm = 0; ipm < numPMs; ipm++)
        {
          QJsonObject jj;
          PMs.at(ipm).writePHSsettingsToJson(jj);
          arr.append(jj);
        }
      pj["Data"] = arr;
  js["PHS"] = pj;

  QJsonObject mcj;
      mcj["Active"] = fDoMCcrosstalk;
      QJsonArray arrVec, arrTP, arrModel;
      for (int ipm=0; ipm<numPMs; ipm++)
        {
          QJsonArray amc;
          for (double val : PMs[ipm].MCcrosstalk) amc << val;
          arrVec.append(amc);

          arrTP << PMs[ipm].MCtriggerProb;
          arrModel << PMs[ipm].MCmodel;
        }
      mcj["ProbDistr"] = arrVec;
      mcj["Model"] = arrModel;
      mcj["TrigProb"] = arrTP;
  js["MCcrosstalk"] = mcj;

  QJsonObject nj;
      nj["Active"] = fDoElNoise;
      QJsonArray ar2;
      for (int ipm=0; ipm<numPMs; ipm++) ar2.append(PMs.at(ipm).ElNoiseSigma);
      nj["NoiseSigma"] = ar2;
  js["Noise"] = nj;

  QJsonObject aj;
      aj["Active"] = fDoADC;
      QJsonArray ar, ar1;
      for (int ipm=0; ipm<numPMs; ipm++)
        {
          ar.append(PMs.at(ipm).ADCmax);
          ar1.append(PMs.at(ipm).ADCbits);
        }
      aj["ADCmax"] = ar;
      aj["ADCbits"] = ar1;
  js["ADC"] = aj;

  js["TimeWindow"] = MeasurementTime;

  json["Electronics"] = js;
}

bool APmHub::readElectronicsFromJson(QJsonObject &json)
{  
   fDoPHS = false;
   fDoMCcrosstalk = false;
   fDoElNoise = false;
   fDoADC = false;
   MeasurementTime = 150.0;

   //compatibility
   for (int ipm=0; ipm<numPMs; ipm++)
   {
       PMs[ipm].MCcrosstalk.clear();
       PMs[ipm].MCmodel = 0;
       PMs[ipm].MCtriggerProb = 0;
   }

   if (!json.contains("Electronics")) return false;
   QJsonObject js = json["Electronics"].toObject();

   QJsonObject pj = js["PHS"].toObject();
   parseJson(pj, "Active", fDoPHS);

   if (pj.contains("Data"))
     {
       QJsonArray ar = pj["Data"].toArray();
       if (ar.size() != numPMs)
         {
           qWarning()<<"PHS json size mismatch!" << "PMs:"<<PMs.count()<<"in json:"<<ar.size();
           return false;
         }
       for (int ipm=0; ipm<numPMs; ipm++)
         {
           QJsonObject jj = ar[ipm].toObject();
           PMs[ipm].readPHSsettingsFromJson(jj);
         }
     }

   if (js.contains("MCcrosstalk"))
     {
       QJsonObject mcj =js["MCcrosstalk"].toObject();
       parseJson(mcj, "Active", fDoMCcrosstalk);

       QJsonArray arrM;
       if (mcj.contains("Data")) arrM = mcj["Data"].toArray();  //old name
       if (mcj.contains("ProbDistr")) arrM = mcj["ProbDistr"].toArray();  //new name
       if (arrM.size() != PMs.count())
         qWarning() << "MCcrosstalk array size mismatch!";
       else
         {
           for (int ipm=0; ipm<numPMs; ipm++)
             {
               QJsonArray amc = arrM[ipm].toArray();
               for (int i=0; i<amc.size(); i++)
                 PMs[ipm].MCcrosstalk << amc[i].toDouble();
             }
         }       
       if (mcj.contains("Model"))
       {
           QJsonArray arrMod = mcj["Model"].toArray();
           if (arrMod.size() != PMs.count())
             qWarning() << "MCcrosstalk/Model array size mismatch!";
           else
             for (int ipm=0; ipm<numPMs; ipm++) PMs[ipm].MCmodel = arrMod[ipm].toDouble();
       }
       if (mcj.contains("TrigProb"))
       {
           QJsonArray arrTP = mcj["TrigProb"].toArray();
           if (arrTP.size() != PMs.count())
             qWarning() << "MCcrosstalk/TrigProb array size mismatch!";
           else
             for (int ipm=0; ipm<numPMs; ipm++) PMs[ipm].MCtriggerProb = arrTP[ipm].toDouble();
       }
     }

   QJsonObject nj = js["Noise"].toObject();
   parseJson(nj, "Active", fDoElNoise);
   if (nj.contains("NoiseSigma"))
     {
       QJsonArray ar = nj["NoiseSigma"].toArray();
       if (ar.size() != numPMs)
         {
           qWarning()<<"Electronics noise json size mismatch!";
           return false;
         }
       for (int ipm=0; ipm<numPMs; ipm++)
           PMs[ipm].ElNoiseSigma = ar[ipm].toDouble();
     }

   QJsonObject aj = js["ADC"].toObject();
   parseJson(aj, "Active", fDoADC);
   if (aj.contains("ADCmax"))
     {
       QJsonArray ar = aj["ADCmax"].toArray();
       if (ar.size() != numPMs)
         {
           qWarning()<<"ADC max json size mismatch!";
           return false;
         }
       for (int ipm=0; ipm<numPMs; ipm++)
           PMs[ipm].ADCmax = ar[ipm].toDouble();
     }
   if (aj.contains("ADCbits"))
     {
       QJsonArray ar = aj["ADCbits"].toArray();
       if (ar.size() != numPMs)
         {
           qWarning()<<"ADC bits json size mismatch!";
           return false;
         }
       for (int ipm=0; ipm<numPMs; ipm++)
           PMs[ipm].ADCbits = ar[ipm].toDouble();
     }

   if (js.contains("TimeWindow"))
        MeasurementTime = js["TimeWindow"].toDouble();

   updateADClevels();

   return true;
}

void APmHub::resetOverrides()
{
    for (APm& pm : PMs) pm.resetOverrides();
}

void APmHub::configure(GeneralSimSettings *SimSet)
{
    WavelengthResolved = SimSet->fWaveResolved;
    AngularResolved = SimSet->fAngResolved;
    AreaResolved = SimSet->fAreaResolved;
    WaveFrom = SimSet->WaveFrom;
    WaveStep = SimSet->WaveStep;
    WaveNodes = SimSet->WaveNodes;
    CosBins = SimSet->CosBins;

    //rebinning PDE data
    RebinPDEs(); //rebin or clear
    //calculating binned angular
    RecalculateAngular(); //rebin or clear
    // MaxQE accelerator
    if (SimSet->fQEaccelerator) calculateMaxQEs();
    //if PHS are configured, prepare histograms
    preparePHSs();
    //prepare crosstalk data
    prepareMCcrosstalk();
}

void APmHub::SetWave(bool wavelengthResolved, double waveFrom, double waveStep, int waveNodes)
{
  WavelengthResolved = wavelengthResolved;
  WaveFrom = waveFrom;
  WaveStep = waveStep;
  WaveNodes = waveNodes;
}

void APmHub::calculateMaxQEs()
{
    //calculating maxQE for accelerator
    //qDebug() << "--Initializing QEcheck accelerator in PMs--";
    MaxQE = 0;
    if (WavelengthResolved) MaxQEvsWave.fill(0, WaveNodes);

    //going trough all PMs and check for the maximum QE
    for (int ipm = 0; ipm < numPMs; ipm++)
      {
        //qDebug()<<"PM ="<<ipm;
        double AngleFactor;
        if (AngularResolved)
          {
            AngleFactor = 0;
            for (int icosAngle =0; icosAngle<CosBins; icosAngle++)
              {
                double cosAngle = 1.0*icosAngle/(CosBins-1);
                double thisFactor = getActualAngularResponse(ipm, cosAngle);
                //qDebug()<<"iAngle"<<icosAngle<<cosAngle<<thisFactor;
                if (thisFactor > AngleFactor) AngleFactor = thisFactor;
              }
          }
        else AngleFactor = 1.0;
        //qDebug()<<"  Angle factor="<<AngleFactor;

        if (WavelengthResolved)
          {//---WaveResolved---
            for (int iWave = 0; iWave<WaveNodes; iWave++) //going through all wave nodes
              {
                double thisVal = getActualPDE(ipm, iWave) * AngleFactor;
                if (thisVal > MaxQEvsWave[iWave]) MaxQEvsWave[iWave] = thisVal;
              }
          }//-------
        else
          {//---NotWaveResolved---
            double thisQE = getActualPDE(ipm, -1);
            //qDebug()<<"  actualPDE="<<thisQE;
            double thisVal = thisQE*AngleFactor;
            //qDebug()<<"  Total QE ="<<thisVal;
            if (thisVal > MaxQE) MaxQE = thisVal;
          }//--------
      }//end cycle by all PMs

    if (WavelengthResolved)
      {
        MaxQE = 0;
        for (int i=0; i<WaveNodes; i++)
            if(MaxQEvsWave[i] > MaxQE) MaxQE = MaxQEvsWave[i];
        //qDebug()<<MaxQEvsWave;
      }
     //qDebug()<<"MaxQE ="<< MaxQE;
}

void APmHub::clear() //does not affect PM types!
{
    for (APm& pm : PMs) pm.clearSPePHSCustomDist();
    PMs.clear();

    numPMs = 0;
}

void APmHub::add(int upperlower, double xx, double yy, double zz, double Psi, int typ) //add new PM
{  
  APmHub::insert(numPMs, upperlower, xx, yy, zz, Psi, typ);
}

void APmHub::add(int upperlower, APmPosAngTypeRecord *pat)
{
  APmHub::insert(numPMs, upperlower, pat->x, pat->y, pat->z, 0, pat->type);
  PMs.last().phi = pat->phi;
  PMs.last().theta = pat->theta;
  PMs.last().psi = pat->psi;
}

void APmHub::insert(int ipm, int upperlower, double xx, double yy, double zz, double Psi, int typ)
{
    if (ipm > numPMs) return; //in =case its automatic append

    APm newPM(xx, yy, zz, Psi, typ);
    newPM.upperLower = upperlower;
    PMs.insert(ipm, newPM);

    numPMs++;
}

void APmHub::remove(int ipm)
{
    if (ipm < 0 || ipm > numPMs-1)
    {
        qDebug()<<"ERROR: attempt to remove non-existent PM # "<<ipm;
        return;
    }

    // APm is not owning the histogram objects -> delete them before removing from the vector
    PMs[ipm].clearSPePHSCustomDist();
    PMs.remove(ipm);

    numPMs--;
}

bool APmHub::removePMtype(int itype)
{
  int numTypes = PMtypes.size();
  if (numTypes < 2) return false;
  bool inUse = false;
  for (int ipm = 0; ipm < numPMs; ipm++)
    {
      if (PMs[ipm].type == itype)
        {
          inUse = true;
          break;
        }
    }
  if (inUse) return false;

  PMtypes.remove(itype);
  //shifting existing ones
  for (int ipm = 0; ipm < numPMs; ipm++)
    {
      int it = PMs[ipm].type;
      if ( it > itype ) PMs[ipm].type = it-1;
    }
  return true;
}

void APmHub::appendNewPMtype(APmType *tp)
{
  PMtypes.append(tp);
}

int APmHub::findPMtype(QString typeName) const
{
  for (int i=0; i<PMtypes.size(); i++)
    if (PMtypes.at(i)->Name == typeName) return i;
  return -1;
}

void APmHub::clearPMtypes()
{
  for (int i=0; i<PMtypes.size(); i++) delete PMtypes[i];
  PMtypes.clear();
}

void APmHub::replaceType(int itype, APmType *newType) {delete PMtypes[itype]; PMtypes[itype] = newType;}

void APmHub::updateTypePDE(int typ, QVector<double> *x, QVector<double> *y) {PMtypes[typ]->PDE_lambda = *x; PMtypes[typ]->PDE = *y;}

void APmHub::scaleTypePDE(int typ, double factor)
{
    for (int i=0; i<PMtypes[typ]->PDE.size(); i++)
        PMtypes[typ]->PDE[i] *= factor;
}

void APmHub::updateTypeAngular(int typ, QVector<double> *x, QVector<double> *y) {PMtypes[typ]->AngularSensitivity_lambda = *x; PMtypes[typ]->AngularSensitivity = *y;}

void APmHub::updateTypeAngularN1(int typ, double val) {PMtypes[typ]->AngularN1 = val;}

void APmHub::updateTypeArea(int typ, QVector<QVector<double> > *vec, double xStep, double yStep){PMtypes[typ]->AreaSensitivity = *vec; PMtypes[typ]->AreaStepX = xStep; PMtypes[typ]->AreaStepY = yStep;}

void APmHub::clearTypeArea(int typ){PMtypes[typ]->AreaSensitivity.resize(0);}

void APmHub::preparePHSs()
{
  for (int ipm=0; ipm<numPMs; ipm++)
      PMs[ipm].preparePHS();
}

void APmHub::prepareMCcrosstalk()
{
  for (int ipm=0; ipm<numPMs; ipm++)
     prepareMCcrosstalkForPM(ipm);
}

void APmHub::prepareMCcrosstalkForPM(int ipm)
{
    delete PMs[ipm].MCsampl; PMs[ipm].MCsampl = 0;

    if (fDoMCcrosstalk && PMs[ipm].MCmodel==0)
      PMs[ipm].MCsampl = new ACustomRandomSampling(RandGen, &PMs.at(ipm).MCcrosstalk);
}

void APmHub::updateADClevels()
{
  for (int ipm=0; ipm<numPMs; ipm++)
  {
      const double& max = PMs.at(ipm).ADCmax;
      const int& bits = PMs.at(ipm).ADCbits;

      PMs[ipm].ADClevels = TMath::Power(2, bits) - 1;
      if (PMs.at(ipm).ADClevels == 0) PMs[ipm].ADCstep = 0;
      else PMs[ipm].ADCstep = max / PMs.at(ipm).ADClevels;
  }
}

void APmHub::CalculateElChannelsStrength()
{
    for (int ipm = 0; ipm<PMs.size(); ipm++)
        PMs[ipm].scaleSPePHS( PMs.at(ipm).relElStrength );
}

double APmHub::GenerateSignalFromOnePhotoelectron(int ipm)
{
    switch ( PMs.at(ipm).SPePHSmode )
    {
      case 0: return PMs.at(ipm).AverageSigPerPhE;
      case 1: return RandGen->Gaus( PMs.at(ipm).AverageSigPerPhE, PMs.at(ipm).SPePHSsigma );
      case 2: return GammaRandomGen->getGamma( PMs.at(ipm).SPePHSshape, PMs.at(ipm).AverageSigPerPhE / PMs.at(ipm).SPePHSshape );
      case 3:
       {
         if (PMs.at(ipm).SPePHShist) return PMs[ipm].SPePHShist->GetRandom();
         else return 0;
       }
      default:;
    }

    qWarning() << "Error: unrecognized type in signal per photoelectron generation";
    return 0;
}

void APmHub::RebinPDEsForType(int itype)
{    
    if (WavelengthResolved)
    {
        if (PMtypes.at(itype)->PDE_lambda.isEmpty())
            PMtypes[itype]->PDEbinned.clear();
        else
            ConvertToStandardWavelengthes(&PMtypes[itype]->PDE_lambda, &PMtypes[itype]->PDE, WaveFrom, WaveStep, WaveNodes, &PMtypes[itype]->PDEbinned);
    }
    else PMtypes[itype]->PDEbinned.clear();
}

void APmHub::RebinPDEsForPM(int ipm)
{
    if (WavelengthResolved)
    {
        if (PMs.at(ipm).PDE_lambda.isEmpty())
            PMs[ipm].PDEbinned.clear();
        else
            ConvertToStandardWavelengthes(&PMs.at(ipm).PDE_lambda, &PMs.at(ipm).PDE, WaveFrom, WaveStep, WaveNodes, &PMs[ipm].PDEbinned);
    }
    else PMs[ipm].PDEbinned.clear();
}

void APmHub::RebinPDEs()
{
    for (int itype = 0; itype < PMtypes.size(); itype++)
        RebinPDEsForType(itype);
    for (int ipm = 0; ipm < numPMs; ipm++)
        RebinPDEsForPM(ipm);
}

void APmHub::RecalculateAngular()
{
    for (int itype = 0; itype < countPMtypes(); itype++)
        RecalculateAngularForType(itype);
    for (int ipm = 0; ipm < numPMs; ipm++)
        RecalculateAngularForPM(ipm);
}

void APmHub::RecalculateAngularForType(int typ)
{
    if (!AngularResolved || PMtypes[typ]->AngularSensitivity_lambda.size() == 0)
        PMtypes[typ]->AngularSensitivityCosRefracted.clear();
    else
    {
        //transforming degrees to cos(Refracted)
        QVector<double> x, y;
        double n1 = PMtypes[typ]->AngularN1;
        double n2 = (*MaterialCollection)[PMtypes[typ]->MaterialIndex]->n;
        //      qDebug()<<"n1 n2 "<<n1<<n2;

        for (int i=PMtypes[typ]->AngularSensitivity_lambda.size()-1; i>=0; i--) //if i++ - data span from 1 down to 0
        {
            double Angle = PMtypes[typ]->AngularSensitivity_lambda[i] * TMath::Pi()/180.0;
            //calculating cos of the transmitted angle
            double sinI = sin(Angle);
            double cosI = cos(Angle);

            double sinT = sinI * n1 / n2;
            double cosT = sqrt(1 - sinT*sinT);
            x.append(cosT);

            //         qDebug()<<"Angle: "<<Angle<< " sinI="<<sinI<<" cosI="<<cosI;
            //         qDebug()<<"sinT="<<sinT<<"cosT="<<cosT;

            //correcting for the reflection loss            
            double Rs = (n1*cosI-n2*cosT)*(n1*cosI-n2*cosT) / ( (n1*cosI+n2*cosT) * (n1*cosI+n2*cosT) );
            double Rp = (n1*cosT-n2*cosI)*(n1*cosT-n2*cosI) / ( (n1*cosT+n2*cosI) * (n1*cosT+n2*cosI) );
            double R = 0.5*(Rs+Rp);
            //         qDebug()<<"Rs Rp"<<Rs<<Rp;
            //         qDebug()<<"Reflection "<<R<<"ReflectionforNormal="<<(n1-n2)*(n1-n2)/(n1+n2)/(n1+n2);

            const double correction = R < 0.99999 ? 1.0/(1.0-R) : 1.0; //T = (1-R)
            //meaning: have to take into account that during measurements part of the light was reflected

            y.append(PMtypes[typ]->AngularSensitivity[i]*correction);
            //         qDebug()<<cosT<<correction<<PMtypeProperties[typ].AngularSensitivity[i]*correction;
        }
        x.replace(0, x[1]); //replace with the last available value
        y.replace(0, y[1]);
        //reusing the function:
        ConvertToStandardWavelengthes(&x, &y, 0, 1.0/(CosBins-1), CosBins, &PMtypes[typ]->AngularSensitivityCosRefracted);
    }
}

void APmHub::RecalculateAngularForPM(int ipm)
{
    if ( !AngularResolved || PMs.at(ipm).AngularSensitivity_lambda.isEmpty() )
        PMs[ipm].AngularSensitivityCosRefracted.clear();
    else
    {
        int typ = PMs[ipm].type;
        //transforming degrees to cos(Refracted)
        QVector<double> x, y;
        double n1 = PMs.at(ipm).AngularN1;
        double n2 = (*MaterialCollection)[PMtypes[typ]->MaterialIndex]->n;
        //      qDebug()<<"n1 n2 "<<n1<<n2;
        for (int i = PMs.at(ipm).AngularSensitivity_lambda.size()-1; i >= 0; i--) //if i++ - data span from 1 down to 0
        {
            double Angle = PMs.at(ipm).AngularSensitivity_lambda.at(i) * TMath::Pi()/180.0;
            //calculating cos of the transmitted angle
            double sinI = sin(Angle);
            double cosI = cos(Angle);

            double sinT = sinI * n1 / n2;
            double cosT = sqrt(1 - sinT*sinT);
            x.append(cosT);

            //         qDebug()<<"Angle: "<<Angle<< " sinI="<<sinI<<" cosI="<<cosI;
            //         qDebug()<<"sinT="<<sinT<<"cosT="<<cosT;

            //correcting for the reflection loss            
            double Rs = (n1*cosI-n2*cosT)*(n1*cosI-n2*cosT) / ( (n1*cosI+n2*cosT) * (n1*cosI+n2*cosT) );
            double Rp = (n1*cosT-n2*cosI)*(n1*cosT-n2*cosI) / ( (n1*cosT+n2*cosI) * (n1*cosT+n2*cosI) );
            double R = 0.5*(Rs+Rp);
            //         qDebug()<<"Rs Rp"<<Rs<<Rp;
            //         qDebug()<<"Reflection "<<R<<"ReflectionforNormal="<<(n1-n2)*(n1-n2)/(n1+n2)/(n1+n2);

            //const double correction = R < 0.99999 ? 1.0/(1.0-R) : 1.0; //Why is it that here it's not this one too?
            double correction = 1.0/(1.0-R);    //T = (1-R)
            //meaning: have to take into account that during measurements part of the light was reflected

            y.append( PMs.at(ipm).AngularSensitivity.at(i) * correction );
            //         qDebug()<<cosT<<correction<<AngularSensitivity[ipm][i]*correction;
        }
        x.replace(0, x[1]); //replace with the last available value
        y.replace(0, y[1]);
        //reusing the function:
        ConvertToStandardWavelengthes(&x, &y, 0, 1.0/(CosBins-1), CosBins, &PMs[ipm].AngularSensitivityCosRefracted);
    }
}

double APmHub::getActualPDE(int ipm, int WaveIndex) const
{  
    /*
  qDebug()<<"------------";
  qDebug()<<"Wave-resolved:"<<WavelengthResolved<<"WaveIndex:"<<WaveIndex<<" PM# "<<ipm;
  qDebug()<<"Overrides-> Effective=" <<effectivePDE[ipm]<<" size ofWaveRes="<<PDEbinned[ipm].size();
  if (PDEbinned[ipm].size()>0) qDebug()<<"   overridePDE at waveindex="<<PDEbinned[ipm][WaveIndex];
  qDebug()<<"Type-> Effective="<<PMtypeProperties[PMtype[ipm]].effectivePDE<<" size of WaveRes="<<PMtypeProperties[PMtype[ipm]].PDEbinned.size();
  if (PMtypeProperties[PMtype[ipm]].PDEbinned.size() > 0) qDebug()<<"   typePDE at this waveindex="<<PMtypeProperties[PMtype[ipm]].PDEbinned[WaveIndex];
    */

    double PDE;
    if (!WavelengthResolved || WaveIndex == -1)
    {
        //Case: Not wavelength-resolved or no spectral data during this photon generation

        if (PMs.at(ipm).effectivePDE != -1.0)
             PDE = PMs.at(ipm).effectivePDE; //override exists
        else PDE = PMtypes.at( PMs.at(ipm).type )->EffectivePDE;
    }
    else
    {
        //Case: Wavelength-resolved AND there is proper waveindex

        //if wave-resolved override exists, use it:
        if ( !PMs.at(ipm).PDEbinned.isEmpty() )
             PDE = PMs.at(ipm).PDEbinned.at(WaveIndex);
        else
        {
            //if effective PDE is overriden, use it
            if (PMs.at(ipm).effectivePDE != -1.0)
                 PDE = PMs.at(ipm).effectivePDE;
            else
            {
                //if type hold wave-resolved info, use it
                const int& iType = PMs.at(ipm).type;
                if (PMtypes.at(iType)->PDEbinned.size() > 0)
                     PDE = PMtypes.at(iType)->PDEbinned.at(WaveIndex);
                else PDE = PMtypes.at(iType)->EffectivePDE; //last resort :)
            }
        }
    }
    //  qDebug()<<"reporting PDE of "<<PDE;

    return PDE;
}

double APmHub::getActualAngularResponse(int ipm, double cosAngle) const
{
    /*
    qDebug()<<"--------------";
    qDebug()<<"AngularResolved: "<<AngularResolved<<" cosAngle= "<<cosAngle<<"  PM# "<<ipm;
    qDebug()<<"Overrides->  Data size="<<AngularSensitivityCosRefracted[ipm].size();
    if (AngularSensitivityCosRefracted[ipm].size() > 0) qDebug()<<"   value at this bin= "<<AngularSensitivityCosRefracted[ipm][cosAngle  *(CosBins-1)];
    qDebug()<<"Type->  Data size="<<PMtypeProperties[PMtype[ipm]].AngularSensitivityCosRefracted.size();
    if (PMtypeProperties[PMtype[ipm]].AngularSensitivityCosRefracted.size() > 0) qDebug()<<"  value at this bin="<<PMtypeProperties[PMtype[ipm]].AngularSensitivityCosRefracted[cosAngle  *(CosBins-1)];
    */

    double AngularResponse = 1.0;

    if (AngularResolved)
    {
        //angular is activated - as the simulation option

        //do we have override data?
        if ( !PMs.at(ipm).AngularSensitivityCosRefracted.isEmpty() )
        {
            //using overrides
            const int bin = cosAngle * (CosBins-1);
            AngularResponse = PMs.at(ipm).AngularSensitivityCosRefracted.at(bin);
        }
        else
        {
            //do we have angular response defined for this pm type?
            const int& itype = PMs.at(ipm).type;
            if ( !PMtypes.at(itype)->AngularSensitivityCosRefracted.isEmpty() )
            {
                //using base type data
                int bin = cosAngle * (CosBins-1);
                AngularResponse = PMtypes.at(itype)->AngularSensitivityCosRefracted.at(bin);
            }
        }
    }

    //    qDebug()<<"Return value = "<<AngularResponse;
    return AngularResponse;
}

double APmHub::getActualAreaResponse(int ipm, double x, double y)
{
    /*
    qDebug()<<"--------------";
    qDebug()<<"AreaResolved: "<<AreaResolved<<" x= "<<x<<" y= "<<y<<"  PM# "<<ipm;
    */

    double AreaResponse = 1.0;

    if (AreaResolved)
    {
        //are there override data?
        if ( !PMs.at(ipm).AreaSensitivity.isEmpty() )
        {
            //use override
            double xStep = PMs.at(ipm).AreaStepX;
            double yStep = PMs.at(ipm).AreaStepY;
            int xNum = PMs.at(ipm).AreaSensitivity.size();
            int yNum = PMs.at(ipm).AreaSensitivity.at(0).size();
            int iX = x/xStep + 0.5*xNum;
            int iY = y/yStep + 0.5*yNum;
            //            qDebug()<<"Xbin= "<<iX<<"   Ybin= "<<iY;
            if (iX<0 || iX>xNum-1 || iY<0 || iY>yNum-1) //outside
                 AreaResponse = 0;
            else AreaResponse = PMs.at(ipm).AreaSensitivity.at(iX).at(iY);
        }
        else
        {
            const APmType *typ = PMtypes.at( PMs.at(ipm).type );

            if ( !typ->AreaSensitivity.isEmpty() ) //are there data for this PM type?
            {
                //use type data
                double xStep = typ->AreaStepX;
                double yStep = typ->AreaStepY;
                int xNum = typ->AreaSensitivity.size();
                int yNum = typ->AreaSensitivity.at(0).size();
                int iX = x/xStep + 0.5*xNum;
                int iY = y/yStep + 0.5*yNum;
                //               qDebug()<<"Xbin= "<<iX<<"   Ybin= "<<iY;
                if (iX<0 || iX>xNum-1 || iY<0 || iY>yNum-1) //outside
                     AreaResponse = 0;
                else AreaResponse = typ->AreaSensitivity.at(iX).at(iY);
            }
        }

        /*
        qDebug()<<"Overrides->  Data size="<<AreaSensitivity[ipm].size();
        if (AreaSensitivity[ipm].size() > 0) qDebug()<<"   value at this bin= "<<AreaSensitivity[ipm][iX][iY];
        qDebug()<<"Type->  Data size="<<PMtypeProperties[PMtype[ipm]].AreaSensitivity.size();
        if (PMtypeProperties[PMtype[ipm]].AreaSensitivity.size() > 0) qDebug()<<"  value at this bin="<<PMtypeProperties[PMtype[ipm]].AreaSensitivity[iX][iY];
        */
    }

    //    qDebug()<<"Return value= "<<AreaResponse;
    return AreaResponse;
}

QVector<QPair<double, int> > APmHub::getPMsSortedByR() const
{
    QVector<QPair<double, int> > snake(numPMs);
    for (int ipm = 0; ipm < numPMs; ipm++)
        snake[ipm] = qMakePair(hypot(PMs[ipm].x, PMs[ipm].y), ipm);
    qSort(snake); // sort by R
    return snake;
}

void APmHub::writePMtypesToJson(QJsonObject &json)
{
  QJsonArray ar;
  for (int i=0; i<PMtypes.size(); i++)
    {
      QJsonObject js;
      PMtypes[i]->writeToJson(js);
      ar.append(js);
    }
  json["PMtypes"] = ar;
}

bool APmHub::readPMtypesFromJson(QJsonObject &json)
{
  if (!json.contains("PMtypes"))
    {
      qWarning() << "No PM types info found in json!";
      return false;
    }

  QJsonArray ar = json["PMtypes"].toArray();
  clearPMtypes();//PMtypes.clear();
  for (int i=0; i<ar.size(); i++)
    {
      PMtypes.append(new APmType());
      QJsonObject js = ar[i].toObject();
      PMtypes.last()->readFromJson(js);
    }
  return (PMtypes.size()>0);
}

bool APmHub::saveCoords(const QString &filename)
{
    QFile outFile(filename);
    outFile.open(QIODevice::WriteOnly);
    if(!outFile.isOpen())
        return false;
    QTextStream outStream(&outFile);
    for(int i = 0; i < PMs.size(); i++)
        PMs[i].saveCoords(outStream);
    return true;
}

bool APmHub::saveAngles(const QString &filename)
{
    QFile outFile(filename);
    outFile.open(QIODevice::WriteOnly);
    if(!outFile.isOpen())
        return false;
    QTextStream outStream(&outFile);
    for(int i = 0; i < PMs.size(); i++)
        PMs[i].saveAngles(outStream);
    return true;
}

bool APmHub::saveTypes(const QString &filename)
{
    QFile outFile(filename);
    outFile.open(QIODevice::WriteOnly);
    if(!outFile.isOpen())
        return false;
    QTextStream outStream(&outFile);
    for(int i = 0; i < PMs.size(); i++)
        outStream << PMs[i].type << "\r\n";
    return true;
}

bool APmHub::saveUpperLower(const QString &filename)
{
    QFile outFile(filename);
    outFile.open(QIODevice::WriteOnly);
    if(!outFile.isOpen())
        return false;
    QTextStream outStream(&outFile);
    for(int i = 0; i < PMs.size(); i++)
        outStream << PMs[i].upperLower << "\r\n";
    return true;
}

bool APmHub::isPDEwaveOverriden(int ipm) const
{
    return (!PMs.at(ipm).PDE.isEmpty());
}

bool APmHub::isPDEwaveOverriden() const
{
    for (int ipm = 0; ipm < numPMs; ipm++)
        if (!PMs.at(ipm).PDE.isEmpty()) return true;
  return false;
}

bool APmHub::isPDEeffectiveOverriden() const
{
  for (int ipm = 0; ipm < numPMs; ipm++)
    if (PMs.at(ipm).effectivePDE != -1.0) return true;
  return false;
}

void APmHub::writePDEeffectiveToJson(QJsonObject &json)
{
  QJsonArray arr;
  for (int ipm = 0; ipm < numPMs; ipm++)
      arr.append( PMs.at(ipm).effectivePDE );
  json["PDEeffective"] = arr;
}

void APmHub::writeRelQE_PDE(QJsonObject &json)
{
  QJsonArray arr;
  for (int i=0; i<numPMs; i++)
    {
      QJsonArray el;
      el << PMs.at(i).relQE_PDE << PMs.at(i).relElStrength;
      arr.append(el);
    }
  json["RelQEandElStr"] = arr;
}

void APmHub::readRelQE_PDE(QJsonObject &json)
{
  for (int i=0; i<numPMs; i++)
    {
      PMs[i].relQE_PDE = 1;
      PMs[i].relElStrength = 1;
    }
  if (json.contains("RelQEandElStr"))
    {
      QJsonArray arr = json["RelQEandElStr"].toArray();
      if (arr.size() == count())
        {
          for (int i=0; i<numPMs; i++)
            {
              QJsonArray ar = arr[i].toArray();
              PMs[i].relQE_PDE = ar[0].toDouble();
              PMs[i].relElStrength = ar[1].toDouble();
            }
        }
    }
}

bool APmHub::readPDEeffectiveFromJson(QJsonObject &json)
{
  if (json.contains("PDEeffective"))
    {
      QJsonArray arr = json["PDEeffective"].toArray();
      if (arr.size() != numPMs)
        {
          qWarning() << "PDE json: size mismatch!";
          return false;
        }
      for (int ipm = 0; ipm < numPMs; ipm++)
          PMs[ipm].effectivePDE = arr[ipm].toDouble();
    }
  return true;
}

void APmHub::writePDEwaveToJson(QJsonObject &json)
{
  QJsonArray arr;
  for (int ipm=0; ipm<numPMs; ipm++)
    {
      QJsonArray arpm;
      writeTwoQVectorsToJArray(PMs.at(ipm).PDE_lambda, PMs.at(ipm).PDE, arpm);
      arr.append(arpm);
    }
  json["PDEwave"] = arr;
}

bool APmHub::readPDEwaveFromJson(QJsonObject &json)
{
  if (json.contains("PDEwave"))
    {
      QJsonArray arr = json["PDEwave"].toArray();
      if (arr.size() != numPMs)
        {
          qWarning() << "PDEwave json: size mismatch!";
          return false;
        }
      for (int ipm=0; ipm<numPMs; ipm++)
      {
        QJsonArray array = arr[ipm].toArray();
        readTwoQVectorsFromJArray(array, PMs[ipm].PDE_lambda, PMs[ipm].PDE);
      }
    }
  return true;
}

void APmHub::setPDEwave(int ipm, QVector<double> *x, QVector<double> *y)
{
    PMs[ipm].PDE_lambda = *x;
    PMs[ipm].PDE = *y;
}

bool APmHub::isAngularOverriden(int ipm) const
{
    return !PMs.at(ipm).AngularSensitivity.isEmpty();
}

bool APmHub::isAngularOverriden() const
{
  for (int ipm = 0; ipm < numPMs; ipm++)
    if ( !PMs.at(ipm ).AngularSensitivity.isEmpty() ) return true;
  return false;
}

void APmHub::writeAngularToJson(QJsonObject &json)
{
  QJsonObject js;
  QJsonArray arr;
  for (int ipm=0; ipm<numPMs; ipm++)
    {
      QJsonArray arpm;
      writeTwoQVectorsToJArray(PMs.at(ipm).AngularSensitivity_lambda, PMs.at(ipm).AngularSensitivity, arpm);
      arr.append(arpm);
    }
  js["AngularResponse"] = arr;
  QJsonArray arrn;
  for (int ipm = 0; ipm < numPMs; ipm++) arrn.append(PMs.at(ipm).AngularN1);
  js["RefrIndex"] = arrn;
  json["Angular"] = js;
}

bool APmHub::readAngularFromJson(QJsonObject &json)
{
  if (json.contains("Angular"))
    {
      QJsonObject js = json["Angular"].toObject();
      QJsonArray arr = js["AngularResponse"].toArray();
      if (arr.size() != numPMs)
        {
          qWarning() << "Angular json: size mismatch!";
          return false;
        }
      for (int ipm=0; ipm<numPMs; ipm++)
      {
        QJsonArray array = arr[ipm].toArray();
        readTwoQVectorsFromJArray(array, PMs[ipm].AngularSensitivity_lambda, PMs[ipm].AngularSensitivity);
      }
      QJsonArray arrn = js["RefrIndex"].toArray();
      if (arrn.size() != numPMs)
        {
          qWarning() << "RefrIndex json: size mismatch!";
          return false;
        }
      for (int ipm = 0; ipm < numPMs; ipm++) PMs[ipm].AngularN1 = arrn[ipm].toDouble();
    }
  return true;
}

void APmHub::setAngular(int ipm, QVector<double> *x, QVector<double> *y)
{
    PMs[ipm].AngularSensitivity_lambda = *x;
    PMs[ipm].AngularSensitivity = *y;
}

bool APmHub::isAreaOverriden(int ipm) const
{
    return !PMs.at(ipm).AreaSensitivity.isEmpty();
}

bool APmHub::isAreaOverriden() const
{
  for (int ipm = 0; ipm < numPMs; ipm++)
     if ( !PMs.at(ipm).AreaSensitivity.isEmpty() ) return true;
  return false;
}

void APmHub::writeAreaToJson(QJsonObject &json)
{
  QJsonObject js;
  QJsonArray arr;
  for (int ipm=0; ipm<numPMs; ipm++)
    {
      QJsonArray ar;
      write2DQVectorToJArray(PMs.at(ipm).AreaSensitivity, ar);
      arr.append(ar);
    }
  js["AreaResponse"] = arr;
  QJsonArray as;
  for (int ipm=0; ipm<numPMs; ipm++)
    {
      QJsonArray ar;
      ar.append(PMs.at(ipm).AreaStepX);
      ar.append(PMs.at(ipm).AreaStepY);
      as.append(ar);
    }
  js["Step"] = as;
  json["Area"] = js;
}

bool APmHub::readAreaFromJson(QJsonObject &json)
{
  if (json.contains("Area"))
    {
      QJsonObject js = json["Area"].toObject();
      QJsonArray arr = js["AreaResponse"].toArray();
      if (arr.size() != numPMs)
        {
          qWarning() << "Area json: size mismatch!";
          return false;
        }
      for (int ipm=0; ipm<numPMs; ipm++)
      {
        QJsonArray array = arr[ipm].toArray();
        read2DQVectorFromJArray(array, PMs[ipm].AreaSensitivity);
      }
      QJsonArray as = js["Step"].toArray();
      if (as.size() != numPMs)
        {
          qWarning() << "Area step json: size mismatch!";
          return false;
        }
      for (int ipm = 0; ipm < numPMs; ipm++)
      {
          QJsonArray ar = as[ipm].toArray();
          PMs[ipm].AreaStepX = ar[0].toDouble();
          PMs[ipm].AreaStepY = ar[1].toDouble();
      }
    }
  return true;
}

void APmHub::setArea(int ipm, QVector<QVector<double> > *vec, double xStep, double yStep)
{
    PMs[ipm].AreaSensitivity = *vec;
    PMs[ipm].AreaStepX = xStep;
    PMs[ipm].AreaStepY = yStep;
}
