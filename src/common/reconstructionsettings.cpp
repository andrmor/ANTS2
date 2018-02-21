#include "reconstructionsettings.h"
#include "ajsontools.h"

#include <QDebug>

bool ReconstructionSettings::readFromJson(QJsonObject &RecJson)
{
  ErrorString = "";

  //general
  QJsonObject gjson = RecJson["General"].toObject();
  fReconstructEnergy = gjson["ReconstructEnergy"].toBool();
  SuggestedEnergy = gjson["InitialEnergy"].toDouble();
  fReconstructZ = gjson["ReconstructZ"].toBool();
  SuggestedZ = gjson["InitialZ"].toDouble();
  Zstrategy = gjson["Zstrategy"].toInt();
  fIncludePassive = gjson["IncludePassives"].toBool();
  fWeightedChi2calculation = gjson["WeightedChi2"].toBool();
  fLimitSearchIfTrueIsSet = false; //compatibility
  parseJson(gjson, "LimitSearchIfTrueIsSet", fLimitSearchIfTrueIsSet);
  RangeForLimitSearchIfTrueSet = 1.0; //compatibility
  parseJson(gjson, "RangeForLimitSearchIfTrueSet", RangeForLimitSearchIfTrueSet);
  LimitSearchGauss = false;
  parseJson(gjson, "LimitSearchGauss", LimitSearchGauss);

  //Dynamic passives - before algorithms for compatibility: CUDA settings can overrite them if old file is loaded
  if (RecJson.contains("DynamicPassives"))
    {
      //new system
      QJsonObject dynPassJson = RecJson["DynamicPassives"].toObject();
      parseJson(dynPassJson, "IgnoreBySignal", fUseDynamicPassivesSignal);
      parseJson(dynPassJson, "SignalLimitLow", SignalThresholdLow);
      parseJson(dynPassJson, "SignalLimitHigh", SignalThresholdHigh);
      parseJson(dynPassJson, "IgnoreByDistance", fUseDynamicPassivesDistance);
      parseJson(dynPassJson, "DistanceLimit", MaxDistance);
    }
  else
    {
      //compatibility - default settings (off) are in the constructor
      fUseDynamicPassivesDistance = false;
      fUseDynamicPassivesSignal = false;
      /*
      //Extracting old data assuming it is PM group 0
      QJsonObject pmJson = Json["PMrelatedSettings"].toObject();
      if (pmJson.contains("Groups"))
        {
          QJsonObject grJson = pmJson["Groups"].toObject();

          if (grJson.contains("DynamicPassiveThreshold"))
            {
              QJsonArray DynamicPassiveThresholds = grJson["DynamicPassiveThreshold"].toArray();
              if (DynamicPassiveThresholds.size()>0)
                  SignalThresholdLow = DynamicPassiveThresholds[0].toDouble();
            }
          if (grJson.contains("DynamicPassiveRanges"))
            {
              QJsonArray DynamicPassiveRanges = grJson["DynamicPassiveRanges"].toArray();
              if (DynamicPassiveRanges.size()>0)
                  MaxDistance = DynamicPassiveRanges[0].toDouble();
            }
        }
      */
    }

  //Algotithm
  QJsonObject ajson = RecJson["AlgorithmOptions"].toObject();
  ReconstructionAlgorithm = ajson["Algorithm"].toInt();
  //cog
  QJsonObject cogjson = ajson["CoGoptions"].toObject();
  fCogForceFixedZ = cogjson["ForceFixedZ"].toBool();
  fCoGStretch = cogjson["DoStretch"].toBool();
  CoGStretchX = cogjson["StretchX"].toDouble();
  CoGStretchY = cogjson["StretchY"].toDouble();
  CoGStretchZ = cogjson["StretchZ"].toDouble();
  if (cogjson.contains("IgnoreLow")) fCoGIgnoreBySignal = cogjson["IgnoreLow"].toBool(); //compatibility
  else fCoGIgnoreBySignal = cogjson["IgnoreBySignal"].toBool();
  if (cogjson.contains("IgnoreThreshold")) CoGIgnoreThresholdLow = cogjson["IgnoreThreshold"].toDouble(); //compatibility
  else CoGIgnoreThresholdLow = cogjson["IgnoreThresholdLow"].toDouble();
  CoGIgnoreThresholdHigh = 1.0e10; //compatibility
  if (cogjson.contains("IgnoreThresholdHigh")) CoGIgnoreThresholdHigh = cogjson["IgnoreThresholdHigh"].toDouble();
  fCoGIgnoreFar = false; //compatibility
  fCoGIgnoreFar = cogjson["IgnoreFar"].toBool();
  if (fCoGIgnoreFar)
    {
      CoGIgnoreDistance = cogjson["IgnoreDistance"].toDouble();
      CoGIgnoreDistance2 = CoGIgnoreDistance*CoGIgnoreDistance;
    }
  if (ReconstructionAlgorithm == 0)
    {
      fUseDynamicPassivesDistance = fCoGIgnoreFar;
      fUseDynamicPassivesSignal = fCoGIgnoreBySignal;
    }

  //CGonCPU
  QJsonObject gcpuJson = ajson["CPUgridsOptions"].toObject();
  CGstartOption = gcpuJson["StartOption"].toInt();
  CGstartX = CGstartY = 0;
  parseJson(gcpuJson, "StartX", CGstartX);
  parseJson(gcpuJson, "StartY", CGstartY);
  CGoptimizeWhat = gcpuJson["OptimizeWhat"].toInt();
  CGnodesXY = gcpuJson["NodesXY"].toInt();
  CGiterations = gcpuJson["Iterations"].toInt();
  CGinitialStep = gcpuJson["InitialStep"].toDouble();
  CGreduction = gcpuJson["Reduction"].toDouble();
  if (ReconstructionAlgorithm == 1)
    {      
      //compatibility
      if (gcpuJson.contains("DynamicPassives"))
        {
          bool fOn = gcpuJson["DynamicPassives"].toBool();
          if (fOn)
          {
              int SignalDist = 0;
              parseJson(gcpuJson, "PassiveType", SignalDist);
              if (SignalDist==0) fUseDynamicPassivesSignal = true;
              else fUseDynamicPassivesDistance = true;
          }
        }
    }

  //RootMini
  QJsonObject rootJson = ajson["RootMinimizerOptions"].toObject();
  RMstartOption = rootJson["StartOption"].toInt();
  RMminuitOption = rootJson["Minuit2Option"].toInt();
  fRMuseML = rootJson["LSorLikelihood"].toInt();
  RMstepX = rootJson["StartStepX"].toDouble();
  RMstepY = rootJson["StartStepY"].toDouble();
  RMstepZ = rootJson["StartStepZ"].toDouble();
  RMstepEnergy = rootJson["StartStepEnergy"].toDouble();
  RMmaxCalls = rootJson["MaxCalls"].toInt();
  fRMsuppressConsole = rootJson["LSsuppressConsole"].toBool();
  if (ReconstructionAlgorithm == 2)
    {      
      //compatibility
      if (rootJson.contains("DynamicPassives"))
        {
          bool fOn = rootJson["DynamicPassives"].toBool();
          if (fOn)
          {
              int SignalDist = 0;
              parseJson(rootJson, "PassiveType", SignalDist);
              if (SignalDist==0) fUseDynamicPassivesSignal = true;
              else fUseDynamicPassivesDistance = true;
          }
        }
    }
  //ANN
  ANNsettings = ajson["FANNsettings"].toObject();
  //CUDA
  CGonCUDAsettings = ajson["CGonCUDAsettings"].toObject();
  //kNNreconstruct
  kNNrecSet = ajson["kNNrecSet"].toObject();

  if (ReconstructionAlgorithm == 4)
    {     
      //compatibility
      if (CGonCUDAsettings.contains("Passives"))
        {
          int SignalDist = 0;
          parseJson(CGonCUDAsettings, "PassiveOption", SignalDist);
          if (SignalDist==0) fUseDynamicPassivesSignal = true;
          else fUseDynamicPassivesDistance = true;
          parseJson(CGonCUDAsettings, "Threshold", SignalThresholdLow);
          SignalThresholdLow = 1.0e10;
          parseJson(CGonCUDAsettings, "MaxDistance", MaxDistance);
        }
    }

  //Multiples
  QJsonObject multJson = RecJson["MultipleEvents"].toObject();
  MultipleEventOption = multJson["Option"].toInt();

  //Limit nodes during reconstruction
  fLimitNodes = false;
  QJsonObject lnJson = RecJson["LimitNodes"].toObject();
  if (!lnJson.isEmpty())
    {
      fLimitNodes = lnJson["Active"].toBool();
      LimitNodesShape = lnJson["Shape"].toInt();
      LimitNodesSize1 = lnJson["Size1"].toDouble();
      LimitNodesSize2 = lnJson["Size2"].toDouble();
    }

  fUseDynamicPassives = fUseDynamicPassivesDistance || fUseDynamicPassivesSignal;
  MaxDistanceSquare = MaxDistance * MaxDistance;

  return true;
}
