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

bool ScriptDrawCollection::remove(QString name)
{
    for (int i=0; i<List.size(); i++)
      if (List.at(i).name == name)
      {
          List.removeAt(i);
          return true;
      }
    return false; //not found
}

void ScriptDrawCollection::append(TObject *obj, QString name, QString type)
{
  List.append(RootDrawObj());
  List.last().Obj = obj;
  List.last().name = name;
  List.last().type = type;
}

void ScriptDrawCollection::removeAllHists()
{
    for (int i=List.size()-1; i>-1; i--)
    {
        QString type = List.at(i).type;
        if (type == "TH1D" || type == "TH2D")
            List.removeAt(i);
    }
}

void ScriptDrawCollection::removeAllGraphs()
{
    for (int i=List.size()-1; i>-1; i--)
    {
        QString type = List.at(i).type;
        if (type == "TGraph")
            List.removeAt(i);
    }
}

void TmpObjHubClass::ClearTracks()
{
    for (int i=0; i<TrackInfo.size(); i++) delete TrackInfo[i];
    TrackInfo.clear();
}

void TmpObjHubClass::ClearTmpHistsPeaks()
{
    for (TH1D* h : PeakHists) delete h;
    PeakHists.clear();
}

void TmpObjHubClass::ClearTmpHistsSigma2()
{
    for (TH1D* h : SigmaHists) delete h;
    SigmaHists.clear();
}

void TmpObjHubClass::Clear()
{
    ClearTracks();
    ClearTmpHistsPeaks();
    ClearTmpHistsSigma2();
    FoundPeaks.clear();
    ChPerPhEl_Peaks.clear();
    ChPerPhEl_Sigma2.clear();
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
