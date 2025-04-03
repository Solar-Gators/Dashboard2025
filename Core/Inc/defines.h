#ifndef DASHBOARD_DEFINES_H
#define DASHBOARD_DEFINES_H

#include <stdbool.h>

// Button bitmask definitions (UART input from steering wheel)
#define BUTTON_LEFT_TURN     (1 << 3) // left side going down
#define BUTTON_HAZARD        (1 << 2)
#define BUTTON_DISPLAY       (1 << 1)
#define BUTTON_FAN           (1 << 0)
#define BUTTON_RIGHT_TURN    (1 << 7) // right side going down
#define BUTTON_HEADLIGHTS    (1 << 6)
#define BUTTON_HORN          (1 << 5)
#define BUTTON_PTT           (1 << 4)

// Button positions (UART input from steering wheel)
#define BUTTON_LEFT_TURN_POS     3
#define BUTTON_HAZARD_POS        2
#define BUTTON_DISPLAY_POS       1
#define BUTTON_FAN_POS           0
#define BUTTON_RIGHT_TURN_POS    7
#define BUTTON_HEADLIGHTS_POS    6
#define BUTTON_HORN_POS          5
#define BUTTON_PTT_POS           4

// GPIO expander output mappings (TCAL9538)
#define OUTPUT_HORN_CTRL        (1 << 1)
#define OUTPUT_FAN_CTRL         (1 << 0)
#define OUTPUT_L_HEAD_CTRL      (1 << 2)
#define OUTPUT_R_HEAD_CTRL      (1 << 3)
#define OUTPUT_FR_LIGHT_CTRL    (1 << 4)
#define OUTPUT_FL_LIGHT_CTRL    (1 << 5)

// GPIO expander output positions (TCAL9538)
#define OUTPUT_HORN_CTRL_POS        0
#define OUTPUT_FAN_CTRL_POS         1
#define OUTPUT_L_HEAD_CTRL_POS      2
#define OUTPUT_R_HEAD_CTRL_POS      3
#define OUTPUT_FR_LIGHT_CTRL_POS    4
#define OUTPUT_FL_LIGHT_CTRL_POS    5


#define DASHBOARD_CRITICAL(code_block) \
    do {                               \
        taskENTER_CRITICAL();          \
        code_block;                    \
        taskEXIT_CRITICAL();           \
    } while (0)

#endif // DASHBOARD_DEFINES_H
