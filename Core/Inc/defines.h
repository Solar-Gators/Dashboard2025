#ifndef DASHBOARD_DEFINES_H
#define DASHBOARD_DEFINES_H

// Button bitmask definitions
// Bit masks of what each bit in the uart message from the steering wheel does
#define BUTTON_LEFT_TURN    (1 << 0)  // Bit 0
#define BUTTON_RIGHT_TURN   (1 << 1)  // Bit 1
#define BUTTON_HAZARD       (1 << 2)  // Bit 2
#define BUTTON_HEADLIGHTS   (1 << 3)  // Bit 3
#define BUTTON_DISPLAY      (1 << 4)  // Bit 4
#define BUTTON_HORN         (1 << 5)  // Bit 5
#define BUTTON_PTT          (1 << 6)  // Bit 6

// GPIO expander output mappings
// Bit masks of bits in the output ports from the expander
#define OUTPUT_HORN_CTRL        (1 << 0) // P0
#define OUTPUT_FAN_CTRL         (1 << 1) // P1
#define OUTPUT_R_HEAD_CTRL      (1 << 2) // P2
#define OUTPUT_L_HEAD_CTRL      (1 << 3) // P3
#define OUTPUT_FL_LIGHT_CTRL    (1 << 4) // P4
#define OUTPUT_FR_LIGHT_CTRL    (1 << 5) // P5

#endif // DASHBOARD_DEFINES_H
