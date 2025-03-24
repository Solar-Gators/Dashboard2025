#include "User.hpp"


TCAL9538RSVR U5; // input 1
TCAL9538RSVR U16; // input 2
TCAL9538RSVR U7; // output

uint16_t adc_buf[ADC_BUF_LEN];
volatile uint8_t dma_flag; // flag for DMA start and stop
static uint8_t cc_enable;
static uint8_t GPIO_Interrupt_Triggered;



uint8_t outputPortState; // variable with state of output port
uint8_t uart_rx; // variable for holding the recieved data over uart from steering wheel, its only sending one byte
LightState lightState;
LightState oldLightsState;
uint8_t hornState;

uint8_t BMS_Status;
uint8_t MC_Status;
uint8_t Array_Status;

uint8_t old_BMS_Status;
uint8_t old_MC_Status;
uint8_t old_Array_Status;

uint8_t UART4_rxBuffer[20];
ILI9341 screen(320, 240);

void CPP_UserSetup(void) {
    // Make sure that timer priorities are configured correctly
    HAL_Delay(10);



    dma_flag = 0;
    cc_enable = 0;

    hornState = 0;

    BMS_Status = 0;
    MC_Status = 0;
    Array_Status = 0;

    old_BMS_Status = 0;
    old_MC_Status = 0;
    old_Array_Status = 0;

    outputPortState = 0; // variable with state of output port
    uart_rx = 0; // variable for holding the recieved data over uart from steering wheel, its only sending one byte
    lightState = LIGHTS_NONE;
    oldLightsState = LIGHTS_NONE;

    if (TCAL9538RSVR_INIT(&U5, &hi2c4, 0b10, 0xFF, 0x00) != HAL_OK) { Error_Handler(); } // inputs
      //if (TCAL9538RSVR_INIT(&U16, &hi2c4, 0b01, 0b00111111, 0b11000000) != HAL_OK) { Error_Handler(); }
    if (TCAL9538RSVR_INIT(&U7, &hi2c4, 0x00, 0b00000000, 0b00000000) != HAL_OK) { Error_Handler(); } // output

    // set outputs to low to start
	outputPortState = ~outputPortState;
    TCAL9538RSVR_SetOutput(&U7, &outputPortState);

	HAL_UART_Receive_IT(&huart4, &uart_rx, 1); // enable uart interrupt

    ILI9341 screen(320, 240);
    screen.Init();
    screen.SetRotation(3);
    screen.ClearScreen(0xFFFF);

    uint16_t x_text = 80;
    uint16_t y_text = 10;
    const char* str1 = "UF Solar Gators\0";
    uint16_t color = 32;
    screen.SetTextSize(2);
    screen.DrawText(x_text, y_text, str1, color);

    x_text = 55;
    y_text = 170;
    const char* str2 = "BMS    MC    Array\0";

    screen.SetTextSize(2);
    screen.DrawText(x_text, y_text, str2, color);

    color  = 0xf800;
    screen.FillCircle(70, 210, 10, color);
    screen.FillCircle(150, 210, 10, color);
    screen.FillCircle(235, 210, 10, color);

}


void StartTask01(void *argument)
{
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for(;;)
  {
	  //HAL_UART_Receive(&huart4, UART4_rxBuffer, 1, HAL_MAX_DELAY);
	  HAL_GPIO_TogglePin(GPIOA, OK_LED_Pin);
    osDelay(500);
  }
  /* USER CODE END 5 */
}

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
	adc_var_avg = 0;
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
    osDelay(20);
  }
  /* USER CODE END StartTask02 */
}

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
	  osDelay(50);
  }
  /* USER CODE END StartTask03 */
}


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

    osDelay(100);
  }
  /* USER CODE END StartTask04 */
}


void StartTask05(void *argument)
{
  /* USER CODE BEGIN StartTask05 */
  uint16_t color = 32;
  /* Infinite loop */
  for(;;)
  {
	if(oldLightsState != lightState){
		HAL_Delay(1);
		if(lightState == LIGHTS_LEFT){
			color = 0x07E0;
			screen.FillCircle(20, 20, 10, color);

			color = 0xFFFF;
			screen.FillCircle(300, 20, 10, color);
		}
		if(lightState == LIGHTS_RIGHT){
			color = 0xFFFF;
			screen.FillCircle(20, 20, 10, color);

			color = 0x07E0;
			screen.FillCircle(300, 20, 10, color);
		}
		if(lightState == LIGHTS_HAZARD){
			color = 0x07E0;
			screen.FillCircle(20, 20, 10, color);

			color = 0x07E0;
			screen.FillCircle(300, 20, 10, color);
		}
		if(lightState == LIGHTS_NONE){

			color = 0xFFFF;
			screen.FillCircle(20, 20, 10, color);

			color = 0xFFFF;
			screen.FillCircle(300, 20, 10, color);
		}
		oldLightsState = lightState;
	}
	if(BMS_Status != old_BMS_Status){
		if(BMS_Status == 1){
			color == 0x07E0;
		}else{
			color == 0xf800;
		}

	}
    BMS_Status = 0;
    MC_Status = 0;
    Array_Status = 0;

    old_BMS_Status = 0;
    old_MC_Status = 0;
    old_Array_Status = 0;

    osDelay(100);
  }
  /* USER CODE END StartTask05 */
}


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
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == UART4)
  	{
		// uart data for lights (blinkers)
		if (uart_rx & BUTTON_LEFT_TURN)
			lightState = LIGHTS_LEFT;
		else if (uart_rx & BUTTON_RIGHT_TURN)
			lightState = LIGHTS_NONE;
		else if (uart_rx & BUTTON_HAZARD)
			lightState = LIGHTS_HAZARD;
		else
			lightState = LIGHTS_NONE;

		// if headlight should be on  
		if (uart_rx & BUTTON_HEADLIGHTS)
			outputPortState |= (OUTPUT_R_HEAD_CTRL | OUTPUT_L_HEAD_CTRL);
		else 
			outputPortState &= ~(OUTPUT_R_HEAD_CTRL | OUTPUT_L_HEAD_CTRL);

		// if display should be on 
		if (uart_rx & BUTTON_DISPLAY)
			outputPortState |= OUTPUT_FL_LIGHT_CTRL;
		else
			outputPortState &= ~OUTPUT_FL_LIGHT_CTRL;

		// if horn should be on
		if (uart_rx & BUTTON_HORN)
			outputPortState |= OUTPUT_HORN_CTRL;
		else
			outputPortState &= ~OUTPUT_HORN_CTRL;

		if (uart_rx & BUTTON_FAN)
			outputPortState |= OUTPUT_FAN_CTRL;
		else
			outputPortState &= ~OUTPUT_FAN_CTRL;

		/*
		TODO: Some code with PTT button (does this just go over can what even is PTT)
		TODO: Some code with Fan (where does fan come from) 
		*/
	}
	HAL_UART_Receive_IT(&huart4, &uart_rx, 1);
}


void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc){
	dma_flag = 1;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	GPIO_Interrupt_Triggered = 1;
}

