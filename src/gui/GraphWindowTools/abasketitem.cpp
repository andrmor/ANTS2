#include "abasketitem.h"

#include "TObject.h"
#include "TNamed.h"
#include "TList.h"

#include "TGraph.h"
#include "TGraph2D.h"
#include "TH1.h"
#include "TH2.h"
#include "TH3.h"
#include "TSystem.h"
#include "TStyle.h"
#include "TF1.h"
#include "TF2.h"
#include "TMultiGraph.h"
#include "TGraphErrors.h"
#include "TProfile.h"
#include "TProfile2D.h"
#include "TEllipse.h"
#include "TPolyLine.h"
#include "TLine.h"
#include "TLegend.h"
#include "TPave.h"
#include "TPaveLabel.h"
#include "TPavesText.h"

#include <QDebug>

TGraph * HistToGraph(TH1 * h)
{
    if (!h) return 0;
    QVector<double> x, f;
    for (int i=1; i<h->GetXaxis()->GetNbins()-1; i++)
    {
        x.append(h->GetBinCenter(i));
        f.append(h->GetBinContent(i));
    }
    return new TGraph(x.size(), x.data(), f.data());
}

ABasketItem::ABasketItem(QString name, QVector<ADrawObject> *drawObjects)
{
  Name = name;
  Type = drawObjects->first().Pointer->ClassName();

  for (int i=0; i<drawObjects->size(); i++)
    {
      TObject * obj = nullptr;
      QString type = (*drawObjects)[i].Pointer->ClassName();
      QString options = (*drawObjects)[i].Options;

      //special cases first
      if (type == "TF1")
      {
          //does not work normal way - recalculated LRFs will replace old ones even in the copied object
          TF1* f = (TF1*)(*drawObjects)[i].Pointer;
          TGraph* g = HistToGraph( f->GetHistogram() );
          g->GetXaxis()->SetTitle( f->GetHistogram()->GetXaxis()->GetTitle() );
          g->GetYaxis()->SetTitle( f->GetHistogram()->GetYaxis()->GetTitle() );
          g->SetLineStyle( f->GetLineStyle());
          g->SetLineColor(f->GetLineColor());
          g->SetLineWidth(f->GetLineWidth());
          g->SetFillColor(0);
          g->SetFillStyle(0);
          obj = g;
          if (!options.contains("same", Qt::CaseInsensitive))
              if (!options.contains("AL", Qt::CaseInsensitive))
                  options += "AL";
          type = g->ClassName();
          if (i == 0) Type = type;
      }
      else if (type == "TF2")
      {
          //does not work normal way - recalculated LRFs will replace old ones even in the copied object
          TF2* f = (TF2*)(*drawObjects)[i].Pointer;
          TH1* h = f->CreateHistogram();
          obj = h;
          type = h->ClassName();
          if (i == 0) Type = type;
      }
      else
      {
          //general case
          //qDebug() << "Using general case for type:"<<type;
          obj = (*drawObjects)[i].Pointer->Clone();
      }

      TNamed * nn = dynamic_cast<TNamed*>(obj);
      if (nn) nn->SetTitle(name.toLocal8Bit().constData());

      DrawObjects.append(ADrawObject(obj, options));
    }
}

ABasketItem::~ABasketItem()
{
    //cannot delete objects here!
}

void ABasketItem::clearObjects()
{
   for (int i=0; i<DrawObjects.size(); i++)
       delete DrawObjects[i].Pointer;
   DrawObjects.clear();
}
