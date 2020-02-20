#ifndef CORRELATIONFILTERS_H
#define CORRELATIONFILTERS_H

#include <QtGui/QPolygonF>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>

#include "TString.h"

class EventsDataClass;
class APmGroupsManager;

//=================Correlation filter=======================
class CorrelationUnitGenericClass
{
  public:
    CorrelationUnitGenericClass(QList<int> Channels, EventsDataClass *EventsDataHub, APmGroupsManager* PMgroups, int ThisPMgroup) :
        Channels(Channels), EventsDataHub(EventsDataHub), PMgroups(PMgroups), ThisPMgroup(ThisPMgroup) {}
    virtual ~CorrelationUnitGenericClass(){}

    virtual const QString getType() {return "undefined";}  //type
    virtual const TString getAxisTitle() {return "undefined";}  //axe title used on correlation histogram
    virtual int getCOBindex() const {return 0;} //index used in ui

    QList<int> Channels; //additional data, e.g. channel number
    EventsDataClass *EventsDataHub;
    APmGroupsManager* PMgroups;
    int ThisPMgroup;

    virtual double getValue(int) {return 0;} //gives value for the event number
    virtual bool isRequireReconstruction() {return true;} //does require recon data to present?

    virtual CorrelationUnitGenericClass* getClone()=0;// {return new CorrelationUnitGenericClass(); } //never used not overloaded

    virtual void saveJSON(QJsonObject &json) const;
};

class CorrelationCutGenericClass
{
  public:
    virtual ~CorrelationCutGenericClass(){}

    virtual const QString getType() {return "undefined";}  //type
    virtual int getCOBindex() const {return 0;} //index used in ui

    int CutOption; //e.g. "left_above" for line or "inside" for ellipse
    QList<double> Data; //e.g. A B C for line or ellipse data or X Y for polygon nodes

    virtual bool filter(double , double ) {return false;} //true - passes

    virtual CorrelationCutGenericClass* getClone()=0;// {return new CorrelationCutGenericClass();} //never used not overloaded

    virtual void saveJSON(QJsonObject &json) const;
};

struct CorrelationFilterStructure
{
   //public
     bool Active;

     //indication    
     int BinX;
     int BinY;
     bool AutoSize;
     double minX;
     double maxX;
     double minY;
     double maxY;

   private:
     //correlation units
     CorrelationUnitGenericClass* X;
     CorrelationUnitGenericClass* Y;

     //cut
     CorrelationCutGenericClass* Cut;

   public:
     //CorrelationFilterStructure() {X = new CorrelationUnitGenericClass(); Y = new CorrelationUnitGenericClass(); Cut = new CorrelationCutGenericClass(); }
     CorrelationFilterStructure(CorrelationUnitGenericClass* unit1, CorrelationUnitGenericClass* unit2, CorrelationCutGenericClass* cut)
       {X = unit1; Y = unit2; Cut = cut;}
     CorrelationFilterStructure(CorrelationFilterStructure *from);
     ~CorrelationFilterStructure();

     void setCorrelationUnitX(CorrelationUnitGenericClass* unit);
     void setCorrelationUnitY(CorrelationUnitGenericClass* unit);
     CorrelationUnitGenericClass* getCorrelationUnitX() {return X;}
     CorrelationUnitGenericClass* getCorrelationUnitY() {return Y;}

     void setCut(CorrelationCutGenericClass* cut);
     CorrelationCutGenericClass* getCut() {return Cut;}

     void copyFrom(CorrelationFilterStructure* source);

     void saveJSON(QJsonObject &json) const;

     static CorrelationFilterStructure* createFromJson(QJsonObject &json, APmGroupsManager* PMgroups, int ThisPMgroup, EventsDataClass* eventsDataHub);
     static CorrelationUnitGenericClass *CorrUnitCreator(QString Type, APmGroupsManager *PMgroups, int ThisPMgroup, EventsDataClass *EventsDataHub);
};

//======= subtypes: corr units ==========
class CU_SingleChannel : public CorrelationUnitGenericClass
{
  public:
    CU_SingleChannel(QList<int> Channels, EventsDataClass *EventsDataHub, APmGroupsManager* PMgroups, int ThisPMgroup) :
        CorrelationUnitGenericClass(Channels, EventsDataHub, PMgroups, ThisPMgroup){}

    const QString getType() { return "SingleChannel";}
    int getCOBindex() const {return 0;}
    virtual const TString getAxisTitle();

    double getValue(int iev);
    bool isRequireReconstruction() {return false;}

    CU_SingleChannel* getClone();
};

class CU_SumAllChannels : public CorrelationUnitGenericClass
{
  public:
    CU_SumAllChannels(QList<int> Channels, EventsDataClass *EventsDataHub, APmGroupsManager* PMgroups, int ThisPMgroup) :
        CorrelationUnitGenericClass(Channels, EventsDataHub, PMgroups, ThisPMgroup){}

    const QString getType() { return "SumAllChannels";}
    int getCOBindex() const {return 1;}
    virtual const TString getAxisTitle() { return "Sum all ch";}

    double getValue(int iev);
    bool isRequireReconstruction() {return false;}

    CU_SumAllChannels* getClone();
};

class CU_SumChannels : public CorrelationUnitGenericClass
{
  public:
    CU_SumChannels(QList<int> Channels, EventsDataClass *EventsDataHub, APmGroupsManager* PMgroups, int ThisPMgroup) :
        CorrelationUnitGenericClass(Channels, EventsDataHub, PMgroups, ThisPMgroup){}

    const QString getType() { return "SumChannels";}
    int getCOBindex() const {return 2;}
    virtual const TString getAxisTitle();

    double getValue(int iev);
    bool isRequireReconstruction() {return false;}

    CU_SumChannels* getClone();
};

class CU_TrueOrLoadedEnergy : public CorrelationUnitGenericClass
{
  public:
    CU_TrueOrLoadedEnergy(QList<int> Channels, EventsDataClass *EventsDataHub, APmGroupsManager* PMgroups, int ThisPMgroup) :
        CorrelationUnitGenericClass(Channels, EventsDataHub, PMgroups, ThisPMgroup){}

    const QString getType() { return "LoadedEnergy";}
    int getCOBindex() const {return 3;}
    virtual const TString getAxisTitle() { return "True or loaded energy";}

    double getValue(int iev);
    bool isRequireReconstruction() {return false;}

    CU_TrueOrLoadedEnergy* getClone();
};

class CU_RecE : public CorrelationUnitGenericClass
{
  public:
     CU_RecE(QList<int> Channels, EventsDataClass *EventsDataHub, APmGroupsManager* PMgroups, int ThisPMgroup) :
        CorrelationUnitGenericClass(Channels, EventsDataHub, PMgroups, ThisPMgroup){}

     const QString getType() { return "RecE";}
     int getCOBindex() const {return 4;}
     virtual const TString getAxisTitle() { return "Rec energy";}
     double getValue(int iev);
     CU_RecE* getClone();
};

class CU_Chi2 : public CorrelationUnitGenericClass
{
  public:
     CU_Chi2(QList<int> Channels, EventsDataClass *EventsDataHub, APmGroupsManager* PMgroups, int ThisPMgroup) :
        CorrelationUnitGenericClass(Channels, EventsDataHub, PMgroups, ThisPMgroup){}

     const QString getType() { return "Chi2";}
     int getCOBindex() const {return 5;}
     virtual const TString getAxisTitle() { return "Chi2";}
     double getValue(int iev);
     CU_Chi2* getClone();
};

class CU_RecX : public CorrelationUnitGenericClass
{
  public:
     CU_RecX(QList<int> Channels, EventsDataClass *EventsDataHub, APmGroupsManager* PMgroups, int ThisPMgroup) :
        CorrelationUnitGenericClass(Channels, EventsDataHub, PMgroups, ThisPMgroup){}

     const QString getType() { return "RecX";}
     int getCOBindex() const {return 6;}
     virtual const TString getAxisTitle() { return "Rec X";}
     double getValue(int iev);
     CU_RecX* getClone();
};
class CU_RecY : public CorrelationUnitGenericClass
{
  public:
     CU_RecY(QList<int> Channels, EventsDataClass *EventsDataHub, APmGroupsManager* PMgroups, int ThisPMgroup) :
        CorrelationUnitGenericClass(Channels, EventsDataHub, PMgroups, ThisPMgroup){}

     const QString getType() { return "RecY";}
     int getCOBindex() const {return 7;}
     virtual const TString getAxisTitle() { return "Rec Y";}
     double getValue(int iev);
     CU_RecY* getClone();
};
class CU_RecZ : public CorrelationUnitGenericClass
{
  public:
     CU_RecZ(QList<int> Channels, EventsDataClass *EventsDataHub, APmGroupsManager* PMgroups, int ThisPMgroup) :
        CorrelationUnitGenericClass(Channels, EventsDataHub, PMgroups, ThisPMgroup){}

     const QString getType() { return "RecZ";}
     int getCOBindex() const {return 8;}
     virtual const TString getAxisTitle() { return "Rec Z";}
     double getValue(int iev);
     CU_RecZ* getClone();
};


//======================= subtypes: cuts ===========================
class Cut_Line : public CorrelationCutGenericClass
{
  public:
    const QString getType() {return "line";}
    int getCOBindex() const {return 0;}

    bool filter(double val1, double val2);
    Cut_Line* getClone();
};

class Cut_Ellipse : public CorrelationCutGenericClass
{
  public:
    const QString getType() {return "ellipse";}
    int getCOBindex() const {return 1;}

    bool filter(double val1, double val2);
    Cut_Ellipse* getClone();
};

class Cut_Polygon : public CorrelationCutGenericClass
{
  public:
    const QString getType() {return "polygon";}
    int getCOBindex() const {return 2;}

    bool filter(double val1, double val2);
    Cut_Polygon* getClone();
};


#endif // CORRELATIONFILTERS_H
