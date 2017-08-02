#ifndef AMONITOR_H
#define AMONITOR_H

#include "amonitorconfig.h"

class TH1D;
class TH2D;
class AGeoObject;

class AMonitor
{
public:
  AMonitor();
  AMonitor(const AGeoObject* MonitorGeoObject);
  ~AMonitor();

//runtime functions
  void fillForParticle(double x, double y, double Time, double Angle, double Energy);
  void fillForPhoton(double x, double y, double Time, double Angle, int waveIndex);

  inline bool isForPhotons() const     {return config.PhotonOrParticle == 0;}
  inline bool isForParticles() const   {return config.PhotonOrParticle != 0;}
  inline bool isUpperSensitive() const {return config.bUpper;}
  inline bool isLowerSensitive() const {return config.bLower;}
  inline bool isStopsTracking() const  {return config.bStopTracking;}
  inline int  getParticleIndex() const {return config.ParticleIndex;}
  inline bool isPrimary() const        {return config.bPrimary;}
  inline bool isSecondary() const      {return config.bSecondary;}

//configuration
  bool readFromGeoObject(const AGeoObject* MonitorRecord);

  void setForPhoton()                  {config.PhotonOrParticle = 0;}
  void setForparticles()               {config.PhotonOrParticle = 1;}
  void setUpperIsSensitive(bool flag)  {config.bUpper = flag;}
  void setLowerIsSensitive(bool flag)  {config.bLower = flag;}
  void setStopsTracking(bool flag)     {config.bStopTracking = flag;}
  void setParticle(int particleIndex)  {config.ParticleIndex = particleIndex;}
  void setPrimaryEnabled(bool flag)    {config.bPrimary = flag;}
  void setSecondaryEnabled(bool flag)  {config.bSecondary = flag;}

  void configureTime(int timeBins, double timeFrom, double timeTo);
  void configureXY(int xBins, int yBins);
  void configureWave(int waveBins, int waveFrom, int waveTo);
  void configureAngle(int angleBins, int angleFrom, int angleTo);
  void configureEnergy(int energyBins, int energyFrom, int energyTo);

// stat data handling
  void clearData();

  TH1D* getTime() const    {return time;}
  TH2D* getXY()   const    {return xy;}
  TH1D* getWave() const    {return wave;}
  TH1D* getAngle() const   {return angle;}
  TH1D* getEnergy() const  {return energy;}

  void appendDataFromAnotherMonitor(AMonitor* from); // used to merge data from several threads

private:
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

};

#endif // AMONITOR_H
