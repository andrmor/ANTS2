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

class ATreeCollectionRecord
{
public:
    QString name;
    TTree* tree;

    ATreeCollectionRecord(QString name, TTree* tree) : name(name), tree(tree) {}
    ATreeCollectionRecord() : name("Undefined"), tree(0) {}
};

class AScriptTreeCollection
{
public:
    AScriptTreeCollection(){}
    ~AScriptTreeCollection();

    QVector<ATreeCollectionRecord> Trees;

    bool    addTree(QString name, TTree* tree);
    TTree*  getTree(QString name);
    int     findIndexOf(QString name); //returns -1 if not found
    void    remove(QString name);
    void    clearAll();
};

//=================================================================
class TmpObjHubClass : public QObject
{
    Q_OBJECT
public:
  TmpObjHubClass();
  ~TmpObjHubClass();

  ARootObjCollection Graphs;
  ARootObjCollection Hists;
  AScriptTreeCollection Trees;

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
