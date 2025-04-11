#pragma once
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

// Macro around code that needs to be in critical region, i.e. no context switching between thread or interrupts
#define DASHBOARD_CRITICAL(code_block) \
    do {                               \
        taskENTER_CRITICAL();          \
        code_block;                    \
        taskEXIT_CRITICAL();           \
    } while (0)

// CAN Message IDs
#define CAN_ID_ANALOG_INPUTS          0x000 // throttle and cc
#define CAN_ID_CRITICAL_SWITCHES      0x001  
#define CAN_ID_VCU_SENSORS            0x002
#define CAN_ID_POWERBOARD             0x003
#define CAN_ID_BMS                    0x004
#define CAN_ID_MITSUBA_MOTOR 0 
// need mitsuba can message for velocity 

// -------------------------
// CRITICAL SWITCHES (CAN ID: CAN_ID_CRITICAL_SWITCHES)
// -------------------------
#define CRITICAL_SWITCHES_BYTE_1_INDEX 1
#define CRITICAL_SWITCHES_BYTE_2_INDEX 2

enum class CRITICAL_SWITCHES_1_BITS {
    SWITCH_IGNITION_BIT        = 0,
    SWITCH_BRAKE_BIT           = 1,
    SWITCH_DIRECTION_BIT       = 2,
    SWITCH_MC_ENABLE_BIT       = 3,
    SWITCH_ARRAY_BIT           = 4,
    SWITCH_ARRAY_PRECHARGE_BIT = 5,
    SWITCH_HORN_BIT            = 6,
    SWITCH_PTT_BIT             = 7,
};

enum class CRITICAL_SWITCHES_2_BITS {
    SWITCH_BLINKERS_BIT        = 0,
    SWITCH_LEFT_TURN_BIT       = 1,
    SWITCH_RIGHT_TURN_BIT      = 2,
    SWITCH_HEADLIGHTS_BIT      = 3,
};

// -------------------------
// VCU SENSORS (CAN ID: CAN_ID_VCU_SENSORS)
// -------------------------
#define VCU_SENSORS_STATUS_BYTE_INDEX 7

enum class VCU_SENSORS_STATUS_BITS {
    VCU_MC_ENABLED_BIT        = 0,
    VCU_ARRAY_ENABLED_BIT     = 1,
};

// -------------------------
// POWER BOARD (CAN ID: CAN_ID_POWERBOARD)
// -------------------------
#define POWERBOARD_SUPPLEMENTAL_BATTERY_VOLTAGE_LSB_INDEX 2
#define POWERBOARD_SUPPLEMENTAL_BATTERY_VOLTAGE_MSB_INDEX 3

// -------------------------
// BMS STATUS (CAN ID: CAN_ID_BMS)
// -------------------------
#define BMS_MAIN_BATTERY_VOLTAGE_LSB_INDEX 1
#define BMS_MAIN_BATTERY_VOLTAGE_BYTE_1_INDEX 2
#define BMS_MAIN_BATTERY_VOLTAGE_BYTE_2_INDEX 3
#define BMS_MAIN_BATTERY_VOLTAGE_MSB_INDEX 4
#define BMS_MAIN_BATTERY_CURRENT_LSB_INDEX 5
#define BMS_MAIN_BATTERY_CURRENT_MSB_INDEX 6
#define BMS_STATUS_BYTE_INDEX 7

enum class BMS_STATUS_BITS {
    BMS_CONTACTORS_CLOSED_BIT   = 0,
};


