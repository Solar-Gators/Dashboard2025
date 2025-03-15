#include "Drivers/TCAL9538RSVR.h"
#include <string.h>

#define TCAL9538RSVR_ADDR 0b11100000
#define TCAL9538RSVR_GPIO_INPUT 0x00
#define TCAL9538RSVR_GPIO_OUTPUT 0x01
#define TCAL9538RSVR_GPIO_INVERSION 0x02
#define TCAL9538RSVR_DIR_CONFIG 0x03
#define TCAL9538RSVR_INT_CONFIG 0x45
#define TCAL9538RSVR_INT_STATUS 0x46


/**
 * redo in c++
 * make virtual i2c base class -> empty functions
 * inheritor being st func
 * specific drivers owning i2c object*
 */



/*class i2c {
    virtual uint8_t read(...) {};
    virtual uint8_t write(...) {};
;}

class st_i2c : public i2c {
    ...
}

class TCL... {
private:
    i2c* dev;
}
*/



uint8_t TCAL9538RSVR_INIT(TCAL9538RSVR *dev, I2C_HandleTypeDef *i2cHandle, uint8_t hardwareAddress, uint8_t direction_bitMask, uint8_t interrupt_bitMask)
{
	uint8_t errNum = 0;
	HAL_StatusTypeDef status;
	dev->input = direction_bitMask;


    dev->i2cHandle = i2cHandle;
    dev->portValues = 0;
    // hardware address should be from 0-3
    // (A0 = GND, A1 = GND) == 0
    hardwareAddress &= 0b00000011;
    dev->deviceAddress = TCAL9538RSVR_ADDR | (hardwareAddress<<1);
    uint8_t hold_bitMask = direction_bitMask;

    status = TCAL9538RSVR_SetDirection(dev, &hold_bitMask);
    errNum += (status != HAL_OK);

    status = TCAL9538RSVR_SetInterrupts(dev, interrupt_bitMask);
    errNum += (status != HAL_OK);

    if (direction_bitMask == 0xFF) {
        uint8_t full = 0xFF;
        status = TCAL9538RSVR_WriteRegister(dev, TCAL9538RSVR_GPIO_INVERSION, &full);
        errNum += (status != HAL_OK);
    }

    return (errNum);
}


/**
 * @brief Sets GPIO expansion direction
 * @param uint8_t : bitMask -> '1' is input
 * 					default is input
 * @retval HAL_StatusTypeDef : HAL_Status
 */
HAL_StatusTypeDef TCAL9538RSVR_SetDirection(TCAL9538RSVR* dev, uint8_t* bitMask)
{
    return TCAL9538RSVR_WriteRegister(dev, TCAL9538RSVR_DIR_CONFIG, bitMask);
}

/**
 * @brief Sets interrupt register
 * @param uint8_t : bitMask -> '0' will enable interrupts for that pin
 * @retval HAL_StatusTypeDef : HAL_Status
 */
HAL_StatusTypeDef TCAL9538RSVR_SetInterrupts(TCAL9538RSVR* dev, uint8_t bitMask)
{
    return TCAL9538RSVR_WriteRegister(dev, TCAL9538RSVR_INT_CONFIG, &bitMask);
}

/**
 * @brief Handles interrupt and returns pin# that triggered
 * 			stores triggered pin in dev->triggeredInterrupt
 * @retval uint8_t : errNum
 */
HAL_StatusTypeDef TCAL9538RSVR_HandleInterrupt(TCAL9538RSVR* dev)
{
    uint8_t errNum = 0;
	HAL_StatusTypeDef status;
    uint8_t triggeredInterrupts = 0;


    // read interrupt status register, puts a bit mask of the pin that triggered the interrupt in intPinBitMask
    status = TCAL9538RSVR_ReadRegister(dev, TCAL9538RSVR_INT_STATUS, &triggeredInterrupts);
    errNum += (status != HAL_OK);

    // Read updated input values
    status = TCAL9538RSVR_ReadInput(dev, &dev->portValues);


    return (errNum);
}




HAL_StatusTypeDef TCAL9538RSVR_ReadInput(TCAL9538RSVR* dev, uint8_t *data)
{
    return TCAL9538RSVR_ReadRegister(dev, TCAL9538RSVR_GPIO_INPUT, data);
}
HAL_StatusTypeDef TCAL9538RSVR_SetOutput(TCAL9538RSVR* dev, uint8_t *data)
{
    return TCAL9538RSVR_WriteRegister(dev, TCAL9538RSVR_GPIO_OUTPUT, data);
}

//low level functions
HAL_StatusTypeDef TCAL9538RSVR_ReadRegister(TCAL9538RSVR *dev, uint8_t reg, uint8_t *data)
{
    return(HAL_I2C_Mem_Read(dev->i2cHandle, dev->deviceAddress, reg, I2C_MEMADD_SIZE_8BIT, data, 1, HAL_MAX_DELAY));
}
HAL_StatusTypeDef TCAL9538RSVR_WriteRegister(TCAL9538RSVR *dev, uint8_t reg, uint8_t *data)
{
    return(HAL_I2C_Mem_Write(dev->i2cHandle, dev->deviceAddress, reg, I2C_MEMADD_SIZE_8BIT, data, 1, HAL_MAX_DELAY));
}
