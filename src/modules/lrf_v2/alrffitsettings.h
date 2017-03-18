#ifndef ALRFFITSETTINGS_H
#define ALRFFITSETTINGS_H

#include <QString>

class QJsonObject;

class ALrfFitSettings
{
public:
 //general
  int dataScanRecon;
  bool fUseGrid;
  bool fForceZeroDeriv;
  bool fFitError;
  bool scale_by_energy;
  bool fFitOnlyLast; //only for composite
 //grouping
  bool fDoGrouping;
  int GroupingIndex;
  bool fAdjustGains;
  QString GrouppingOption;
 //Limit for group
  bool fLimitGroup;
  int igrToMake;
 //LRF type
  bool f3D;
  int LRFtype;
 //nodes
  int nodesx, nodesy;
 //compression
  bool fCompressed;
  double k, r0, lam;
  double comprParams[3];
  double *compr;

  explicit ALrfFitSettings();
  bool readFromJson(QJsonObject &json);
  void dump();
};


#endif // ALRFFITSETTINGS_H
