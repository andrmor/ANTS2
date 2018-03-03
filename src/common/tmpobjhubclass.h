#ifndef TMPOBJHUBCLASS_H
#define TMPOBJHUBCLASS_H

#include <QList>
#include <QString>
#include <QVector>
#include <QObject>

#include "arootobjcollection.h"

class ARootGraphRecord;
class ARootHistRecord;

class TObject;
class TrackHolderClass;
class TH1D;
class TTree;

class TmpObjHubClass : public QObject
{
    Q_OBJECT
public:
  TmpObjHubClass();
  ~TmpObjHubClass();

  ARootObjCollection Graphs;
  ARootObjCollection Hists;
  ARootObjCollection Trees;

  double PreEnAdd, PreEnMulti;

  QVector<TrackHolderClass*> TrackInfo;
  void ClearTracks();

  QVector<TH1D*> PeakHists;
  void ClearTmpHistsPeaks();
  QVector< QVector<double> > FoundPeaks;
  QVector<double> ChPerPhEl_Peaks;

  QVector<TH1D*> SigmaHists;
  void ClearTmpHistsSigma2();
  QVector<double> ChPerPhEl_Sigma2;

public slots:
  void Clear();

};

#endif // TMPOBJHUBCLASS_H
