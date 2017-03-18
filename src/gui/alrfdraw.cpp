#include "alrfdraw.h"
#include "alrfmoduleselector.h"
#include "eventsdataclass.h"
#include "pms.h"
#include "apositionenergyrecords.h"
#include "graphwindowclass.h"
#include "ajsontools.h"

#include <QDebug>

#include "TF1.h"
#include "TH1.h"
#include "TH2D.h"
#include "TStyle.h"
#include "TGraph.h"
#include "TGraph2D.h"
#include "TF2.h"

bool ALrfDrawSettings::ReadFromJson(const QJsonObject &json)
{
  parseJson(json, "DataSource", datais);
  parseJson(json, "PlotLRF", plot_lrf);
  parseJson(json, "PlotSecondaryLRF", draw_second);
  parseJson(json, "PlotData", plot_data);
  parseJson(json, "PlotDiff", plot_diff);
  parseJson(json, "FixedMin", fixed_min);
  parseJson(json, "FixedMax", fixed_max);
  parseJson(json, "Min", minDraw);
  parseJson(json, "Max", maxDraw);
  parseJson(json, "CurrentGroup", CurrentGroup);
  parseJson(json, "CheckZ", check_z);
  parseJson(json, "Z", z0);
  parseJson(json, "DZ", dz); ;
  parseJson(json, "EnergyScaling", scale_by_energy);
  parseJson(json, "FunctionPointsX", FunctionPointsX);
  parseJson(json, "FunctionPointsY", FunctionPointsY);
  parseJson(json, "Bins", bins);
  parseJson(json, "ShowNodes", showNodePositions);

  return true;
}

void ALrfDraw::reportError(QString text)
{
    LastError = text;
    qWarning() << "LRF drawing error:"<< text;
}

bool ALrfDraw::extractOptionsAndVerify(int PMnumber, const QJsonObject &json)
{
    if (PMnumber<0 || PMnumber>PMs->count()-1)
      {
        reportError("Wrong PM number");
        return false;
      }
    if (!LRFs->isAllLRFsDefined(fUseOldModule))
      {
        reportError("LRFs are not defined");
        return false;
      }

    bool fOK = Options.ReadFromJson(json);
    if (!fOK)
      {
        reportError("Json read failed");
        return false;
      }

    if (!Options.plot_lrf && !Options.plot_data && !Options.plot_diff)
      {
        reportError("Nothing to show!");
        return false;
      }

    int npts = EventsDataHub->Events.size();
    if ( Options.plot_data || Options.plot_diff )
      {
        if (Options.datais == 0)
          {
            if (EventsDataHub->isScanEmpty())
              {
                qWarning() << "There is no scan data, skipping data plot";
                Options.plot_data = false;
                Options.plot_diff = false;
              }
            if (EventsDataHub->Scan.size() != npts)
              {
                reportError("Scan size not equal to Events size!");
                return false;
              }
          }
        else
          {
            if (!EventsDataHub->isReconstructionReady(Options.CurrentGroup) )
              {
                qWarning() << "Reconstruction was not performed, skipping data plot";
                Options.plot_data = false;
                Options.plot_diff = false;
              }
            if (EventsDataHub->ReconstructionData[Options.CurrentGroup].size() != npts)
              {
                reportError("Error: Reconstruction size not equal to Events size!");
                return false;
              }
          }
      }

    return true;
}

ALrfDraw::ALrfDraw(ALrfModuleSelector *LRFs, bool fUseOldModule, EventsDataClass *EventsDataHub, pms *PMs, GraphWindowClass *GraphWindow) :
  LRFs(LRFs), fUseOldModule(fUseOldModule), EventsDataHub(EventsDataHub), PMs(PMs), GraphWindow(GraphWindow) {}

bool ALrfDraw::DrawRadial(int PMnumber, const QJsonObject &json)
{
  GraphWindow->ShowAndFocus();
  LastError.clear();
  bool fOk = extractOptionsAndVerify(PMnumber, json);
  if (!fOk) return false;

  TF1 *tf1, *tf1bis;
  TH2D *hist2D;

  //only calculations of LRFs, no drawing
  if (Options.plot_lrf)
      {
        QJsonObject module_js = json["ModuleSpecific"].toObject();
        tf1 = LRFs->getRootFunctionMainRadial(fUseOldModule, PMnumber, Options.z0, module_js);

        tf1->SetNpx(Options.FunctionPointsX);
        tf1->SetFillColor(0);
        tf1->SetFillStyle(0);
        if (Options.fixed_min) tf1->SetMinimum(Options.minDraw);
        if (Options.fixed_max) tf1->SetMaximum(Options.maxDraw);

        tf1->GetXaxis()->SetTitle("Radial distance, mm");
        tf1->GetYaxis()->SetTitle("LRF");
        TString str = "LRF of pm# ";
        str += PMnumber;
        tf1->SetTitle(str);
        //GraphWindow->DrawWithoutFocus(tf1);  --cannot do it here if data are shown: hides behind!

        if (Options.draw_second)
          {
            tf1bis = LRFs->getRootFunctionSecondaryRadial(fUseOldModule, PMnumber, Options.z0, module_js);
            if (tf1bis)
              {
                tf1bis->SetLineColor(1);
                tf1bis->SetLineStyle(9);
                tf1bis->SetFillColor(0);
                tf1bis->SetFillStyle(0);
                tf1bis->SetNpx(Options.FunctionPointsX);
                if (Options.fixed_min) tf1bis->SetMinimum(Options.minDraw);
                if (Options.fixed_max) tf1bis->SetMaximum(Options.maxDraw);
                //GraphWindow->DrawWithoutFocus(tf1bis, "same");  --same
              }
            else qWarning() << "Could not create second LRF";
          }
      }

  //drawing data
    if (Options.plot_data || Options.plot_diff)
      {
        int npts = EventsDataHub->Events.size();

        double minx, maxx, miny, maxy;
        minx = maxx = miny = maxy = 0;
        if (Options.plot_lrf)
        {
            tf1->GetRange(minx, maxx);
            TH1* h = tf1->GetHistogram();
            miny = h->GetMinimum();
            maxy = h->GetMaximum() * (1.0 + gStyle->GetHistTopMargin());
            //qDebug() << minx<<maxx<<miny<<maxy;
        }
        hist2D = new TH2D("tmpTestHist", "", Options.bins, minx, maxx, Options.bins, miny, maxy);

      if (!Options.plot_lrf)
        {
  #if ROOT_VERSION_CODE < ROOT_VERSION(6,0,0)
          hist2D->SetBit(TH1::kCanRebin);
  #endif
        }

        double factor = 1.0;
        double x0 = PMs->X(PMnumber);
        double y0 = PMs->Y(PMnumber);

        for (int i=0; i<npts; i++)
          {
            double *pos;
            if (!Options.datais)
              pos = EventsDataHub->Scan[i]->Points[0].r;
            else
              {
                if (!EventsDataHub->ReconstructionData[Options.CurrentGroup][i]->GoodEvent) continue;
                pos = EventsDataHub->ReconstructionData[Options.CurrentGroup][i]->Points[0].r;
                factor = EventsDataHub->ReconstructionData[Options.CurrentGroup][i]->Points[0].energy;
              }

            if (Options.check_z && (pos[2]<Options.z0-Options.dz || pos[2]>Options.z0+Options.dz)) continue;

            double r = hypot(pos[0]-x0, pos[1]-y0);
            double fdata = EventsDataHub->Events[i][PMnumber];
            if (Options.scale_by_energy) fdata /= factor;            
            if (Options.plot_diff)
              {
                double fdiff = fdata - LRFs->getLRF(fUseOldModule, PMnumber, pos);
                hist2D->Fill(r, fdiff);
              }
            else hist2D->Fill(r, fdata);
          }

        hist2D->GetXaxis()->SetTitle("Radial distance, mm");
        hist2D->GetYaxis()->SetTitle("Signal");

        GraphWindow->DrawWithoutFocus(hist2D, "colz", false);
      }

    //to be on top!
    //Drawing LRF(s)
    if (Options.plot_lrf)
      {
        TString opt = "";
        if (Options.plot_data || Options.plot_diff) opt = "same";

        GraphWindow->DrawWithoutFocus(tf1, opt, false);
        if (Options.draw_second && tf1bis)
          GraphWindow->DrawWithoutFocus(tf1bis, "same", false);
      }

    //Drawing nodes
    if (Options.showNodePositions)
      {
         QVector <double> GrX;
         bool fOk = LRFs->getNodes(fUseOldModule, PMnumber, GrX);
         if (fOk)
           {
             QVector <double> GrY(GrX.size(), 0);             
             auto tgraph = GraphWindow->MakeGraph(&GrX, &GrY, kBlue, "", "", 2, 6, 0, 0, "", true);
             GraphWindow->Draw(tgraph, "P same", false);
           }
      }

    GraphWindow->UpdateRootCanvas();

    return true;
}

bool ALrfDraw::DrawXY(int PMnumber, const QJsonObject &json)
{
    GraphWindow->ShowAndFocus();
    LastError.clear();
    bool fOk = extractOptionsAndVerify(PMnumber, json);
    if (!fOk) return false;

    double x, y, z, fdata, fdiff; //r,
    QVector<double> xx, yy, ffdata, ffdiff; //rr,
    int counter = 0;
    double factor = 1.0;

    TF2 *tf2, *tf2bis;

    if ( Options.plot_data || Options.plot_diff )
      {
        int npts = EventsDataHub->Events.size();

        for (int i=0; i<npts; i++)
          {
            if (Options.datais == 0)
              {
                x = EventsDataHub->Scan[i]->Points[0].r[0];
                y = EventsDataHub->Scan[i]->Points[0].r[1];
                z = EventsDataHub->Scan[i]->Points[0].r[2];
              }
            else
              {
                if (!EventsDataHub->ReconstructionData[Options.CurrentGroup][i]->GoodEvent) continue;
                x = EventsDataHub->ReconstructionData[Options.CurrentGroup][i]->Points[0].r[0];
                y = EventsDataHub->ReconstructionData[Options.CurrentGroup][i]->Points[0].r[1];
                z = EventsDataHub->ReconstructionData[Options.CurrentGroup][i]->Points[0].r[2];
                factor = EventsDataHub->ReconstructionData[Options.CurrentGroup][i]->Points[0].energy;
              }

            if (Options.check_z && (z<Options.z0-Options.dz || z>Options.z0+Options.dz)) continue;

            //r = sqrt((x-x0)*(x-x0) + (y-y0)*(y-y0));
            fdata = EventsDataHub->Events[i][PMnumber];
            if (Options.scale_by_energy) fdata /= factor;

            if (Options.plot_diff)
            {
                fdiff = fdata - LRFs->getLRF(fUseOldModule, PMnumber, x, y, z);
                ffdiff << fdiff;
            }
            else ffdata << fdata;

            xx << x;
            yy << y;
            //rr << r;

            counter++;
          }
      }

    if (Options.plot_lrf)
      {
        QJsonObject module_js = json["ModuleSpecific"].toObject();
        tf2 = LRFs->getRootFunctionMainXY(fUseOldModule, PMnumber, Options.z0, module_js);

        tf2->SetNpx(Options.FunctionPointsX);
        tf2->SetNpy(Options.FunctionPointsY);
        GraphWindow->DrawWithoutFocus(tf2, "surf", false);

        if (Options.draw_second)
            {
              //have to draw in the same range of coordnates - ROOT's "same" option does not work with "surf"
              tf2bis = LRFs->getRootFunctionSecondaryXY(fUseOldModule, PMnumber, Options.z0, module_js);
              if (tf2bis)
              {
                  double min = tf2bis->GetMinimum();
                  double max = tf2bis->GetMaximum();
                  tf2bis->GetZaxis()->SetRangeUser(min, max);
                  tf2bis->SetLineColor(1);
                  tf2bis->SetLineStyle(9);
                  tf2bis->SetNpx(Options.FunctionPointsX);
                  tf2bis->SetNpy(Options.FunctionPointsY);
                  GraphWindow->DrawWithoutFocus(tf2bis, "surf same", false);
              }
            }
      }

    if (Options.plot_data)
      {
        auto tgraph2D = new TGraph2D(ffdata.size(), xx.data(), yy.data(), ffdata.data());
        tgraph2D->SetMarkerStyle(7);
        if (Options.plot_lrf) GraphWindow->DrawWithoutFocus(tgraph2D, "samepcol", false);
        else                  GraphWindow->DrawWithoutFocus(tgraph2D, "pcol", false);
      }
    else if (Options.plot_diff)
      {
        auto tgraph2D = new TGraph2D(ffdiff.size(), xx.data(), yy.data(), ffdiff.data());
        tgraph2D->SetMarkerStyle(7);
        if (Options.plot_lrf || Options.plot_data) GraphWindow->DrawWithoutFocus(tgraph2D, "samepcol", false);
        else                                       GraphWindow->DrawWithoutFocus(tgraph2D, "pcol", false);
      }
    GraphWindow->UpdateRootCanvas();

    return true;
}

