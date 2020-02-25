#ifndef APMANDDUMMY_H
#define APMANDDUMMY_H

struct APMandDummy
{
    double X, Y;
    int UpperLower = -1;

    APMandDummy(double X, double Y, int ul) : X(X), Y(Y), UpperLower(ul) {}
    APMandDummy(){}
};

#endif // APMANDDUMMY_H
