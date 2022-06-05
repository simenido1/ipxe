//from libavutil/libm.h
#include "fdlibm.h"
double round(double x)
{
    return (x > 0) ? floor(x + 0.5) : ceil(x - 0.5);
}

float roundf(float x)
{
    return (x > 0) ? floor(x + 0.5) : ceil(x - 0.5);
}

double trunc(double x)
{
    return (x > 0) ? floor(x) : ceil(x);
}

float truncf(float x)
{
    return (x > 0) ? floor(x) : ceil(x);
}