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
#include "aglobalsettings.h"
#include "geometrywindowclass.h"
#include "acompton.h"
#include "aenergydepositioncell.h"
#include "amessage.h"

#include "TMath.h"
#include "TGraph.h"
#include "TH1D.h"
#include "TRandom2.h"

//Qt
#include <QDebug>
#include <QObject>
#include <QFileDialog>

//#include "TBufferJSON.h"
//#include "tmpobjhubclass.h"
//#include "TAttMarker.h"

#include "acommonfunctions.h"
#include "TH1D.h"
#include "TH1.h"
void MainWindow::on_pobTest_clicked()
{
    TH1D * hist = new TH1D("", "", 5, 0, 5);
    for (int i=0; i<5; i++)
        hist->Fill(i, 1.0+i);
    GraphWindow->Draw(hist, "hist");

    TH1D * h = new TH1D("", "", 100, -2, 8);
    //for (int i=0; i<100000; i++) h->Fill( GetRandomFromHist(hist, Detector->RandGen) );
    for (int i=0; i<100000; i++) h->Fill( GetRandomBinFromHist(hist, Detector->RandGen) );
    GraphWindow->Draw(h, "hist");

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
#include "atrackingdataimporter.h"
#include "simulationmanager.h"
#include "aeventtrackingrecord.h"
#include "outputwindow.h"
void MainWindow::on_pobTest_2_clicked()
{
    ATrackingDataImporter di(SimulationManager->TrackBuildOptions, &EventsDataHub->TrackingHistory, nullptr);
    QString err = di.processFile("D:/DOWNLOADS/tracks-0.txt", 0);
    if (!err.isEmpty())
    {
        qDebug() << err;
        return;
    }

    Owindow->OutTextClear();
    QStringList pn = MpCollection->getListOfParticleNames();
    if (EventsDataHub->TrackingHistory.size() == 0)
        qDebug() << "TrackingHistory is empty!";
    else
    {
        for (size_t i = 0; i<EventsDataHub->TrackingHistory.size(); i++)
        {
            Owindow->OutText( QString("Event #%1").arg(i) );
            auto * er = EventsDataHub->TrackingHistory.at(i);
            if (!er) qDebug() << "Empty!!!";
            else
                for (auto * pr : er->PrimaryParticleRecords)
                {
                    QString str;
                    pr->logToString(str, 0, pn, true);
                    Owindow->OutText(str);
                }
        }
    }
    Owindow->SetTab(0);
    Owindow->showNormal();

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
    auto hist1 = new TH1D("Testrndm","uniform", GlobSet.BinsX, 0,1.);
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
