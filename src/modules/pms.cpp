//ANTS
#include "pms.h"
#include "amaterialparticlecolection.h"
#include "generalsimsettings.h"
#include "apmposangtyperecord.h"
#include "pmtypeclass.h"
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



void APm::saveCoords(QTextStream &file) const { file << x << " " << y << " " << z << "\r\n"; }

void APm::saveCoords2D(QTextStream &file) const { file << x << " " << y << "\r\n"; }

void APm::saveAngles(QTextStream &file) const { file << phi << " " << theta << " " << psi << "\r\n"; }

pms::pms(AMaterialParticleCollection *materialCollection, TRandom2 *randGen)
{    
    MaterialCollection = materialCollection;
    RandGen = randGen;

    numPMs = 0;
    pms::clear();

    WavelengthResolved = false;
    AngularResolved = false;
    AreaResolved = false;   
    fDoPHS = false;
    fDoElNoise = false;
    fDoADC = false;
    //wave info
    WaveFrom = 200.0;
    WaveStep = 5;
    WaveNodes = 121;
    //number of bins for angular response
    CosBins = 1000;


    GammaRandomGen = new AGammaRandomGenerator();

    PMtypes.append(new PMtypeClass("Type1"));

    ///PMgroupDescription.append("Default group");

    MeasurementTime = 150;
}

pms::~pms()
{
  //qDebug() << "  --- PMs destructor";
  delete GammaRandomGen;
  clearPMtypes();
}

void pms::writeInividualOverridesToJson(QJsonObject &json)
{
  QJsonObject js;
  //PDE
  if (isPDEeffectiveOverriden()) //saving PDEeffectives
    writePDEeffectiveToJson(js);
  if (isPDEwaveOverriden()) //wave resolved PDE
    writePDEwaveToJson(js);
  //Angular response
  if (isAngularOverriden()) //angle resolved sensitivity
    writeAngularToJson(js);
  //Area response
  if (isAreaOverriden()) //angle resolved sensitivity
    writeAreaToJson(js);
  //relative QE / ElStr if at least one is not 1
  writeRelQE_PDE(js);

  json["IndividualPMoverrides"] = js;
}

bool pms::readInividualOverridesFromJson(QJsonObject &json)
{
  if (!json.contains("IndividualPMoverrides"))
    {
      qWarning() << "Json does not contain individual PMs override data!";
      return false;
    }
  QJsonObject js = json["IndividualPMoverrides"].toObject();
  pms::resetOverrides();
  readPDEeffectiveFromJson(js);
  readPDEwaveFromJson(js);
  readAngularFromJson(js);
  readAreaFromJson(js);
  //relative QE / ElStr if at least one is not 1
  readRelQE_PDE(js);
  return true;
}

void pms::writePHSsettingsToJson(int ipm, QJsonObject &json)
{  //0 - use average value; 1 - normal distr; 2 - Gamma distr; 3 - custom distribution
  json["Mode"] = SPePHSmode[ipm];
  json["Average"] = AverageSignalPerPhotoelectron[ipm];
  switch (SPePHSmode[ipm])
    {
    case 0: break;
    case 1:
      json["Sigma"] = SPePHSsigma[ipm];
      break;
    case 2:
      json["Shape"] = SPePHSshape[ipm];
      break;
    case 3:
      {
        QJsonArray ar;
        writeTwoQVectorsToJArray(SPePHS_x[ipm], SPePHS[ipm], ar);
        json["Distribution"] = ar;
        break;
      }
    default: qWarning() << "Unknown SPePHS mode";
    }
}

bool pms::readPHSsettingsFromJson(int ipm, QJsonObject &json)
{
  parseJson(json, "Mode", SPePHSmode[ipm]);
  parseJson(json, "Average", AverageSignalPerPhotoelectron[ipm]);
  parseJson(json, "Sigma", SPePHSsigma[ipm]);
  parseJson(json, "Shape", SPePHSshape[ipm]);

  SPePHS_x[ipm].clear();
  SPePHS[ipm].clear();
  if (json.contains("Distribution"))
    {
      QJsonArray ar = json["Distribution"].toArray();
      readTwoQVectorsFromJArray(ar, SPePHS_x[ipm], SPePHS[ipm]);
    }
  preparePHS(ipm);

  return true;
}

void pms::writeElectronicsToJson(QJsonObject &json)
{
  QJsonObject js;

  QJsonObject pj;
      pj["Active"] = fDoPHS;
      QJsonArray arr;
      for (int ipm=0; ipm<numPMs; ipm++)
        {
          QJsonObject jj;
          writePHSsettingsToJson(ipm, jj);
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
          ar.append(ADCmax[ipm]);
          ar1.append(ADCbits[ipm]);
        }
      aj["ADCmax"] = ar;
      aj["ADCbits"] = ar1;
  js["ADC"] = aj;

  js["TimeWindow"] = MeasurementTime;

  json["Electronics"] = js;
}

bool pms::readElectronicsFromJson(QJsonObject &json)
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
           readPHSsettingsFromJson(ipm, jj);
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
           ADCmax[ipm] = ar[ipm].toDouble();
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
           ADCbits[ipm] = ar[ipm].toDouble();
     }

   if (js.contains("TimeWindow"))
        MeasurementTime = js["TimeWindow"].toDouble();

   updateADClevels();

   return true;
}

void pms::resetOverrides()
{
  for (int ipm=0; ipm<numPMs; ipm++)
    {
      effectivePDE[ipm] = -1;
      PDE_lambda[ipm].clear();
      PDE[ipm].clear();
      PDEbinned[ipm].clear();
      AngularSensitivity_lambda[ipm].clear();
      AngularSensitivity[ipm].clear();
      AngularSensitivityCosRefracted[ipm].clear();
      AngularN1[ipm] = -1;
      AreaSensitivity[ipm].clear();
      AreaStepX[ipm] = 123;
      AreaStepY[ipm] = 123;
    }
}

void pms::configure(GeneralSimSettings *SimSet)
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

void pms::SetWave(bool wavelengthResolved, double waveFrom, double waveStep, int waveNodes)
{
  WavelengthResolved = wavelengthResolved;
  WaveFrom = waveFrom;
  WaveStep = waveStep;
  WaveNodes = waveNodes;
}

void pms::calculateMaxQEs()
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

void pms::clear() //does not affect PM types!
{
    PMs.clear();
    PDE.clear();
    PDE_lambda.clear();
    PDEbinned.clear();
    effectivePDE.clear();
    AngularSensitivity.clear();
    AngularSensitivity_lambda.clear();
    AngularN1.clear();
    AngularSensitivityCosRefracted.clear();
    AreaSensitivity.clear();
    AreaStepX.clear();
    AreaStepY.clear();

    AverageSignalPerPhotoelectron.clear();
    SPePHSmode.clear();
    SPePHSsigma.clear();
    SPePHSshape.clear();    
    for (int ipm=0; ipm<numPMs; ipm++) clearSPePHS(ipm);
    SPePHS_x.clear();
    SPePHS.clear();
    SPePHShist.clear();

    ADCmax.clear();
    ADCbits.clear();
    ADCstep.clear();
    ADClevels.clear();

    numPMs = 0;
}

void pms::add(int upperlower, double xx, double yy, double zz, double Psi, int typ) //add new PM
{  
  pms::insert(numPMs, upperlower, xx, yy, zz, Psi, typ);
}

void pms::add(int upperlower, APmPosAngTypeRecord *pat)
{
  pms::insert(numPMs, upperlower, pat->x, pat->y, pat->z, 0, pat->type);
  PMs.last().phi = pat->phi;
  PMs.last().theta = pat->theta;
  PMs.last().psi = pat->psi;
}

void pms::insert(int ipm, int upperlower, double xx, double yy, double zz, double Psi, int typ)
{
    if (ipm > numPMs) return; //in =case its automatic append

    numPMs++;

    //updating individual properties
    APm newPM(xx, yy, zz, Psi, typ);
    newPM.upperLower = upperlower;
    PMs.insert(ipm, newPM);

    QVector<double> tmp;
    PDE.insert(ipm, tmp);
    PDE_lambda.insert(ipm, tmp);
    PDEbinned.insert(ipm, tmp);
    effectivePDE.insert(ipm, -1); //-1 = undefined
    AngularSensitivity.insert(ipm, tmp);
    AngularSensitivity_lambda.insert(ipm, tmp);
    AngularN1.insert(ipm, -1); //-1 = undefined
    AngularSensitivityCosRefracted.insert(ipm, tmp);
    QVector<QVector<double> > tmp2;
    AreaSensitivity.insert(ipm, tmp2);
    AreaStepX.insert(ipm, 123); //just a strange value to see if error
    AreaStepY.insert(ipm, 123);

    AverageSignalPerPhotoelectron.insert(ipm, 1);
    SPePHSmode.insert(ipm, 0);
    SPePHSsigma.insert(ipm, 0);
    SPePHSshape.insert(ipm, 2);
    SPePHS_x.insert(ipm, tmp);
    SPePHS.insert(ipm, tmp);
    SPePHShist.insert(ipm, 0);

    ADCmax.insert(ipm, 65535);
    ADClevels.insert(ipm, 65535);
    ADCbits.insert(ipm, 16);
    ADCstep.insert(ipm, 1);
}

void pms::remove(int ipm)
{
    if (ipm<0 || ipm>numPMs-1)
    {
        qDebug()<<"ERROR: attempt to remove non-existent PM # "<<ipm;
        return;
    }

    numPMs--;
    PMs.remove(ipm);

    PDE.remove(ipm);
    PDE_lambda.remove(ipm);
    PDEbinned.remove(ipm);
    effectivePDE.remove(ipm);
    AngularSensitivity.remove(ipm);
    AngularSensitivity_lambda.remove(ipm);
    AngularN1.remove(ipm);
    AngularSensitivityCosRefracted.remove(ipm);
    AreaSensitivity.remove(ipm);
    AreaStepX.remove(ipm);
    AreaStepY.remove(ipm);

    AverageSignalPerPhotoelectron.remove(ipm);
    SPePHSmode.remove(ipm);
    SPePHSsigma.remove(ipm);
    SPePHSshape.remove(ipm);
    SPePHS_x.remove(ipm);
    SPePHS.remove(ipm);
    SPePHShist.remove(ipm);

    ADCmax.remove(ipm);
    ADClevels.remove(ipm);
    ADCbits.remove(ipm);
    ADCstep.remove(ipm);
}

/*
void pms::setTypeProperties(int typ, PMtypeClass* tp)
{
    PMtypeProperties[typ] = *tp;
}
*/
/*
void pms::setTypePropertiesScalar(int typ, const PMtypeClass *tp)
{
    PMtypeProperties[typ].name = tp->name;
    PMtypeProperties[typ].SiPM = tp->SiPM;
    PMtypeProperties[typ].MaterialIndex = tp->MaterialIndex;
    PMtypeProperties[typ].Shape = tp->Shape;
    PMtypeProperties[typ].SizeX = tp->SizeX;
    PMtypeProperties[typ].SizeY = tp->SizeY;
    PMtypeProperties[typ].PixelsX = tp->PixelsX;
    PMtypeProperties[typ].PixelsY = tp->PixelsY;
    PMtypeProperties[typ].DarkCountRate = tp->DarkCountRate;
    PMtypeProperties[typ].RecoveryTime = tp->RecoveryTime;
    PMtypeProperties[typ].effectivePDE = tp->effectivePDE;
}
*/
bool pms::removePMtype(int itype)
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

void pms::appendNewPMtype(PMtypeClass *tp)
{
  PMtypes.append(tp);
}

int pms::findPMtype(QString typeName)
{
  for (int i=0; i<PMtypes.size(); i++)
    if (PMtypes.at(i)->name == typeName) return i;
  return -1;
}

void pms::clearPMtypes()
{
  for (int i=0; i<PMtypes.size(); i++) delete PMtypes[i];
  PMtypes.resize(0);
}

void pms::replaceType(int itype, PMtypeClass *newType) {delete PMtypes[itype]; PMtypes[itype] = newType;}

void pms::updateTypePDE(int typ, QVector<double> *x, QVector<double> *y) {PMtypes[typ]->PDE_lambda = *x; PMtypes[typ]->PDE = *y;}

void pms::scaleTypePDE(int typ, double factor)
{
    for (int i=0; i<PMtypes[typ]->PDE.size(); i++)
        PMtypes[typ]->PDE[i] *= factor;
}

void pms::updateTypeAngular(int typ, QVector<double> *x, QVector<double> *y) {PMtypes[typ]->AngularSensitivity_lambda = *x; PMtypes[typ]->AngularSensitivity = *y;}

void pms::updateTypeAngularN1(int typ, double val) {PMtypes[typ]->n1 = val;}

void pms::updateTypeArea(int typ, QVector<QVector<double> > *vec, double xStep, double yStep){PMtypes[typ]->AreaSensitivity = *vec; PMtypes[typ]->AreaStepX = xStep; PMtypes[typ]->AreaStepY = yStep;}

void pms::clearTypeArea(int typ){PMtypes[typ]->AreaSensitivity.resize(0);}

void pms::setElChanSPePHS(int ipm, QVector<double> *x, QVector<double> *y)
{
    SPePHS_x[ipm] = *x;
    SPePHS[ipm] = *y;

//    double sumPro = 0;
//    double sum = 0;
//    for (int i=0; i<SPePHS[ipm].size(); i++)
//    {
//      sum += SPePHS[ipm][i];
//      sumPro += SPePHS[ipm][i]*SPePHS_x[ipm][i];
//    }
//    AverageSignalPerPhotoelectron[ipm] = sumPro/sum; //zero check is in the loader
}

void pms::preparePHS(int ipm)
{
  if (SPePHShist[ipm])
    {
      delete SPePHShist[ipm];
      SPePHShist[ipm] = 0;
    }

  int size = SPePHS_x[ipm].size();
  if (size < 1) return;

  SPePHShist[ipm] = new TH1D("SPePHS"+ipm,"SPePHS", size, SPePHS_x[ipm].at(0), SPePHS_x[ipm].at(size-1));
  for (int j = 1; j<size+1; j++)  SPePHShist[ipm]->SetBinContent(j, SPePHS[ipm].at(j-1));
  SPePHShist[ipm]->GetIntegral(); //to make thread safe
}

void pms::preparePHSs()
{
  for (int ipm=0; ipm<numPMs; ipm++) preparePHS(ipm);
}

void pms::prepareMCcrosstalk()
{
  for (int ipm=0; ipm<numPMs; ipm++)
     prepareMCcrosstalkForPM(ipm);
}

void pms::prepareMCcrosstalkForPM(int ipm)
{
    delete PMs[ipm].MCsampl; PMs[ipm].MCsampl = 0;

    if (fDoMCcrosstalk && PMs[ipm].MCmodel==0)
      PMs[ipm].MCsampl = new ACustomRandomSampling(RandGen, &PMs.at(ipm).MCcrosstalk);
}

void pms::clearSPePHS(int ipm)
{
    SPePHS_x[ipm].clear();
    SPePHS[ipm].clear();
    if (SPePHShist[ipm])
    {
        delete SPePHShist[ipm];
        SPePHShist[ipm] = 0;
    }
}

void pms::setSPePHSmode(int ipm, int mode) //0-const, 1-normal, 2-gamma, 3-custom
{
  SPePHSmode[ipm] = mode;
  if (mode != 3) clearSPePHS(ipm);
}

void pms::setADC(int ipm, double max, int bits)
{
  ADCmax[ipm] = max;
  ADCbits[ipm] = bits;

  ADClevels[ipm] = TMath::Power(2, bits) - 1;
    ADCstep[ipm] = max / ADClevels[ipm];
}

void pms::updateADClevels()
{
  for (int ipm=0; ipm<numPMs; ipm++)
  {
      double max = ADCmax[ipm];
      int bits = ADCbits[ipm];

      ADClevels[ipm] = TMath::Power(2, bits) - 1;
      if (ADClevels[ipm] == 0)
           ADCstep[ipm] = 0;
      else ADCstep[ipm] = max / ADClevels[ipm];
  }
}

void pms::CopySPePHSdata(int ipmFrom, int ipmTo)
{
    AverageSignalPerPhotoelectron[ipmTo] = AverageSignalPerPhotoelectron[ipmFrom];
    SPePHSmode[ipmTo] = SPePHSmode[ipmFrom];
    SPePHSsigma[ipmTo] = SPePHSsigma[ipmFrom];
    SPePHSshape[ipmTo] = SPePHSshape[ipmFrom];

    if (SPePHS_x[ipmFrom].size()>0)
    {
        SPePHS_x[ipmTo] = SPePHS_x[ipmFrom];
        SPePHS[ipmTo] = SPePHS[ipmFrom];
        SPePHShist[ipmTo] = new TH1D(*SPePHShist[ipmFrom]);
    }
}

void pms::CopyMCcrosstalkData(int ipmFrom, int ipmTo)
{
    PMs[ipmTo].MCcrosstalk = PMs[ipmFrom].MCcrosstalk;
    PMs[ipmTo].MCmodel = PMs[ipmFrom].MCmodel;
    PMs[ipmTo].MCtriggerProb = PMs[ipmFrom].MCtriggerProb;
}

void pms::CopyElNoiseData(int ipmFrom, int ipmTo)
{
    PMs[ipmTo].ElNoiseSigma = PMs.at(ipmFrom).ElNoiseSigma;
}

void pms::CopyADCdata(int ipmFrom, int ipmTo)
{
    ADCmax[ipmTo] = ADCmax[ipmFrom];
    ADCbits[ipmTo] = ADCbits[ipmFrom];
    ADCstep[ipmTo] = ADCstep[ipmFrom];
}

void pms::ScaleSPePHS(int ipm, double gain)
{
    double NowAverage = getAverageSignalPerPhotoelectron(ipm);
    if (NowAverage == gain) return; //nothing to change

    if (fabs(gain) > 1e-20) gain /= NowAverage; else gain = 0;

    int mode = getSPePHSmode(ipm); //0 - use average value; 1 - normal distr; 2 - Gamma distr; 3 - custom distribution

    if (mode < 3) setAverageSignalPerPhotoelectron(ipm, NowAverage * gain);
    else if (mode == 3)
    {
        //custom SPePHS - have to adjust the distribution
        QVector<double> x = *getSPePHS_x(ipm);
        QVector<double> y = *getSPePHS(ipm);
        for (int ix=0; ix<x.size(); ix++) x[ix] *= gain;

        setElChanSPePHS(ipm, &x, &y);
    }
}

void pms::CalculateElChannelsStrength()
{
    for (int ipm = 0; ipm<PMs.size(); ipm++)
    {
        double factor = PMs[ipm].relElStrength;
        ScaleSPePHS(ipm, factor);
    }
}

double pms::GenerateSignalFromOnePhotoelectron(int ipm)
{
    switch (SPePHSmode[ipm])
    {
      case 0: return AverageSignalPerPhotoelectron[ipm];
      case 1: return RandGen->Gaus(AverageSignalPerPhotoelectron[ipm], SPePHSsigma[ipm]);
      case 2: return GammaRandomGen->getGamma(SPePHSshape[ipm], AverageSignalPerPhotoelectron[ipm]/SPePHSshape[ipm]);
      case 3:
       {
         if (SPePHShist[ipm]) return SPePHShist[ipm]->GetRandom();
         //else return AverageSignalPerPhotoelectron[ipm];
         else return 0;
       }
      default:;
    }

    qWarning() << "Error: unrecognized type in signal per photoelectron generation";
    return 0;
}

void pms::RebinPDEsForType(int typ)
{
    if (WavelengthResolved)
    {
        if (PMtypes[typ]->PDE_lambda.size() == 0) PMtypes[typ]->PDEbinned.clear();
        else
            ConvertToStandardWavelengthes(&PMtypes[typ]->PDE_lambda, &PMtypes[typ]->PDE, WaveFrom, WaveStep, WaveNodes, &PMtypes[typ]->PDEbinned);
    }
    else PMtypes[typ]->PDEbinned.clear();
}

void pms::RebinPDEsForPM(int ipm)
{
    if (WavelengthResolved)
    {
        if (PDE_lambda[ipm].size() == 0) PDEbinned[ipm].clear();
        else
            ConvertToStandardWavelengthes(&PDE_lambda[ipm], &PDE[ipm], WaveFrom, WaveStep, WaveNodes, &PDEbinned[ipm]);
    }
    else PDEbinned[ipm].clear();
}

void pms::RebinPDEs()
{
    //for all base PM types
       // qDebug() << "WaveFrom,Nodes,Step"<< WaveFrom<<WaveNodes<<WaveStep;
    for (int i=0; i<PMtypes.size(); i++) pms::RebinPDEsForType(i);
    //for all PMs
    for (int i=0; i<numPMs; i++) pms::RebinPDEsForPM(i);
}

void pms::RecalculateAngular()
{
    //for all base PM types
    for (int i=0; i<countPMtypes(); i++) pms::RecalculateAngularForType(i);
    //for all PMs
    for (int i=0; i<numPMs; i++) pms::RecalculateAngularForPM(i);
}

void pms::RecalculateAngularForType(int typ)
{
    if (!AngularResolved || PMtypes[typ]->AngularSensitivity_lambda.size() == 0)
        PMtypes[typ]->AngularSensitivityCosRefracted.clear();
    else
    {
        //transforming degrees to cos(Refracted)
        QVector<double> x, y;
        double n1 = PMtypes[typ]->n1;
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
            //double Rs = (n1*cosI-n2*cosT)*(n1*cosI-n2*cosT) / (n1*cosI+n2*cosT)/(n1*cosI+n2*cosT);
            //double Rp = (n1*cosT-n2*cosI)*(n1*cosT-n2*cosI) / (n1*cosT+n2*cosI)/(n1*cosT+n2*cosI);
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
        x.replace(0, x[1]); //replace fith last reliable value
        y.replace(0, y[1]);
        //reusing the function:
        ConvertToStandardWavelengthes(&x, &y, 0, 1.0/(CosBins-1), CosBins, &PMtypes[typ]->AngularSensitivityCosRefracted);
    }
}

void pms::RecalculateAngularForPM(int ipm)
{
    if (!AngularResolved || AngularSensitivity_lambda[ipm].size() == 0)
        AngularSensitivityCosRefracted[ipm].clear();
    else
    {
        int typ = PMs[ipm].type;
        //transforming degrees to cos(Refracted)
        QVector<double> x, y;
        double n1 = AngularN1[ipm];
        double n2 = (*MaterialCollection)[PMtypes[typ]->MaterialIndex]->n;
        //      qDebug()<<"n1 n2 "<<n1<<n2;
        for (int i=AngularSensitivity_lambda[ipm].size()-1; i>=0; i--) //if i++ - data span from 1 down to 0
        {
            double Angle = AngularSensitivity_lambda[ipm][i] * TMath::Pi()/180.0;
            //calculating cos of the transmitted angle
            double sinI = sin(Angle);
            double cosI = cos(Angle);

            double sinT = sinI * n1 / n2;
            double cosT = sqrt(1 - sinT*sinT);
            x.append(cosT);

            //         qDebug()<<"Angle: "<<Angle<< " sinI="<<sinI<<" cosI="<<cosI;
            //         qDebug()<<"sinT="<<sinT<<"cosT="<<cosT;

            //correcting for the reflection loss
            //double Rs = (n1*cosI-n2*cosT)*(n1*cosI-n2*cosT) / (n1*cosI+n2*cosT)/(n1*cosI+n2*cosT);
            //double Rp = (n1*cosT-n2*cosI)*(n1*cosT-n2*cosI) / (n1*cosT+n2*cosI)/(n1*cosT+n2*cosI);
            double Rs = (n1*cosI-n2*cosT)*(n1*cosI-n2*cosT) / ( (n1*cosI+n2*cosT) * (n1*cosI+n2*cosT) );
            double Rp = (n1*cosT-n2*cosI)*(n1*cosT-n2*cosI) / ( (n1*cosT+n2*cosI) * (n1*cosT+n2*cosI) );
            double R = 0.5*(Rs+Rp);
            //         qDebug()<<"Rs Rp"<<Rs<<Rp;
            //         qDebug()<<"Reflection "<<R<<"ReflectionforNormal="<<(n1-n2)*(n1-n2)/(n1+n2)/(n1+n2);

            //const double correction = R < 0.99999 ? 1.0/(1.0-R) : 1.0; //Why is it that here it's not this one too?
            double correction = 1.0/(1.0-R);    //T = (1-R)
            //meaning: have to take into account that during measurements part of the light was reflected

            y.append(AngularSensitivity[ipm][i]*correction);
            //         qDebug()<<cosT<<correction<<AngularSensitivity[ipm][i]*correction;
        }
        x.replace(0, x[1]); //replace fith last reliable value
        y.replace(0, y[1]);
        //reusing the function:
        ConvertToStandardWavelengthes(&x, &y, 0, 1.0/(CosBins-1), CosBins, &AngularSensitivityCosRefracted[ipm]);
    }
}

/*
void pms::setADCbitsForType(int typ, int bits)
{
  PMtypeProperties[typ].ADCbits = bits;
  int levels = TMath::Power(2, bits)-1;
  PMtypeProperties[typ].ADCstep = PMtypeProperties[typ].MaxSignal / levels;
}
*/

double pms::getActualPDE(int ipm, int WaveIndex)
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
        //WavelengthResolved is false or no spectral data during photon generation

        if (effectivePDE[ipm] != -1) PDE = effectivePDE[ipm]; //override exists
        else PDE = PMtypes[PMs[ipm].type]->effectivePDE;
    }
    else
    {
        //Wave-resolved AND there is proper waveindex

        //if wave-resolved override exists, use it:
        if (!PDEbinned[ipm].isEmpty()) PDE = PDEbinned[ipm][WaveIndex];
        else
        {
            //if effective PDE is overriden, use it
            if (effectivePDE[ipm] != -1) PDE = effectivePDE[ipm];
            else
            {
                //if type hold wave-resolved info, use it
                if (PMtypes[PMs[ipm].type]->PDEbinned.size() > 0) PDE = PMtypes[PMs[ipm].type]->PDEbinned[WaveIndex];
                else PDE = PMtypes[PMs[ipm].type]->effectivePDE; //last resort :)
            }
        }
    }
    //  qDebug()<<"reporting PDE of "<<PDE;

    return PDE;
}

double pms::getActualAngularResponse(int ipm, double cosAngle)
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
        if (AngularSensitivityCosRefracted[ipm].size() > 0)
        {
            //using overrides
            int bin = cosAngle  *(CosBins-1);
            AngularResponse = AngularSensitivityCosRefracted[ipm][bin];
        }
        else
        {
            //do we have angular response defined for this pm type?
            if (PMtypes[PMs[ipm].type]->AngularSensitivityCosRefracted.size() > 0)
            {
                //using base type data
                int bin = cosAngle  *(CosBins-1);
                AngularResponse = PMtypes[PMs[ipm].type]->AngularSensitivityCosRefracted[bin];
            }
        }
    }

    //    qDebug()<<"Return value = "<<AngularResponse;
    return AngularResponse;
}

double pms::getActualAreaResponse(int ipm, double x, double y)
{
    /*
    qDebug()<<"--------------";
    qDebug()<<"AreaResolved: "<<AreaResolved<<" x= "<<x<<" y= "<<y<<"  PM# "<<ipm;
    */

    double AreaResponse = 1.0;
    int iX, iY;
    double xStep, yStep;
    int xNum, yNum;

    if (AreaResolved)
    {
        //area sensitivity is activated as the simulation option

        //are there override data?
        if (AreaSensitivity[ipm].size() > 0)
        {
            //use override
            xStep = AreaStepX[ipm];
            yStep = AreaStepY[ipm];
            xNum = AreaSensitivity[ipm].size();
            yNum = AreaSensitivity[ipm][0].size();
            //iX = (x + 0.5*xNum*xStep)/xStep;
            //iY = (y + 0.5*yNum*yStep)/yStep;
            iX = x/xStep + 0.5*xNum;
            iY = y/yStep + 0.5*yNum;
            //            qDebug()<<"Xbin= "<<iX<<"   Ybin= "<<iY;
            if (iX<0 || iX>xNum-1 || iY<0 || iY>yNum-1)
            {
                //outside
                AreaResponse = 0;
            }
            else AreaResponse = AreaSensitivity[ipm][iX][iY];
        }
        else
        {
            PMtypeClass *typ = PMtypes[PMs[ipm].type];
            //are there data for this PM type?
            if (typ->AreaSensitivity.size() > 0)
            {
                //use type data
                xStep = typ->AreaStepX;
                yStep = typ->AreaStepY;
                int xNum = typ->AreaSensitivity.size();
                int yNum = typ->AreaSensitivity[0].size();
                //iX = (x + 0.5*xNum*xStep)/xStep;
                //iY = (y + 0.5*yNum*yStep)/yStep;
                iX = x/xStep + 0.5*xNum;
                iY = y/yStep + 0.5*yNum;
                //               qDebug()<<"Xbin= "<<iX<<"   Ybin= "<<iY;
                if (iX<0 || iX>xNum-1 || iY<0 || iY>yNum-1)
                {
                    //outside
                    AreaResponse = 0;
                }
                else AreaResponse = PMtypes[PMs[ipm].type]->AreaSensitivity[iX][iY];
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

/*
int pms::getGroupByDescription(QString desc)
{
  for (int igroup = 0; igroup<PMgroupDescription.size(); igroup++)
      if (PMgroupDescription.at(igroup) == desc) return igroup;

  return -1; //not found
}

int pms::definePMgroup(QString desc)
{
    PMgroupDescription.append(desc);
    return PMgroupDescription.size()-1;
}

void pms::clearPMgroups()
{
    PMgroupDescription.resize(1);

    for (int i=0; i<numPMs; i++)
    {
        PMs[i].group = 0;
        PMs[i].relGain = 1.0;
    }
}

bool pms::isPMgroup0empty()
{
    for (int ipm=0; ipm<numPMs; ipm++)
        if (PMs[ipm].group == 0) return false;

    return true;
}
*/

QVector<QPair<double, int> > pms::getPMsSortedByR() const
{
    QVector<QPair<double, int> > snake(numPMs);
    for (int ipm = 0; ipm < numPMs; ipm++)
        snake[ipm] = qMakePair(hypot(PMs[ipm].x, PMs[ipm].y), ipm);
    qSort(snake); // sort by R
    return snake;
}

void pms::writePMtypesToJson(QJsonObject &json)
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

bool pms::readPMtypesFromJson(QJsonObject &json)
{
  if (!json.contains("PMtypes"))
    {
      ErrorString = "No PM types info foind in json!";
      qWarning()<<ErrorString;
      return false;
    }

  QJsonArray ar = json["PMtypes"].toArray();
  clearPMtypes();//PMtypes.clear();
  for (int i=0; i<ar.size(); i++)
    {
      PMtypes.append(new PMtypeClass());
      QJsonObject js = ar[i].toObject();
      PMtypes.last()->readFromJson(js);
    }
  return (PMtypes.size()>0);
}

bool pms::saveCoords(const QString &filename)
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

bool pms::saveAngles(const QString &filename)
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

bool pms::saveTypes(const QString &filename)
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

bool pms::saveUpperLower(const QString &filename)
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

int pms::getPixelsX(int ipm) const{return PMtypes[PMs[ipm].type]->PixelsX;}

int pms::getPixelsY(int ipm) const{return PMtypes[PMs[ipm].type]->PixelsY;}

double pms::SizeX(int ipm) const {return PMtypes[PMs[ipm].type]->SizeX;}

double pms::SizeY(int ipm) const {return PMtypes[PMs[ipm].type]->SizeY;}

double pms::SizeZ(int ipm) const {return PMtypes[PMs[ipm].type]->SizeZ;}

bool pms::isSiPM(int ipm) const {return PMtypes[PMs[ipm].type]->SiPM;}

bool pms::isPDEwaveOverriden() const
{
  if (PDE.size() != numPMs)
    {
      qCritical() << "PDE size:"<<AngularSensitivity.size();
      exit(-1);
    }
  for (int i=0; i<numPMs; i++)
    if (!PDE[i].isEmpty()) return true;
  return false;
}

bool pms::isPDEeffectiveOverriden() const
{
  for (int i=0; i<numPMs; i++)
    if (effectivePDE.at(i) != -1) return true;
  return false;
}

void pms::writePDEeffectiveToJson(QJsonObject &json)
{
  QJsonArray arr;
  for (int i=0; i<numPMs; i++) arr.append(effectivePDE[i]);
  json["PDEeffective"] = arr;
}

void pms::writeRelQE_PDE(QJsonObject &json)
{
//  bool fFound = false;
//  for (int i=0; i<numPMs; i++)
//    {
//      if (PMs.at(i).relQE_PDE !=1 || PMs.at(i).relElStrength !=1)
//        {
//          fFound = true;
//          break;
//        }
//    }
//  if (!fFound) return;

  QJsonArray arr;
  for (int i=0; i<numPMs; i++)
    {
      QJsonArray el;
      el << PMs.at(i).relQE_PDE << PMs.at(i).relElStrength;
      arr.append(el);
    }
  json["RelQEandElStr"] = arr;
}

void pms::readRelQE_PDE(QJsonObject &json)
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

bool pms::readPDEeffectiveFromJson(QJsonObject &json)
{
  if (json.contains("PDEeffective"))
    {
      QJsonArray arr = json["PDEeffective"].toArray();
      if (arr.size() != numPMs)
        {
          qWarning() << "PDE json: size mismatch!";
          return false;
        }
      for (int i=0; i<numPMs; i++) effectivePDE[i] = arr[i].toDouble();
    }
  return true;
}

void pms::writePDEwaveToJson(QJsonObject &json)
{
  QJsonArray arr;
  for (int ipm=0; ipm<numPMs; ipm++)
    {
      QJsonArray arpm;
      writeTwoQVectorsToJArray(PDE_lambda[ipm], PDE[ipm], arpm);
      arr.append(arpm);
    }
  json["PDEwave"] = arr;
}

bool pms::readPDEwaveFromJson(QJsonObject &json)
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
        readTwoQVectorsFromJArray(array, PDE_lambda[ipm], PDE[ipm]);
      }
    }
  return true;
}

bool pms::isAngularOverriden() const
{
  if (AngularSensitivity.size() != numPMs)
    {
      qWarning() << "AngularSensitivity size:"<<AngularSensitivity.size();
      exit(-1);
    }
  for (int i=0; i<numPMs; i++)
    if (!AngularSensitivity[i].isEmpty()) return true;
  return false;
}

void pms::writeAngularToJson(QJsonObject &json)
{
  QJsonObject js;
  QJsonArray arr;
  for (int ipm=0; ipm<numPMs; ipm++)
    {
      QJsonArray arpm;
      writeTwoQVectorsToJArray(AngularSensitivity_lambda[ipm], AngularSensitivity[ipm], arpm);
      arr.append(arpm);
    }
  js["AngularResponse"] = arr;
  QJsonArray arrn;
  for (int i=0; i<numPMs; i++) arrn.append(AngularN1[i]);
  js["RefrIndex"] = arrn;
  json["Angular"] = js;
}

bool pms::readAngularFromJson(QJsonObject &json)
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
        readTwoQVectorsFromJArray(array, AngularSensitivity_lambda[ipm], AngularSensitivity[ipm]);
      }
      QJsonArray arrn = js["RefrIndex"].toArray();
      if (arrn.size() != numPMs)
        {
          qWarning() << "RefrIndex json: size mismatch!";
          return false;
        }
      for (int i=0; i<numPMs; i++) AngularN1[i] = arrn[i].toDouble();
    }
  return true;
}

bool pms::isAreaOverriden() const
{
  if (AreaSensitivity.size() != numPMs)
    {
      qWarning() << "AreaSensitivity size:"<<AreaSensitivity.size();
      exit(-1);
    }
  for (int i=0; i<numPMs; i++)
    if (!AreaSensitivity[i].isEmpty()) return true;
  return false;
}

void pms::writeAreaToJson(QJsonObject &json)
{
  QJsonObject js;
  QJsonArray arr;
  for (int ipm=0; ipm<numPMs; ipm++)
    {
      QJsonArray ar;
      write2DQVectorToJArray(AreaSensitivity[ipm], ar);
      arr.append(ar);
    }
  js["AreaResponse"] = arr;
  QJsonArray as;
  for (int ipm=0; ipm<numPMs; ipm++)
    {
      QJsonArray ar;
      ar.append(AreaStepX.at(ipm));
      ar.append(AreaStepY.at(ipm));
      as.append(ar);
    }
  js["Step"] = as;
  json["Area"] = js;
}

bool pms::readAreaFromJson(QJsonObject &json)
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
        read2DQVectorFromJArray(array, AreaSensitivity[ipm]);
      }
      QJsonArray arrn = js["Step"].toArray();
      if (arrn.size() != numPMs)
        {
          qWarning() << "RefrIndex json: size mismatch!";
          return false;
        }
      for (int i=0; i<numPMs; i++) AngularN1[i] = arrn[i].toDouble();
    }
  return true;
}
