#ifndef DASHBOARD_DEFINES_H
#define DASHBOARD_DEFINES_H

#include <stdbool.h>

// Button bitmask definitions (UART input from steering wheel)

#define BUTTON_LEFT_TURN     (1 << 3)
#define BUTTON_HAZARD        (1 << 2)
#define BUTTON_DISPLAY       (1 << 1)
#define BUTTON_FAN           (1 << 0)

#define BUTTON_RIGHT_TURN    (1 << 7)
#define BUTTON_HEADLIGHTS    (1 << 6)
#define BUTTON_HORN          (1 << 5)
#define BUTTON_DISPLAY       (1 << 4)

// GPIO expander output mappings (TCAL9538)
#define OUTPUT_HORN_CTRL        (1 << 0)
#define OUTPUT_FAN_CTRL         (1 << 1)
#define OUTPUT_L_HEAD_CTRL      (1 << 2)
#define OUTPUT_R_HEAD_CTRL      (1 << 3)
#define OUTPUT_FR_LIGHT_CTRL    (1 << 4)
#define OUTPUT_FL_LIGHT_CTRL    (1 << 5)

// Enum for state of blinkers
typedef enum {
    LIGHTS_NONE = 0,
    LIGHTS_LEFT = 1,
    LIGHTS_RIGHT = 2,
    LIGHTS_HAZARD = 3
} LightState;

#endif // DASHBOARD_DEFINES_H
