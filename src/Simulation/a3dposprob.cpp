#include "a3dposprob.h"

A3DPosProb::A3DPosProb(double x, double y, double z, double prob)
{
    R[0] = x;
    R[1] = y;
    R[2] = z;

    Probability = prob;
}
