#ifndef MATH_CONST_H
#define MATH_CONST_H

#include <math.h>

#define PI_2 1.5707963267948966192313216916398f
#define PI 3.1415926535897932384626433832795f
#define ONE_DIV_2PI 0.15915494309189533576888376337251f

#define SIGNF(x) ((x >= 0) ? 1 : -1)

inline float angle_norm(float angle)
{
    angle = fmodf(angle, 2 * PI);
    if(angle < 0) angle += 2 * PI;
    return angle;
}

#endif // MATH_CONST_H