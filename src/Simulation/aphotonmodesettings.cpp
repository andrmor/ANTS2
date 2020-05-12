#include "aphotonmodesettings.h"
#include "ajsontools.h"

void APhotonSimSettings::writeToJson(QJsonObject &json) const
{

}

void APhotonSimSettings::readFromJson(const QJsonObject & json)
{
    //control
    QJsonObject js;
    bool bOK = parseJson(json, "ControlOptions", js);
    if (bOK)
    {
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

void APhotonSim_PerNodeSettings::writeToJson(QJsonObject & json) const
{

}

void APhotonSim_PerNodeSettings::readFromJson(const QJsonObject & json)
{
    /*
    JsonToComboBox(ppj, "PhotPerNodeMode", ui->cobScanNumPhotonsMode);
    JsonToSpinBox(ppj, "PhotPerNodeConstant", ui->sbScanNumPhotons);
    JsonToSpinBox(ppj, "PhotPerNodeUniMin", ui->sbScanNumMin);
    JsonToSpinBox(ppj, "PhotPerNodeUniMax", ui->sbScanNumMax);
    JsonToLineEditDouble(ppj, "PhotPerNodeGaussMean", ui->ledScanGaussMean);
    JsonToLineEditDouble(ppj, "PhotPerNodeGaussSigma", ui->ledScanGaussSigma);
    if (ppj.contains("PhotPerNodeCustom"))
      {
        QJsonArray ja = ppj["PhotPerNodeCustom"].toArray();
        int size = ja.size();
        if (size > 0)
          {
            double* xx = new double[size];
            int* yy    = new int[size];
            for (int i=0; i<size; i++)
              {
                xx[i] = ja[i].toArray()[0].toDouble();
                yy[i] = ja[i].toArray()[1].toInt();
              }
            histScan = new TH1I("histPhotDistr","Photon distribution", size-1, xx);
            histScan->SetXTitle("Number of generated photons");
            histScan->SetYTitle("Relative probability");
            for (int i = 1; i<size+1; i++) histScan->SetBinContent(i, yy[i-1]);
            histScan->GetIntegral();
            delete[] xx;
            delete[] yy;
            ui->pbScanDistrShow->setEnabled(true);
            ui->pbScanDistrDelete->setEnabled(true);
          }
      }
    */
}

void APhotonSim_FixedPhotSettings::writeToJson(QJsonObject & json) const
{

}

void APhotonSim_FixedPhotSettings::readFromJson(const QJsonObject & json)
{
    /*
    ui->cbFixWavelengthPointSource->setChecked(false);  //compatibility
    JsonToCheckbox(wdj, "UseFixedWavelength", ui->cbFixWavelengthPointSource);
    JsonToSpinBox(wdj, "WaveIndex", ui->sbFixedWaveIndexPointSource);

    JsonToLineEditDouble(pdj, "FixedX", ui->ledSingleDX);
    JsonToLineEditDouble(pdj, "FixedY", ui->ledSingleDY);
    JsonToLineEditDouble(pdj, "FixedZ", ui->ledSingleDZ);
    JsonToLineEditDouble(pdj, "Cone", ui->ledConeAngle);
    ui->cobFixedDirOrCone->setCurrentIndex(0); //compatibility
    JsonToComboBox(pdj, "Fixed_or_Cone", ui->cobFixedDirOrCone);
    JsonToCheckbox(pdj, "Random", ui->cbRandomDir);
    */
}

void APhotonSim_SingleSettings::writeToJson(QJsonObject & json) const
{

}

void APhotonSim_SingleSettings::readFromJson(const QJsonObject & json)
{
    parseJson(json, "SingleX", X);
    parseJson(json, "SingleY", Y);
    parseJson(json, "SingleZ", Z);
}

void APhotonSim_ScanSettings::writeToJson(QJsonObject & json) const
{

}

void APhotonSim_ScanSettings::readFromJson(const QJsonObject & json)
{
    /*
    JsonToLineEditDouble(rsj, "ScanX0", ui->ledOriginX);
    JsonToLineEditDouble(rsj, "ScanY0", ui->ledOriginY);
    JsonToLineEditDouble(rsj, "ScanZ0", ui->ledOriginZ);
    ui->cbSecondAxis->setChecked(false);
    ui->cbThirdAxis->setChecked(false);
    if (rsj.contains("AxesData"))
      {
        QJsonArray ar = rsj["AxesData"].toArray();
        if (ar.size()>0)
          {
            QJsonObject js = ar[0].toObject();
            JsonToLineEditDouble(js, "dX", ui->led0X);
            JsonToLineEditDouble(js, "dY", ui->led0Y);
            JsonToLineEditDouble(js, "dZ", ui->led0Z);
            JsonToSpinBox (js, "Nodes", ui->sb0nodes);
            JsonToComboBox(js, "Option", ui->cob0dir);
          }
        if (ar.size()>1)
          {
            QJsonObject js = ar[1].toObject();
            JsonToLineEditDouble(js, "dX", ui->led1X);
            JsonToLineEditDouble(js, "dY", ui->led1Y);
            JsonToLineEditDouble(js, "dZ", ui->led1Z);
            JsonToSpinBox (js, "Nodes", ui->sb1nodes);
            JsonToComboBox(js, "Option", ui->cob1dir);
            ui->cbSecondAxis->setChecked(true);
          }
        if (ar.size()>2)
          {
            QJsonObject js = ar[2].toObject();
            JsonToLineEditDouble(js, "dX", ui->led2X);
            JsonToLineEditDouble(js, "dY", ui->led2Y);
            JsonToLineEditDouble(js, "dZ", ui->led2Z);
            JsonToSpinBox (js, "Nodes", ui->sb2nodes);
            JsonToComboBox(js, "Option", ui->cob2dir);
            ui->cbThirdAxis->setChecked(true);
          }
      }
      */
}

void APhotonSim_FloodSettings::writeToJson(QJsonObject &json) const
{

}

void APhotonSim_FloodSettings::readFromJson(const QJsonObject &json)
{
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

void APhotonSim_CustomNodeSettings::writeToJson(QJsonObject &json) const
{
    json["FileWithNodes"] = NodesFileName;
}

void APhotonSim_CustomNodeSettings::readFromJson(const QJsonObject &json)
{
    parseJson(json, "FileWithNodes", NodesFileName);
}
