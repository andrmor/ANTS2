#ifndef AEVENTFILTERINGSETTINGS_H
#define AEVENTFILTERINGSETTINGS_H

#include <QVector>
#include <QtGui/QPolygonF>
#include <QJsonObject>
#include <QString>

#include "TString.h"

struct CorrelationFilterStructure;
//class AGeoObject;
//class DetectorClass;
class APmGroupsManager;
class EventsDataClass;

class AEventFilteringSettings
{
public:
    AEventFilteringSettings() {}

    //bool readFromJson(QJsonObject& json, DetectorClass* Detector, EventsDataClass* EventsDataHub);
    bool readFromJson(QJsonObject& json, APmGroupsManager* PMgroups, int ThisPMgroup, EventsDataClass* EventsDataHub);

    //filters - threaded
      //event number filter
    bool fEventNumberFilter;
    int EventNumberFilterMin, EventNumberFilterMax;
    bool fMultipleScanEvents;
      //sum cutoff
    bool fSumCutOffFilter;
    bool fSumCutUsesGains;
    bool fCutOffsForPassivePMs;
    double SumCutOffMin, SumCutOffMax;
      //individual cut-off filters
    bool fCutOffFilter;
      //rec energy filter
    bool fEnergyFilter;
    double EnergyFilterMin, EnergyFilterMax;
      //loaded energy filter
    bool fLoadedEnergyFilter;
    double LoadedEnergyFilterMin, LoadedEnergyFilterMax;
      //chi2 filter
    bool fChi2Filter;
    double Chi2FilterMin, Chi2FilterMax;
      //spatial: object
    bool fSpF_LimitToObj;
    TString SpF_LimitToObj;
      //spatial: custom
    bool fSpF_custom;
    int SpF_RecOrScan;
    int SpF_shape;
    double SpF_SizeX, SpF_SizeY;
    double SpF_Diameter;
    double SpF_Side, SpF_Angle;
    double SpF_X0, SpF_Y0;
    int SpF_CutOutsideInside;
    bool fSpF_allZ;
    double SpF_Zfrom, SpF_Zto;
    double SpF_halfSizeX, SpF_halfSizeY, SpF_radius2; // calculated in this module
    QPolygonF SpF_polygon; // calculated in this module

    //filters - after thread data merge
      //correlationFilter
    bool fCorrelationFilters;
    QVector <CorrelationFilterStructure* > CorrelationFilters; //link to filters configured at ReconstructionWindow
      //kNN filter
    bool fKNNfilter;
    double KNNfilterMin, KNNfilterMax;
    int KNNfilterAverageOver;

    //misc
    QString ErrorString;
};

#endif // AEVENTFILTERINGSETTINGS_H
