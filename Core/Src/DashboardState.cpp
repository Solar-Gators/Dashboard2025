#include "DashboardState.hpp"

void DashboardState::reset() {
    lightState = LIGHTS_NONE;
    oldLightState = LIGHTS_NONE;

    outputPortState = 0;
    hornState = 0;
    fanState = 0;
    headlightState = 0;
    pttState = 0;
    displayState = 0;

    bmsStatus = 0;
    mcStatus = 0;
    arrayContactorsStatus = 0;
    arrayPrechargeStatus = 0;

    old_bmsStatus = 0;
    old_mcStatus = 0;
    old_arrayContactorsStatus = 0;
    old_arrayPrechargeStatus = 0;

    uart_rx = 0;
    old_uart_rx = 0;
    updateRequested = 0;
}

// gets called only when something changes in the dashboard state
void DashboardState::updateFromUART() {

    // uart data for lights (blinkers)
    if (uart_rx & BUTTON_HAZARD) {
        lightState = LIGHTS_HAZARD;
    }
    else if (uart_rx & BUTTON_LEFT_TURN) {
        lightState = LIGHTS_LEFT;
    }
    else if (uart_rx & BUTTON_RIGHT_TURN) {
        lightState = LIGHTS_RIGHT;
    }
    else {
        lightState = LIGHTS_NONE;
    }

    // if the light state has changed, reset the blink time and update the output port state
    if (oldLightState != lightState) {
        lastBlinkTime = HAL_GetTick(); // reset blink time if light state changes
        oldLightState = lightState;
        if (lightState == LIGHTS_HAZARD) {
            outputPortState |= (OUTPUT_FL_LIGHT_CTRL | OUTPUT_FR_LIGHT_CTRL);
        }
        else if (lightState == LIGHTS_LEFT) {
            outputPortState |= OUTPUT_FL_LIGHT_CTRL;
            outputPortState &= ~OUTPUT_FR_LIGHT_CTRL;
        }
        else if (lightState == LIGHTS_RIGHT) {
            outputPortState |= OUTPUT_FR_LIGHT_CTRL;
            outputPortState &= ~OUTPUT_FL_LIGHT_CTRL;
        }
        else {
            outputPortState &= ~(OUTPUT_FL_LIGHT_CTRL | OUTPUT_FR_LIGHT_CTRL);
        }
    }

    // if headlight should be on  
    if (uart_rx & BUTTON_HEADLIGHTS) {
        outputPortState |= (OUTPUT_R_HEAD_CTRL | OUTPUT_L_HEAD_CTRL);
        headlightState = 1;
    }
    else  {
        outputPortState &= ~(OUTPUT_R_HEAD_CTRL | OUTPUT_L_HEAD_CTRL);
        headlightState = 0;
    }

    // if horn should be on
    if (uart_rx & BUTTON_HORN) {
        outputPortState |= OUTPUT_HORN_CTRL;
        hornState = 1;
    }
    else {
        outputPortState &= ~OUTPUT_HORN_CTRL;
        hornState = 0;
    }

    // if fan should be on
    if (uart_rx & BUTTON_FAN) {
        outputPortState |= OUTPUT_FAN_CTRL;
        fanState = 1;
    }
    else {
        outputPortState &= ~OUTPUT_FAN_CTRL;
        fanState = 0;
    }

    // if PTT should be on
    if (uart_rx & BUTTON_PTT) {
        pttState = 1;
    }
    else {
        pttState = 0;
    }

    // TODO: do something with displayState cause this isn't setup to go anywhere
    if (uart_rx & BUTTON_DISPLAY) {
        displayState = 1;
    }
    else {
        displayState = 0;
    }

}

HAL_StatusTypeDef DashboardState::writeToPort(TCAL9538RSVR &U7) {
	uint8_t inverted = ~outputPortState;
    return TCAL9538RSVR_SetOutput(&U7, &inverted); // write to output port
}

void DashboardState::blinkLights() {
    switch (lightState) {
        case LIGHTS_HAZARD:
            outputPortState ^= (OUTPUT_FL_LIGHT_CTRL | OUTPUT_FR_LIGHT_CTRL);
            break;
        case LIGHTS_LEFT:
            outputPortState ^= OUTPUT_FL_LIGHT_CTRL;
            break;
        case LIGHTS_RIGHT:
            outputPortState ^= OUTPUT_FR_LIGHT_CTRL;
            break;
        case LIGHTS_NONE:
            outputPortState &= ~(OUTPUT_FL_LIGHT_CTRL | OUTPUT_FR_LIGHT_CTRL);
            break;
    }
}

float DashboardState::getSuppBattVoltage() {
    // convert to votls from mV
    uint16_t voltage = (supp_batt_voltage_msb << 8) | supp_batt_voltage_lsb;
    return (float)voltage / 1000.0f;
}

float DashboardState::getMotorPower() {
    // P = V * I
    uint16_t temp;

    // voltage (encoded in 0.5V)
    temp = (motor_voltage_msb << 8) | motor_voltage_lsb;
    float voltage = (float)temp / 2.0f;

    // current (encoded in 1A)
    temp = (motor_current_msb << 8) | motor_current_lsb;
    float current = (float)temp;

    if (!motor_current_direction) {
        current *= -1.0f;
    }

    return voltage * current;
}

float DashboardState::getCarSpeed() {
    uint32_t motor_rpm = ((uint32_t)motor_rpm_msb << 8) | motor_rpm_lsb;
    // convert to m/s from rpm
    double inches_per_sec = (motor_rpm * WHEEL_CIRCUMFERENCE_IN) / 60;
    double miles_per_sec = inches_per_sec / 63360; // 1 mile = 63360 inches
    float miles_per_hour = (miles_per_sec * 3600); // 1 hour = 3600 seconds
    return miles_per_hour;
}
