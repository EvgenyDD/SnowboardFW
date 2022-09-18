#ifndef DM_H
#define DM_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
	DM_BAT, // reserved mode

	DM_SPARKLE,

	DM_RIDE0,
	DM_RIDE_SPARKLE,

	DM_IDLE,
	// modes

	DM_RANDOM_COLOR,

	DM_RAINBOW_ROTATE,

	DM_ROTATING_STRIPES,

	DM_PENDULUM,
	DM_STRIPPER_FIXED,
	DM_POLICE,

	DM_COUNT
} DM_SHOW_T;

void dm_init(void);

DM_SHOW_T dm_get_mode(void);
void dm_switch(DM_SHOW_T mode);

// special modes
void dm_switch_mode_vbat(void);
void dm_switch_mode_sparkle(void);

void dm_switch_parameter(void);
void dm_switch_mode_next(void);

void dm_poll(uint32_t diff_ms);

#endif // DM_H