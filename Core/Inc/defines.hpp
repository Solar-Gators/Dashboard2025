#pragma once
#include <stdbool.h>

// blinkling light (hazards and turn signals) blink interval
#define BLINK_INTERVAL_MS 500 // milliseconds

#define WHEEL_CIRCUMFERENCE_IN 69.12 // inches

// screen constants
#define TEXT_SIZE             2
#define INDICATOR_RADIUS      10

#define TITLE_TEXT_X          70
#define TITLE_TEXT_Y          10

#define LABELS_TEXT_X         20 
#define LABELS_TEXT_Y         40

#define BMS_CIRCLE_X          35 
#define MC_CIRCLE_X           75
#define ARRAY_CIRCLE_X       125
#define REGEN_BRAKING_CIRCLE_X 175
#define INDICATOR_CIRCLE_Y    70

#define LEFT_SIGNAL_X         20
#define RIGHT_SIGNAL_X        300
#define SIGNAL_Y              20

#define DIRECTION_TEXT_X      20
#define DIRECTION_TEXT_Y      120

#define STATS_LABELS_X               20  // where the numbers begin
#define STATS_VALUES_X      100 // where the values begin
#define CAR_SPEED_LABEL_Y         160
#define MOTOR_POWER_LABEL_Y         180
#define VOLTAGE_SUPP_BATT_LABEL_Y        200 

#define VALUE_WIDTH           150   // space to clear for redrawing value
#define VALUE_HEIGHT          60   // space to clear for redrawing value

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
#define CAN_ID_VCU_SENSORS            0x007
#define CAN_ID_POWERBOARD             0x003
#define CAN_ID_BMS_FULL_BATT_INFO 0x004
#define CAN_ID_BMS_POWER_CONSUM_INFO 0x006
#define CAN_ID_MITSUBA_MOTOR_FRAME_0          0x08850225
// need mitsuba can message for velocity 

// CAN id for requestion frame0 from mitsuba motor
#define CAN_ID_MITSUBA_MOTOR_REQUEST 0x08F89540

// -------------------------
// VCU SENSORS (CAN ID: CAN_ID_VCU_SENSORS)
// -------------------------
#define VCU_SENSORS_STATUS_BYTE_INDEX 1

enum class VCU_SENSORS_STATUS_BITS {
    VCU_MC_ENABLED_BIT_POS        = 0,
    VCU_ARRAY_ENABLED_BIT_POS      = 2,
    VCU_DIRECTION_BIT_POS     = 3,
};

// -------------------------
// POWER BOARD (CAN ID: CAN_ID_POWERBOARD)
// -------------------------
#define POWERBOARD_SUPPLEMENTAL_BATTERY_VOLTAGE_LSB_INDEX 1
#define POWERBOARD_SUPPLEMENTAL_BATTERY_VOLTAGE_MSB_INDEX 4

// -------------------------
// BMS STATUS FULL BATT INFO (CAN ID: CAN_ID_BMS_FULL_BATT_INFOR)
// -------------------------
#define BMS_MAIN_BATTERY_VOLTAGE_LSB_INDEX 0
#define BMS_MAIN_BATTERY_VOLTAGE_BYTE_1_INDEX 1
#define BMS_MAIN_BATTERY_VOLTAGE_BYTE_2_INDEX 2
#define BMS_MAIN_BATTERY_VOLTAGE_MSB_INDEX 3

#define BMS_MAIN_BATTERY_CURRENT_LSB_INDEX 4
#define BMS_MAIN_BATTERY_CURRENT_BYTE_1_INDEX 5
#define BMS_MAIN_BATTERY_CURRENT_BYTE_2_INDEX 6
#define BMS_MAIN_BATTERY_CURRENT_MSB_INDEX 7

// -------------------------
// BMS STATUS POWER CONSUMPTION INFO (CAN ID: CAN_ID_BMS_POWER_CONSUM_INFO)
// -------------------------
#define BMS_CONTACTORS_CLOSED_INDEX 5 // 1 = closed


// --------------------------
// MITSUBA MOTOR (CAN ID: CAN_ID_MITSUBA_MOTOR)
// --------------------------
// bit 46 downto bit 35 is motor rotating speed
// bit 9 downto 0 is voltage
// bit 18 downto 10 is current
// bit 19 is battery current direction
#define MITSUBA_RPM_VELOCITY_LSB_BIT_INDEX 35 // 1rpm/lsb
#define MITSUBA_RPM_VELOCITY_LEN 12

#define MITSUBA_VOLTAGE_LSB_BIT_INDEX 0 // 0.5V/lsb
#define MITSUBA_VOLTAGE_LEN 10

#define MITSUBA_CURRENT_LSB_BIT_INDEX 10 // 1A/lsb
#define MITSUBA_CURRENT_LEN 9

#define MITSUBA_BATTERY_CURRENT_DIRECTION_BIT_INDEX 19 // 0 = plus current (discharge), 1 = minus current (charge)
