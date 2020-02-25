#ifndef APMDUMMYSTRUCTURE_H
#define APMDUMMYSTRUCTURE_H

class QJsonObject;

struct APMdummyStructure
{
  int PMtype;      //index of the PM type
  int UpperLower;  //Belongs to a PM plane: 0- upper, 1 - lower
  double r[3];     //PMs center coordinates
  double Angle[3]; //Phi, Theta, Psi;

  void writeToJson(QJsonObject & json) const;
  bool readFromJson(const QJsonObject & json);
};

#endif // APMDUMMYSTRUCTURE_H
