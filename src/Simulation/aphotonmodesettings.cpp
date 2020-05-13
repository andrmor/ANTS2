#include "aphotonmodesettings.h"
#include "ajsontools.h"

#include <QDebug>

void APhotonSimSettings::clearSettings()
{
    GenMode      = Single;
    ScintType    = Primary;
    bMultiRun    = false;
    NumRuns      = 1;
    bLimitToVol  = false;
    LimitVolume.clear();

    PerNodeSettings.clearSettings();
    FixedPhotSettings.clearSettings();
    SingleSettings.clearSettings();
    ScanSettings.clearSettings();
    FloodSettings.clearSettings();
    CustomNodeSettings.clearSettings();
}

void APhotonSimSettings::writeToJson(QJsonObject &json) const
{

}

void APhotonSimSettings::readFromJson(const QJsonObject & json)
{
    clearSettings();

    //control
    QJsonObject js;
    bool bOK = parseJson(json, "ControlOptions", js);
    if (!bOK)
    {
        qWarning() << "Bad format of json in read photon mode sim settings";
        return;
    }

    int iGenMode = 0;
    parseJson(js, "Single_Scan_Flood", iGenMode);
    if (iGenMode >= 0 && iGenMode < 5) GenMode = static_cast<ANodeGenEnum>(iGenMode);

    int iScintType = 0;
    parseJson(js, "Primary_Secondary", iScintType);
    ScintType = ( iScintType == 1 ? Secondary : Primary );

    parseJson(js, "MultipleRuns", bMultiRun);
    parseJson(js, "MultipleRunsNumber", NumRuns);

    bLimitToVol = false;
    LimitVolume.clear();
    if (js.contains("GenerateOnlyInPrimary"))                                    //old system
    {
        bLimitToVol = true;
        LimitVolume = "PrScint";
    }
    else if (js.contains("LimitNodesTo"))
    {
        if (js.contains("LimitNodes")) parseJson(js, "LimitNodes", bLimitToVol); //new system
        else  bLimitToVol = true;                                                //semi-old system
        parseJson(js, "LimitNodesTo", LimitVolume);
    }

    bOK = parseJson(json, "PhotPerNodeOptions", js);
    if (bOK) PerNodeSettings.readFromJson(js);

    bOK = parseJson(json, "WaveTimeOptions", js);
    if (bOK) FixedPhotSettings.readFromJson(js);
    bOK = parseJson(json, "PhotonDirectionOptions", js);
    if (bOK) FixedPhotSettings.readFromJson(js); //not a bug, using the same object with fixed settings

    bOK = parseJson(json, "SinglePositionOptions", js);
    if (bOK) SingleSettings.readFromJson(js);

    bOK = parseJson(json, "RegularScanOptions", js);
    if (bOK) ScanSettings.readFromJson(js);

    bOK = parseJson(json, "FloodOptions", js);
    if (bOK) FloodSettings.readFromJson(js);

    bOK = parseJson(json, "CustomNodesOptions", js);
    if (bOK) CustomNodeSettings.readFromJson(js);
}

void APhotonSim_PerNodeSettings::clearSettings()
{
    Mode     = Constant;
    Number   = 10;
    Min      = 10;
    Max      = 12;
    Mean     = 100.0;
    Sigma    = 10.0;
    CustomDist.clear();
}

void APhotonSim_PerNodeSettings::writeToJson(QJsonObject & json) const
{

}

void APhotonSim_PerNodeSettings::readFromJson(const QJsonObject & json)
{
    clearSettings();

    int iMode = 0;
    parseJson(json, "PhotPerNodeMode", iMode);
    switch (iMode)
    {
    default: qWarning() << "Unknown photon per mode mode, using 'Fixed'";
    case 0 : Mode = Constant; break;
    case 1 : Mode = Uniform;  break;
    case 2 : Mode = Gauss;    break;
    case 3 : Mode = Custom;   break;
    }

    parseJson(json, "PhotPerNodeConstant",   Number);
    parseJson(json, "PhotPerNodeUniMin",     Min);
    parseJson(json, "PhotPerNodeUniMax",     Max);
    parseJson(json, "PhotPerNodeGaussMean",  Mean);
    parseJson(json, "PhotPerNodeGaussSigma", Sigma);

    QJsonArray ar;
    parseJson(json, "PhotPerNodeCustom", ar);
    CustomDist.reserve(ar.size());
    for (int i = 0; i < ar.size(); i++)
    {
        QJsonArray el = ar[i].toArray();
        double x = el[0].toDouble();
        double y = el[1].toDouble();
        CustomDist << ADPair(x, y);
    }
}

void APhotonSim_FixedPhotSettings::clearSettings()
{
    bFixWave      = false;
    FixWaveIndex  = -1;
    DirectionMode = Isotropic;
    FixDX         = 0;
    FixDY         = 0;
    FixDZ         = 1.0;
    FixConeAngle  = 10.0;
}

void APhotonSim_FixedPhotSettings::writeToJson(QJsonObject & json) const
{

}

void APhotonSim_FixedPhotSettings::readFromJson(const QJsonObject & json)
{
    clearSettings();

    parseJson(json, "UseFixedWavelength", bFixWave);
    parseJson(json, "WaveIndex", FixWaveIndex);

    bool bRand = true;
    parseJson(json, "Random", bRand);
    int iMode = 0;
    parseJson(json, "Fixed_or_Cone", iMode);
    if (bRand) DirectionMode = Isotropic;
    else       DirectionMode = (iMode == 0 ? Vector : Cone);
    parseJson(json, "FixedX", FixDX);
    parseJson(json, "FixedY", FixDY);
    parseJson(json, "FixedZ", FixDZ);
    parseJson(json, "Cone", FixConeAngle);
}

void APhotonSim_SingleSettings::clearSettings()
{
    X = 0;
    Y = 0;
    Z = 0;
}

void APhotonSim_SingleSettings::writeToJson(QJsonObject & json) const
{

}

void APhotonSim_SingleSettings::readFromJson(const QJsonObject & json)
{
    clearSettings();

    parseJson(json, "SingleX", X);
    parseJson(json, "SingleY", Y);
    parseJson(json, "SingleZ", Z);
}

void APhotonSim_ScanSettings::clearSettings()
{
    X0 = 0;
    Y0 = 0;
    Z0 = 0;
    ScanRecords.clear();
    ScanRecords.resize(3);
    ScanRecords[0].bEnabled = true;
}

void APhotonSim_ScanSettings::writeToJson(QJsonObject & json) const
{

}

void APhotonSim_ScanSettings::readFromJson(const QJsonObject & json)
{
    clearSettings();

    parseJson(json, "ScanX0", X0);
    parseJson(json, "ScanY0", Y0);
    parseJson(json, "ScanZ0", Z0);

    QJsonArray ar;
    bool bOK = parseJson(json, "AxesData", ar);
    if (bOK)
    {
        for (int i = 0; i < 3 && i < ar.size(); i++)
        {
            QJsonObject js = ar[i].toObject();
            APhScanRecord & r = ScanRecords[i];
            r.bEnabled = true;
            parseJson(js, "Enabled", r.bEnabled);
            parseJson(js, "dX", r.DX);
            parseJson(js, "dY", r.DY);
            parseJson(js, "dZ", r.DZ);
            parseJson(js, "Nodes", r.Nodes);
            int iOpt = 0;
            parseJson(js, "Option", iOpt);
            r.bBiDirect = (iOpt == 1);
        }
    }
    ScanRecords[0].bEnabled = true;
}

void APhotonSim_FloodSettings::clearSettings()
{
    Nodes    = 100;
    Shape     = Rectangular;
    Xfrom    = -15.0;
    Xto      =  15.0;
    Yfrom    = -15.0;
    Yto      =  15.0;
    X0       = 0;
    Y0       = 0;
    OuterD   = 300.0;
    InnerD   = 0;
    ZMode    = Fixed;
    Zfixed   = 0;
    Zfrom    = 0;
    Zto      = 0;
}

void APhotonSim_FloodSettings::writeToJson(QJsonObject &json) const
{

}

void APhotonSim_FloodSettings::readFromJson(const QJsonObject &json)
{
    clearSettings();

    parseJson(json, "Nodes", Nodes);

    int iShape = 0;
    parseJson(json, "Shape", iShape);
    Shape = (iShape == 1 ? Ring : Rectangular);

    parseJson(json, "Xfrom", Xfrom);
    parseJson(json, "Xto",   Xto);
    parseJson(json, "Yfrom", Yfrom);
    parseJson(json, "Yto",   Yto);

    parseJson(json, "CenterX", X0);
    parseJson(json, "CenterY", Y0);

    parseJson(json, "DiameterOut", OuterD);
    parseJson(json, "DiameterIn",  InnerD);

    int iZopt = 0;
    parseJson(json, "Zoption", iZopt);
    ZMode = (iZopt == 1 ? Range : Fixed);

    parseJson(json, "Zfixed", Zfixed);
    parseJson(json, "Zfrom", Zfrom);
    parseJson(json, "Zto", Zto);
}

void APhotonSim_CustomNodeSettings::clearSettings()
{
    NodesFileName.clear();
}

void APhotonSim_CustomNodeSettings::writeToJson(QJsonObject &json) const
{

}

void APhotonSim_CustomNodeSettings::readFromJson(const QJsonObject &json)
{
    clearSettings();

    parseJson(json, "FileWithNodes", NodesFileName);
}
