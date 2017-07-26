#include "tmpobjhubclass.h"
#include "atrackrecords.h"

#include <QDebug>

#include "TH1.h"

RootDrawObj::RootDrawObj()
{
  Obj = 0;

  MarkerColor = 4; MarkerStyle = 20; MarkerSize = 1;
  LineColor = 4;   LineStyle = 1;    LineWidth = 1;
}

RootDrawObj::~RootDrawObj()
{
  if (Obj)
    {
      delete Obj;
      //qDebug() << "Deleting" << name << type;
    }
}

int ScriptDrawCollection::findIndexOf(QString name)
{
  for (int i=0; i<List.size(); i++)
    if (List.at(i).name == name) return i;
  return -1; //not found
}

void ScriptDrawCollection::append(TObject *obj, QString name, QString type)
{
  List.append(RootDrawObj());
  List.last().Obj = obj;
  List.last().name = name;
  List.last().type = type;
}

void TmpObjHubClass::ClearTracks()
{
    for (int i=0; i<TrackInfo.size(); i++) delete TrackInfo[i];
    TrackInfo.clear();
}

void TmpObjHubClass::ClearTmpHists()
{
    for (TH1D* h : tmpHists) delete h;
    tmpHists.clear();
}

void TmpObjHubClass::Clear()
{
    ClearTracks();
    ClearTmpHists();
    FoundPeaks.clear();
}

TmpObjHubClass::TmpObjHubClass()
{
  PreEnAdd = 0;
  PreEnMulti = 1.0;
}

TmpObjHubClass::~TmpObjHubClass()
{
  Clear();
}
