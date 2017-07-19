#ifndef AMONITOR_H
#define AMONITOR_H

class TH1D;
class TH2D;
class AGeoObject;

class AMonitorBaseStat
{
public:
  AMonitorBaseStat() : time(0), xy(0) {}

  void clear();
  void configureTime(int timeBins, double timeFrom, double timeTo);
  void configureXY(int xBins, double xFrom, double xTo,
                   int yBins, double yFrom, double yTo);
  void setActive(bool flag) {bActive = flag;}
  void setUpperIsSensitive(bool flag) {bUpper = flag;}
  void setLowerIsSensitive(bool flag) {bLower = flag;}

  inline bool isActive() const {return bActive;}
  inline bool isUpperSensitive() const {return bUpper;}
  inline bool isLowerSensitive() const {return bLower;}

  void fill(double x, double y, double Time);

  TH1D* getTime() const {return time;}
  TH2D* getXY()   const {return xy;}

  void appendFrom(AMonitorBaseStat* from);

protected:
  TH1D* time;
  TH2D* xy;

  //runtime selectors
  bool bActive;
  bool bUpper; // direction: z>0 is upper, z<0 is lower
  bool bLower;
};

class AMonitorPhotonStat : public AMonitorBaseStat
{
public:
  AMonitorPhotonStat() : wave(0) {}
  ~AMonitorPhotonStat();

  void clear();
  void configureWave(int waveBins, int waveFrom, int waveTo);

  void fill(double x, double y, double Time, int waveIndex);

  TH1D* getWave() const {return wave;}

  void appendFrom(AMonitorPhotonStat* from);

protected:
  TH1D* wave;

  //runtime selectors
};

class AMonitorParticleStat : public AMonitorBaseStat
{
public:

  void setParticle(int particleIndex) {ParticleIndex = particleIndex;}
  void setPrimaryEnabled(bool flag) {bPrimary = flag;}
  void setSecondaryEnabled(bool flag) {bSecondary = flag;}

protected:
  //runtime selectors
  int ParticleIndex;
  bool bPrimary;
  bool bSecondary;
};

class AMonitor
{
public:
  AMonitor() {}
  AMonitor(const AGeoObject* MonitorRecord);

  void clearData();

  bool readFrom(const AGeoObject* MonitorRecord);

  void appendFrom(AMonitor* from);

  AMonitorPhotonStat   PhotonStat;
  AMonitorParticleStat ParticleStat;
};

#endif // AMONITOR_H
