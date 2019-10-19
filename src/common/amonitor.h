#ifndef AMONITOR_H
#define AMONITOR_H

#include "amonitorconfig.h"

#include <QString>
#include <vector>

class TH1D;
class TH2D;
class AGeoObject;
class QJsonObject;

class AMonitor
{
public:
  AMonitor();
  AMonitor(const AGeoObject* MonitorGeoObject);
  ~AMonitor();

//runtime functions
  void fillForParticle(double x, double y, double Time, double Angle, double Energy);
  void fillForPhoton(double x, double y, double Time, double Angle, int waveIndex);

  inline bool isForPhotons() const         {return config.PhotonOrParticle == 0;}
  inline bool isForParticles() const       {return config.PhotonOrParticle != 0;}
  inline bool isUpperSensitive() const     {return config.bUpper;}
  inline bool isLowerSensitive() const     {return config.bLower;}
  inline bool isStopsTracking() const      {return config.bStopTracking;}
  inline int  getParticleIndex() const     {return config.ParticleIndex;}
  inline bool isAcceptingPrimary() const   {return config.bPrimary;}
  inline bool isAcceptingSecondary() const {return config.bSecondary;}
  inline bool isAcceptingDirect() const    {return config.bDirect;}
  inline bool isAcceptingIndirect() const  {return config.bIndirect;}

//configuration
  bool readFromGeoObject(const AGeoObject* MonitorRecord);

  void overrideDataFromJson(const QJsonObject & json);

// stat data handling
  void clearData();

  TH1D* getTime() const    {return time;}
  TH2D* getXY()   const    {return xy;}
  TH1D* getWave() const    {return wave;}
  TH1D* getAngle() const   {return angle;}
  TH1D* getEnergy() const  {return energy;}

  int getHits() const;
  const QString getName() const {return name;}

  void appendDataFromAnotherMonitor(AMonitor* from); // used to merge data from several threads

  const AMonitorConfig & getConfig() const {return config;}

private:
  QString name;

  TH1D* time;
  TH2D* xy;
  TH1D* angle;
  TH1D* wave;
  TH1D* energy;

  AMonitorConfig config;

  void initXYHist();
  void initTimeHist();
  void initWaveHist();
  void initAngleHist();
  void initEnergyHist();

  void update1D(const QJsonObject &json, TH1D *& old);
};

#endif // AMONITOR_H
