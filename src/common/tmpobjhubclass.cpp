#include "tmpobjhubclass.h"
#include "atrackrecords.h"

#include <QDebug>

#include "TH1.h"
#include "TTree.h"


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
    //  qDebug() << ">>> TMPHub: Clear requested";
    ClearTracks();
    ClearTmpHistsPeaks();
    ClearTmpHistsSigma2();
    FoundPeaks.clear();
    ChPerPhEl_Peaks.clear();
    ChPerPhEl_Sigma2.clear();
    //  qDebug() << ">>> TMPHub: Clear done!";
}

TmpObjHubClass::TmpObjHubClass()
{
  PreEnAdd = 0;
  PreEnMulti = 1.0;
}

TmpObjHubClass::~TmpObjHubClass()
{
  Clear();

  Graphs.clear();
  Hists.clear();
}

AScriptTreeCollection::~AScriptTreeCollection()
{
    clearAll();
}

bool AScriptTreeCollection::addTree(QString name, TTree *tree)
{
    if ( findIndexOf(name) != -1) return false;
    Trees.append(ATreeCollectionRecord(name, tree));
    return true;
}

TTree *AScriptTreeCollection::getTree(QString name)
{
    int index = findIndexOf(name);
    if (index == -1) return 0;

    return Trees[index].tree;
}

int AScriptTreeCollection::findIndexOf(QString name)
{
    for (int i=0; i<Trees.size(); i++)
      if (Trees.at(i).name == name) return i;
    return -1; //not found
}

void AScriptTreeCollection::remove(QString name)
{
    for (int i=0; i<Trees.size(); i++)
      if (Trees.at(i).name == name)
      {
          delete Trees[i].tree;
          Trees.removeAt(i);
          return;
      }
}

void AScriptTreeCollection::clearAll()
{
    for (int i=0; i<Trees.size(); i++)
        delete Trees[i].tree;
    Trees.clear();
}
