#ifndef TMPOBJHUBCLASS_H
#define TMPOBJHUBCLASS_H

#include <QList>
#include <QString>
#include <QVector>

class TObject;
class TrackHolderClass;

class RootDrawObj
{
  public:
    TObject* Obj;
    QString name; // it is also the title
    QString type; // object type (e.g. TH1D)

    QString Xtitle, Ytitle, Ztitle;
    int MarkerColor, MarkerStyle, MarkerSize, LineColor, LineStyle, LineWidth;

    RootDrawObj();
    ~RootDrawObj();
};

class ScriptDrawCollection
{
public:
   QList<RootDrawObj> List;

   int findIndexOf(QString name); //returns -1 if not found
   void append(TObject* obj, QString name, QString type);
   void clear() {List.clear();}
};


//=================================================================
class TmpObjHubClass
{
public:  
  ScriptDrawCollection ScriptDrawObjects;

  double PreEnAdd, PreEnMulti;

  QVector<TrackHolderClass*> TrackInfo;
  void ClearTracks();

  TmpObjHubClass();
  ~TmpObjHubClass();
};

#endif // TMPOBJHUBCLASS_H
