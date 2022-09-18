#ifndef PHYS_ENGINE_H
#define PHYS_ENGINE_H

void phys_engine_poll(float ts, float angle);
float phys_engine_get_angle(void);
float phys_engine_get_w(void);

void phys_engine_reset(void);

float approx_atan2(float y, float x);

#endif // PHYS_ENGINE_H