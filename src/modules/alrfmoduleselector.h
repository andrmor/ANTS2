#ifndef ALRFMODULESELECTOR_H
#define ALRFMODULESELECTOR_H

#include "sensorlrfs.h" //cannot make forward: class SensorLRFs - connect does not work!

struct APoint;
class TF1;
class TF2;
class APmHub;
namespace LRF {
  class ARepository;
  class ASensorGroup;
}

#include <QJsonObject>
#include <memory>

//WARN: copied instances are only valid until clear() is called.
//      If copy is used after that, crash!!!
class ALrfModuleSelector
{
  APmHub *PMs;
public:
  ALrfModuleSelector(APmHub *PMs);

  bool isAllLRFsDefined() const;
  bool isAllLRFsDefined(bool fUseOldModule) const;

  double getLRF(int pmt, const double *r);
  double getLRF(bool fUseOldModule, int pmt, const double *r);
  double getLRF(int pmt, const APoint &pos);
  double getLRF(int pmt, double x, double y, double z);
  double getLRF(bool fUseOldModule, int pmt, double x, double y, double z);
  double getLRFErr(int pmt, const double *r);
  double getLRFErr(int pmt, const APoint &pos);
  double getLRFErr(int pmt, double x, double y, double z);

  void clear(int numPMs); //Warning: don't call unless there are no copies!

  void saveActiveLRFs_v2(QJsonObject &LRFjson);
  void saveActiveLRFs_v3(QJsonObject &LRFjson);
  void loadAll_v2(QJsonObject &json);
  void loadAll_v3(QJsonObject &json);
  QJsonObject getLRFmakeJson() const;
  QJsonObject getLRFv3makeJson() const;

  void selectOld() {fOldSelected = true;}
  void selectNew() {fOldSelected = false;}
  bool isOldSelected() const;
  bool isAllSlice3Dold() const;
  SensorLRFs* getOldModule();
  LRF::ARepository* getNewModule();

  TF1* getRootFunctionMainRadial(bool fUseOldModule, int ipm, double z, QJsonObject &json);
  TF1* getRootFunctionSecondaryRadial(bool fUseOldModule, int ipm, double z, QJsonObject &json);
  bool getNodes(bool fUseOldModule, int ipm, QVector<double> &GrX);

  TF2 *getRootFunctionMainXY(bool fUseOldModule, int ipm, double z, QJsonObject &json);
  TF2 *getRootFunctionSecondaryXY(bool fUseOldModule, int ipm, double z, QJsonObject &json);

  ALrfModuleSelector copyToCurrentThread();

private:
  bool fOldSelected;

  //Used for multi-threading getLRF(Err)
  bool fUseNewModCopy;
  std::shared_ptr<const LRF::ASensorGroup> NewModuleCopy;

  std::shared_ptr<SensorLRFs> OldModule;
  std::shared_ptr<LRF::ARepository> NewModule;

  TF1* getRootFunctionRadialNewModule(const LRF::ASensorGroup &lrfs, int ipm, double z, QJsonObject &json);
  TF2 *getRootFunctionXYNewModule(const LRF::ASensorGroup &lrfs, int ipm, double z, QJsonObject &json);
};

#endif // ALRFMODULESELECTOR_H
