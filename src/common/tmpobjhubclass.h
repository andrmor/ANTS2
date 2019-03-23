#ifndef TMPOBJHUBCLASS_H
#define TMPOBJHUBCLASS_H

#include "arootobjcollection.h"

#include <QObject>
#include <QVector>

class TH1D;

class TmpObjHubClass : public QObject
{
    Q_OBJECT

public:
  ~TmpObjHubClass();

  //for script units
  ARootObjCollection Graphs;
  ARootObjCollection Hists;
  ARootObjCollection Trees;

  //preprocessing: energy channel
  double PreEnAdd = 0;
  double PreEnMulti = 1.0;

  //signal->photoelectrons: from peaks
  QVector<TH1D*> PeakHists;
  void ClearTmpHistsPeaks();
  QVector< QVector<double> > FoundPeaks;
  QVector<double> ChPerPhEl_Peaks;

  //signal->photoelectrons: from statistics
  QVector<TH1D*> SigmaHists;
  void ClearTmpHistsSigma2();
  QVector<double> ChPerPhEl_Sigma2;

public slots:
  void Clear();

};

#endif // TMPOBJHUBCLASS_H
