//ANTS2 modules and windows
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "checkupwindowclass.h"
#include "amaterialparticlecolection.h"
#include "apmhub.h"
#include "outputwindow.h"
#include "detectorclass.h"
#include "eventsdataclass.h"
#include "graphwindowclass.h"
#include "globalsettingsclass.h"
#include "scatteronmetal.h"
#include "phscatclaudiomodel.h"
#include "geometrywindowclass.h"
#include "aphoton.h"
#include "acompton.h"
#include "atrackrecords.h"
#include "aenergydepositioncell.h"
#include "amessage.h"
#include "atracerstateful.h"

#include "TVirtualGeoTrack.h"
#include "TLegend.h"
#include "TGeoManager.h"
#include "TMath.h"
#include "TGraph.h"
#include "TH1D.h"
#include "TRandom2.h"

//Qt
#include <QDebug>
#include <QObject>
#include <QFileDialog>

static QVector<TrackHolderClass> tracks;
static TVector3 NormViz;

//#include "TBufferJSON.h"
//#include "tmpobjhubclass.h"
//#include "TAttMarker.h"

//#include "TH1D.h"
//#include "TGraph.h"
void MainWindow::on_pobTest_clicked()
{
//    double tau1 = 100.0;
//    double tau2 = 100.0;
//    int num = 10000000;

//    TH1D* h = new TH1D("h1", "", 1000, 0, 1000);
//    for (int i=0; i<num; i++)
//    {
//        double t = -1.0 * tau1 * log(1.0 - Detector->RandGen->Rndm());
//        t += Detector->RandGen->Exp(tau2);
//        h->Fill(t);
//    }
//    GraphWindow->Draw(h);
//    GraphWindow->AddCurrentToBasket("exp_pdf");

//    TGraph* g = new TGraph();
//    double a = tau1 * tau2 / (tau1 + tau2);
//    for (double t=0; t<1000; t++)
//    {
//        double f = 1.0 - (tau1 + tau2)/tau2 * exp(-t/tau2 ) + tau1/tau2 * exp(-t/a);
//        g->SetPoint(t, t, f*num);
//    }
//    GraphWindow->Draw(g, "AL");
//    GraphWindow->AddCurrentToBasket("formula");


//    QByteArray ba;
//    EventsDataHub->packEventsToByteArray(0, EventsDataHub->countEvents(), ba);

//    EventsDataHub->clear();
//    EventsDataHub->unpackEventsFromByteArray(ba);

//    ////////////////////////////////////////////////////////////////////
//    QDataStream in(ba);
//    QString st;
//    int events;
//    int numPMs;
//    in >> st;
//    in >> events;
//    in >> numPMs;

//    QVector<float> otherVector;
//    in >> otherVector; //load
//    qDebug() << st << events << numPMs;
//    qDebug() << otherVector;


//   if (TmpHub->ChPerPhEl_Peaks.size() != TmpHub->ChPerPhEl_Sigma2.size()) return;
//   if (TmpHub->ChPerPhEl_Peaks.size() == 0) return;

//   TGraph* g = new TGraph;
//   for (int ipm = 0; ipm < TmpHub->ChPerPhEl_Peaks.size(); ipm++)
//   {
//       g->SetPoint(ipm, TmpHub->ChPerPhEl_Peaks.at(ipm), TmpHub->ChPerPhEl_Sigma2.at(ipm));
//   }
//   GraphWindow->Draw(g, "A*");
}

//include <QElapsedTimer>
void MainWindow::on_pobTest_2_clicked()
{

//    std::vector<mydata> v;
//    v.push_back( mydata(1.0, 100.0) );

//    QElapsedTimer t;
//    t.start();
//    for (int i=0; i<1000000; i++)
//        RandomNTime(100.0, v);
//    int el = t.elapsed();
//    qDebug() << el;



//    QByteArray ba;
//    EventsDataHub->packReconstructedToByteArray(ba);

//    EventsDataHub->resetReconstructionData(0);
//    EventsDataHub->unpackReconstructedFromByteArray(0, EventsDataHub->countEvents(), ba);
//    EventsDataHub->fReconstructionDataReady = true;
//    emit EventsDataHub->requestEventsGuiUpdate();

//    if (TmpHub->ChPerPhEl_Peaks.size() != TmpHub->ChPerPhEl_Sigma2.size()) return;
//    if (TmpHub->ChPerPhEl_Peaks.size() == 0) return;

//    TGraph* g1 = new TGraph();
//    TGraph* g2 = new TGraph();
//    for (int ipm = 0; ipm < TmpHub->ChPerPhEl_Peaks.size(); ipm++)
//    {
//        g1->SetPoint(ipm, ipm, TmpHub->ChPerPhEl_Peaks.at(ipm));
//        g2->SetPoint(ipm, ipm, TmpHub->ChPerPhEl_Sigma2.at(ipm));
//    }
//    g1->SetMarkerStyle(4);  //round
//    g2->SetMarkerStyle(3);  //star
//    g1->SetMarkerColor(4);  //blue
//    g2->SetMarkerColor(2);  //red

//    g1->SetTitle("From peaks");
//    g2->SetTitle("From stat");

//    GraphWindow->Draw(g1, "AP");
//    GraphWindow->Draw(g2, "P same");
}

/*
//#include "TPrincipal.h"
//#include "TBrowser.h"
{
  if (!EventsDataHub->isTimed())
    {
      message("There are no timed data!", this);
      return;
    }
  ///QVector< QVector < QVector <float> > > TimedEvents; //event timebin pm

  // we are looking for correlation between different PMs! //not compatible with loaded energy files
  int m = EventsDataHub->TimedEvents[0].size();       //number of data points = time bins
  int n = EventsDataHub->TimedEvents[0][0].size();    //number of variables = number of PMs
  qDebug() << "Number of variables (PMs):"<<n<<"  Number of data points (time bins):"<<m;

  //TPrincipal* principal = new TPrincipal(n,"ND");
  TPrincipal* principal = new TPrincipal(n,"D");

  Double_t* data = new Double_t[n];

  TMatrixD Input(m, n); ///Andr    //(Int_t nrows, Int_t ncols); |m  -n
  for (int i = 0; i < m; i++)  //cycle on points (i.e. over time bins)
    {
      QString test = "Time bin# " + QString::number(i)+"> ";
      for (int j = 0; j < n; j++) //cycle on variables (i.e. PMs)
        {
          data[j] = EventsDataHub->TimedEvents[0][i][j];
          Input[i][j] = data[j]; ///Andr
          test += QString::number(data[j]) + " ";
        }
      qDebug() << test;

      principal->AddRow(data);
    }
  delete [] data;

    // Do the actual analysis
    principal->MakePrincipals();

    // Print out the result on
    principal->Print();

    // Test the PCA
    principal->Test();


    // Make some histograms of the orginal, principal, residue, etc data
    //principal->MakeHistograms("name","XPE");

    // Start a browser, so that we may browse the histograms generated above
    //TBrowser* b = new TBrowser("principalBrowser", principal);

    const TMatrixD* EigenVectors = principal->GetEigenVectors();
    //qDebug() << "Eigen Vectors"<< (*EigenVectors)[0][0] << (*EigenVectors)[0][1] << (*EigenVectors)[0][2];
    //qDebug() << "Eigen Vectors"<< (*EigenVectors)[1][0] << (*EigenVectors)[1][1] << (*EigenVectors)[1][2];
    //qDebug() << "Eigen Vectors"<< (*EigenVectors)[2][0] << (*EigenVectors)[2][1] << (*EigenVectors)[2][2];

    const TVectorD * MeanValues = principal->GetMeanValues();
    //qDebug() << "Mean values:"<< (*MeanValues)[0]<<(*MeanValues)[1]<<(*MeanValues)[2];
    for (int i=0; i<n; i++)
      {
        for (int j=0; j<m; j++)
          Input[j][i] -= (*MeanValues)[i];
      }

    Input *= *EigenVectors;

    for (int iprin=0; iprin<n; iprin++)
      {
        if (iprin>5) break; //too much on the same graph
        QVector<double> x, y;
        for (int i=0; i<m; i++)
          {
            x.append(i);
            y.append(Input[i][iprin]);
          }
        if (iprin==0) GraphWindow->ShowGraph(&x, &y, 1, "Time", "", 2);
        else GraphWindow->ShowGraph(&x, &y, 1+iprin, "", "", 2, 1, 1, 2, "same");
      }

}
*/

void MainWindow::on_pbST_AngleCos_clicked()
{
  //vector photon dir
  TVector3 ph(ui->ledST_i->text().toDouble(), ui->ledST_j->text().toDouble(), ui->ledST_k->text().toDouble());
  ph = ph.Unit();

  //vector surface normal
  TVector3 surf(-ui->ledST_si->text().toDouble(), -ui->ledST_sj->text().toDouble(), -ui->ledST_sk->text().toDouble());
  surf = surf.Unit();

  double cos = ph*surf;
  double ang = 180.0/3.1415926*acos(cos);

  ui->pbST_AngleCos->setText("Theta="+QString::number(ang, 'g', 3)+"  cos="+QString::number(cos, 'g', 3));
}

AOpticalOverride* InitOverride(Ui::MainWindow *ui, AMaterialParticleCollection& MatCol)
{
  MatCol.AddNewMaterial(true);
  MatCol.AddNewMaterial(true);
  MatCol[0]->n = ui->ledST_Ref1->text().toDouble();
  MatCol[1]->n = ui->ledST_Ref2->text().toDouble();
  double wave = ui->ledST_wave->text().toDouble();
  MatCol.SetWave(false, wave, wave, 0, 1); //conversion of iWave=-1 to wavelength will give wave

  if (ui->cobOverrideTesterModel->currentIndex() == 0)
    {
      //Claudio's model
      PhScatClaudioModel* sc;
      switch (ui->cobST_ClaudioModelVersion->currentIndex())
        {
        case 0:
          sc = new PhScatClaudioModelV2d2(&MatCol, 0, 1);
          break;
        case 1:
          sc = new PhScatClaudioModelV2d1(&MatCol, 0, 1);
          break;
        case 2:
          sc = new PhScatClaudioModelV2(&MatCol, 0, 1);
          break;
        default:
          message("Unexpected scattering model index");
          exit(666);
        }

      sc->sigma_alpha = ui->ledST_SigmaAlpha->text().toDouble();
      sc->sigma_h = ui->ledST_SigmaSpike->text().toDouble();
      sc->albedo = ui->ledST_albedo->text().toDouble();

      switch(ui->conST_heightModel->currentIndex())
        {
          case 0: sc->HeightDistribution = empirical; break;
          case 1: sc->HeightDistribution = gaussian; break;
          case 2: sc->HeightDistribution = exponential; break;
        default: exit(-123);
        }
      switch(ui->cobST_slopeModel->currentIndex())
        {
          case 0: sc->SlopeDistribution = trowbridgereitz; break;
          case 1: sc->SlopeDistribution = cooktorrance; break;
          case 2: sc->SlopeDistribution = bivariatecauchy; break;
        default: exit(-123);
        }
      return sc;
    }
  else if (ui->cobOverrideTesterModel->currentIndex() == 1)
    {
      //DielectricToMetal model
      ScatterOnMetal* sm = new ScatterOnMetal(&MatCol, 0, 1);
      sm->RealN = ui->ledST_Ref2->text().toDouble();
      sm->ImaginaryN = ui->ledImNAfter->text().toDouble();
      return sm;
    }
  else
    {
      //FS_NP (Former Neves) model
      FSNPOpticalOverride* nm = new FSNPOpticalOverride(&MatCol, 0, 1);
      nm->Albedo = ui->ledST_AlbedoNeves->text().toDouble();
      return nm;
    }
}

void MainWindow::on_pbCSMtestmany_clicked()
{
  // Have to create a fake material collection with two materials
  // to provide refractive indices and iwave->wavelength conversion to the override class
  AMaterialParticleCollection MatCol;
  //creating and filling override object and this fake material collection
  AOpticalOverride* ov = InitOverride(ui, MatCol);

  //surface normal and photon direction
  TVector3 SurfNorm(ui->ledST_si->text().toDouble(),
                    ui->ledST_sj->text().toDouble(),
                    ui->ledST_sk->text().toDouble());
  SurfNorm = SurfNorm.Unit();
  NormViz = SurfNorm; //visualization of tracks
  SurfNorm = - SurfNorm; //In ANTS2 navigator provides normal in the opposite direction
  double N[3];
  N[0] = SurfNorm.X();
  N[1] = SurfNorm.Y();
  N[2] = SurfNorm.Z();
  TVector3 PhotDir(ui->ledST_i->text().toDouble(),
                   ui->ledST_j->text().toDouble(),
                   ui->ledST_k->text().toDouble());
  PhotDir = PhotDir.Unit();
  //double K[3];

  //report configuration
  ov->printConfiguration(-1);

  //other inits
  tracks.clear();
  double d = 0.5; //offset - for drawing only
  tracks.append(TrackHolderClass(10, kRed)); //incoming photon track
  tracks.last().Nodes.append(TrackNodeStruct(d, d ,d, 0));
  tracks.last().Nodes.append(TrackNodeStruct(d-PhotDir.X(), d-PhotDir.Y(), d-PhotDir.Z(), 0));

  //preparing and running cycle with photons
  int abs, spike, lobe, lamb;
  abs = spike = lobe = lamb = 0;
  TH1D* hist1;
  bool fDegrees = ui->cbST_cosToDegrees->isChecked();
  if (fDegrees) hist1 = new TH1D("statScatter", "Angle_scattered", 100, 0, 0);
  else          hist1 = new TH1D("statScatter", "Cos_scattered", 100, 0, 0);

  APhoton ph;
  ph.SimStat = new ASimulationStatistics();

  ATracerStateful Resources;
  Resources.RandGen = Detector->RandGen;
  for (int i=0; i<ui->sbST_number->value(); i++)
    {
      ph.v[0] = PhotDir.X(); //old has output direction after full cycle!
      ph.v[1] = PhotDir.Y();
      ph.v[2] = PhotDir.Z();
      ph.waveIndex = -1;
      ov->calculate(Resources, &ph, N);

      Color_t col;
      Int_t type;
      if (ov->Status == AOpticalOverride::Absorption)
        {
          abs++;
          continue;
        }
      else if (ov->Status == AOpticalOverride::SpikeReflection)
        {
          spike++;
          type = 0;
          col = 6; //0,magenta for Spike
        }
      else if (ov->Status == AOpticalOverride::LobeReflection)
        {
          lobe++;
          type = 1;
          col = 7; //1,teal for Lobe
        }
      else if (ov->Status == AOpticalOverride::LambertianReflection)
        {
          lamb++;
          type = 2;
          col = 3; //2,grean for lambert
        }
      else
        {
          type = 666;
          col = kBlue; //blue for error
        }

      tracks.append(TrackHolderClass(type, col));
      tracks.last().Nodes.append(TrackNodeStruct(d,d,d, 0));
      tracks.last().Nodes.append(TrackNodeStruct(d+ph.v[0], d+ph.v[1], d+ph.v[2], 0));

      double costr = -SurfNorm[0]*ph.v[0] -SurfNorm[1]*ph.v[1] -SurfNorm[2]*ph.v[2];

      if (fDegrees) hist1->Fill(180.0/3.1415926535*acos(costr));
      else          hist1->Fill(costr);
    }
  //show cos angle hist
  GraphWindow->Draw(hist1);
  //show tracks
  MainWindow::on_pbST_showTracks_clicked();
  //show stat of processes
  int sum = abs + spike + lobe + lamb;
  QString str = QString::number(abs) + "/" +
      QString::number(spike) + "/" +
      QString::number(lobe) + "/" +
      QString::number(lamb);
  if (sum>0)
    {
      str += "   (" + QString::number(1.0*abs/sum, 'g', 3) + "/" +
            QString::number(1.0*spike/sum, 'g', 3) + "/" +
            QString::number(1.0*lobe/sum, 'g', 3) + "/" +
            QString::number(1.0*lamb/sum, 'g', 3) + ")";
    }
  ui->leST_out->setText(str);
  delete ov;
  delete ph.SimStat;
}

void MainWindow::on_pbST_uniform_clicked()
{
  //   Have to create a fake material collection with two materials
  // to provide refractive indices and iwave->wavelength conversion to the override class
  AMaterialParticleCollection MatCol;
  //creating and filling override object and this fake material collection
  AOpticalOverride* ov = InitOverride(ui, MatCol);

  TVector3 SurfNorm(ui->ledST_si->text().toDouble(),
                    ui->ledST_sj->text().toDouble(),
                    ui->ledST_sk->text().toDouble());
  SurfNorm = SurfNorm.Unit();
  SurfNorm = - SurfNorm; //In ANTS2 navigator provides normal in the opposite direction
  double N[3];
  N[0] = SurfNorm.X();
  N[1] = SurfNorm.Y();
  N[2] = SurfNorm.Z();

  //TVector3 PhotDir(ui->ledST_i->text().toDouble(),
  //                 ui->ledST_j->text().toDouble(),
  //                 ui->ledST_k->text().toDouble());
  //PhotDir = PhotDir.Unit();
  double K[3];

  ov->printConfiguration(-1);

  int abs, spike, lobe, lamb;
  abs = spike = lobe = lamb = 0;
  TH1D* hist1;
  bool fDegrees = ui->cbST_cosToDegrees->isChecked();
  if (fDegrees) hist1 = new TH1D("statScatter", "Angle_scattered", 100, 0, 0);
  else          hist1 = new TH1D("statScatter", "Cos_scattered", 100, 0, 0);

  int num = ui->sbST_number->value();

  APhoton ph;
  ph.SimStat = new ASimulationStatistics();

  ATracerStateful Resources;
  Resources.RandGen = Detector->RandGen;
  for (int i=0; i<num; i++)
    {
      //diffuse illumination - lambertian is used
      double sin2angle = Detector->RandGen->Rndm();
      double angle = asin(sqrt(sin2angle));
      double yOff = cos(angle), zOff = -sin(angle);
      N[0] = 0; N[1] = 0; N[2] = -1;   // convention of photon tracer - normal is med1->med2
      K[0] = 0; K[1] = yOff; K[2] = zOff;  // -z direction on xy plane (incidence angle from 90 to 0)


      ph.v[0] = K[0];
      ph.v[1] = K[1];
      ph.v[2] = K[2];
      ph.waveIndex = -1;
      ov->calculate(Resources, &ph, N);

      if (ov->Status == AOpticalOverride::Absorption)
        {
          abs++;
          continue;
        }
      else if (ov->Status == AOpticalOverride::SpikeReflection)
          spike++;
      else if (ov->Status == AOpticalOverride::LobeReflection)
          lobe++;
      else if (ov->Status == AOpticalOverride::LambertianReflection)
          lamb++;
      else
        {
          qCritical()<<"Unknown process!";
          exit(666);
        }

      double costr = -N[0]*K[0] -N[1]*K[1] -N[2]*K[2];  // after scatter, K will be in positive Z direction
      if (fDegrees) hist1->Fill(180.0/3.1415926535*acos(costr));
      else          hist1->Fill(costr);
    }

  //show cos angle hist
  GraphWindow->Draw(hist1);

  //show stat of processes
  int sum = abs + spike + lobe + lamb;
  QString str = "Scat probability: "+QString::number(1.0*(sum-abs)/num, 'g', 4) + "  Processes:"+
      QString::number(abs) + "/" +
      QString::number(spike) + "/" +
      QString::number(lobe) + "/" +
      QString::number(lamb);
  if (sum>0)
    {
      str += "   (" + QString::number(1.0*abs/sum, 'g', 3) + "/" +
            QString::number(1.0*spike/sum, 'g', 3) + "/" +
            QString::number(1.0*lobe/sum, 'g', 3) + "/" +
            QString::number(1.0*lamb/sum, 'g', 3) + ")";
    }
  ui->leST_out->setText(str);
  delete ov;
  delete ph.SimStat;
}

void MainWindow::on_pbST_RvsAngle_clicked()
{
  // Have to create a fake material collection with two materials
  // to provide refractive indices and iwave->wavelength conversion to the override class
  AMaterialParticleCollection MatCol;
  //creating and filling override object and this fake material collection
  AOpticalOverride* ov = InitOverride(ui, MatCol);

  int num = ui->sbST_number->value();
  QVector<double> Spike(91, 0), Lobe(91, 0), Diffuse(91, 0), Total(91, 0), Angle;
  double N[3], K[3];
  N[0] = 0;
  N[1] = 0;
  N[2] = 1.0;

  APhoton ph;
  ph.SimStat = new ASimulationStatistics();

  ATracerStateful Resources;
  Resources.RandGen = Detector->RandGen;
  for (int iA=0; iA<91; iA++) //cycle by angle of incidence
    {
      double angle = iA;
      if (angle == 90) angle = 89.9;
      Angle.append(angle);
      //angle->photon direction
      double cosA = cos(3.1415926535*angle/180.0);
      double sinA = sin(3.1415926535*angle/180.0);
      for (int i=0; i<num; i++) //cycle by photons
        {
          //have to reset since K is modified by the override object
          K[0] = sinA;
          K[1] = 0;
          K[2] = cosA;


          ph.v[0] = K[0];
          ph.v[1] = K[1];
          ph.v[2] = K[2];
          ph.waveIndex = -1;
          ov->calculate(Resources, &ph, N);

          switch (ov->Status)
            {
            case AOpticalOverride::Absorption: continue; break;
            case AOpticalOverride::SpikeReflection: Spike[iA]++; break;
            case AOpticalOverride::LobeReflection: Lobe[iA]++; break;
            case AOpticalOverride::LambertianReflection: Diffuse[iA]++; break;
            default:
              qCritical()<<"Unknown process!";
              exit(666);
            }
        }
      Spike[iA] /= num;
      Lobe[iA] /= num;
      Diffuse[iA] /= num;
      Total[iA] = Spike[iA]+Lobe[iA]+Diffuse[iA];
    }

  TGraph *gS, *gL, *gD, *gT;
  gT = GraphWindow->MakeGraph(&Angle, &Total,   2, "Angle", "", 0, 1, 1, 2, "", true);
  gT->SetMinimum(0);
  gT->SetTitle("Total");
  gS = GraphWindow->MakeGraph(&Angle, &Spike,   1, "Angle", "", 0, 1, 1, 1, "", true);
  gS->SetTitle("Spike");
  gL = GraphWindow->MakeGraph(&Angle, &Lobe,    3, "Angle", "", 0, 1, 1, 1, "", true);
  gL->SetTitle("Lobe");
  gD = GraphWindow->MakeGraph(&Angle, &Diffuse, 4, "Angle", "", 0, 1, 1, 1, "", true);
  gD->SetTitle("Diffuse");

  GraphWindow->Draw(gT, "AL");
  GraphWindow->Draw(gS, "Lsame");
  GraphWindow->Draw(gL, "Lsame");
  GraphWindow->Draw(gD, "Lsame");

  TLegend *leg = new TLegend(0.1, 0.7, 0.3, 0.9);
  leg->SetFillColor(0);
  //leg->SetHeader("test legend");
  leg->AddEntry(gT, "Total", "lp");
  leg->AddEntry(gS, "Spike", "lp");
  leg->AddEntry(gL, "Lobe", "lp");
  leg->AddEntry(gD, "Diffuse", "lp");
  GraphWindow->Draw(leg, "same");

  delete ov;
  delete ph.SimStat;
}

static QVector<double> vParam, vTot, vSpike, vLobe, vDiff, vDir;
static QString parName;
void MainWindow::on_pbST_ReflectionVsParameter_clicked()
{
  ATracerStateful Resources;
  Resources.RandGen = Detector->RandGen;
  //input string processing
  QString inS = ui->leST_Parameter->text().simplified();
  QStringList inL = inS.split(",", QString::SkipEmptyParts);
  qDebug() << inL.size()<<inL;
  if (inL.size()!=4)
    {
      message("Input line format: Parameter, From, To, Step", this);
      return;
    }
  QList<QString> parameters;
  //============== list of parameter names ==============
  parameters << "n1" << "n2" << "alb" << "sigA" << "sigS" << "k2";
  parName = inL[0];
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
  int steps = (to-from)/step;
  qDebug() << "Steps:"<<steps;

  // Have to create a fake material collection with two materials
  // to provide refractive indices and iwave->wavelength conversion to the override class
  AMaterialParticleCollection MatCol;
  //creating and filling override object and this fake material collection
  AOpticalOverride* ov = InitOverride(ui, MatCol);

  PhScatClaudioModel* ClaudMod = dynamic_cast<PhScatClaudioModel*>(ov);
  ScatterOnMetal* ScatOnMet = dynamic_cast<ScatterOnMetal*>(ov);

  TVector3 SurfNorm(ui->ledST_si->text().toDouble(),
                    ui->ledST_sj->text().toDouble(),
                    ui->ledST_sk->text().toDouble());
  SurfNorm = SurfNorm.Unit();
  SurfNorm = - SurfNorm; //In ANTS2 navigator provides normal in the opposite direction
  double N[3];
  N[0] = SurfNorm.X();
  N[1] = SurfNorm.Y();
  N[2] = SurfNorm.Z();
  TVector3 PhotDir(ui->ledST_i->text().toDouble(),
                   ui->ledST_j->text().toDouble(),
                   ui->ledST_k->text().toDouble());
  PhotDir = PhotDir.Unit();
  double K[3];

  int num = ui->sbST_number->value();
  vParam.clear();
  vSpike.clear();
  vLobe.clear();
  vTot.clear();
  vDiff.clear();
  vDir.clear();

  APhoton ph;
  ph.SimStat = new ASimulationStatistics();

  for (int iS = 0; iS<steps; iS++)
    { //cycle by parameter steps
      double parVal = from + iS*step;
      vParam.append(parVal);
      vSpike.append(0);
      vLobe.append(0);
      vTot.append(0);
      vDiff.append(0);
      vDir.append(0);
      if (parName == "n1")
         MatCol[0]->n = parVal;
      else if (parName == "n2")
        {
         MatCol[1]->n = parVal;
         if (ScatOnMet) ScatOnMet->RealN = parVal;
        }
      else if (parName == "alb")
        {
         if (ClaudMod) ClaudMod->albedo = parVal;
        }
      else if (parName == "sigA")
        {
         if (ClaudMod) ClaudMod->sigma_alpha = parVal;
        }
      else if (parName == "sigS")
        {
         if (ClaudMod) ClaudMod->sigma_h = parVal;
        }
      else if (parName == "k2")
        {
         if (ScatOnMet) ScatOnMet->ImaginaryN = parVal;
        }

      for (int iphot=0; iphot<num; iphot++)
        { //cycle by photons
          switch ( ui->cobST_PhotDirIso->currentIndex() )
            {
            case 0: //given direction
              K[0] = PhotDir.X();
              K[1] = PhotDir.Y();
              K[2] = PhotDir.Z();
              break;
            case 1: //uniform distr
              {
                double a=0, b=0, r2=1;
                while (r2 > 0.25)
                  {
                    a  = Detector->RandGen->Rndm() - 0.5;
                    b  = Detector->RandGen->Rndm() - 0.5;
                    r2 =  a*a + b*b;
                  }
                K[2] = ( -1.0 + 8.0 * r2 );
                double scale = 8.0 * TMath::Sqrt(0.25 - r2);
                K[0] = a*scale;
                K[1] = b*scale;
                //accept only in the same direction (2Pi) as the normal of the surface (normal is already inverted - it points to the second material)
                if (K[0]*N[0]+K[1]*N[1]+K[2]*N[2]<0)
                  {
                    K[0]=-K[0];
                    K[1]=-K[1];
                    K[2]=-K[2];
                  }
                break;
              }
            case 2: //diffuse (lambertian) distr
              {
                double sin2angle = Detector->RandGen->Rndm();
                double angle = asin(sqrt(sin2angle));
                double yOff = cos(angle), zOff = -sin(angle);
                N[0] = 0; N[1] = 0; N[2] = -1;   // convention of photon tracer - normal is med1->med2
                K[0] = 0; K[1] = yOff; K[2] = zOff;  // -z direction on xy plane (incidence angle from 90 to 0)
                break;
              }
            default:
              message("Unknown type of photon irradiation!", this);
              return;
            }

          ph.v[0] = K[0];
          ph.v[1] = K[1];
          ph.v[2] = K[2];
          ph.waveIndex = -1;
          ov->calculate(Resources, &ph, N);

          switch (ov->Status)
            {
            case AOpticalOverride::Absorption: continue;
            case AOpticalOverride::SpikeReflection: vSpike[iS]++; vDir[iS]++; break;
            case AOpticalOverride::LobeReflection: vLobe[iS]++; vDir[iS]++; break;
            case AOpticalOverride::LambertianReflection: vDiff[iS]++; break;
            default:
              qCritical()<<"Unknown process!";
              exit(666);
            }
          vTot[iS]++;
        } //end cycle by photons
      vSpike.last() /= num;
      vDir.last() /= num;
      vLobe.last() /= num;
      vDiff.last() /= num;
      vTot.last() /= num;
    } //end cycle by parameter steps

  on_cobST_ShowWhatRef_activated(ui->cobST_ShowWhatRef->currentIndex());

  delete ov;
  delete ph.SimStat;
}

void MainWindow::on_pbST_VsParameterHelp_clicked()
{
  QString str =
      "Format: Parameter_name, StartValue, EndValue, Step\n"
      "Parameter names:\n"
      "sigA - SigmaAlpha\n"
      "sigS - SigmaSpike\n"
      "alb  - albedo\n"
      "n1   - Refractive index 1\n"
      "n2   - Refractive index 2\n"
      "k2   - Imaginary part of refractive index 2\n";
  message(str, this);
}

void MainWindow::on_cobST_ShowWhatRef_activated(int index)
{
  switch (index)
    {
    case 0:
      GraphWindow->MakeGraph(&vParam, &vTot, 4, parName.toLocal8Bit().data(), "Sum reflectivity", 20, 1, 1, 1);
      break;
    case 1:
      GraphWindow->MakeGraph(&vParam, &vDiff, 4, parName.toLocal8Bit().data(), "Diffuse reflectivity", 20, 1, 1, 1);
      break;
    case 2:
      GraphWindow->MakeGraph(&vParam, &vDir, 4, parName.toLocal8Bit().data(), "Directed reflectivity", 20, 1, 1, 1);
      break;
    case 3:
      GraphWindow->MakeGraph(&vParam, &vSpike, 4, parName.toLocal8Bit().data(), "Spike reflectivity", 20, 1, 1, 1);
      break;
    case 4:
      GraphWindow->MakeGraph(&vParam, &vLobe, 4, parName.toLocal8Bit().data(), "Lobe reflectivity", 20, 1, 1, 1);
      break;
    default:;
    }
}

void MainWindow::on_pbST_showTracks_clicked()
{
  if (tracks.isEmpty()) return;
  int selector = ui->cobST_trackType->currentIndex() - 1;
  if (selector == 3) return; //do not show any tracks

  Detector->GeoManager->ClearTracks();
  GeometryWindow->ClearRootCanvas();
  //showing surface
  Int_t track_index = Detector->GeoManager->AddTrack(1,22);
  TVirtualGeoTrack* track = Detector->GeoManager->GetTrack(track_index);  
  double d = 0.5;  
  double f = 0.5;

  //surface normal
  track->AddPoint(d, d, d, 0);
  track->AddPoint(d+NormViz.X(), d+NormViz.Y(), d+NormViz.Z(), 0);
  track->SetLineWidth(3);
  track->SetLineColor(1);
  //surf
  track_index = Detector->GeoManager->AddTrack(1,22);
  track = Detector->GeoManager->GetTrack(track_index);
  TVector3 perp = NormViz.Orthogonal();
  perp.Rotate(0.25*3.1415926535, NormViz);
  for (int i=0; i<5; i++)
    {
      track->AddPoint(d+f*perp.X(), d+f*perp.Y(), d+f*perp.Z(), 0);
      perp.Rotate(0.5*3.1415926535, NormViz);
    }
  track->SetLineWidth(2);
  track->SetLineColor(1);

  //photon tracks  
  int numTracks = 0;
  for(int i = 1; i<tracks.count() && numTracks<10000; i++)
    {
      const TrackHolderClass* th = &tracks.at(i);
      //filter
      if (selector>-1)  //-1 - show all
        if (selector != th->UserIndex) continue;

      track_index = Detector->GeoManager->AddTrack(1,22);
      track = Detector->GeoManager->GetTrack(track_index);
      track->SetLineColor(th->Color);
      track->SetLineWidth(1);
      for (int iNode=0; iNode<th->Nodes.size(); iNode++)
        track->AddPoint(th->Nodes[iNode].R[0], th->Nodes[iNode].R[1], th->Nodes[iNode].R[2], th->Nodes[iNode].Time);
    }
  //intitial photon track
  track_index = Detector->GeoManager->AddTrack(1,22);
  track = Detector->GeoManager->GetTrack(track_index);
  TrackHolderClass* th = &tracks.first();
  track->SetLineColor(kRed);
  track->SetLineWidth(2);
  for (int iNode=0; iNode<th->Nodes.size(); iNode++)
    track->AddPoint(th->Nodes[iNode].R[0], th->Nodes[iNode].R[1], th->Nodes[iNode].R[2], th->Nodes[iNode].Time);

  GeometryWindow->show();
  ShowTracks();
}

void MainWindow::on_pbShowComptonAngles_clicked()
{
    int Maxi = ui->sbTmp->value();
    GammaStructure G0, G1;

    G0.energy = ui->dsbTmp->value();
    TVector3 gdir ( 0, 1./1.4142135623730950488016887242097, 1./1.4142135623730950488016887242097 );
    G0.direction = gdir;

    auto hist1 = new TH1D("ComptonTestha","Angle of scattered gamma",181,0,180);
    hist1->GetXaxis()->SetTitle("Angle, degrees");

    for (int i=0;i<Maxi;i++)
    {
        G1 = Compton(&G0, Detector->RandGen);
        double angle = G1.direction.Angle(G0.direction);
        hist1->Fill(angle*180./3.14159);
    }
    GraphWindow->Draw(hist1);
}

void MainWindow::on_pbShowComptonEnergies_clicked()
{
    int Maxi = ui->sbTmp->value();
    GammaStructure G0, G1;

    G0.energy = ui->dsbTmp->value();
    TVector3 gdir ( 0, 1./1.4142135623730950488016887242097, 1./1.4142135623730950488016887242097 );
    G0.direction = gdir;

    auto hist1 = new TH1D("ComptonTesthf","Energy of scattered gamma",101,0,G0.energy);
    hist1->GetXaxis()->SetTitle("Energy, keV");

    for (int i=0;i<Maxi;i++)
    {
        G1 = Compton(&G0, Detector->RandGen);
        double energy = G1.energy;
        hist1->Fill(energy);
    }
    GraphWindow->Draw(hist1);
}

void MainWindow::on_pbCheckRandomGen_clicked()
{
    auto hist1 = new TH1D("Testrndm","uniform", GlobSet->BinsX, 0,1.);
    for (int i=0; i<100000; i++) hist1->Fill(Detector->RandGen->Rndm());
    GraphWindow->Draw(hist1);
}

void MainWindow::on_pbShowCheckUpWindow_clicked()
{
  if (!CheckUpWindow) return;
  CheckUpWindow->show();
}

void MainWindow::on_pbShowEnergyDeposition_clicked()
{
  if (EventsDataHub->EventHistory.isEmpty())
    {
      message("No data available!", this);
      return; //nothing to show
    }
  int index = ui->sbParticleIndexForShowDepEnergy->value();
  int i=-1;
  int Number;
  do
    {
      i++;
      if (i>EnergyVector.size()-1)
        {
          message("Deposition information not found for this particle", this);
          return;
        }
      Number = EnergyVector.at(i)->index;
    }
  while(Number != index);
  //on exit: i - first record with user-defined particle#

  QVector<double> x,y;
  x.resize(0);
  y.resize(0);

  double Length = 0;

  //info on this particle starts at i
  for (i; i<EnergyVector.size();i++)
    {
      if (EnergyVector[i]->index != index) break; //another particle starts here

      double DeltaLength = EnergyVector[i]->cellLength;
      Length += DeltaLength;
      x.append(Length);
      if (DeltaLength == 0) DeltaLength = 1; //0 protection for point deposition events
      y.append(EnergyVector[i]->dE/DeltaLength);
    }

  if (x.size() == 0 )
    {
      message("This particle did not deposit any energy", this);
      GraphWindow->hide();
      return;
   }

  GraphWindow->MakeGraph(&x, &y, kBlue, "Distance, mm", "Deposited energy linear density, keV/mm");
}

void MainWindow::on_pbShowDetailedLog_clicked()
{
    Owindow->showNormal();
    Owindow->raise();
    Owindow->activateWindow();
    Owindow->ShowEventHistoryLog();
    Owindow->SetTab(3);
}
