#ifndef ALRFDRAW_H
#define ALRFDRAW_H

#include <QJsonObject>

class ALrfModuleSelector;
class EventsDataClass;
class GraphWindowClass;
class pms;
class TF1;
class TH2D;

class ALrfDrawSettings
{
public:
    int datais;             //0-scan, 1-recon data
    bool plot_lrf;
    bool draw_second;       //draw second LRF curve
    bool plot_data;
    bool plot_diff;   
    bool fixed_min;
    bool fixed_max;
    double minDraw;
    double maxDraw;
    int CurrentGroup;
    bool check_z;
    double z0;
    double dz;    
    bool scale_by_energy;
    int FunctionPointsX;   //from glob set
    int FunctionPointsY;   //from glob set
    int bins;
    bool showNodePositions;


    ALrfDrawSettings() :
        datais(1), draw_second(false), plot_lrf(true), plot_data(false), plot_diff(false),
        fixed_min(false), fixed_max(false), minDraw(0), maxDraw(100), CurrentGroup(0),
        check_z(false), z0(0), dz(1), scale_by_energy(false),
        FunctionPointsX(30), FunctionPointsY(30), bins(200), showNodePositions(false) {}

    bool ReadFromJson(const QJsonObject &json);
};

class ALrfDraw
{
public:
    ALrfDraw(ALrfModuleSelector* LRFs, bool fUseOldModule, EventsDataClass* EventsDataHub, pms* PMs, GraphWindowClass *GraphWindow);

    bool DrawRadial(int ipm, const QJsonObject &json);
    bool DrawXY(int ipm, const QJsonObject &json);

private:
    ALrfModuleSelector *LRFs;
    EventsDataClass *EventsDataHub;
    pms *PMs;
    GraphWindowClass *GraphWindow;
    ALrfDrawSettings Options;
    QString LastError;

    bool fUseOldModule;

    bool extractOptionsAndVerify(int PMnumber, const QJsonObject &json);
    void reportError(QString text);
};

#endif // ALRFDRAW_H
