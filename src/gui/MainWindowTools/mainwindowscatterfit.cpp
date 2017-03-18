#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "graphwindowclass.h"
#include "detectorclass.h"
#include "phscatclaudiomodel.h"
#include "amaterialparticlecolection.h"
#include "globalsettingsclass.h"
#include "simulationmanager.h"
#include "outputwindow.h"
#include "asandwich.h"
#include "slab.h"
#include "ageoobject.h"
#include "eventsdataclass.h"
#include "amessage.h"
#include "afiletools.h"

#include <QDebug>
#include <QFileDialog>
#include <QDateTime>
#include <QLineEdit>

#include "TMath.h"
#include "Math/Functor.h"
#include "Minuit2/Minuit2Minimizer.h"
#include "TH2D.h"
#include "TH2.h"
#include "TGraphErrors.h"
#include "TGraph2D.h"

static QVector<double> LXeH, ExpSignals, ExpUncert;
bool fUseExperUncert;

struct FitRecStruct
{
  double albedo, n2, abs, scale;
  double chi2;
  FitRecStruct(double albedo, double n2, double abs, double scale, double chi2) :
    albedo(albedo), n2(n2), abs(abs), scale(scale), chi2(chi2){}
};

static QList<FitRecStruct> FitResults;
//static TH2D* histChi2AN;
static int VizMode = 0;
static TGraph2D gr2D;
static TGraph gr1D;
static double bestAlb, bestN2, bestAbs, bestScale;
static double bestChi2 = 1e30;
static QDateTime bestTime;

static bool fAbort = false;

double TestFunctor(const double *p)
//parameter 0 is albedo
//parameter 1 is refractive index of Teflon  (mat index 3)
//parameter 2 is absorption coefficient for LXe  (mat index 0)
//parameter 3 is scaling factor
//parameter 4 is the MainWindow pointer
{
  if (fAbort) return 1e30;

  void *thisvalue;
  memcpy(&thisvalue, &p[4], sizeof(void *));
  MainWindow* MW = (MainWindow*)thisvalue;

  qDebug() << "Functor call (A, N, Abs, Scale):"<<p[0]<<p[1]<<p[2]<<p[3];

  //NAUS Detector: LXe mat index = 0,  Teflon mat index = 3
  (*MW->Detector->MpCollection)[0]->abs = p[2];
  (*MW->Detector->MpCollection)[3]->n = p[1];
  PhScatClaudioModel* ov = dynamic_cast<PhScatClaudioModel*>( (*MW->Detector->MpCollection)[0]->OpticalOverrides[3] );
  if (!ov)
    {
      qDebug() << "Override object NOT found!";
      fAbort = true;
      return 1e30;
    }
  ov->albedo = p[0];
  //reconstruct detector in the cycle

  //run simulation cycle for all LXe height
  double chi2 = 0;
  for (int iXeLev=0; iXeLev<LXeH.size(); iXeLev++)
    {
      MW->fSimDataNotSaved = false; //the best candidate for the ready flag

      //layer #5 is PrScint - this is the LXe level!
      MW->setFloodZposition(-0.5*LXeH[iXeLev]+0.05);
      //MW->SandwichLayers[5].Height = LXeH[iXeLev];
      //MW->SandwichLayers[5].updateGui();
   //MW->Detector->Sandwich->Slabs[1]->height = LXeH[iXeLev];  ***!!!
      AGeoObject* obj = MW->Detector->Sandwich->World->findObjectByName("PrScint");
      obj->getSlabModel()->height = LXeH[iXeLev];
      obj->UpdateFromSlabModel(obj->getSlabModel());
      //emit MW->Detector->Sandwich->RequestGuiUpdate();
      MW->ReconstructDetector();
      qDebug() << "----Setting height to"<<LXeH[iXeLev]<<"and phot bombs position to"<<-0.5*LXeH[iXeLev]+0.05;
      qApp->processEvents();

      MW->on_pbSimulate_clicked();
      do
        {
          qApp->processEvents();          
          if (MW->SimulationManager->Runner->isFinished() && !MW->SimulationManager->Runner->wasSuccessful())
            {
              qDebug()<<"Sim failed or aborted by user!";
              fAbort = true;
              return 1e30;
            }
        }
      while (MW->fSimDataNotSaved == false);
      //qDebug() << "Sim completed!";

      //collecting statistics
      int size = MW->EventsDataHub->Events.size();
      if (size == 0)
        {
          qDebug() << "Cannot continue: Simulation did not return any events!";
          fAbort = true;
          return 1e30;
        }
//      double average = 0;
//      for (int i=0; i<size; i++)
//        average += MW->EventsDataHub->Events[i].at(0); //0th pm
//      average /= size;

      double S = 0;
      double S2 = 0;
      for (int i=0; i<size; i++)
        {
          double sig = MW->EventsDataHub->Events[i].at(0); //0th pm
          S += sig;
          S2 += sig*sig;
        }
      double average = S / size;
      double SimSigma2;
      if (size>4)
        {
          SimSigma2 = 1.0/(size-1)*(S2 - 2.0*average*S + average*average*size); //sigma2 of the set
          SimSigma2 /= size; //uncertainty2 of the mean
        }
      else
        SimSigma2 = average;

      double ExpSigma2 = (fUseExperUncert) ? ExpUncert[iXeLev]*ExpUncert[iXeLev]*p[3]*p[3] : 0;
      double sigma2 = SimSigma2 + ExpSigma2;

      double exper = p[3]*ExpSignals[iXeLev]; //scaled experimental signal
      double chi2Contrib = (average-exper) * (average-exper) / sigma2;
      qDebug() << "--Av sim signal:"<<average<<"  Scaled exper signal:"<<exper<<"  SimSigma:"<<sqrt(SimSigma2)<< "ExpSigma:"<<sqrt(ExpSigma2) << "chi2 increment:"<<chi2Contrib;
      chi2 += chi2Contrib;
    }

  qDebug() << "-Chi2 obtained:"<<chi2;
  FitResults.append(FitRecStruct(p[0], p[1], p[2], p[3], chi2));

  MW->on_cobSF_chi2Vs_activated(VizMode);

  if (chi2<bestChi2)
    {
      bestChi2 = chi2;
      bestAlb = p[0];
      bestN2 = p[1];
      bestAbs = p[2];
      bestScale = p[3];
      bestTime = QDateTime::currentDateTime();

      QString s = "New best chi2 (" + QString::number(bestChi2)+ ") found at " + bestTime.toString("H:m:s")+"\n";
      s+= "Albedo = " + QString::number(bestAlb) +
          "  N2 = " + QString::number(bestN2) +
          "  Abs = " + QString::number(bestAbs) +
          "  Scale = " + QString::number(bestScale) +
          "\n";
      MW->Owindow->OutText(s);
    }
  return chi2;
}

bool ReadValues(QLineEdit* le, double* p, int size)
{
  QString inS = le->text().simplified();
  QStringList inL = inS.split("/", QString::SkipEmptyParts);
  qDebug() << inL.size()<<inL;
  if (inL.size() != size) return false;
//      message("Input line format: Start / Step / Min / Max", le->parent());
  for (int i=0; i<size; i++)
    {
      bool ok;
      p[i] = inL[i].toDouble(&ok);
      if (!ok) return false;
    }
  return true;
}

void MainWindow::on_pbTestFit_clicked()
{
  AGeoObject* obj = Detector->Sandwich->World->findObjectByName("PrScint");
  if (!obj || !obj->getSlabModel())
  {
      message("Cannot find PrScint slab", this);
      return;
  }

  //reading parameters
  double albP[4], nP[4], absP[4], scP[4];
  bool ok = ReadValues(ui->leSF_albedo, albP, 4);
  if (!ok)
    {
      message("Albedo input error! Line format: Start / Step / Min / Max", this);
      return;
    }
  ok = ReadValues(ui->leSF_nT, nP, 4);
  if (!ok)
    {
      message("Nteflon input error! Line format: Start / Step / Min / Max", this);
      return;
    }
  ok = ReadValues(ui->leSF_abs, absP, 4);
  if (!ok)
    {
      message("Absorption input error! Line format: Start / Step / Min / Max", this);
      return;
    }
  ok = ReadValues(ui->leSF_sc, scP, 4);
  if (!ok)
    {
      message("Scale input error! Line format: Start / Step / Min / Max", this);
      return;
    }


  fAbort = false;
  fUseExperUncert = ui->cbSF_useUncertExpData->isChecked();
  bestChi2 = 1e30;
  VizMode = ui->cobSF_chi2Vs->currentIndex();

  //configure sim
  ui->tabwidMain->setCurrentIndex(1);
  ui->twSourcePhotonsParticles->setCurrentIndex(0);
  ui->twSingleScan->setCurrentIndex(2);

  //Loading "experimental" data
  QString FileName = QFileDialog::getOpenFileName(this, "Load data file", GlobSet->LastOpenDir, "Data files (*.txt *.dat);;All files (*)");
  if (FileName.isEmpty()) return;
  qApp->processEvents();

  if (fUseExperUncert)
    {
      if (LoadDoubleVectorsFromFile(FileName, &LXeH, &ExpSignals, &ExpUncert) != 0)
        {
          qDebug() << "Data input error";
          return;
        }
      qDebug() << "Input data:";
      for (int i=0; i<LXeH.size(); i++)
        qDebug() << LXeH[i]<<ExpSignals[i]<<ExpUncert[i];
      qDebug() << "----------------------------";
    }
  else
    {
      if (LoadDoubleVectorsFromFile(FileName, &LXeH, &ExpSignals) != 0)
        {
          qDebug() << "Data input error";
          return;
        }
      qDebug() << "Input data:";
      for (int i=0; i<LXeH.size(); i++)
        qDebug() << LXeH[i]<<ExpSignals[i];
      qDebug() << "----------------------------";
    }

  FitResults.clear();

  //making a copy of det and sim config
  QJsonObject json;
  writeDetectorToJson(json);
  writeSimSettingsToJson(json, true);

  //Creating ROOT minimizer
  ROOT::Minuit2::Minuit2Minimizer *RootMinimizer = new ROOT::Minuit2::Minuit2Minimizer(ROOT::Minuit2::kSimplex);//(ROOT::Minuit2::kMigrad);
  RootMinimizer->SetMaxFunctionCalls(500);
  RootMinimizer->SetMaxIterations(500);
 // RootMinimizer->SetTolerance(0.01);
  RootMinimizer->SetPrintLevel(1);
  // 1 standard
  // 2 try to improve minimum (slower)
  RootMinimizer->SetStrategy(1);

  ROOT::Math::Functor *Funct = new ROOT::Math::Functor(&TestFunctor, 5);
  RootMinimizer->SetFunction(*Funct);
  //setting up variables   -  start step min max
  RootMinimizer->SetLimitedVariable(0, "Alb",  albP[0], albP[1], albP[2], albP[3]); //albedo
  RootMinimizer->SetLimitedVariable(1, "Ntef", nP[0],   nP[1],   nP[2],   nP[3]); //refr index of teflon
  RootMinimizer->SetLimitedVariable(2, "Abs",  absP[0], absP[1], absP[2], absP[3]); //absorption coefficient in LXe
  RootMinimizer->SetLimitedVariable(3, "Scale",scP[0],  scP[1],  scP[2],  scP[3]); //Scaling factor - applied to experimental data
  //prepare to transfer pointer
  double dPoint;
  void *thisvalue = this;
  memcpy(&dPoint, &thisvalue, sizeof(void *));
  //We need to fix for the possibility that double isn't enough to store void*
  RootMinimizer->SetFixedVariable(4, "p", dPoint);
  qDebug() << "Minimizer created and configured";

  // do the minimization
  bool fOK = RootMinimizer->Minimize();
  fOK = fOK && !fAbort;

  //report results
  qDebug()<<"Minimization success? "<<fOK;
  QString str;
  if (fOK)
    {
      str = "Minimization success\n";
      const double *xs = RootMinimizer->X();
      str += "Albedo: " + QString::number(xs[0])+"\n";
      str += "Refr index: " + QString::number(xs[1])+"\n";
      str += "Absorption coefficient: " + QString::number(xs[2])+"\n";
      str += "Scaling factor: " + QString::number(xs[3])+"\n";
    }
  else
    {
      str = "Minimization failed\n";
      if (bestChi2<1e30)
        {
          str += "Best configuration found during search:\n";
          str += "Albedo: " + QString::number(bestAlb)+"\n";
          str += "Refr index: " + QString::number(bestN2)+"\n";
          str += "Absorption coefficient: " + QString::number(bestAbs)+"\n";
          str += "Scaling factor: " + QString::number(bestScale)+"\n";
          str += "Chi2: " + QString::number(bestChi2)+"\n";
        }
    }
  Owindow->SetTab(0);
  Owindow->showNormal();
  Owindow->OutText(str);

  //loading back settings of det and sim
  fSimDataNotSaved = false;
  readDetectorFromJson(json);
  readSimSettingsFromJson(json);
}

void MainWindow::on_cobSF_chi2Vs_activated(int index)
{
  if (FitResults.size()<2) return;

  switch (index)
    {
    case 0: return;
    case 1:
      {
        int gs = gr1D.GetN();
        for (int i=gs-1; i>-1; i--)
          gr1D.RemovePoint(i);
        for (int i=0; i<FitResults.size(); i++)
          gr1D.SetPoint(i, i, FitResults[i].chi2);
        gr1D.GetXaxis()->SetTitle("Iteration#");
        gr1D.GetYaxis()->SetTitle("Chi2");
        gr1D.SetMarkerStyle(20);
        GraphWindow->Draw(&gr1D, "APL", true, false);
        break;
      }
    case 2:
      {
        int gs = gr2D.GetN();
        for (int i=gs-1; i>-1; i--)
          gr2D.RemovePoint(i);
        for (int i=0; i<FitResults.size(); i++)
          gr2D.SetPoint(i, FitResults[i].albedo, FitResults[i].n2, FitResults[i].chi2);
        gr2D.GetXaxis()->SetTitle("Albedo");
        gr2D.GetYaxis()->SetTitle("n2");
        gr2D.GetZaxis()->SetTitle("Chi2");
        gr2D.SetMarkerStyle(20);
        GraphWindow->Draw(&gr2D, "pcolz", true, false);
        break;
      }
    case 3:
      {
        int gs = gr2D.GetN();
        for (int i=gs-1; i>-1; i--)
          gr2D.RemovePoint(i);
        for (int i=0; i<FitResults.size(); i++)
          gr2D.SetPoint(i, FitResults[i].albedo, FitResults[i].abs, FitResults[i].chi2);
        gr2D.GetXaxis()->SetTitle("Albedo");
        gr2D.GetYaxis()->SetTitle("Abs, mm-1");
        gr2D.GetZaxis()->SetTitle("Chi2");
        gr2D.SetMarkerStyle(20);
        GraphWindow->Draw(&gr2D, "pcol", true, false);
        break;
      }
    case 4:
      {
        int gs = gr2D.GetN();
        for (int i=gs-1; i>-1; i--)
          gr2D.RemovePoint(i);
        for (int i=0; i<FitResults.size(); i++)
          gr2D.SetPoint(i, FitResults[i].n2, FitResults[i].abs, FitResults[i].chi2);
        gr2D.GetXaxis()->SetTitle("n2");
        gr2D.GetYaxis()->SetTitle("Abs, mm-1");
        gr2D.GetZaxis()->SetTitle("Chi2");
        gr2D.SetMarkerStyle(20);
        GraphWindow->Draw(&gr2D, "pcol", true, false);
        break;
      }
    }
}

void MainWindow::on_pbSF_SvsL_clicked()
{
  //NAUS Detector: LXe mat index = 0,  Teflon mat index n2 = 3  (n1->n2 mat: 0->3)
  if (
       (*Detector->MpCollection).countMaterials()<1 ||
       (*Detector->MpCollection)[0]->OpticalOverrides.size()<4
      )
    {
      message("Material collection is not ompatible, make sure you use \"naus\" detector example!", this);
      return;
    }

  //input string processing
  QString inS = ui->leSF_parameter->text().simplified();
  QStringList inL = inS.split(",", QString::SkipEmptyParts);
  qDebug() << inL.size()<<inL;
  if (inL.size()!=4)
    {
      message("Input line format: Parameter, From, To, Step", this);
      return;
    }
  QList<QString> parameters;
  //============== list of parameter names ==============
  parameters << "len" << "n1" << "n2" << "alb" << "sigA" << "sigS" << "abs" << "ref";
  QString parName = inL[0];
  if (!parameters.contains(parName))
    {
      QString str;
      for (int i=0; i<parameters.size(); i++) str += parameters[i]+"\n";
      message("Unknown parameter! Defined parameters are:\n"+str, this);
      return;
    }
  bool ok;
  double from, to, step;
  from = inL[1].toDouble(&ok);
  if (!ok)
    {
      message("Format error of the \"From\" value", this);
      return;
    }
  to = inL[2].toDouble(&ok);
  if (!ok)
    {
      message("Format error of the \"To\" value", this);
      return;
    }
  step = inL[3].toDouble(&ok);
  if (!ok)
    {
      message("Format error of the \"Step\" value", this);
      return;
    }
  if (from>to)
    {
      message("From is larger than To!", this);
      return;
    }
  qDebug() << from << to << step;
  int steps = 1+(0.0001+to-from)/step;
  qDebug() << "Steps:"<<steps;

  PhScatClaudioModel* ovTef = dynamic_cast<PhScatClaudioModel*>( (*Detector->MpCollection)[0]->OpticalOverrides[3] );
  if (parName=="alb" || parName=="sigA" || parName == "sigS")
   if (!ovTef)
    {
      message("Override object NOT found! Make sure you use \"naus\" detector example", this);
      return;
    }
  BasicOpticalOverride* ovSt = dynamic_cast<BasicOpticalOverride*>( (*Detector->MpCollection)[0]->OpticalOverrides[2] );
  if (parName=="ref")
   if (!ovSt)
    {
      message("Override object NOT found! Make sure you use \"naus\" detector example", this);
      return;
    }

   //making a copy of det and sim config
   QJsonObject json;
   writeDetectorToJson(json);
   writeSimSettingsToJson(json, true);

   QVector<double> vP, vS, vSigma;
   TGraphErrors *gr = 0;

   for (int iZ=0; iZ<steps; iZ++)
     {
       fSimDataNotSaved = false; //the best candidate for the ready flag

       //the addresses change each iteration after sim
       PhScatClaudioModel* ovTef = dynamic_cast<PhScatClaudioModel*>( (*Detector->MpCollection)[0]->OpticalOverrides[3] );
       BasicOpticalOverride* ovSt = dynamic_cast<BasicOpticalOverride*>( (*Detector->MpCollection)[0]->OpticalOverrides[2] );

       double parVal = from + step*iZ;
       qDebug() << "----Setting parameter value to"<<parVal;
       vP << parVal;

       if (parName == "len")
         {
           //layer #5 is PrScint - this is the LXe level!
           setFloodZposition(-0.5*parVal+0.05);
           //SandwichLayers[5].Height = parVal;
           //SandwichLayers[5].updateGui();
//           Detector->Sandwich->Slabs[1]->height = parVal;    ***!!!
           AGeoObject* obj = Detector->Sandwich->World->findObjectByName("PrScint");
           obj->getSlabModel()->height = parVal;
           obj->UpdateFromSlabModel(obj->getSlabModel());
           //emit Detector->Sandwich->RequestGuiUpdate();
         }
       else if (parName == "n1")
         (*Detector->MpCollection)[0]->n = parVal;
       else if (parName == "n2")
         (*Detector->MpCollection)[3]->n = parVal;
       else if (parName == "alb")
          ovTef->albedo = parVal;
       else if (parName == "sigA")
          ovTef->sigma_alpha = parVal;
       else if (parName == "sigS")
          ovTef->sigma_h = parVal;
       else if (parName == "abs")
          (*Detector->MpCollection)[0]->abs = parVal;
       else if (parName == "ref")
         {
          if (parVal>1) parVal = 1;
          if (parVal<0) parVal = 0;
          ovSt->probRef = parVal;
          ovSt->probLoss = 1.0 - parVal;
         }

       ReconstructDetector();
       qApp->processEvents();

       on_pbSimulate_clicked();
       do
         {
           qApp->processEvents();
           if (SimulationManager->Runner->isFinished() && !SimulationManager->Runner->wasSuccessful())
             {
               message("Sim failed!", this);
               return;
             }
         }
       while (fSimDataNotSaved == false);
       //qDebug() << "Sim completed!";

       //collecting statistics
       int size = EventsDataHub->Events.size();
       if (size == 0)
         {
           message("Simulation did not return any events!", this);
           return;
         }
       double S = 0;
       double S2 = 0;
       for (int i=0; i<size; i++)
         {
           double sig = EventsDataHub->Events[i].at(0); //0th pm
           S += sig;
           S2 += sig*sig;
         }
       double average = S / size;

       //double sigma = (size>9) ? sqrt(1.0/(size-1)*(S2 - 2.0*average*S + average*average*size)) : 0;
       double sigma2;
       if (size>4)
         {
           sigma2 = 1.0/(size-1)*(S2 - 2.0*average*S + average*average*size); //sigma2 of the set
           sigma2 /= size; //uncertainty2 of the mean
         }
       else
         sigma2 = 0;
       double sigma = sqrt(sigma2);

       vS << average;
       vSigma << sigma;
       qDebug() << parVal << average << sigma;

       if (iZ>0)
         {
           if (gr) delete gr;
           TGraphErrors *gr = new TGraphErrors(iZ+1, vP.data(), vS.data(),   0, vSigma.data());
           gr->GetXaxis()->SetTitle(parName.toLatin1().data());
           gr->GetYaxis()->SetTitle("Signal");
           gr->SetTitle("Signal with sigma");
           gr->SetMarkerColor(4);
           gr->SetLineColor(4);
           gr->SetMarkerStyle(20);
           gr->SetEditable(false);
           GraphWindow->Draw(gr,"ALP");
           qApp->processEvents();
         }
     }

   //loading back settings of det and sim
   fSimDataNotSaved = false;
   readDetectorFromJson(json);
   readSimSettingsFromJson(json);
}

void MainWindow::on_pbSF_help_clicked()
{
  QString str =
      "Format: Parameter_name, StartValue, EndValue, Step\n"
      "Parameter names:\n"
      "len  - Camera length\n"
      "sigA - SigmaAlpha\n"
      "sigS - SigmaSpike\n"
      "alb  - albedo\n"
      "n1   - Refractive index of LXe\n"
      "n2   - Refractive index of teflon\n"
      "abs  - Absorption in LXe\n"
      "ref  - Reflection coeff of steel\n";
  message(str, this);
}

static double bestTeAbs;
double TestFunctorSimplistic(const double *p)
//parameter 0 is TeflonAbs (mat index 3)
//parameter 1 is absorption coefficient for LXe  (mat index 0)
//parameter 2 is scaling factor
//parameter 3 is the MainWindow pointer
{
  if (fAbort) return 1e30;

  void *thisvalue;
  memcpy(&thisvalue, &p[3], sizeof(void *));
  MainWindow* MW = (MainWindow*)thisvalue;

  qDebug() << "Functor call (TeAbs, Abs, Scale):"<<p[0]<<p[1]<<p[2];

  //NAUS Detector: LXe mat index = 0,  Teflon mat index = 3
  (*MW->Detector->MpCollection)[0]->abs = p[1];
  BasicOpticalOverride* ov = dynamic_cast<BasicOpticalOverride*>( (*MW->Detector->MpCollection)[0]->OpticalOverrides[3] );
  if (!ov)
    {
      qDebug() << "Override object of LXe->Teflon(simplistic model) NOT found!";
      fAbort = true;
      return 1e30;
    }
  ov->probLoss = p[0];
  ov->probDiff = 1.0-p[0];
  ov->probRef = 0;
  //reconstruct detector in the cycle

  //run simulation cycle for all LXe height
  double chi2 = 0;
  for (int iXeLev=0; iXeLev<LXeH.size(); iXeLev++)
    {
      MW->fSimDataNotSaved = false; //the best candidate for the ready flag

      //layer #5 is PrScint - this is the LXe level!
      MW->setFloodZposition(-0.5*LXeH[iXeLev]+0.05);
      //MW->SandwichLayers[5].Height = LXeH[iXeLev];
      //MW->SandwichLayers[5].updateGui();
//      MW->Detector->Sandwich->Slabs[1]->height = LXeH[iXeLev];    ***!!!
      AGeoObject* obj = MW->Detector->Sandwich->World->findObjectByName("PrScint");
      obj->getSlabModel()->height = LXeH[iXeLev];
      obj->UpdateFromSlabModel(obj->getSlabModel());
      //emit MW->Detector->Sandwich->RequestGuiUpdate();
      MW->ReconstructDetector();
      qDebug() << "----Setting height to"<<LXeH[iXeLev]<<"and phot bombs position to"<<-0.5*LXeH[iXeLev]+0.05;
      qApp->processEvents();

      MW->on_pbSimulate_clicked();
      do
        {
          qApp->processEvents();
          if (MW->SimulationManager->Runner->isFinished() && !MW->SimulationManager->Runner->wasSuccessful())
            {
              qDebug()<<"Sim failed or aborted by user!";
              fAbort = true;
              return 1e30;
            }
        }
      while (MW->fSimDataNotSaved == false);
      //qDebug() << "Sim completed!";

      //collecting statistics
      int size = MW->EventsDataHub->Events.size();
      if (size == 0)
        {
          qDebug() << "Cannot continue: Simulation did not return any events!";
          fAbort = true;
          return 1e30;
        }
//      double average = 0;
//      for (int i=0; i<size; i++)
//        average += MW->EventsDataHub->Events[i].at(0); //0th pm
//      average /= size;
      double S = 0;
      double S2 = 0;
      for (int i=0; i<size; i++)
        {
          double sig = MW->EventsDataHub->Events[i].at(0); //0th pm
          S += sig;
          S2 += sig*sig;
        }
      double average = S / size;

      //double sigma2 = (size>9) ? 1.0/(size-1)*(S2 - 2.0*average*S + average*average*size) : average;
      double SimSigma2;
      if (size>4)
        {
          SimSigma2 = 1.0/(size-1)*(S2 - 2.0*average*S + average*average*size); //sigma2 of the set
          SimSigma2 /= size; //uncertainty2 of the average
        }
      else
        SimSigma2 = average;

      double ExpSigma2 = (fUseExperUncert) ? ExpUncert[iXeLev]*ExpUncert[iXeLev]*p[2]*p[2] : 0;
      double sigma2 = SimSigma2 + ExpSigma2;

      double exper = p[2]*ExpSignals[iXeLev]; //scaled experimental signal
      double chi2Contrib = (average-exper) * (average-exper) / sigma2;
      qDebug() << "--Av sim signal:"<<average<<"  Scaled exper signal:"<<exper<<"  SimSigma:"<<sqrt(SimSigma2)<< "ExpSigma:"<<sqrt(ExpSigma2) << "chi2 increment:"<<chi2Contrib;
      chi2 += chi2Contrib;
    }

  qDebug() << "-Chi2 obtained:"<<chi2;
  //FitResults.append(FitRecStruct(p[0], p[1], p[2], p[3], chi2));
  //MW->on_cobSF_chi2Vs_activated(VizMode);

  if (chi2<bestChi2)
    {
      bestChi2 = chi2;
      bestTeAbs = p[0];
      bestAbs = p[1];
      bestScale = p[2];
      bestTime = QDateTime::currentDateTime();

      QString s = "New best chi2 (" + QString::number(bestChi2)+ ") found at " + bestTime.toString("H:m:s")+"\n";
      s+= "  TeflonAbs = " + QString::number(bestTeAbs) +
          "  LXeAbs = " + QString::number(bestAbs) +
          "  Scale = " + QString::number(bestScale) +
          "\n";
      MW->Owindow->OutText(s);
    }
  return chi2;
}

void MainWindow::on_pbTestFit_simplistic_clicked()
{
    AGeoObject* obj = Detector->Sandwich->World->findObjectByName("PrScint");
    if (!obj || !obj->getSlabModel())
    {
        message("Cannot find PrScint slab", this);
        return;
    }

  //reading parameters
  double teAbsP[4], lxeAbsP[4], scP[4];
  bool ok = ReadValues(ui->leSF_TeflonAbs, teAbsP, 4);
  if (!ok)
    {
      message("Te abs input error! Line format: Start / Step / Min / Max", this);
      return;
    }
  ok = ReadValues(ui->leSF_LXeAbs_simpl, lxeAbsP, 4);
  if (!ok)
    {
      message("LXe absorption input error! Line format: Start / Step / Min / Max", this);
      return;
    }
  ok = ReadValues(ui->leSF_sc_simpl, scP, 4);
  if (!ok)
    {
      message("Scale input error! Line format: Start / Step / Min / Max", this);
      return;
    }


  fAbort = false;
  bestChi2 = 1e30;
  fUseExperUncert = ui->cbSF_useUncertExpData_Simp->isChecked();
  //VizMode = ui->cobSF_chi2Vs->currentIndex();

  //configure sim
  ui->tabwidMain->setCurrentIndex(1);
  ui->twSourcePhotonsParticles->setCurrentIndex(0);
  ui->twSingleScan->setCurrentIndex(2);

  //Loading "experimental" data
  QString FileName = QFileDialog::getOpenFileName(this, "Load data file - uncertainties are NOT taken into account!", GlobSet->LastOpenDir, "Data files (*.txt *.dat);;All files (*)");
  if (FileName.isEmpty()) return;
  qApp->processEvents();

  if (fUseExperUncert)
    {
      if (LoadDoubleVectorsFromFile(FileName, &LXeH, &ExpSignals, &ExpUncert) != 0)
        {
          qDebug() << "Data input error";
          return;
        }
      qDebug() << "Input data:";
      for (int i=0; i<LXeH.size(); i++)
        qDebug() << LXeH[i]<<ExpSignals[i]<<ExpUncert[i];
      qDebug() << "----------------------------";
    }
  else
    {
      if (LoadDoubleVectorsFromFile(FileName, &LXeH, &ExpSignals) != 0)
        {
          qDebug() << "Data input error";
          return;
        }
      qDebug() << "Input data:";
      for (int i=0; i<LXeH.size(); i++)
        qDebug() << LXeH[i]<<ExpSignals[i];
      qDebug() << "----------------------------";
    }

  FitResults.clear();

  //making a copy of det and sim config
  QJsonObject json;
  writeDetectorToJson(json);
  writeSimSettingsToJson(json, true);

  //Creating ROOT minimizer
  ROOT::Minuit2::Minuit2Minimizer *RootMinimizer = new ROOT::Minuit2::Minuit2Minimizer(ROOT::Minuit2::kSimplex);//(ROOT::Minuit2::kMigrad);
  RootMinimizer->SetMaxFunctionCalls(500);
  RootMinimizer->SetMaxIterations(500);
 // RootMinimizer->SetTolerance(0.01);
  RootMinimizer->SetPrintLevel(1);
  // 1 standard
  // 2 try to improve minimum (slower)
  RootMinimizer->SetStrategy(1);

  ROOT::Math::Functor *Funct = new ROOT::Math::Functor(&TestFunctorSimplistic, 4);
  RootMinimizer->SetFunction(*Funct);
  //setting up variables   -  start step min max
  RootMinimizer->SetLimitedVariable(0, "TeAbs",teAbsP[0], teAbsP[1], teAbsP[2], teAbsP[3]); //TeAbs in override
  RootMinimizer->SetLimitedVariable(1, "Abs",  lxeAbsP[0],lxeAbsP[1],lxeAbsP[2],lxeAbsP[3]); //absorption coefficient in LXe
  RootMinimizer->SetLimitedVariable(2, "Scale",scP[0],    scP[1],    scP[2],    scP[3]); //Scaling factor - applied to experimental data
  //prepare to transfer pointer
  double dPoint;
  void *thisvalue = this;
  memcpy(&dPoint, &thisvalue, sizeof(void *));
  //We need to fix for the possibility that double isn't enough to store void*
  RootMinimizer->SetFixedVariable(3, "p", dPoint);
  qDebug() << "Minimizer created and configured";

  // do the minimization
  bool fOK = RootMinimizer->Minimize();
  fOK = fOK && !fAbort;

  //report results
  qDebug()<<"Minimization success? "<<fOK;
  QString str;
  if (fOK)
    {
      str = "Minimization success\n";
      const double *xs = RootMinimizer->X();
      str += "Teflon abs: " + QString::number(xs[0])+"\n";
      str += "Absorption coefficient: " + QString::number(xs[1])+"\n";
      str += "Scaling factor: " + QString::number(xs[2])+"\n";
    }
  else
    {
      str = "Minimization failed\n";
      if (bestChi2<1e30)
        {
          str += "Best configuration found during search:\n";
          str += "Teflon abs: " + QString::number(bestTeAbs)+"\n";
          str += "LXe absorption coefficient: " + QString::number(bestAbs)+"\n";
          str += "Scaling factor: " + QString::number(bestScale)+"\n";
          str += "Chi2: " + QString::number(bestChi2)+"\n";
        }
    }
  Owindow->SetTab(0);
  Owindow->showNormal();
  Owindow->OutText(str);

  //loading back settings of det and sim
  fSimDataNotSaved = false;
  readDetectorFromJson(json);
  readSimSettingsFromJson(json);
}
