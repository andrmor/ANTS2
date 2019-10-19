#ifndef AMONITOR_H
#define AMONITOR_H

#include "amonitorconfig.h"

#include <QString>
#include <QVector>
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

  /*
  void setForPhoton()                  {config.PhotonOrParticle = 0;}
  void setForparticles()               {config.PhotonOrParticle = 1;}
  void setUpperIsSensitive(bool flag)  {config.bUpper = flag;}
  void setLowerIsSensitive(bool flag)  {config.bLower = flag;}
  void setStopsTracking(bool flag)     {config.bStopTracking = flag;}
  void setParticle(int particleIndex)  {config.ParticleIndex = particleIndex;}
  void setPrimaryEnabled(bool flag)    {config.bPrimary = flag;}
  void setSecondaryEnabled(bool flag)  {config.bSecondary = flag;}
  void setNoPrior(bool flag)           {config.bNoPriorInteractions = flag;}
  */

  void configureXY(int xBins, int yBins);
  void configureTime  (int timeBins,   double timeFrom,   double timeTo);
  void configureWave  (int waveBins,   double waveFrom,   double waveTo);
  void configureAngle (int angleBins,  double angleFrom,  double angleTo);
  void configureEnergy(int energyBins, double energyFrom, double energyTo);

  void overrideDataFromJson(const QJsonObject & json);
  void overrideEnergyData(double from, double to, const std::vector<double> &binContent, const std::vector<double> &stats); // vec contain underflow and overflow bins!
  void overrideAngleData (const QVector<double> & vec);  // vec contain underflow and overflow bins!
  void overrideTimeData  (const QVector<double> & vec);   // vec contain underflow and overflow bins!
  void overrideXYData    (const QVector<QVector<double>> & vec); // vec contain underflow and overflow bins! [y][x] in increasing order

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
  TH1D *create1D(const QJsonObject &json, bool bEnergy = false);
};

#endif // AMONITOR_H
