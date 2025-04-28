#pragma once
#include "defines.hpp"
#include "TCAL9538RSVR.h"
#include "ILI9341.hpp"
#include <stdint.h>
#include <stdbool.h>

// Enum for state of blinkers
typedef enum {
    LIGHTS_NONE = 0,
    LIGHTS_LEFT = 1,
    LIGHTS_RIGHT = 2,
    LIGHTS_HAZARD = 3
} LightState;

struct DashboardState {
    // lights
    LightState lightState = LIGHTS_NONE;
    LightState oldLightStateScreen = LIGHTS_NONE;
    LightState oldLightState = LIGHTS_NONE;

    // outputs
    uint8_t outputPortState = 0;
    uint8_t hornState = 0;
    uint8_t fanState = 0;
    uint8_t headlightState = 0;
    uint8_t pttState = 0;
    uint8_t displayState = 0;

    // blink time
    uint32_t lastBlinkTime = 0; // time of last blink

    // only have these as debugging for the screen, only wanna write to screen when they are updated/changed
    uint8_t oldHornState = 0;
    uint8_t oldFanState = 0;
    uint8_t oldHeadlightState = 0;

    // other system statuses
    uint8_t bmsStatus = 0;
    uint8_t mcStatus = 0;
    uint8_t arrayStatus = 0;

    uint8_t old_bmsStatus = 0;
    uint8_t old_mcStatus = 0;
    uint8_t old_arrayStatus = 0;

    // uart input
    uint8_t uart_rx = 0;
    uint8_t old_uart_rx = 0;
    uint8_t updateRequested = 0; // flag to indicate that the dashboard state needs to be updated
                                 // when uart recieves a new message

    // can stuff that shows up on screen

    uint8_t supp_batt_voltage_lsb = 0; // supplemental battery voltage LSB (mV)
    uint8_t supp_batt_voltage_msb = 0; // supplemental battery voltage MSB (mV)

    uint8_t motor_rpm_lsb = 0; // motor RPM LSB (1rpm)
    uint8_t motor_rpm_msb = 0; // motor RPM MSB (1rpm)

    uint8_t motor_current_lsb = 0; // motor current LSB (1A)
    uint8_t motor_current_msb = 0; // motor current MSB (1A)

    uint8_t motor_voltage_lsb = 0; // motor voltage LSB (0.5V)
    uint8_t motor_voltage_msb = 0; // motor voltage MSB (0.5V)

    uint8_t motor_current_direction = 0; // motor current direction (1 = forward, 0 = reverse)

    /*
    float supp_batt_voltage = 0.0f; // supplemental battery voltage
    float main_batt_power = 0.0f; // main battery power
    float velocity = 0.0f; // velocity of the car
    */

    // functions
    void reset();
    void updateFromUART();  // update dashboard state from UART input
    HAL_StatusTypeDef writeToPort(TCAL9538RSVR &U7); // write current state of dashboard to output port
    void blinkLights(); // toggle state of blinking lights (hazards and turn signals) 
    float getSuppBattVoltage(); // get supplemental battery voltage in volts
    float getMotorPower(); // get motor power in watts
    float getCarVelocity(); // get car velocity in m/s
};
