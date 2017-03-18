#ifndef APMPOSANGTYPERECORD_H
#define APMPOSANGTYPERECORD_H

#include <QJsonArray>

class APmPosAngTypeRecord
{
public:  
    double x,y,z,phi,theta,psi;
    int type;

    APmPosAngTypeRecord(){}
    APmPosAngTypeRecord(double x, double y, double z, double phi, double theta, double psi, int type)
      : x(x), y(y), z(z), phi(phi), theta(theta), psi(psi), type(type){}

    QJsonArray writeToJArrayAutoZ();
    void readFromJArrayAutoZ(QJsonArray &arr);
    QJsonArray writeToJArrayFull();
    void readFromJArrayFull(QJsonArray &arr);
};

#endif // APMPOSANGTYPERECORD_H
