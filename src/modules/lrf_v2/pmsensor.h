#ifndef PMSENSOR_H
#define PMSENSOR_H

#include "lrf2.h"
#include <vector>

class QJsonObject;

class PMsensor
{
public:
  PMsensor();
  PMsensor(int index);
  //PMsensor(QJsonObject &json); ///*** bad control, better not to use?
  ~PMsensor() {;}

  void writeJSON(QJsonObject &json) const;
  void readOldJSON(QJsonObject &json, int *group, const double *lrfx, const double *lrfy);
  void readJSON(QJsonObject &json);

  void SetShift(double dx_, double dy_) { dx = dx_; dy = dy_; }
  //void SetCenter(double x0, double y0) { dx = -x0; dy = -y0; }
  void SetPhi(double phi);
  void SetFlip(bool flip_) { flip = flip_; }
  void NullTransform();
  void SetTransform(double dx_, double dy_, double phi_, bool flip_);
  void GetTransform(double *dx_, double *dy_, double *phi_, bool *flip_) const;
  void SetGain(double gain) { this->gain = gain; }
  void SetLRF(LRF2* lrf) { this->lrf = lrf; }
  LRF2* GetLRF() {return lrf;}
  double GetGain() const {return gain;}
  int GetIndex() const { return index; }
  //int GetGroup() const {return group;}
  bool GetFlip() const {return flip;}

  void transform(const double *pos_world, double *pos_local) const;
  void transformBack(const double *pos_local, double *pos_world) const;
  ///double *transform(const double *pos_world) const; // returns pointer to local position

  void getGlobalMinMax(double *minmax) const; //minmax[0] = xmin; [1] = xmax; [2] = ymin; [3] = ymax;

  bool inDomain(double *pos_world) const;
  double eval(const double *pos_world) const;
  double evalLocal(double *pos_local) const { return lrf->eval(pos_local)*gain; }
  double evalErr(const double *pos_world) const;
  double evalErrLocal( double *pos_local) const { return lrf->evalErr(pos_local)*gain; }
  double evalDrvX( double *pos_world) const;
  double evalDrvY( double *pos_world) const;

private:
  int index;  //index in PM module
  //int group;  // group id
  double gain;// relative gain (1 for root PMT)

  // transform
  double dx;  // x shift
  double dy;  // y shift
  double phi; // rotation
  double sinphi; // sin(phi);
  double cosphi; // cos(phi);
  bool flip;  // mirror

  //LRF - SensorGroup is the owner
  LRF2* lrf;   // pointer to LRF
};

#endif // PMSENSOR_H
