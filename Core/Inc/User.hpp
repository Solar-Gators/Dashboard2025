#ifndef USER_HPP_
#define USER_HPP_

#include <stddef.h>

#include "main.h"

#include "cmsis_os.h"

#include "TCAL9538RSVR.h"
#include "defines.h"
#include "ILI9341.hpp"

#define ADC_BUF_LEN 10 // ADC DMA Buffer size

#define CHECK_BIT(var,pos) !!((var) & (1<<(pos)))


extern "C" void CPP_UserSetup(void);

extern "C" I2C_HandleTypeDef hi2c4;
extern "C" ADC_HandleTypeDef hadc1;
extern "C" DMA_HandleTypeDef hdma_adc1;
extern "C" CAN_HandleTypeDef hcan1;
extern "C" CAN_HandleTypeDef hcan2;
extern "C" UART_HandleTypeDef huart4;


void Update_CAN_Message1(uint8_t flags[8], uint8_t* Input1, uint8_t* Input2);
void CruiseControlManagement();








#endif /* USER_HPP_ */
