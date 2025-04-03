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
    arrayStatus = 0;

    old_bmsStatus = 0;
    old_mcStatus = 0;
    old_arrayStatus = 0;

    uart_rx = 0;
    old_uart_rx = 0;
    updateRequested = 0;
}

void DashboardState::updateFromUART() {
    // uart data for lights (blinkers)
    if (uart_rx & BUTTON_HAZARD)
        lightState = LIGHTS_HAZARD;
    else if (uart_rx & BUTTON_LEFT_TURN)
        lightState = LIGHTS_LEFT;
    else if (uart_rx & BUTTON_RIGHT_TURN)
        lightState = LIGHTS_RIGHT;
    else {
        lightState = LIGHTS_NONE;
        outputPortState &= ~(OUTPUT_FL_LIGHT_CTRL | OUTPUT_FR_LIGHT_CTRL);
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
