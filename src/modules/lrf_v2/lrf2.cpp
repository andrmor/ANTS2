#include "lrf2.h"

//#include <math.h>
//#include <string.h>

#if 0
double LRF2::GetRmax()
{
    double corner[4];
    corner[0] = hypot(GetXmin(), GetYmin());
    corner[1] = sqrt((GetXmax()-x0_)*(GetXmax()-x0_) + (GetYmin()-y0_)*(GetYmin()-y0_));
    corner[2] = sqrt((GetXmin()-x0_)*(GetXmin()-x0_) + (GetYmax()-y0_)*(GetYmax()-y0_));
    corner[4] = sqrt((GetXmax()-x0_)*(GetXmax()-x0_) + (GetYmax()-y0_)*(GetYmax()-y0_));
    double rmax = 0.;
    for (int i=0; i<4; i++)
        rmax = corner[i]>rmax ? corner[i] : rmax;
    return rmax;
}
#endif

double LRF2::fitError(int /*npts*/, const double * /*x*/, const double * /*y*/, const double * /*z*/, const double * /*data*/, bool /*grid*/)
{
    return 0;
}
