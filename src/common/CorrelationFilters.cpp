#include "CorrelationFilters.h"

#include "apmgroupsmanager.h"
#include "eventsdataclass.h"
#include "ajsontools.h"
#include "apositionenergyrecords.h"

#include <QDebug>

CorrelationFilterStructure::CorrelationFilterStructure(CorrelationFilterStructure *from)
{
    X = 0;
    Y = 0;
    Cut = 0;
    copyFrom(from);
}

CorrelationFilterStructure::~CorrelationFilterStructure()
{
    if (X) delete X;
    if (Y) delete Y;
    if (Cut) delete Cut;
}

void CorrelationFilterStructure::setCorrelationUnitX(CorrelationUnitGenericClass *unit)
{
    if (X) delete X;
    X = unit;
}

void CorrelationFilterStructure::setCorrelationUnitY(CorrelationUnitGenericClass *unit)
{
    if (Y) delete Y;
    Y = unit;
}

void CorrelationFilterStructure::setCut(CorrelationCutGenericClass *cut)
{
    if (Cut) delete Cut;
    Cut = cut;
}

void CorrelationFilterStructure::copyFrom(CorrelationFilterStructure *source)
{
  Active = source->Active;
  BinX = source->BinX;
  BinY = source->BinY;
  AutoSize = source->AutoSize;
  minX = source->minX;
  maxX = source->maxX;
  minY = source->minY;
  maxY = source->maxY;

  setCorrelationUnitX(source->getCorrelationUnitX()->getClone());
  setCorrelationUnitY(source->getCorrelationUnitY()->getClone());
  setCut(source->getCut()->getClone());
}

void CorrelationFilterStructure::saveJSON(QJsonObject &json) const
{
  json["Active"] = Active;
  json["BinX"] = BinX;
  json["BinY"] = BinY;
  json["AutoSize"] = AutoSize;
  json["minX"] = minX;
  json["maxX"] = maxX;
  json["minY"] = minY;
  json["maxY"] = maxY;

  QJsonArray jsonArray;
  QJsonObject tmpX, tmpY;
  X->CorrelationUnitGenericClass::saveJSON(tmpX);
  tmpX["Type"] = X->getType();
  jsonArray.append(tmpX);
  Y->CorrelationUnitGenericClass::saveJSON(tmpY);
  tmpY["Type"] = Y->getType();
  jsonArray.append(tmpY);
  json["XY"] = jsonArray;
  QJsonObject cut;
  Cut->CorrelationCutGenericClass::saveJSON(cut);
  cut["Type"] = Cut->getType();
  json["Cut"] = cut;
}

CorrelationFilterStructure *CorrelationFilterStructure::createFromJson(QJsonObject &json, APmGroupsManager* PMgroups, int ThisPMgroup, EventsDataClass *eventsDataHub)
{
    QJsonArray jsonXY = json["XY"].toArray();

    // unit1
    QJsonObject jsonX = jsonXY[0].toObject();
    QString type;
    parseJson(jsonX, "Type", type);
    QJsonArray ar = jsonX["Array"].toArray();
    QList<int> Channels;
    for (int i=0; i<ar.size(); i++)
        Channels.append(ar[i].toInt());
    CorrelationUnitGenericClass* X = CorrUnitCreator(type, PMgroups, ThisPMgroup, eventsDataHub);
    X->Channels = Channels;

    //unit2
    QJsonObject jsonY = jsonXY[1].toObject();
    parseJson(jsonY, "Type", type);
    ar = jsonY["Array"].toArray();
    Channels.clear();
    for (int i=0; i<ar.size(); i++)
        Channels.append(ar[i].toInt());
    CorrelationUnitGenericClass* Y = CorrUnitCreator(type, PMgroups, ThisPMgroup, eventsDataHub);
    Y->Channels = Channels;

    //cut
    CorrelationCutGenericClass* cut;
    QJsonObject cutJSON = json["Cut"].toObject();
    QString CutType = cutJSON["Type"].toString();
    if (CutType == "line") cut = new Cut_Line();
    else if (CutType == "ellipse") cut = new Cut_Ellipse();
    else if (CutType == "polygon") cut = new Cut_Polygon();
    else
      {
        qWarning("Unknown cut type!");
        return 0;
      }
    int CutOption;
    parseJson(cutJSON, "CutOption", CutOption);
    cut->CutOption = CutOption;
    ar = cutJSON["Data"].toArray();
    QList<double> Data;
    for (int id=0; id<ar.size(); id++) Data.append(ar[id].toDouble());
    cut->Data = Data;

    //creating filter
    CorrelationFilterStructure* tmp = new CorrelationFilterStructure(X, Y, cut);

    //misc
    parseJson(json, "Active", tmp->Active);
    parseJson(json, "BinX", tmp->BinX);
    parseJson(json, "BinY", tmp->BinY);
    parseJson(json, "AutoSize", tmp->AutoSize);
    parseJson(json, "minX", tmp->minX);
    parseJson(json, "maxX", tmp->maxX);
    parseJson(json, "minY", tmp->minY);
    parseJson(json, "maxY", tmp->maxY);

    return tmp;
}

CorrelationUnitGenericClass* CorrelationFilterStructure::CorrUnitCreator(QString Type, APmGroupsManager *PMgroups, int ThisPMgroup, EventsDataClass *EventsDataHub)
{
  if (Type == "SingleChannel") return new CU_SingleChannel(QList<int>(), EventsDataHub, PMgroups, ThisPMgroup);
  if (Type == "SumAllChannels") return new CU_SumAllChannels(QList<int>(), EventsDataHub, PMgroups, ThisPMgroup);
  if (Type == "SumChannels") return new CU_SumChannels(QList<int>(), EventsDataHub, PMgroups, ThisPMgroup);
  if (Type == "LoadedEnergy") return new CU_LoadedEnergy(QList<int>(), EventsDataHub, PMgroups, ThisPMgroup);
  if (Type == "RecE") return new CU_RecE(QList<int>(), EventsDataHub, PMgroups, ThisPMgroup);
  if (Type == "Chi2") return new CU_Chi2(QList<int>(), EventsDataHub, PMgroups, ThisPMgroup);
  if (Type == "RecX") return new CU_RecX(QList<int>(), EventsDataHub, PMgroups, ThisPMgroup);
  if (Type == "RecY") return new CU_RecY(QList<int>(), EventsDataHub, PMgroups, ThisPMgroup);
  if (Type == "RecZ") return new CU_RecZ(QList<int>(), EventsDataHub, PMgroups, ThisPMgroup);

  //if unknown type, returns 0
  return 0;
}

const TString CU_SingleChannel::getAxisTitle() { TString tmp = "Ch"; tmp += Channels[0]; return tmp;}

double CU_SingleChannel::getValue(int iev)
{
    //return EventsDataHub->Events.at(iev)[Channels[0]] * Detector->PMs->getGain(Channels[0]);
  return EventsDataHub->Events.at(iev)[Channels[0]] * PMgroups->Groups.at(ThisPMgroup)->PMS.at(Channels[0]).gain;
}

CU_SingleChannel *CU_SingleChannel::getClone()
{
  CU_SingleChannel* copy = new CU_SingleChannel(Channels, EventsDataHub, PMgroups, ThisPMgroup);
  return copy;
}

double CU_SumAllChannels::getValue(int iev)
{
  double sum = 0;
  for (int i=0; i<Channels.size(); i++)
    {
      int thisPM = Channels[i];
      //sum += EventsDataHub->Events.at(iev)[thisPM] * Detector->PMs->getGain(thisPM);
      sum += EventsDataHub->Events.at(iev)[thisPM] * PMgroups->Groups.at(ThisPMgroup)->PMS.at(thisPM).gain;
    }
  return sum;
}

CU_SumAllChannels *CU_SumAllChannels::getClone()
{
  CU_SumAllChannels* copy = new CU_SumAllChannels(Channels, EventsDataHub, PMgroups, ThisPMgroup);
  return copy;
}


const TString CU_SumChannels::getAxisTitle()
{
    TString tmp = "Ch";
    for (int i=0; i<Channels.size(); i++)
    {
        tmp += Channels[i];
        tmp += "+";
    }
    if (tmp.Length()>0) tmp.Resize(tmp.Length()-1);
    return tmp;
}

double CU_SumChannels::getValue(int iev)
{
    double sum = 0;
    for (int i=0; i<Channels.size(); i++)
    {
        int thisPM = Channels[i];
        //sum += EventsDataHub->Events.at(iev)[thisPM] * Detector->PMs->getGain(thisPM);
        sum += EventsDataHub->Events.at(iev)[thisPM] * PMgroups->Groups.at(ThisPMgroup)->PMS.at(thisPM).gain;
    }
  return sum;
}

CU_SumChannels *CU_SumChannels::getClone()
{
  CU_SumChannels* copy = new CU_SumChannels(Channels, EventsDataHub, PMgroups, ThisPMgroup);
  return copy;
}

double CU_LoadedEnergy::getValue(int iev)
{
    return EventsDataHub->Events.at(iev).last();
}

CU_LoadedEnergy *CU_LoadedEnergy::getClone()
{
  CU_LoadedEnergy* copy = new CU_LoadedEnergy(Channels, EventsDataHub, PMgroups, ThisPMgroup);
  return copy;
}

double CU_RecE::getValue(int iev)
{
    return EventsDataHub->ReconstructionData.at(ThisPMgroup).at(iev)->Points[0].energy;
}

CU_RecE *CU_RecE::getClone()
{
  CU_RecE* copy = new CU_RecE(Channels, EventsDataHub, PMgroups, ThisPMgroup);
  return copy;
}

double CU_Chi2::getValue(int iev)
{
    return EventsDataHub->ReconstructionData.at(ThisPMgroup).at(iev)->chi2;
}

CU_Chi2 *CU_Chi2::getClone()
{
  CU_Chi2* copy = new CU_Chi2(Channels, EventsDataHub, PMgroups, ThisPMgroup);
  return copy;
}

double CU_RecX::getValue(int iev)
{
    return EventsDataHub->ReconstructionData.at(ThisPMgroup).at(iev)->Points[0].r[0];
}

CU_RecX *CU_RecX::getClone()
{
  CU_RecX* copy = new CU_RecX(Channels, EventsDataHub, PMgroups, ThisPMgroup);
  return copy;
}

double CU_RecY::getValue(int iev)
{
    return EventsDataHub->ReconstructionData.at(ThisPMgroup).at(iev)->Points[0].r[1];
}

CU_RecY *CU_RecY::getClone()
{
  CU_RecY* copy = new CU_RecY(Channels, EventsDataHub, PMgroups, ThisPMgroup);
  return copy;
}

double CU_RecZ::getValue(int iev)
{
    return EventsDataHub->ReconstructionData.at(ThisPMgroup).at(iev)->Points[0].r[2];
}

CU_RecZ *CU_RecZ::getClone()
{
  CU_RecZ* copy = new CU_RecZ(Channels, EventsDataHub, PMgroups, ThisPMgroup);
  return copy;
}


bool Cut_Line::filter(double val1, double val2)
{
  if (CutOption == 1)
    {
      if ( (Data[0]*val1 + Data[1]*val2) > Data[2] ) return false; //cuts above
      else return true;
    }
  else
    {
      if ( (Data[0]*val1 + Data[1]*val2) > Data[2] ) return true; //cuts below
      else return false;
    }
}

Cut_Line *Cut_Line::getClone()
{
  Cut_Line* copy = new Cut_Line();
  copy->CutOption = CutOption; copy->Data = Data;
  return copy;
}


bool Cut_Ellipse::filter(double val1, double val2)
{
  // 0-x 1-y 2-r1 3-r2 4-(-angle)
  bool outside = false;
  if ( (Data[2]<1.0e-10 && Data[2]>-1.0e-10) || (Data[3]<1.0e-10 && Data[3]>-1.0e-10) ) outside = true;
  else
    {
      double sinA = sin(-Data[4]*0.017453292519);
      double cosA = cos(-Data[4]*0.017453292519);
      double dx = val1 - Data[0];
      double dy = val2 - Data[1];

      double tmp1 = cosA*dx - sinA*dy;
      double tmp2 = sinA*dx + cosA*dy;
      double rad = tmp1*tmp1/Data[2]/Data[2] + tmp2*tmp2/Data[3]/Data[3];

      if (rad > 1) outside = true;
    }

  if (CutOption == 0) return !outside; //cut outside
  else return outside;
}

Cut_Ellipse *Cut_Ellipse::getClone()
{
  Cut_Ellipse* copy = new Cut_Ellipse();
  copy->CutOption = CutOption; copy->Data = Data;
  return copy;
}


bool Cut_Polygon::filter(double val1, double val2)
{
  QVector<QPointF> tmp(0);
  for (int i=0; i<Data.size()/2; i++) tmp.append(QPointF(Data[i*2], Data[i*2+1]));
  QPolygonF poly(tmp);

  bool inside = poly.containsPoint(QPointF(val1, val2), Qt::OddEvenFill);

  if (CutOption == 0) return inside;
  else return !inside;
}

Cut_Polygon *Cut_Polygon::getClone()
{
  Cut_Polygon* copy = new Cut_Polygon();
  copy->CutOption = CutOption; copy->Data = Data;
  return copy;
}

void CorrelationUnitGenericClass::saveJSON(QJsonObject &json) const
{
    QJsonArray jsonArray;
    for (int i=0; i<Channels.size(); i++) jsonArray.append(Channels[i]);
    json["Array"] = jsonArray;
}

void CorrelationCutGenericClass::saveJSON(QJsonObject &json) const
{
    json["CutOption"] = CutOption;
    QJsonArray jsonArray;
    for (int i=0; i<Data.size(); i++) jsonArray.append(Data[i]);
    json["Data"] = jsonArray;
}
