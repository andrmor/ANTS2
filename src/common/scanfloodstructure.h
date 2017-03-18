#ifndef SCANFLOODSTRUCTURE
#define SCANFLOODSTRUCTURE

#include <QString>

struct ScanFloodStructure
{
  QString Description;
  bool Active;
  double Probability;
  double AverageValue;
  double Spread;
};

#endif // SCANFLOODSTRUCTURE

