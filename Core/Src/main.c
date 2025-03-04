/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "TCAL9538RSVR.h"
#include "defines.h"
//#include "sg_can.hpp"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

// Returns 0 or 1 if bit at pos in var is set
#define CHECK_BIT(var,pos) !!((var) & (1<<(pos)))

#define ADC_BUF_LEN 10 // ADC DMA Buffer size

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

CAN_HandleTypeDef hcan1;
CAN_HandleTypeDef hcan2;

I2C_HandleTypeDef hi2c4;

UART_HandleTypeDef huart4;

/* Definitions for HeartBeat */
osThreadId_t HeartBeatHandle;
const osThreadAttr_t HeartBeat_attributes = {
  .name = "HeartBeat",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Critical_Inputs */
osThreadId_t Critical_InputsHandle;
const osThreadAttr_t Critical_Inputs_attributes = {
  .name = "Critical_Inputs",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for ReadIOExpander */
osThreadId_t ReadIOExpanderHandle;
const osThreadAttr_t ReadIOExpander_attributes = {
  .name = "ReadIOExpander",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Outputs_Control */
osThreadId_t Outputs_ControlHandle;
const osThreadAttr_t Outputs_Control_attributes = {
  .name = "Outputs_Control",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for CAN_Mutex */
osSemaphoreId_t CAN_MutexHandle;
const osSemaphoreAttr_t CAN_Mutex_attributes = {
  .name = "CAN_Mutex"
};
/* USER CODE BEGIN PV */

TCAL9538RSVR U5; // input 1
TCAL9538RSVR U16; // input 2
TCAL9538RSVR U7; // output

static uint8_t GPIO_Interrupt_Triggered;


static uint8_t cc_enable = 0;


uint8_t outputPortState = 0; // variable with state of output port
uint8_t uart_rx = 0; // variable for holding the recieved data over uart from steering wheel, its only sending one byte
uint8_t prev_uart_rx = 0; // variable to help with toggle logic
LightState lightState = LIGHTS_NONE;

uint16_t adc_buf[ADC_BUF_LEN]; // variable to store ADC DMA Buffers
volatile uint8_t dma_flag = 0; // flag for DMA start and stop

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_I2C4_Init(void);
static void MX_CAN1_Init(void);
static void MX_CAN2_Init(void);
static void MX_UART4_Init(void);
void StartTask01(void *argument);
void StartTask02(void *argument);
void StartTask03(void *argument);
void StartTask04(void *argument);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void Update_CAN_Message1(uint8_t flags[8], uint8_t* Input1, uint8_t* Input2)
{
	/*
	 * Byte 0:
	 * 0 - Ignition Switch
	 * 1 - Breaks
	 * 2 - Direction
	 * 3 - MC Enable
	 * 4 - Array
	 * 5 - Array Pre-Charge
	 * 6 - Horn (global boolean)
	 * 7 - PTT (global boolean)
	 *
	 * Byte 1:
	 * 0 - Blinkers
	 * 1 - Left Turn
	 * 2 - Right Turn
	 * 3 - BMS ?
	 * 4 -
	 *
	 * Debounce buttons
	 *
	 *
	 */

	static uint8_t prev_input1 = 0;
	static uint8_t prev_input2 = 0;


	flags[3] = 0;
	flags[4] = 0;


	// Detect rising edges for each flag
	uint8_t risingEdges_flag1 = (~prev_input1) & *Input1;
	uint8_t risingEdges_flag2 = (~prev_input2) & *Input2;


	flags[1] ^= CHECK_BIT(risingEdges_flag2, 4) << 0; // Main
	flags[1] ^= CHECK_BIT(risingEdges_flag2, 5) << 1; // Break
	flags[1] ^= CHECK_BIT(risingEdges_flag2, 0) << 2; // Mode
	flags[1] ^= CHECK_BIT(risingEdges_flag1, 5) << 3; // MC
	flags[1] ^= CHECK_BIT(risingEdges_flag1, 6) << 4; // Array
	flags[1] ^= CHECK_BIT(risingEdges_flag1, 4) << 5; // Extra 1
	//flags[1] |= CHECK_BIT(outputPortState, 5) << 6; // Horn
	//flags[1] |= CHECK_BIT(outputPortState, 6) << 7; // PTT (push to talk)


	//flags[2] |= CHECK_BIT(outputPortState, 2) << 0; // Blinkers
	//flags[2] |= CHECK_BIT(outputPortState, 0) << 1; // Left Turn Signal
	//flags[2] |= CHECK_BIT(outputPortState, 1) << 2; // Right Turn Signal
	flags[2] ^= CHECK_BIT(risingEdges_flag1, 7) << 3; //?

	cc_enable ^= CHECK_BIT(risingEdges_flag2, 1);

	prev_input1 = *Input1;
	prev_input2 = *Input2;

}

void CruiseControlManagement()
{

	/**
	 * store boolean for cc on or off
	 * s
	 * cc- (cc set)
	 * cc+ (cc reset)
	 *
	 * lock in val (turn on off)
	 *
	 * */
}

// GPIO Expander Interrupt Handler
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	GPIO_Interrupt_Triggered = 1;
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_I2C4_Init();
  MX_CAN1_Init();
  MX_CAN2_Init();
  MX_UART4_Init();
  /* USER CODE BEGIN 2 */
  if (TCAL9538RSVR_INIT(&U5, &hi2c4, 0b10, 0xFF, 0x00) != HAL_OK) { Error_Handler(); } // inputs
  //if (TCAL9538RSVR_INIT(&U16, &hi2c4, 0b01, 0b00111111, 0b11000000) != HAL_OK) { Error_Handler(); }
  if (TCAL9538RSVR_INIT(&U7, &hi2c4, 0x00, 0b00000000, 0b00000000) != HAL_OK) { Error_Handler(); } // output

  // set outputs to low to start
  //TCAL9538RSVR_SetOutput(&U7, &outputPortState);

  HAL_CAN_Start(&hcan1);

  //HAL_UART_Receive_IT(&huart4, &uart_rx, 1); // enables uart interrupt, it will call the interrupt when one byte is recieved
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of CAN_Mutex */
  CAN_MutexHandle = osSemaphoreNew(1, 1, &CAN_Mutex_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of HeartBeat */
  HeartBeatHandle = osThreadNew(StartTask01, NULL, &HeartBeat_attributes);

  /* creation of Critical_Inputs */
  Critical_InputsHandle = osThreadNew(StartTask02, NULL, &Critical_Inputs_attributes);

  /* creation of ReadIOExpander */
  ReadIOExpanderHandle = osThreadNew(StartTask03, NULL, &ReadIOExpander_attributes);

  /* creation of Outputs_Control */
  Outputs_ControlHandle = osThreadNew(StartTask04, NULL, &Outputs_Control_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_MultiModeTypeDef multimode = {0};
  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SEQ_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.NbrOfConversion = 10;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc1.Init.OversamplingMode = DISABLE;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the ADC multi-mode
  */
  multimode.Mode = ADC_MODE_INDEPENDENT;
  if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_2CYCLES_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Rank = ADC_REGULAR_RANK_2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Rank = ADC_REGULAR_RANK_3;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Rank = ADC_REGULAR_RANK_4;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Rank = ADC_REGULAR_RANK_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Rank = ADC_REGULAR_RANK_6;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Rank = ADC_REGULAR_RANK_7;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Rank = ADC_REGULAR_RANK_8;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Rank = ADC_REGULAR_RANK_9;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Rank = ADC_REGULAR_RANK_10;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief CAN1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_CAN1_Init(void)
{

  /* USER CODE BEGIN CAN1_Init 0 */

  /* USER CODE END CAN1_Init 0 */

  /* USER CODE BEGIN CAN1_Init 1 */

  /* USER CODE END CAN1_Init 1 */
  hcan1.Instance = CAN1;
  hcan1.Init.Prescaler = 2;
  hcan1.Init.Mode = CAN_MODE_NORMAL;
  hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan1.Init.TimeSeg1 = CAN_BS1_2TQ;
  hcan1.Init.TimeSeg2 = CAN_BS2_1TQ;
  hcan1.Init.TimeTriggeredMode = DISABLE;
  hcan1.Init.AutoBusOff = DISABLE;
  hcan1.Init.AutoWakeUp = DISABLE;
  hcan1.Init.AutoRetransmission = DISABLE;
  hcan1.Init.ReceiveFifoLocked = DISABLE;
  hcan1.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN1_Init 2 */

  /* USER CODE END CAN1_Init 2 */

}

/**
  * @brief CAN2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_CAN2_Init(void)
{

  /* USER CODE BEGIN CAN2_Init 0 */

  /* USER CODE END CAN2_Init 0 */

  /* USER CODE BEGIN CAN2_Init 1 */

  /* USER CODE END CAN2_Init 1 */
  hcan2.Instance = CAN2;
  hcan2.Init.Prescaler = 2;
  hcan2.Init.Mode = CAN_MODE_NORMAL;
  hcan2.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan2.Init.TimeSeg1 = CAN_BS1_2TQ;
  hcan2.Init.TimeSeg2 = CAN_BS2_1TQ;
  hcan2.Init.TimeTriggeredMode = DISABLE;
  hcan2.Init.AutoBusOff = DISABLE;
  hcan2.Init.AutoWakeUp = DISABLE;
  hcan2.Init.AutoRetransmission = DISABLE;
  hcan2.Init.ReceiveFifoLocked = DISABLE;
  hcan2.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN2_Init 2 */

  /* USER CODE END CAN2_Init 2 */

}

/**
  * @brief I2C4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C4_Init(void)
{

  /* USER CODE BEGIN I2C4_Init 0 */

  /* USER CODE END I2C4_Init 0 */

  /* USER CODE BEGIN I2C4_Init 1 */

  /* USER CODE END I2C4_Init 1 */
  hi2c4.Instance = I2C4;
  hi2c4.Init.Timing = 0x00100D14;
  hi2c4.Init.OwnAddress1 = 0;
  hi2c4.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c4.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c4.Init.OwnAddress2 = 0;
  hi2c4.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c4.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c4.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c4) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c4, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c4, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C4_Init 2 */

  /* USER CODE END I2C4_Init 2 */

}

/**
  * @brief UART4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_UART4_Init(void)
{

  /* USER CODE BEGIN UART4_Init 0 */

  /* USER CODE END UART4_Init 0 */

  /* USER CODE BEGIN UART4_Init 1 */

  /* USER CODE END UART4_Init 1 */
  huart4.Instance = UART4;
  huart4.Init.BaudRate = 9600;
  huart4.Init.WordLength = UART_WORDLENGTH_8B;
  huart4.Init.StopBits = UART_STOPBITS_1;
  huart4.Init.Parity = UART_PARITY_NONE;
  huart4.Init.Mode = UART_MODE_TX_RX;
  huart4.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart4.Init.OverSampling = UART_OVERSAMPLING_16;
  huart4.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart4.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN UART4_Init 2 */

  /* USER CODE END UART4_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(OK_LED_GPIO_Port, OK_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : Alert1_Pin */
  GPIO_InitStruct.Pin = Alert1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(Alert1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : OK_LED_Pin */
  GPIO_InitStruct.Pin = OK_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(OK_LED_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI2_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{

  if (huart->Instance == UART4)
  {
    uint8_t new_presses = uart_rx & ~prev_uart_rx;

    // if left turn button was pressed
    if (new_presses & BUTTON_LEFT_TURN)
    {
      if (lightState == LIGHTS_LEFT)
        lightState = LIGHTS_NONE;
      else
        lightState = LIGHTS_LEFT;
    }

    // if right turn button was pressed
    if (new_presses & BUTTON_RIGHT_TURN)
    {
      if (lightState == LIGHTS_RIGHT)
        lightState = LIGHTS_NONE;
      else
        lightState = LIGHTS_RIGHT;
    }

    // if hazard button was pressed
    if (new_presses & BUTTON_HAZARD)
    {
      if (lightState == LIGHTS_HAZARD)
        lightState = LIGHTS_NONE;
      else
        lightState = LIGHTS_HAZARD;
    }

    // if headlight button was pressed
    if (new_presses & BUTTON_HEADLIGHTS)
    {
      // toggle headlight state
      outputPortState ^= OUTPUT_R_HEAD_CTRL;
      outputPortState ^= OUTPUT_L_HEAD_CTRL;
    }

    // if display button was pressed, (I think this is toggle)
    if (new_presses & BUTTON_DISPLAY)
    {
      // toggle display state
      outputPortState ^= OUTPUT_FL_LIGHT_CTRL;
      outputPortState ^= OUTPUT_FR_LIGHT_CTRL;
    }

    // if horn button is being pressed currently
    if (uart_rx & BUTTON_HORN)
      outputPortState |= OUTPUT_HORN_CTRL;
    else
      outputPortState &= ~OUTPUT_HORN_CTRL;

    // if PTT button is being pressed currently
    /*
    TODO: Some code with PTT button (does this just go over can what even is PTT)
    TODO: Some code with Fan (where does fan come from) 
    */


    prev_uart_rx = uart_rx;
  }

  HAL_UART_Receive_IT(&huart4, &uart_rx, 1); // reenables uart interrupt
}
/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartTask01 */
/**
  * @brief  Function implementing the HeartBeat thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartTask01 */
void StartTask01(void *argument)
{
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for(;;)
  {
	  HAL_GPIO_TogglePin(GPIOA, OK_LED_Pin);
    osDelay(500);
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartTask02 */

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc){
	dma_flag = 1;
}


/**
* @brief Function implementing the Critical_Inputs thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask02 */
void StartTask02(void *argument)
{
  /* USER CODE BEGIN StartTask02 */

	uint16_t adc_var_avg = 0;

	int HAL_CAN_BUSY = 0;
	uint64_t messages_sent = 0;
	static uint8_t update_cc = 0;

	CAN_TxHeaderTypeDef TxHeader;
	uint8_t TxData[8] = { 0 };
	uint32_t TxMailbox = { 0 };

	TxHeader.IDE = CAN_ID_STD; // Standard ID (not extended)
	TxHeader.StdId = 0x0; // 11 bit Identifier
	TxHeader.RTR = CAN_RTR_DATA; // Std RTR Data frame
	TxHeader.DLC = 8; // 8 bytes being transmitted
	TxData[0] = 1;


	// Start ADC with DMA
	uint8_t adc_data[2];



  	  // Transmit over CAN
  	  HAL_CAN_AddTxMessage(&hcan1, &TxHeader, TxData, &TxMailbox);


  for (;;)
  {

	HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buf, ADC_BUF_LEN);
	// Start ADC with DMA
	while (!(dma_flag));

	// Stop ADC with DMA
	HAL_ADC_Stop_DMA(&hadc1);
	dma_flag = 0;
	// Copy ADC buffer and compute average
	for (int i = 0; i < ADC_BUF_LEN; i++)
	{
		adc_var_avg += adc_buf[i];
	}
	adc_var_avg /= ADC_BUF_LEN;

	adc_data[0] = adc_var_avg & 0xFF;
	adc_data[1] = (adc_var_avg >> 8) & 0x0F;

	if (cc_enable)
	{
		if (update_cc)
		{
			TxData[5] = adc_data[0];
			TxData[6] = adc_data[1];
			update_cc = 0;
		}
	}
	else
	{
		TxData[5] = 0;
		TxData[6] = 0;
		update_cc = 1;
	}


	TxData[0] = 0;
	TxData[1] = adc_data[0];
	TxData[2] = adc_data[1];
	//Update_CAN_Message1(TxData, &U5.portValues, &U16.portValues);
    // Wait until the ADC DMA completes
	  // Send CAN messages
	  while (!HAL_CAN_GetTxMailboxesFreeLevel(&hcan1));
	  HAL_StatusTypeDef status;
	  status = HAL_CAN_AddTxMessage(&hcan1, &TxHeader, TxData, &TxMailbox);
	  messages_sent++;
	  if (status == HAL_ERROR)
	  {
		  Error_Handler();
	  }
	  else if (status == HAL_BUSY)
	  {
		  HAL_CAN_BUSY++;
	  }
    osDelay(1);
  }
  /* USER CODE END StartTask02 */
}

/* USER CODE BEGIN Header_StartTask03 */
/**
* @brief Function implementing the ReadIOExpander thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask03 */
void StartTask03(void *argument)
{
  /* USER CODE BEGIN StartTask03 */

	int debounce_count = 0;
	int HAL_CAN_BUSY = 0;
	uint64_t messages_sent = 0;

	CAN_TxHeaderTypeDef TxHeader;
	uint8_t TxData[8] = { 0 };
	uint32_t TxMailbox = { 0 };

	TxHeader.IDE = CAN_ID_STD; // Standard ID (not extended)
	TxHeader.StdId = 0x7FF; // 11 bit Identifier !!Change!!
	TxHeader.RTR = CAN_RTR_DATA; // Std RTR Data frame
	TxHeader.DLC = 8; // 8 bytes being transmitted
	TxData[0] = 1;

	Update_CAN_Message1(TxData, &U5.portValues, &U16.portValues);

	/* Infinite loop */
	for(;;)
	{
	  // Read TCAL Input and update flags
	  if (GPIO_Interrupt_Triggered)
	  {
		  if (debounce_count++ <= 2) {continue;}
		  if (TCAL9538RSVR_HandleInterrupt(&U5) != HAL_OK){ Error_Handler(); }
		  //if (TCAL9538RSVR_HandleInterrupt(&U16) != HAL_OK){ Error_Handler(); }


		  Update_CAN_Message1(TxData, &U5.portValues, &U16.portValues);
		  GPIO_Interrupt_Triggered = 0;
		  debounce_count = 0;
	  }

	  // Send CAN messages
	  while (!HAL_CAN_GetTxMailboxesFreeLevel(&hcan1));
	  HAL_StatusTypeDef status;
	  status = HAL_CAN_AddTxMessage(&hcan1, &TxHeader, TxData, &TxMailbox);
	  messages_sent++;
	  if (status == HAL_ERROR)
	  {
		  Error_Handler();
	  }
	  else if (status == HAL_BUSY)
	  {
		  HAL_CAN_BUSY++;
	  }
	  osDelay(1);
  }
  /* USER CODE END StartTask03 */
}

/* USER CODE BEGIN Header_StartTask04 */
/**
* @brief Function implementing the Outputs_Control thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask04 */
void StartTask04(void *argument)
{
  /* USER CODE BEGIN StartTask04 */

  uint32_t lastBlinkTime = HAL_GetTick();
  const uint32_t blinkInterval = 500;

  /* Infinite loop */
  for(;;)
  {

    // blinking logic needs to be done here now
    // use lightState variable to see what should be turned on and then 
    // update outputPortState

    uint32_t currentTick = HAL_GetTick();

    if (currentTick - lastBlinkTime > blinkInterval)
    {
      lastBlinkTime = currentTick;
      if (lightState == LIGHTS_LEFT)
        outputPortState ^= OUTPUT_FL_LIGHT_CTRL;
      else if (lightState == LIGHTS_RIGHT)
        outputPortState ^= OUTPUT_FR_LIGHT_CTRL;
      else if (lightState == LIGHTS_HAZARD)
        outputPortState ^= (OUTPUT_FL_LIGHT_CTRL | OUTPUT_FR_LIGHT_CTRL);
      else if (lightState == LIGHTS_NONE)
        outputPortState &= ~(OUTPUT_FL_LIGHT_CTRL | OUTPUT_FR_LIGHT_CTRL);
    }

    if(TCAL9538RSVR_SetOutput(&U7, &outputPortState) != HAL_OK)
    {
    	Error_Handler();
    }

    osDelay(1);
  }
  /* USER CODE END StartTask04 */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM3 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM3) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
