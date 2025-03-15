//Authors: Jonathon Brown, Justin Lopez

#ifdef __cplusplus
extern "C"{
#endif

#ifndef TCAL9538RSVR_H
#define TCAL9538RSVR_H

#include "stm32l4xx_hal.h"
#include <stdint.h>


typedef struct
{
    I2C_HandleTypeDef *i2cHandle;
    uint8_t deviceAddress;
    uint8_t portValues;
    uint8_t input;

}TCAL9538RSVR;

uint8_t TCAL9538RSVR_INIT(TCAL9538RSVR *dev, I2C_HandleTypeDef *i2cHandle, uint8_t hardwareAddress, uint8_t direction_bitMask, uint8_t interrupt_bitMask);

HAL_StatusTypeDef TCAL9538RSVR_SetDirection(TCAL9538RSVR* dev, uint8_t* bitMask);
HAL_StatusTypeDef TCAL9538RSVR_SetInterrupts(TCAL9538RSVR* dev, uint8_t bitMask);
HAL_StatusTypeDef TCAL9538RSVR_HandleInterrupt(TCAL9538RSVR* dev);



HAL_StatusTypeDef TCAL9538RSVR_ReadInput(TCAL9538RSVR* dev, uint8_t *data);
HAL_StatusTypeDef TCAL9538RSVR_SetOutput(TCAL9538RSVR* dev, uint8_t *data);

//low level functions
HAL_StatusTypeDef TCAL9538RSVR_ReadRegister(TCAL9538RSVR *dev, uint8_t reg, uint8_t *data);
HAL_StatusTypeDef TCAL9538RSVR_WriteRegister(TCAL9538RSVR *dev, uint8_t reg, uint8_t *data);


#endif

#ifdef __cplusplus
}

#endif
