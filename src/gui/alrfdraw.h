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
    int     datais = 1;              //0-scan, 1-recon data
    bool    plot_lrf = true;
    bool    draw_second = false;     //draw second LRF curve
    bool    plot_data = false;
    bool    plot_diff = false;
    bool    fixed_min = false;
    bool    fixed_max = false;
    double  minDraw = 0;
    double  maxDraw = 100;
    int     CurrentGroup = 0;
    bool    check_z = false;
    double  z0 = 0;
    double  dz = 1.0;
    bool    scale_by_energy = false;
    int     FunctionPointsX = 30;    //from glob set
    int     FunctionPointsY = 30;    //from glob set
    int     bins = 200;
    bool    showNodePositions = false;

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
    EventsDataClass    *EventsDataHub;
    pms                *PMs;
    GraphWindowClass   *GraphWindow;
    ALrfDrawSettings    Options;
    QString             LastError;

    bool fUseOldModule;

    bool extractOptionsAndVerify(int PMnumber, const QJsonObject &json);
    void reportError(const QString& text);
};

#endif // ALRFDRAW_H
