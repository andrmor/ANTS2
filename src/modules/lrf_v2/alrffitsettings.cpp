#include "alrffitsettings.h"
#include "ajsontools.h"

#include <QDebug>
#include <QJsonObject>

ALrfFitSettings::ALrfFitSettings()
{
  dataScanRecon = 0; //Scan
  fUseGrid = true;
  fForceZeroDeriv = true;
  fFitError = false;
  scale_by_energy = true;

  fDoGrouping = false;
  GroupingIndex = 0;
  GrouppingOption = "Undefined";

  fLimitGroup = false;
  igrToMake = 0;

  f3D = false;
  LRFtype = 0;

  nodesx = 3;
  nodesy = 3;
  k = 123;
  r0 = 123;
  lam = 123;
  comprParams[0] = k;
  comprParams[1] = r0;
  comprParams[2] = lam;
  fCompressed = false;
  compr = 0;
}

bool ALrfFitSettings::readFromJson(QJsonObject &json)
{
  if (!json.contains("DataSelector")) return false;

  //general
  dataScanRecon = 0; //0 - Scan, 1 - Reconstr data
  parseJson(json, "DataSelector", dataScanRecon);
  fUseGrid = true;
  parseJson(json, "UseGrid", fUseGrid);
  fForceZeroDeriv = true;
  parseJson(json, "ForceZeroDeriv", fForceZeroDeriv);

  fForceNonNegative = false;
  parseJson(json, "ForceNonNegative", fForceNonNegative);
  fForceNonIncreasingInR = false;
  parseJson(json, "ForceNonIncreasingInR", fForceNonIncreasingInR);
  fForceInZ = 0;
  parseJson(json, "ForceInZ", fForceInZ);

  fFitError = false;
  parseJson(json, "StoreError", fFitError);
  scale_by_energy = true;
  parseJson(json, "UseEnergy", scale_by_energy);

  //grouping
  fDoGrouping = false;
  fAdjustGains = true;
  parseJson(json, "UseGroupping", fDoGrouping);
  if (fDoGrouping)
    {
      parseJson(json, "GrouppingType", GroupingIndex);
      GroupingIndex++; //historic, before "no groupping" option was cob index 0
      parseJson(json, "AdjustGains", fAdjustGains); ///*** !!! adjust gains should be group dependent
    }
  else GroupingIndex = 0;
  GrouppingOption = "Undefined";
  parseJson(json, "GroupingOption", GrouppingOption);

  //Limit for group
  fLimitGroup = false;
  parseJson(json, "LimitGroup", fLimitGroup);
  igrToMake = 0;
  parseJson(json, "GroupToMake", igrToMake);

  //LRF type
  f3D = false;
  parseJson(json, "LRF_3D", f3D);
  LRFtype = 0;
  parseJson(json, "LRF_type", LRFtype);
  if (f3D) LRFtype += 4; /// *** !!! absolute number

  //nodes
  nodesx = 3;
  parseJson(json, "Nodes_x", nodesx);
  nodesy = 3;
  parseJson(json, "Nodes_y", nodesy);
  //compression
  k = 123;
  parseJson(json, "Compression_k", k);
  r0 = 123;
  parseJson(json, "Compression_r0", r0);
  lam = 123;
  parseJson(json, "Compression_lam", lam);
  comprParams[0] = k;
  comprParams[1] = r0;
  comprParams[2] = lam;
  fCompressed = false;
  parseJson(json, "LRF_compress", fCompressed);
  compr = (fCompressed) ? comprParams : 0;

  return true;
}

void ALrfFitSettings::dump()
{
  qDebug() << "=== LRF settings dump ===";
  qDebug() << ">> Scan/recon:"<< dataScanRecon << " use grid:"<<fUseGrid << " force zero deriv:"<< fForceZeroDeriv << " store error:"<< fFitError<< "use E:"<< scale_by_energy;
  qDebug() << ">> Do group?"<< fDoGrouping << " GroupingOption:" << GrouppingOption;
  if (fDoGrouping) qDebug() << ">> GrouppingType" << GroupingIndex <<" adjust gains?"<< fAdjustGains;
  qDebug() << ">> Do specific group?" << fLimitGroup;
  if (fLimitGroup) qDebug() << ">> GroupToMake:" << igrToMake;
  qDebug() << ">> LRF is 3D?" << f3D << "LRF type index:"<< LRFtype;
  qDebug() << ">> Nodes in x and y:"<< nodesx << nodesy;
  qDebug() << ">> Use compression?" << fCompressed  << " compr pointer:"<< compr;
  if (fCompressed)
    {
      qDebug() << ">>  k, r0, lam"<< k<< r0 << lam;
      qDebug() << ">>  at compr:" << compr[0] << compr[1]<< compr[2];
    }
  qDebug() << "=========================";
}

