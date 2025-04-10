#include "User.hpp"
#include "DashboardState.hpp"

TCAL9538RSVR U5; // input 1
TCAL9538RSVR U16; // input 2
TCAL9538RSVR U7; // output

uint16_t adc_buf[ADC_BUF_LEN];
volatile uint8_t dma_flag; // flag for DMA start and stop
static uint8_t cc_enable;
static uint8_t GPIO_Interrupt_Triggered;

DashboardState dashboardState; // Dashboard state object

ILI9341 screen(320, 240);

void CPP_UserSetup(void) {
    // Make sure that timer priorities are configured correctly
    HAL_Delay(10);

	dashboardState.reset(); // reset dashboard state

    dma_flag = 0;
    cc_enable = 0;

	HAL_CAN_Start(&hcan1); // start CAN1

	// =========== GPIO INIT =============

    if (TCAL9538RSVR_INIT(&U5, &hi2c4, 0b10, 0xFF, 0x00) != HAL_OK) { Error_Handler(); } // inputs
    //if (TCAL9538RSVR_INIT(&U16, &hi2c4, 0b01, 0b00111111, 0b11000000) != HAL_OK) { Error_Handler(); }
    if (TCAL9538RSVR_INIT(&U7, &hi2c4, 0x00, 0b00000000, 0b00000000) != HAL_OK) { Error_Handler(); } // output

    // set outputs to low to start
	if (dashboardState.writeToPort(U7) != HAL_OK) { Error_Handler(); }

	// Set up UART4 for receiving data from the steering wheel
	HAL_UART_Receive_IT(&huart4, &dashboardState.uart_rx, 1); // enable uart interrupt

	// ========== SCREEN INIT =============
	
    screen.Init();
    screen.SetRotation(3);
    screen.ClearScreen(RGB565_WHITE);

    uint16_t x_text = 70;
    uint16_t y_text = 10;
    const char* str1 = "UF Solar Gators :)\0";
    screen.SetTextSize(2);
    screen.DrawText(x_text, y_text, str1, RGB565_BLACK);

    x_text = 55;
    y_text = 170;
    const char* str2 = "BMS    MC    Array\0";

    screen.SetTextSize(2);
    screen.DrawText(x_text, y_text, str2, RGB565_BLACK);

    screen.FillCircle(70, 210, 10, RGB565_RED);
    screen.FillCircle(150, 210, 10, RGB565_RED);
    screen.FillCircle(235, 210, 10, RGB565_RED);

	// temp to help debug  
	const char* str3 = "HedLit Horn  Fan\0";
    screen.SetTextSize(2);
    screen.DrawText(55, 80, str3, RGB565_BLACK);  // Labels above the circles

    screen.FillCircle(70, 120, 10, RGB565_RED);   // Headlights
    screen.FillCircle(150, 120, 10, RGB565_RED);  // Horn
    screen.FillCircle(235, 120, 10, RGB565_RED);  // Fan
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

    uint32_t currentTick = HAL_GetTick();

    if (currentTick - lastBlinkTime > blinkInterval)
    {
      	lastBlinkTime = currentTick;
		// enter critical section because we do read-modify-write operations in that function
		DASHBOARD_CRITICAL(
			dashboardState.blinkLights()
	  	);
	}

	if (dashboardState.updateRequested) {
		// enter critical section because we do read-modify-write operations in that function
		DASHBOARD_CRITICAL(
			dashboardState.updateFromUART()
		);
		dashboardState.updateRequested = 0; // reset update requested flag
	}

	if (dashboardState.writeToPort(U7) != HAL_OK) { Error_Handler(); } // write to output port

    osDelay(100);
  }
  /* USER CODE END StartTask04 */
}


void StartTask05(void *argument)
{
	/* USER CODE BEGIN StartTask05 */
	uint16_t color;
	bool lightStateChanged = false;
	bool bmsStatusChanged = false;
	bool mcStatusChanged = false;
	bool arrayStatusChanged = false;
	bool hornStateChanged = false;
	bool fanStateChanged = false;
	bool headlightStateChanged = false;
  /* Infinite loop */
  for(;;)
  {
	DASHBOARD_CRITICAL( // critical region for all of these read-read operations that are not atomic
		lightStateChanged = dashboardState.oldLightState != dashboardState.lightState;
		bmsStatusChanged = dashboardState.old_bmsStatus != dashboardState.bmsStatus;
		mcStatusChanged = dashboardState.old_mcStatus != dashboardState.mcStatus;
		arrayStatusChanged = dashboardState.old_arrayStatus != dashboardState.arrayStatus;
		hornStateChanged = dashboardState.oldHornState != dashboardState.hornState;
		fanStateChanged = dashboardState.oldFanState != dashboardState.fanState;
		headlightStateChanged = dashboardState.oldHeadlightState != dashboardState.headlightState;

		dashboardState.oldLightState = dashboardState.lightState;
		dashboardState.old_bmsStatus = dashboardState.bmsStatus;
		dashboardState.old_mcStatus = dashboardState.mcStatus;
		dashboardState.old_arrayStatus = dashboardState.arrayStatus;
		dashboardState.oldHornState = dashboardState.hornState;
		dashboardState.oldFanState = dashboardState.fanState;
		dashboardState.oldHeadlightState = dashboardState.headlightState;
	); // end critical section

	if(lightStateChanged){
		HAL_Delay(1);
		if(dashboardState.lightState == LIGHTS_LEFT){
			color = RGB565_GREEN;
			screen.FillCircle(20, 20, 10, color);

			color = RGB565_WHITE;
			screen.FillCircle(300, 20, 10, color);
		}
		if(dashboardState.lightState == LIGHTS_RIGHT){
			color = RGB565_WHITE;
			screen.FillCircle(20, 20, 10, color);

			color = RGB565_GREEN;
			screen.FillCircle(300, 20, 10, color);
		}
		if(dashboardState.lightState == LIGHTS_HAZARD){
			color = RGB565_GREEN;
			screen.FillCircle(20, 20, 10, color);

			color = RGB565_GREEN;
			screen.FillCircle(300, 20, 10, color);
		}
		if(dashboardState.lightState == LIGHTS_NONE){

			color = RGB565_WHITE;
			screen.FillCircle(20, 20, 10, color);

			color = RGB565_WHITE;
			screen.FillCircle(300, 20, 10, color);
		}
	}
	if(bmsStatusChanged){
		if (dashboardState.bmsStatus) color = RGB565_GREEN;
		else color = RGB565_RED;
		screen.FillCircle(70, 210, 10, color);
	}
	if(mcStatusChanged){
		if (dashboardState.mcStatus) color = RGB565_GREEN;
		else color = RGB565_RED;
		screen.FillCircle(150, 210, 10, color);
	}
	if(arrayStatusChanged){
		if (dashboardState.arrayStatus) color = RGB565_GREEN;
		else color = RGB565_RED;
		screen.FillCircle(235, 210, 10, color);
	}

	// temp debug stuff
	if (hornStateChanged) {
		if (dashboardState.hornState) color = RGB565_GREEN;
		else color = RGB565_RED;
		screen.FillCircle(150, 120, 10, color);
	}
	if (fanStateChanged) {
		if (dashboardState.fanState) color = RGB565_GREEN;
		else color = RGB565_RED;
		screen.FillCircle(235, 120, 10, color);
	}
	if (headlightStateChanged) {
		if (dashboardState.headlightState) color = RGB565_GREEN;
		else color = RGB565_RED;
		screen.FillCircle(70, 120, 10, color);
	}

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
	if (dashboardState.hornState) // Horn
		flags[1] |= (1 << 6);
	else
		flags[1] &= ~(1 << 6);
	if (dashboardState.pttState) // PTT (Push to Talk)
		flags[1] |= (1 << 7);
	else
		flags[1] &= ~(1 << 7);

	/*
		NOTE:
			currently sending state of turn signal, turn signal on or off
			if wanting to send actual control of turn signal light, need to send outputPortState variable instead of lightState	
	*/
	if (dashboardState.lightState == LIGHTS_HAZARD) // Blinkers
		flags[2] |= (1 << 0);
	else
		flags[2] &= ~(1 << 0);
	if (dashboardState.lightState == LIGHTS_LEFT) // Left Turn
		flags[2] |= (1 << 1);
	else
		flags[2] &= ~(1 << 1);
	if (dashboardState.lightState == LIGHTS_RIGHT) // Right Turn
		flags[2] |= (1 << 2);
	else
		flags[2] &= ~(1 << 2);
	if (dashboardState.headlightState) // Headlights
		flags[2] |= (1 << 3);
	else
		flags[2] &= ~(1 << 3);

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

void Init_CAN_Filter1(CAN_HandleTypeDef &hcan1)
{
  CAN_FilterTypeDef canfilterconfig;
  canfilterconfig.FilterActivation = CAN_FILTER_ENABLE;
  canfilterconfig.FilterBank = 18;
  canfilterconfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;
  canfilterconfig.FilterMode = CAN_FILTERMODE_IDLIST;
  canfilterconfig.FilterScale = CAN_FILTERSCALE_32BIT;
  canfilterconfig.SlaveStartFilterBank = 20;

  // CAN ID"S TO ACCEPT GO HERE, 4 ACCEPTED IN LIST MODE
  canfilterconfig.FilterIdHigh = 0x000 << 5;
  canfilterconfig.FilterIdLow = 0x000 << 5;
  canfilterconfig.FilterMaskIdHigh = 0x000 << 5;
  canfilterconfig.FilterMaskIdLow = 0x000 << 5;

  HAL_CAN_ConfigFilter(&hcan1, &canfilterconfig);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == UART4)
  	{
		if (dashboardState.uart_rx != dashboardState.old_uart_rx)
		{
			dashboardState.updateRequested = 1; // set flag to update dashboard state
			dashboardState.old_uart_rx = dashboardState.uart_rx;
		}
	}
	HAL_UART_Receive_IT(&huart4, &dashboardState.uart_rx, 1);
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
	dma_flag = 1;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	GPIO_Interrupt_Triggered = 1;
}

