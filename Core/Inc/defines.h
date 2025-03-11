#ifndef DASHBOARD_DEFINES_H
#define DASHBOARD_DEFINES_H

#include <stdbool.h>

// Button bitmask definitions (UART input from steering wheel)
#define BUTTON_LEFT_TURN    (1 << 0)  // Bit 0
#define BUTTON_RIGHT_TURN   (1 << 1)  // Bit 1
#define BUTTON_HAZARD       (1 << 2)  // Bit 2
#define BUTTON_HEADLIGHTS   (1 << 3)  // Bit 3
#define BUTTON_DISPLAY      (1 << 4)  // Bit 4
#define BUTTON_HORN         (1 << 5)  // Bit 5
#define BUTTON_PTT          (1 << 6)  // Bit 6

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

/* UNUSED IDEA, only really useful for readability i think
// Struct to store the dashboard state
typedef struct {
    bool horn;
    bool ptt;
    bool headlight;
    bool display;
    LightState blinker_state;
} DashboardState;
*/

#endif // DASHBOARD_DEFINES_H
