#ifndef ATRACKRECORDS
#define ATRACKRECORDS

#include <QVector>

#include "TMathBase.h"
#include "TColor.h"

struct TrackNodeStruct
{
  Double_t R[3];
  Double_t Time;

  TrackNodeStruct(const Double_t* r, Double_t time) {R[0]=r[0]; R[1]=r[1]; R[2]=r[2]; Time=time;}
  TrackNodeStruct(Double_t* r, Double_t time) {R[0]=r[0]; R[1]=r[1]; R[2]=r[2]; Time=time;}
  TrackNodeStruct(Double_t x, Double_t y, Double_t z, Double_t time) {R[0]=x; R[1]=y; R[2]=z; Time=time;}
  TrackNodeStruct(){}
};

class TrackHolderClass
{
public:
  QVector<TrackNodeStruct> Nodes;
  Color_t Color;
  Int_t Width;
  Int_t Style = 1;
  Int_t UserIndex;

  TrackHolderClass(Int_t index, Color_t color = kBlack) {UserIndex = index; Color = color;}
  TrackHolderClass() {UserIndex = 0; Color = kBlack;}
};

#endif // ATRACKRECORDS

