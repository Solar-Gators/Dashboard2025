/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "Drivers/TCAL9538RSVR.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define Alert1_Pin GPIO_PIN_2
#define Alert1_GPIO_Port GPIOC
#define Alert1_EXTI_IRQn EXTI2_IRQn
#define D0_Pin GPIO_PIN_0
#define D0_GPIO_Port GPIOA
#define D1_Pin GPIO_PIN_1
#define D1_GPIO_Port GPIOA
#define D2_Pin GPIO_PIN_2
#define D2_GPIO_Port GPIOA
#define D3_Pin GPIO_PIN_3
#define D3_GPIO_Port GPIOA
#define D4_Pin GPIO_PIN_4
#define D4_GPIO_Port GPIOA
#define D5_Pin GPIO_PIN_5
#define D5_GPIO_Port GPIOA
#define D6_Pin GPIO_PIN_6
#define D6_GPIO_Port GPIOA
#define D7_Pin GPIO_PIN_7
#define D7_GPIO_Port GPIOA
#define Backlight_PWM_Pin GPIO_PIN_4
#define Backlight_PWM_GPIO_Port GPIOC
#define C_D_Pin GPIO_PIN_0
#define C_D_GPIO_Port GPIOB
#define WR_Pin GPIO_PIN_1
#define WR_GPIO_Port GPIOB
#define RD_Pin GPIO_PIN_2
#define RD_GPIO_Port GPIOB
#define Parallel_CS_Pin GPIO_PIN_12
#define Parallel_CS_GPIO_Port GPIOB
#define OK_LED_Pin GPIO_PIN_8
#define OK_LED_GPIO_Port GPIOA
#define ERROR_LED_Pin GPIO_PIN_9
#define ERROR_LED_GPIO_Port GPIOA
#define STROBE_LED_Pin GPIO_PIN_10
#define STROBE_LED_GPIO_Port GPIOA
#define D_C_Pin GPIO_PIN_4
#define D_C_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
