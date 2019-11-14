#include "abasketmanager.h"
#include "afiletools.h"
#include "ajsontools.h"

#include <QStringList>
#include <QJsonDocument>
#include <QDebug>

#include "TObject.h"
#include "TNamed.h"
#include "TF1.h"
#include "TF2.h"
#include "TGraph.h"
#include "TGraphErrors.h"
#include "TAxis.h"
#include "TH1.h"
#include "TH2.h"
#include "TFile.h"
#include "TKey.h"
#include "TLegend.h"
#include "TLegendEntry.h"

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

    QMap<TObject*, TObject*> OldToNew;
    TLegend * Legend = nullptr;

    for (int i = 0; i < drawObjects.size(); i++)
    {
        const ADrawObject & drObj = drawObjects.at(i);

        QString type = drObj.Pointer->ClassName();
        QString options = drObj.Options;

        TObject * tobj = nullptr;
        //preparing to clone: process special cases
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
            tobj = drObj.Pointer;
        }

        TObject * clone;
        TLegend * OldLegend = dynamic_cast<TLegend*>(tobj);
        if (OldLegend)
        {
            Legend = new TLegend(*OldLegend); // after cloning Legend has invalid object pointers, so have to use copy constructor
            clone = Legend;
        }
        else
        {
            clone = tobj->Clone();
            OldToNew[drObj.Pointer] = clone;
            //qDebug() << "to Basket, old-->cloned" << drObj.Pointer << "-->" << clone;
        }

        item.DrawObjects.append( ADrawObject(clone, options) );
    }

    if (Legend)
    {
        TList * elist = Legend->GetListOfPrimitives();
        int num = elist->GetEntries();
        for (int ie = 0; ie < num; ie++)
        {
            TLegendEntry * en = static_cast<TLegendEntry*>( (*elist).At(ie) );
            const QString text = en->GetLabel();
            //qDebug() << "Old entry obj:"<< en->GetObject() << " found?" << OldToNew[ en->GetObject() ];
            en->SetObject( OldToNew[ en->GetObject() ] ); // will override the label
            en->SetLabel(text.toLatin1().data());
        }
    }

    Basket << item;
}

void ABasketManager::update(int index, const QVector<ADrawObject> & drawObjects)
{
    if (index < 0 || index >= Basket.size()) return;

    add(Basket[index].Name, drawObjects);
    std::swap(Basket[index], Basket.last());

    Basket.remove(Basket.size()-1);
}

const QVector<ADrawObject> ABasketManager::getCopy(int index) const
{
    QVector<ADrawObject> res;

    QMap<TObject*, TObject*> OldToNew;
    TLegend * Legend = nullptr;

    if (index >= 0 && index < Basket.size())
    {
        for (const ADrawObject & obj : Basket.at(index).DrawObjects)
        {
            TObject * clone;
            TLegend * OldLegend = dynamic_cast<TLegend*>(obj.Pointer);
            if (OldLegend)
            {
                Legend = new TLegend(*OldLegend); // if cloned, Legend has invalid object pointers
                clone = Legend;
            }
            else
            {
                clone = obj.Pointer->Clone();
                OldToNew[obj.Pointer] = clone;
                //qDebug() << "From basket, old-->cloned" << obj.Pointer << "-->" << clone;
            }

            res << ADrawObject(clone, obj.Options);
        }

        if (Legend)
        {
            TList * elist = Legend->GetListOfPrimitives();
            int num = elist->GetEntries();
            for (int ie = 0; ie < num; ie++)
            {
                TLegendEntry * en = static_cast<TLegendEntry*>( (*elist).At(ie) );
                QString text = en->GetLabel();
                //qDebug() << "Old entry obj:"<< en->GetObject() << " found?" << OldToNew[ en->GetObject() ];
                en->SetObject( OldToNew[ en->GetObject() ] ); // will override the label
                en->SetLabel(text.toLatin1().data());
            }
        }
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

void ABasketManager::saveAll(const QString & fileName)
{
    TFile f(fileName.toLocal8Bit(), "RECREATE");

    int objectIndex = 0;
    QJsonArray BasketArray;
    for (int ib = 0; ib < Basket.size(); ib++)
    {
        QJsonObject ItemJson;
        ItemJson["ItemName"] = Basket.at(ib).Name;

        QJsonArray ItemArray;
        const QVector<ADrawObject> & DrawObjects = Basket.at(ib).DrawObjects;
        for (int io = 0; io < DrawObjects.size(); io++)
        {
            const ADrawObject & obj = DrawObjects.at(io);
            TString KeyName = "#";
            KeyName += objectIndex;
            objectIndex++;
            obj.Pointer->Write(KeyName);

            QJsonObject js;
            js["Name"] = obj.Name;
            js["Options"] = obj.Options;
            js["Enabled"] = obj.bEnabled;
            TLegend * Legend = dynamic_cast<TLegend*>(obj.Pointer);
            if (Legend)
            {
                QJsonArray links;
                TList * elist = Legend->GetListOfPrimitives();
                int num = elist->GetEntries();
                for (int ie = 0; ie < num; ie++)
                {
                    TLegendEntry * en = static_cast<TLegendEntry*>( (*elist).At(ie));
                    //qDebug() << "Entry obj:"<< en->GetObject();
                    links.append(findPointerInDrawObjects(DrawObjects, en->GetObject()));
                }
                js["LegendLinks"] = links;
            }
            ItemArray.append(js);
        }
        ItemJson["ItemObjects"] = ItemArray;

        BasketArray.append(ItemJson);
    }

    QJsonDocument doc;
    doc.setArray(BasketArray);
    QString descStr(doc.toJson());
    qDebug() << descStr;

    TNamed desc;
    desc.SetTitle(descStr.toLocal8Bit().data());
    desc.Write("BasketDescription_v2");

    f.Close();

    /*
    //Old system - files created using it still can be loaded
    QString str;
    TFile f(fileName.toLocal8Bit(), "RECREATE");

    //qDebug() << "----Items:"<<Basket.size()<<"----";
    int index = 0;
    for (int ib=0; ib<Basket.size(); ib++)
    {
        //qDebug() << ib<<">"<<Basket[ib].Name;
        str += getName(ib) + '\n';

        QVector<ADrawObject> & DrawObjects = getDrawObjects(ib);
        str += QString::number( DrawObjects.size() );

        for (int i = 0; i < DrawObjects.size(); i++)
        {
            ADrawObject & Obj = DrawObjects[i];
            qDebug() << "Saving>>"<<i<<Obj.Pointer<<Obj.Pointer->GetName()<<Obj.Pointer->ClassName();
            TString name = "";
            name += index;
            TNamed* nameO = dynamic_cast<TNamed*>(Obj.Pointer);
            if (nameO) nameO->SetName(name);

            TString KeyName = "#";
            KeyName += index;
            Obj.Pointer->Write(KeyName);

            str += '|' + Obj.Options;

            index++;


            TLegend * Legend = dynamic_cast<TLegend*>(Obj.Pointer);
            if (Legend)
            {
                TList * elist = Legend->GetListOfPrimitives();
                int num = elist->GetEntries();
                for (int ie = 0; ie < num; ie++)
                {
                    TLegendEntry * en = static_cast<TLegendEntry*>( (*elist).At(ie));
                    qDebug() << "Entry obj:"<< en->GetObject();
                }
            }
        }

        str += '\n';
    }

    TNamed desc;
    desc.SetTitle(str.toLocal8Bit().data());
    desc.Write("BasketDescription");

    f.Close();
    */
}

const QString ABasketManager::appendBasket(const QString & fileName)
{
    TFile f(fileName.toLocal8Bit().data());

    // log block
//    int numKeys = f.GetListOfKeys()->GetEntries();
//    qDebug() << "Keys:"<<numKeys;
//    for (int i=0; i<numKeys; i++)
//    {
//        TKey * key = (TKey*)f.GetListOfKeys()->At(i);
//        QString type = key->GetClassName();
//        QString ObjName = key->GetName();
//        QString title = key->GetTitle();
//        qDebug() << title << ObjName << type;
//    }

    bool ok = true;
    TNamed* desc = (TNamed*)f.Get("BasketDescription_v2");
    if (desc)
    {
        //new system
        QString text = desc->GetTitle();
        QJsonDocument doc(QJsonDocument::fromJson(text.toLatin1().data()));
        QJsonArray BasketArray = doc.array();
        //qDebug() << BasketArray;

        int basketSize = BasketArray.size();
        int KeyIndex = 0;
        for (int iBasketItem = 0; iBasketItem < basketSize; iBasketItem++ )
        {
            QJsonObject ItemJson = BasketArray[iBasketItem].toObject();
            //qDebug() << "Item"<<iBasketItem << ItemJson;

            QString    ItemName  = ItemJson["ItemName"].toString();
            QJsonArray ItemArray = ItemJson["ItemObjects"].toArray();
            int        ItemSize  = ItemArray.size();

            QVector<ADrawObject> drawObjects;
            int LegendIndex = -1;
            QJsonArray LegendLinks;
            for (int iDrawObj = 0; iDrawObj < ItemSize; iDrawObj++)
            {
                QJsonObject js = ItemArray[iDrawObj].toObject();
                QString Name     = js["Name"].toString();
                QString Options  = js["Options"].toString();
                bool    bEnabled = js["Enabled"].toBool();

                TKey *key = (TKey*)f.GetListOfKeys()->At(KeyIndex);
                KeyIndex++;

                TObject * p = key->ReadObj();
                if (p)
                {
                    ADrawObject Obj(p, Options);
                    Obj.Name = Name;
                    Obj.bEnabled = bEnabled;
                    drawObjects << Obj;

                    TLegend * Legend = dynamic_cast<TLegend*>(p);
                    if (Legend)
                    {
                        LegendIndex = iDrawObj;
                        LegendLinks = js["LegendLinks"].toArray();
                    }
                }
                else
                {
                    TString nm(QString("Corrupted_%1").arg(Name).toLatin1().data());
                    p = new TNamed(nm , nm);
                    qWarning() << "Corrupted TKey in basket file" << fileName << " object:" << Name;
                }
            }

            if (LegendIndex != -1)
            {
                TLegend * Legend = static_cast<TLegend*>(drawObjects[LegendIndex].Pointer);
                TList * elist = Legend->GetListOfPrimitives();
                int num = elist->GetEntries();
                for (int ie = 0; ie < num; ie++)
                {
                    TLegendEntry * en = static_cast<TLegendEntry*>( (*elist).At(ie) );
                    QString text = en->GetLabel();
                    TObject * p = nullptr;
                    int iObj = LegendLinks[ie].toInt();
                    if (iObj >= 0 && iObj < drawObjects.size())
                        p = drawObjects[iObj].Pointer;
                    en->SetObject(p);                       // will override the label
                    en->SetLabel(text.toLatin1().data());   // so restore it
                }
            }

            if (!drawObjects.isEmpty())
            {
                ABasketItem item;
                item.Name = ItemName;
                item.DrawObjects = drawObjects;
                item.Type = drawObjects.first().Pointer->ClassName();
                Basket << item;
                drawObjects.clear();
            }
        }
    }
    else
    {
        desc = (TNamed*)f.Get("BasketDescription");
        if (desc)
        {
            //old system
            QString text = desc->GetTitle();
            //qDebug() << "Basket description:"<<text;
            //qDebug() << "Number of keys:"<<f->GetListOfKeys()->GetEntries();

            QStringList sl = text.split('\n',QString::SkipEmptyParts);

            int numLines = sl.size();
            int basketSize =  numLines/2;
            //qDebug() << "Description lists" << basketSize << "objects in the basket";


            int indexFileObject = 0;
            if (numLines % 2 == 0 ) // should be even number of lines
            {
                for (int iDrawObject = 0; iDrawObject < basketSize; iDrawObject++ )
                {
                    //qDebug() << ">>>>Object #"<< iDrawObject;
                    QString name = sl[iDrawObject*2];
                    bool ok;
                    QStringList fields = sl[iDrawObject*2+1].split('|');
                    if (fields.size()<2)
                    {
                        qWarning()<<"Too short descr line";
                        ok=false;
                        break;
                    }

                    const QString sNumber = fields[0];

                    int numObj = sNumber.toInt(&ok);
                    if (!ok)
                    {
                        qWarning() << "Num obj convertion error!";
                        ok=false;
                        break;
                    }
                    if (numObj != fields.size()-1)
                    {
                        qWarning()<<"Number of objects vs option strings mismatch:"<<numObj<<fields.size()-1;
                        ok=false;
                        break;
                    }

                    //qDebug() << "Name:"<< name << "objects:"<< numObj;

                    QVector<ADrawObject> drawObjects;
                    for (int iDrawObj = 0; iDrawObj < numObj; iDrawObj++)
                    {
                        TKey *key = (TKey*)f.GetListOfKeys()->At(indexFileObject++);
                        //key->SetMotherDir(0);
                        //QString type = key->GetClassName();
                        //TString objName = key->GetName();
                        //qDebug() << "-->"<< iDrawObj <<"   "<<objName<<"  "<<type<<"   "<<fields[iDrawObj+1];

                        TObject * p = key->ReadObj();
                        if (p) drawObjects << ADrawObject(p, fields[iDrawObj+1]);
                        //qDebug() << p;

                        /*
                    TLegend * Legend = dynamic_cast<TLegend*>(p);
                    if (Legend)
                    {
                        TList * elist = Legend->GetListOfPrimitives();
                        int num = elist->GetEntries();
                        for (int ie = 0; ie < num; ie++)
                        {
                            TLegendEntry * en = static_cast<TLegendEntry*>( (*elist).At(ie));
                            qDebug() << "Entry obj:"<< en->GetObject();
                        }
                    }
                    */
                    }

                    if (!drawObjects.isEmpty())
                    {
                        ABasketItem item;
                        item.Name = name;
                        item.DrawObjects = drawObjects;
                        item.Type = drawObjects.first().Pointer->ClassName();
                        Basket << item;
                        drawObjects.clear();
                    }
                }
            }
            else ok = false;
        }
        else
        {
            f.Close();
            return QString("%1: this is not a valid ANTS2 basket file!").arg(fileName);
        }
    }

    f.Close();
    if (!ok) return QString("%1: corrupted basket file").arg(fileName);
    return "";
}

const QString ABasketManager::appendTxtAsGraph(const QString & fileName)
{
    QVector<double> x, y;
    QVector<QVector<double> *> V = {&x, &y};
    const QString res = LoadDoubleVectorsFromFile(fileName, V);
    if (!res.isEmpty()) return res;

    TGraph* gr = new TGraph(x.size(), x.data(), y.data());
    gr->SetMarkerStyle(20);
    ABasketItem item;
    item.Name = "Graph";
    item.DrawObjects << ADrawObject(gr, "APL");
    item.Type = gr->ClassName();
    Basket << item;

    return "";
}

const QString ABasketManager::appendTxtAsGraphErrors(const QString &fileName)
{
    QVector<double> x, y, err;
    QVector<QVector<double> *> V = {&x, &y, &err};
    const QString res = LoadDoubleVectorsFromFile(fileName, V);
    if (!res.isEmpty()) return res;

    TGraphErrors* gr = new TGraphErrors(x.size(), x.data(), y.data(), 0, err.data());
    gr->SetMarkerStyle(20);
    ABasketItem item;
    item.Name = "GraphErrors";
    item.DrawObjects << ADrawObject(gr, "APL");
    item.Type = gr->ClassName();
    Basket << item;

    return "";
}

void ABasketManager::appendRootHistGraphs(const QString & fileName)
{
    QByteArray ba = fileName.toLocal8Bit();
    const char *c_str = ba.data();
    TFile * f = new TFile(c_str);

    const int numKeys = f->GetListOfKeys()->GetEntries();
    //qDebug() << "File contains" << numKeys << "TKeys";

    for (int i = 0; i < numKeys; i++)
    {
        TKey * key = (TKey*)f->GetListOfKeys()->At(i);
        QString Type = key->GetClassName();
        QString Name = key->GetName();
        //qDebug() << i << Type << Name;

        if (Type.startsWith("TH") || Type.startsWith("TProfile") || Type.startsWith("TGraph"))
        {
            TObject * p = key->ReadObj();
            if (p)
            {
                ABasketItem item;
                    item.Name = Name;
                    item.DrawObjects << ADrawObject(p, "");
                    item.Type = p->ClassName();
                    if (item.Name.isEmpty()) item.Name = QString("%1#%2").arg(item.Type).arg(i);
                Basket << item;
                //qDebug() << "  appended";
            }
            else qWarning() << "Failed to read object of type" << Type << "from file " << fileName;
        }
        //else qDebug() << "  ignored";
    }

    f->Close();
}

void ABasketManager::reorder(const QVector<int> &indexes, int to)
{
    QVector< ABasketItem > ItemsToMove;
    for (int i = 0; i < indexes.size(); i++)
    {
        const int index = indexes.at(i);
        ItemsToMove << Basket.at(index);
        Basket[index]._flag = true;       // mark to be deleted
    }

    for (int i = 0; i < ItemsToMove.size(); i++)
    {
        Basket.insert(to, ItemsToMove.at(i));
        to++;
    }

    for (int i = Basket.size()-1; i >= 0; i--)
        if (Basket.at(i)._flag)
            Basket.remove(i);
}

int ABasketManager::findPointerInDrawObjects(const QVector<ADrawObject> &DrawObjects, TObject *obj) const
{
    for (int i=0; i<DrawObjects.size(); i++)
        if (DrawObjects.at(i).Pointer == obj) return i;
    return -1;
}
