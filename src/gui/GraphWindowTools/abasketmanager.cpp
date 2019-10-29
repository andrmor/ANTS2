#include "abasketmanager.h"

#include <QStringList>
#include <QDebug>

#include "TObject.h"
#include "TNamed.h"
#include "TF1.h"
#include "TF2.h"
#include "TGraph.h"
#include "TAxis.h"
#include "TH1.h"
#include "TH2.h"

ABasketManager::ABasketManager()
{
    NotValidItem << ADrawObject(new TNamed("Not valid index", "Not valid index"), "");
}

ABasketManager::~ABasketManager()
{
    clear();

    delete NotValidItem.first().Pointer;
    NotValidItem.clear();
}

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

void ABasketManager::add(const QString & name, const QVector<ADrawObject> & drawObjects)
{
    ABasketItem item;
    item.Name = name;
    item.Type = drawObjects.first().Pointer->ClassName();

    for (int i = 0; i < drawObjects.size(); i++)
    {
        const ADrawObject & drObj = drawObjects.at(i);

        TObject * tobj = nullptr;
        QString type = drObj.Pointer->ClassName();
        QString options = drObj.Options;

        //special cases
        if (type == "TF1")
        {
            //does not work normal way - recalculated LRFs will replace old ones even in the copied object
            TF1* f = (TF1*)drObj.Pointer;
            TGraph* g = HistToGraph( f->GetHistogram() );
            g->GetXaxis()->SetTitle( f->GetHistogram()->GetXaxis()->GetTitle() );
            g->GetYaxis()->SetTitle( f->GetHistogram()->GetYaxis()->GetTitle() );
            g->SetLineStyle( f->GetLineStyle());
            g->SetLineColor(f->GetLineColor());
            g->SetLineWidth(f->GetLineWidth());
            g->SetFillColor(0);
            g->SetFillStyle(0);
            tobj = g;
            if (!options.contains("same", Qt::CaseInsensitive))
                if (!options.contains("AL", Qt::CaseInsensitive))
                    options += "AL";
            type = g->ClassName();
            if (i == 0) item.Type = type;
        }
        else if (type == "TF2")
        {
            //does not work normal way - recalculated LRFs will replace old ones even in the copied object
            TF2* f = (TF2*)drObj.Pointer;
            TH1* h = f->CreateHistogram();
            tobj = h;
            type = h->ClassName();
            if (i == 0) item.Type = type;
        }
        else
        {
            //general case
            tobj = drObj.Pointer->Clone();
        }

        item.DrawObjects.append( ADrawObject(tobj, options) );
    }

    Basket << item;
}

const QVector<ADrawObject> ABasketManager::getCopy(int index) const
{
    QVector<ADrawObject> res;

    if (index >= 0 && index < Basket.size())
    {
        for (const ADrawObject & obj : Basket.at(index).DrawObjects)
            res << ADrawObject(obj.Pointer->Clone(), obj.Options);
    }

    return res;
}

void ABasketManager::clear()
{
    for (int ib=0; ib<Basket.size(); ib++)
        Basket[ib].clearObjects();
    Basket.clear();
}

void ABasketManager::remove(int index)
{
    if (index < 0 || index >= Basket.size()) return;
    Basket[index].clearObjects();
    Basket.remove(index);
}

QVector<ADrawObject> & ABasketManager::getDrawObjects(int index)
{
    if (index < 0 || index >= Basket.size())
    {
        qWarning() << "Basket manager: index" << index << "is out of bounds!";
        return NotValidItem;
    }
    return Basket[index].DrawObjects;
}

const QString ABasketManager::getType(int index) const
{
    if (index < 0 || index >= Basket.size()) return "";
    return Basket[index].Type;
}

int ABasketManager::size() const
{
    return Basket.size();
}

const QString ABasketManager::getName(int index) const
{
    if (index < 0 || index >= Basket.size()) return "";
    return Basket.at(index).Name;
}

void ABasketManager::rename(int index, const QString & newName)
{
    if (index < 0 || index >= Basket.size()) return;
    Basket[index].Name = newName;
}

const QStringList ABasketManager::getItemNames() const
{
    QStringList res;
    for (const ABasketItem & item : Basket)
        res << item.Name;
    return res;
}
