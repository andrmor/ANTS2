#ifndef APHOTONMODESETTINGS_H
#define APHOTONMODESETTINGS_H

#include <QString>
#include <QVector>

class QJsonObject;

// potons per node
typedef std::pair<double, double> ADPair;
class APhotonSim_PerNodeSettings
{
public:
    enum APhPerNodeEnum {Constant = 0, Uniform = 1, Gauss = 2, Custom = 3, Poisson = 4};

    APhPerNodeEnum  Mode     = Constant;
    int             Number   = 10;
    int             Min      = 10;
    int             Max      = 12;
    double          Mean     = 100.0;
    double          Sigma    = 10.0;
    double          PoisMean = 10.0;
    QVector<ADPair> CustomDist;

    void writeToJson(QJsonObject & json) const;
    void readFromJson(const QJsonObject & json);
    void clearSettings();
};

// fixed properties of photon
class APhotonSim_FixedPhotSettings
{
public:
    enum AModeEnum {Vector = 0, Cone = 1};

    bool      bFixWave      = false;
    int       FixWaveIndex  = -1;

    bool      bIsotropic    = true;
    AModeEnum DirectionMode = Vector;
    double    FixDX         = 0;
    double    FixDY         = 0;
    double    FixDZ         = 1.0;
    double    FixConeAngle  = 10.0;

    void writeWaveToJson(QJsonObject & json) const;
    void writeDirToJson(QJsonObject & json) const;
    void readWaveFromJson(const QJsonObject & json);
    void readDirFromJson(const QJsonObject & json);
    void clearWaveSettings();
    void clearDirSettings();
};

// single node
class APhotonSim_SingleSettings
{
public:
    double X = 0;
    double Y = 0;
    double Z = 0;

    void writeToJson(QJsonObject & json) const;
    void readFromJson(const QJsonObject & json);
    void clearSettings();
};

// scan
struct APhScanRecord
{
    bool   bEnabled  = false;
    double DX        = 10.0;
    double DY        = 0;
    double DZ        = 0;
    int    Nodes     = 10;
    bool   bBiDirect = false;
};
class APhotonSim_ScanSettings
{
public:
    double X0 = 0;
    double Y0 = 0;
    double Z0 = 0;
    QVector<APhScanRecord> ScanRecords;

    void writeToJson(QJsonObject & json) const;
    void readFromJson(const QJsonObject & json);
    void clearSettings();

    int  countEvents() const;
};

// flood
class APhotonSim_FloodSettings
{
public:
    enum AShapeEnum {Rectangular = 0, Ring = 1};
    enum AZEnum     {Fixed = 0, Range = 1};

    int        Nodes    = 100;
    AShapeEnum Shape     = Rectangular;
    double     Xfrom    = -15.0;
    double     Xto      =  15.0;
    double     Yfrom    = -15.0;
    double     Yto      =  15.0;
    double     X0       = 0;
    double     Y0       = 0;
    double     OuterD   = 300.0;
    double     InnerD   = 0;
    AZEnum     ZMode    = Fixed;
    double     Zfixed   = 0;
    double     Zfrom    = 0;
    double     Zto      = 0;

    void clearSettings();
    void writeToJson(QJsonObject & json) const;
    void readFromJson(const QJsonObject & json);
};

// custom nodes
class APhotonSim_CustomNodeSettings
{
public:
    enum ModeEnum {CustomNodes = 0, PhotonsDirectly = 1};

    QString   FileName;
    ModeEnum  Mode = CustomNodes;

    //runtime properties
    int NumEventsInFile = 0;

    void writeToJson(QJsonObject & json) const;
    void readFromJson(const QJsonObject & json);
    void clearSettings();
};

// spatial distribution in node
#include "a3dposprob.h"
class APhotonSim_SpatDistSettings
{
public:
    enum ModeEnum {DirectMode = 0, FormulaMode = 1, SplineMode = 2};

    bool bEnabled = false;
    ModeEnum Mode = DirectMode;

    QVector<A3DPosProb> LoadedMatrix;
    QString Formula;

    double  RangeX = 100.0;
    double  RangeY = 100.0;
    double  RangeZ = 100.0;

    int     BinsX = 10;
    int     BinsY = 10;
    int     BinsZ = 1;

    void writeToJson(QJsonObject & json) const;
    void readFromJson(const QJsonObject & json);
    void clearSettings();
};

// -------------- main -----------------

class APhotonSimSettings
{
public:
    enum ANodeGenEnum    {Single = 0, Scan = 1, Flood = 2, File = 3, Script = 4};
    enum AScintTypeEnum  {Primary, Secondary};

    // top level settings
    ANodeGenEnum    GenMode      = Single;
    AScintTypeEnum  ScintType    = Primary;
    bool            bMultiRun    = false;
    int             NumRuns      = 1;
    bool            bLimitToVol  = false;
    QString         LimitVolume;

    // sub-settings
    APhotonSim_PerNodeSettings    PerNodeSettings;
    APhotonSim_FixedPhotSettings  FixedPhotSettings;
    APhotonSim_SingleSettings     SingleSettings;
    APhotonSim_ScanSettings       ScanSettings;
    APhotonSim_FloodSettings      FloodSettings;
    APhotonSim_CustomNodeSettings CustomNodeSettings;
    APhotonSim_SpatDistSettings   SpatialDistSettings;

    int getActiveRuns() const;

    void writeToJson(QJsonObject & json) const;
    void readFromJson(const QJsonObject & json);
    void clearSettings();
};

#endif // APHOTONMODESETTINGS_H
