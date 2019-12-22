#include "phys_engine.h"
#include "math_const.h"
#include <math.h>
#include <stdint.h>

float c_damp = 0.8;
float c_scale = 7;

static float w = 0; // velocity
static float a = -0.9; // angle

void phys_engine_reset(void)
{
    w = 0;
    a = 0;
}

float approx_atan2(float y, float x)
{
    const float n1 = 0.97239411f;
    const float n2 = -0.19194795f;
    float result = 0.0f;
    if(x != 0.0f)
    {
        const union {
            float flVal;
            uint32_t nVal;
        } tYSign = {y};
        const union {
            float flVal;
            uint32_t nVal;
        } tXSign = {x};
        if(fabsf(x) >= fabsf(y))
        {
            union {
                float flVal;
                uint32_t nVal;
            } tOffset = {PI};
            // Add or subtract PI based on y's sign.
            tOffset.nVal |= tYSign.nVal & 0x80000000u;
            // No offset if x is positive, so multiply by 0 or based on x's sign.
            tOffset.nVal *= tXSign.nVal >> 31;
            result = tOffset.flVal;
            const float z = y / x;
            result += (n1 + n2 * z * z) * z;
        }
        else // Use atan(y/x) = pi/2 - atan(x/y) if |y/x| > 1.
        {
            union {
                float flVal;
                uint32_t nVal;
            } tOffset = {PI_2};

            // Add or subtract PI/2 based on y's sign.
            tOffset.nVal |= tYSign.nVal & 0x80000000u;
            result = tOffset.flVal;
            const float z = x / y;
            result -= (n1 + n2 * z * z) * z;
        }
    }
    else if(y > 0.0f)
    {
        result = PI_2;
    }
    else if(y < 0.0f)
    {
        result = -PI_2;
    }
    return result;
}

static float square_root(float x)
{
    unsigned int i = *(unsigned int *)&x;

    // adjust bias
    i += 127 << 23;
    // approximation of square root
    i >>= 1;

    return *(float *)&i;
}

static float cosine(float x)
{
    x *= ONE_DIV_2PI;
    x -= .25f + floor(x + 0.25f);
    x *= 16.f * (fabsf(x) - 0.5f);
#if EXTRA_PRECISION
    x += 0.225f * x * (fabsf(x) - 1.0f);
#endif
    return x;
}

void phys_engine_poll(float ts, float angle)
{
    float normed = angle_norm(angle - a);
    w += (c_scale * /*square_root*/ (cosine(normed)) - SIGNF(w) * c_damp) * ts;
    // w = 1;
    a += w * ts;
}

float phys_engine_get_angle(void) { return a; }
float phys_engine_get_w(void) { return w; }
