#ifndef A3DPOSPROB_H
#define A3DPOSPROB_H

class A3DPosProb
{
public:
    A3DPosProb(double x, double y, double z, double prob);
    A3DPosProb() {}

    double R[3];
    double Probability;
};

#endif // A3DPOSPROB_H
