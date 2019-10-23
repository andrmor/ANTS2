#ifndef APMS_SI_H
#define APMS_SI_H

#include "ascriptinterface.h"

#include <QVariant>

class AConfiguration;
class APmHub;

class APms_SI : public AScriptInterface
{
  Q_OBJECT

public:
  APms_SI(AConfiguration* Config);
  ~APms_SI() {}

  bool IsMultithreadCapable() const override {return true;}

public slots:
  int      CountPM() const;

  double   GetPMx(int ipm);
  double   GetPMy(int ipm);
  double   GetPMz(int ipm);

  bool     IsPmCenterWithin(int ipm, double x, double y, double distance_in_square);
  bool     IsPmCenterWithinFast(int ipm, double x, double y, double distance_in_square) const;

  QVariant GetPMtypes();
  QVariant GetPMpositions() const;

  void     RemoveAllPMs();
  bool     AddPMToPlane(int UpperLower, int type, double X, double Y, double angle = 0);
  bool     AddPM(int UpperLower, int type, double X, double Y, double Z, double phi, double theta, double psi);
  void     SetAllArraysFullyCustom();

  void     SetPDE(int ipm, double PDE);
  void     SetPDEvsWave(int ipm, QVariantList ArrayOf_ArrayWavePDE);

  void     SetPDE_factor(int ipm, double value);
  void     SetPDE_factors(QVariant CommonValue_or_Array);

  void     SetAngularResponse(int ipm, QVariantList ArrayOf_ArrayAngleResponse, double refIndex = 1.0);
  void     SetAreaResponse(int ipm, QVariantList MatrixOfResponses, double StepX, double StepY);

  void     SetSPE_factor(int ipm, double value);
  void     SetSPE_factors(QVariant CommonValue_or_Array);

  double   GetPDE(int ipm, int WaveIndex = -1, double Angle = 0, double Xlocal = 0, double Ylocal = 0);

private:
  AConfiguration* Config;
  APmHub* PMs;

  bool checkValidPM(int ipm);
  bool checkAddPmCommon(int UpperLower, int type);
  void setFactors(QVariant CommonValue_or_Array, bool bDoPDE);
};

#endif // APMS_SI_H
