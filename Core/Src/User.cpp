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

	// -------------------------
	// CAN INIT
	// -------------------------
	Init_CAN_Filter1(hcan1); // set up CAN filter for CAN1
	Init_CAN_Filter2(hcan2); // set up CAN filter for CAN1
	HAL_CAN_Start(&hcan1); // start CAN1
	HAL_CAN_Start(&hcan2); // start CAN1
	if (HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
	{
		Error_Handler();
	}
	if (HAL_CAN_ActivateNotification(&hcan2, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
	{
		Error_Handler();
	}

	// -------------------------
	// GPIO EXPANDERS INIT
	// -------------------------

    if (TCAL9538RSVR_INIT(&U5, &hi2c4, 0b10, 0xFF, 0x00) != HAL_OK) { Error_Handler(); } // inputs
    if (TCAL9538RSVR_INIT(&U16, &hi2c4, 0b01, 0xFF, 0x00) != HAL_OK) { Error_Handler(); }
    if (TCAL9538RSVR_INIT(&U7, &hi2c4, 0x00, 0b00000000, 0b00000000) != HAL_OK) { Error_Handler(); } // output

    // set outputs to low to start
	if (dashboardState.writeToPort(U7) != HAL_OK) { Error_Handler(); }

	// Set up UART4 for receiving data from the steering wheel
	HAL_UART_Receive_IT(&huart4, &dashboardState.uart_rx, 1); // enable uart interrupt

	// -------------------------
	// SCREEN INIT 
	// -------------------------
	
	HAL_Delay(100); // wait for screen to power on
    screen.Init();
    screen.SetRotation(1);
    screen.ClearScreen(RGB565_WHITE);
}


void StartTask01(void *argument)
{
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  	CAN_TxHeaderTypeDef TxHeader;
	uint8_t TxData[8] = { 0 };
	uint32_t TxMailbox = { 0 };

	TxHeader.IDE = CAN_ID_EXT;
	TxHeader.ExtId = CAN_ID_MITSUBA_MOTOR_REQUEST;
	TxHeader.RTR = CAN_RTR_DATA;
	TxHeader.DLC = 1;
	TxData[0] = 1; // bit 0 = request for frame 0

	for(;;) {
		// toggle ok LED
		HAL_GPIO_TogglePin(OK_LED_GPIO_Port, OK_LED_Pin);

		// also send can message to request frame 0 from mitsuba motor
		int wait = 0;
		while (!HAL_CAN_GetTxMailboxesFreeLevel(&hcan1) && wait++ < 10000)
			osDelay(1);
		HAL_StatusTypeDef status;
		status = HAL_CAN_AddTxMessage(&hcan1, &TxHeader, TxData, &TxMailbox);

		if (status == HAL_ERROR)
		{
			Error_Handler();
		}

		osDelay(200);
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

	CAN_TxHeaderTypeDef TxHeader = { 0 };
	uint32_t TxMailbox = { 0 };
	uint8_t TxData[8] = { 0 };

	TxHeader.IDE = CAN_ID_STD; // Standard ID (not extended)
	TxHeader.StdId = 0x0; // 11 bit Identifier
	TxHeader.RTR = CAN_RTR_DATA; // Std RTR Data frame
	TxHeader.DLC = 8; // 8 bytes being transmitted
	TxData[0] = 0;

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
    // Wait until the ADC DMA completes
	  // Send CAN messages
	  int wait = 0;
	  while (!HAL_CAN_GetTxMailboxesFreeLevel(&hcan1) && wait++ < 10000)
		osDelay(1);
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

	CAN_TxHeaderTypeDef TxHeader = { 0 };
	uint8_t TxData[8] = { 0 };
	uint32_t TxMailbox = { 0 };
	TxData[1] |= (1 << 5); // Set direction to forward to start
	TxData[1] |= (1 << 4); // Set mc enable to true to start

	TxHeader.IDE = CAN_ID_STD; // Standard ID (not extended)
	TxHeader.StdId = 0x7FF; // 11 bit Identifier !!Change!! lol still need to change
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
		  DASHBOARD_CRITICAL(
		  	if (TCAL9538RSVR_HandleInterrupt(&U5) != HAL_OK) { Error_Handler(); }
		  	if (TCAL9538RSVR_HandleInterrupt(&U16) != HAL_OK){ Error_Handler(); }
		  );

		  Update_CAN_Message1(TxData, &U5.portValues, &U16.portValues);
		  GPIO_Interrupt_Triggered = 0;
		  debounce_count = 0;
	  }

	  if (dashboardState.update_can_message_1) {
		  Update_CAN_Message1(TxData, &U5.portValues, &U16.portValues);
		  dashboardState.update_can_message_1 = 0;
	  }

	  // Send CAN messages
	  int wait = 0;
	  while (!HAL_CAN_GetTxMailboxesFreeLevel(&hcan1) && wait++ < 10000)
 		osDelay(1);
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

  dashboardState.lastBlinkTime = HAL_GetTick(); // initialize last blink time

  /* Infinite loop */
  for(;;)
  {

    uint32_t currentTick = HAL_GetTick();

    if (currentTick - dashboardState.lastBlinkTime > BLINK_INTERVAL_MS)
    {
      	dashboardState.lastBlinkTime = currentTick;
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
		dashboardState.update_can_message_1 = 1; // set flag to update CAN message 1
	}

	DASHBOARD_CRITICAL(
		if (dashboardState.writeToPort(U7) != HAL_OK) { Error_Handler(); } // write to output port
	);

    osDelay(50);
  }
  /* USER CODE END StartTask04 */
}


void StartTask05(void *argument)
{
    uint16_t color;
    bool lightStateChanged = false;
    bool bmsStatusChanged = false;
    bool mcStatusChanged = false;
    bool arrayStatusChanged = false;
    bool directionChanged = false;
    bool desiredBMSStatusChanged = false;
	bool desiredArrayStatusChanged = false;
    bool regenBrakingChanged = false;
    bool screenResetStatusChanged = false;

    Init_Screen(screen);

    for (;;)
    {
        DASHBOARD_CRITICAL(
            lightStateChanged = dashboardState.oldLightStateScreen != dashboardState.lightState;
            bmsStatusChanged = dashboardState.old_bmsStatus != dashboardState.bmsStatus;
            mcStatusChanged = dashboardState.old_mcStatus != dashboardState.mcStatus;
            arrayStatusChanged = dashboardState.old_arrayStatus != dashboardState.arrayStatus;
            directionChanged = dashboardState.old_direction != dashboardState.direction;
            screenResetStatusChanged = dashboardState.displayState != dashboardState.old_displayState;
            desiredBMSStatusChanged = dashboardState.old_desiredBMSStatus != dashboardState.desiredBMSStatus;
			desiredArrayStatusChanged = dashboardState.old_desiredArrayStatus != dashboardState.desiredArrayStatus;
            regenBrakingChanged = dashboardState.regenBraking != dashboardState.old_regenBraking;

            dashboardState.oldLightStateScreen = dashboardState.lightState;
            dashboardState.old_bmsStatus = dashboardState.bmsStatus;
            dashboardState.old_mcStatus = dashboardState.mcStatus;
            dashboardState.old_arrayStatus = dashboardState.arrayStatus;
            dashboardState.old_direction = dashboardState.direction;
            dashboardState.old_displayState = dashboardState.displayState;
            dashboardState.old_desiredBMSStatus = dashboardState.desiredBMSStatus;
            dashboardState.old_regenBraking = dashboardState.regenBraking;
			dashboardState.old_desiredArrayStatus = dashboardState.desiredArrayStatus;
        );

        if (screenResetStatusChanged) {
			Init_Screen(screen);
        }

        if (lightStateChanged || screenResetStatusChanged) {
            HAL_Delay(1);
            DASHBOARD_CRITICAL(
                switch (dashboardState.lightState) {
                    case LIGHTS_LEFT:
                        screen.FillCircle(LEFT_SIGNAL_X, SIGNAL_Y, INDICATOR_RADIUS, RGB565_GREEN);
                        screen.FillCircle(RIGHT_SIGNAL_X, SIGNAL_Y, INDICATOR_RADIUS, RGB565_WHITE);
                        break;
                    case LIGHTS_RIGHT:
                        screen.FillCircle(LEFT_SIGNAL_X, SIGNAL_Y, INDICATOR_RADIUS, RGB565_WHITE);
                        screen.FillCircle(RIGHT_SIGNAL_X, SIGNAL_Y, INDICATOR_RADIUS, RGB565_GREEN);
                        break;
                    case LIGHTS_HAZARD:
                        screen.FillCircle(LEFT_SIGNAL_X, SIGNAL_Y, INDICATOR_RADIUS, RGB565_GREEN);
                        screen.FillCircle(RIGHT_SIGNAL_X, SIGNAL_Y, INDICATOR_RADIUS, RGB565_GREEN);
                        break;
                    case LIGHTS_NONE:
                    default:
                        screen.FillCircle(LEFT_SIGNAL_X, SIGNAL_Y, INDICATOR_RADIUS, RGB565_WHITE);
                        screen.FillCircle(RIGHT_SIGNAL_X, SIGNAL_Y, INDICATOR_RADIUS, RGB565_WHITE);
                        break;
                }
            );
        }

        if (bmsStatusChanged || screenResetStatusChanged) {
            DASHBOARD_CRITICAL(
                color = dashboardState.bmsStatus ? RGB565_GREEN : RGB565_RED;
                screen.FillCircle(BMS_CIRCLE_X, INDICATOR_CIRCLE_Y, INDICATOR_RADIUS, color);
            );
        }

        if (desiredBMSStatusChanged || screenResetStatusChanged) {
            DASHBOARD_CRITICAL(
                color = dashboardState.desiredBMSStatus ? RGB565_GREEN : RGB565_RED;
                screen.FillCircle(BMS_CIRCLE_X, INDICATOR_CIRCLE_Y + 25, INDICATOR_RADIUS, color);
            );
        }

		if (desiredArrayStatusChanged || screenResetStatusChanged) {
			DASHBOARD_CRITICAL(
				color = dashboardState.desiredArrayStatus ? RGB565_GREEN : RGB565_RED;
				screen.FillCircle(ARRAY_CIRCLE_X, INDICATOR_CIRCLE_Y + 25, INDICATOR_RADIUS, color);
			);
		}

        if (mcStatusChanged || screenResetStatusChanged) {
            DASHBOARD_CRITICAL(
                color = dashboardState.mcStatus ? RGB565_GREEN : RGB565_RED;
                screen.FillCircle(MC_CIRCLE_X, INDICATOR_CIRCLE_Y, INDICATOR_RADIUS, color);
            );
        }

        if (arrayStatusChanged || screenResetStatusChanged) {
            DASHBOARD_CRITICAL(
                color = dashboardState.arrayStatus ? RGB565_GREEN : RGB565_RED;
                screen.FillCircle(ARRAY_CIRCLE_X, INDICATOR_CIRCLE_Y, INDICATOR_RADIUS, color);
            );
        }

        if (directionChanged || screenResetStatusChanged) {
            DASHBOARD_CRITICAL(
                if (dashboardState.direction) {
                    screen.DrawText(DIRECTION_TEXT_X, DIRECTION_TEXT_Y, "Forward\0", RGB565_BLACK);
                } else {
                    screen.DrawText(DIRECTION_TEXT_X, DIRECTION_TEXT_Y, "Reverse\0", RGB565_BLACK);
                }
            );
        }

        if (regenBrakingChanged || screenResetStatusChanged) {
            DASHBOARD_CRITICAL(
                color = dashboardState.regenBraking ? RGB565_GREEN : RGB565_RED;
                screen.FillCircle(REGEN_BRAKING_CIRCLE_X, INDICATOR_CIRCLE_Y, INDICATOR_RADIUS, color);
            );
        }

        float motor_power, car_speed;
        DASHBOARD_CRITICAL(
            motor_power = dashboardState.getMotorPower();
            car_speed = dashboardState.getCarSpeed();
        );

        char buffer[16];

        DASHBOARD_CRITICAL(
			screen.DrawRect(STATS_VALUES_X, CAR_SPEED_LABEL_Y, VALUE_WIDTH, VALUE_HEIGHT, RGB565_WHITE);
            if (dashboardState.mcStatus) {
                int speed_whole = (int)car_speed;
                int speed_frac = (int)((car_speed - speed_whole) * 100);
                snprintf(buffer, sizeof(buffer), "%d.%02d MPH", speed_whole, speed_frac);
                screen.DrawText(STATS_VALUES_X, CAR_SPEED_LABEL_Y, buffer, RGB565_BLACK);

                int power_whole = (int)motor_power;
                int power_frac = (int)((motor_power - power_whole) * 10);
                snprintf(buffer, sizeof(buffer), "%d.%01d W", power_whole, power_frac);
                screen.DrawText(STATS_VALUES_X, MOTOR_POWER_LABEL_Y, buffer, RGB565_BLACK);
            } else {
                screen.DrawText(STATS_VALUES_X, CAR_SPEED_LABEL_Y, "Need MC", RGB565_BLACK);
                screen.DrawText(STATS_VALUES_X, MOTOR_POWER_LABEL_Y, "Need MC", RGB565_BLACK);
            }

			uint32_t supp_batt_voltage_mV = dashboardState.supp_batt_0_mv |
				(dashboardState.supp_batt_1_mv << 8) |
				(dashboardState.supp_batt_2_mv << 16) |
				(dashboardState.supp_batt_3_mv << 24);
			int voltage_whole = supp_batt_voltage_mV / 1000;
			int voltage_frac = (supp_batt_voltage_mV % 1000) / 10;
			snprintf(buffer, sizeof(buffer), "%d.%02d V", voltage_whole, voltage_frac);
			char str[4];
            screen.DrawText(STATS_VALUES_X, VOLTAGE_SUPP_BATT_LABEL_Y, buffer, RGB565_BLACK);
			sprintf(str, "%d", dashboardState.dead_bms_message_code);
			screen.DrawText(20, 220, str, RGB565_BLACK);
        );

        osDelay(200);
    }
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
	uint8_t changedEdges_flag2 = (*Input2) ^ prev_input2;

	flags[1] ^= CHECK_BIT(changedEdges_flag2, 4) << 0; // Main
	if (!CHECK_BIT(*Input2, 5) || dashboardState.brake_debug) // Break
	{
		flags[1] |= (1 << 1); // Break
	}
	else
	{
		flags[1] &= ~(1 << 1); // Break
	}

	// CHECK_BIT(risingEdges_flag2, 1) all unused buttons
	// CHECK_BIT(risingEdges_flag2, 2)
	// CHECK_BIT(risingEdges_flag2, 3)
	flags[2] ^= CHECK_BIT(risingEdges_flag2, 0) << 4; // regen breaking

	flags[1] ^= CHECK_BIT(risingEdges_flag1, 4) << 2; // Array
	flags[1] ^= CHECK_BIT(risingEdges_flag1, 5) << 3; // BMS Contactors
	flags[1] ^= CHECK_BIT(risingEdges_flag1, 6) << 4; // MC Enable
	flags[1] ^= CHECK_BIT(risingEdges_flag1, 7) << 5; // Direction


	dashboardState.desiredBMSStatus = flags[1] & (1 << 3); // Desired BMS contactors status (0 = open, 1 = closed) 
	dashboardState.regenBraking = flags[2] & (1 << 4); // Regen braking (0 = off, 1 = on)
	dashboardState.desiredArrayStatus = flags[1] & (1 << 2); // Desired array status (0 = off, 1 = on)
	
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
  	CAN_FilterTypeDef filter = {0};
    filter.FilterActivation = ENABLE;
    filter.FilterBank = 0;
    filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    filter.FilterMode = CAN_FILTERMODE_IDMASK;
    filter.FilterScale = CAN_FILTERSCALE_32BIT;

    // Accept ALL messages (for debug/testing)
    filter.FilterIdHigh = 0x0000;
    filter.FilterIdLow = 0x0000;
    filter.FilterMaskIdHigh = 0x0000;
    filter.FilterMaskIdLow = 0x0000;

    HAL_CAN_ConfigFilter(&hcan1, &filter);
}

void Init_CAN_Filter2(CAN_HandleTypeDef &hcan2)
{
  	CAN_FilterTypeDef filter = {0};
    filter.FilterActivation = ENABLE;
    filter.FilterBank = 0;
    filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    filter.FilterMode = CAN_FILTERMODE_IDMASK;
    filter.FilterScale = CAN_FILTERSCALE_32BIT;

    // Accept ALL messages (for debug/testing)
    filter.FilterIdHigh = 0x0000;
    filter.FilterIdLow = 0x0000;
    filter.FilterMaskIdHigh = 0x0000;
    filter.FilterMaskIdLow = 0x0000;

    HAL_CAN_ConfigFilter(&hcan2, &filter);
}

void Init_Screen(ILI9341 &screen)
{
	screen.Init();
	screen.SetRotation(1);
	screen.ClearScreen(RGB565_WHITE);
	screen.SetTextSize(TEXT_SIZE);

	DASHBOARD_CRITICAL(
		screen.DrawText(TITLE_TEXT_X, TITLE_TEXT_Y, "UF Solar Gators :D\0", RGB565_BLACK);

		// labels (mc, array, and bms status)
		screen.DrawText(LABELS_TEXT_X, LABELS_TEXT_Y, "BMS MC Arry Rgn\0", RGB565_BLACK);

		// labels for stats
		screen.DrawText(STATS_LABELS_X, CAR_SPEED_LABEL_Y, "Speed: \0", RGB565_BLACK);
		screen.DrawText(STATS_LABELS_X, MOTOR_POWER_LABEL_Y, "Power: \0", RGB565_BLACK);
		screen.DrawText(STATS_LABELS_X, VOLTAGE_SUPP_BATT_LABEL_Y, "VSupp: \0", RGB565_BLACK);
						
		// circules for those labels lol
		screen.FillCircle(BMS_CIRCLE_X, INDICATOR_CIRCLE_Y, INDICATOR_RADIUS, RGB565_RED);
		screen.FillCircle(BMS_CIRCLE_X, INDICATOR_CIRCLE_Y+25, INDICATOR_RADIUS, RGB565_RED);
		screen.FillCircle(MC_CIRCLE_X, INDICATOR_CIRCLE_Y, INDICATOR_RADIUS, RGB565_RED);
		screen.FillCircle(ARRAY_CIRCLE_X, INDICATOR_CIRCLE_Y, INDICATOR_RADIUS, RGB565_RED);
		screen.FillCircle(ARRAY_CIRCLE_X, INDICATOR_CIRCLE_Y+25, INDICATOR_RADIUS, RGB565_RED);
		screen.FillCircle(REGEN_BRAKING_CIRCLE_X, INDICATOR_CIRCLE_Y, INDICATOR_RADIUS, RGB565_RED);

		if (dashboardState.direction) {
			screen.DrawText(DIRECTION_TEXT_X, DIRECTION_TEXT_Y, "Forward\0", RGB565_BLACK);
		}
		else {
			screen.DrawText(DIRECTION_TEXT_X, DIRECTION_TEXT_Y, "Reverse\0", RGB565_BLACK);
		}
	);
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

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef RxHeader;
    uint8_t RxData[8];

    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &RxHeader, RxData) != HAL_OK)
    {
        Error_Handler();
    }
    if (hcan->Instance == CAN1) {
		// vcu sends mc and array status
		if (RxHeader.IDE == CAN_ID_STD && RxHeader.StdId == CAN_ID_VCU_SENSORS)
		{
			uint8_t statusByte = RxData[VCU_SENSORS_STATUS_BYTE_INDEX];

			dashboardState.mcStatus = CHECK_BIT(
				statusByte,
				(int)VCU_SENSORS_STATUS_BITS::VCU_MC_ENABLED_BIT_POS
			);
			dashboardState.arrayStatus = CHECK_BIT(
				statusByte,
				(int)VCU_SENSORS_STATUS_BITS::VCU_ARRAY_ENABLED_BIT_POS
			);
			dashboardState.direction = CHECK_BIT(
				statusByte,
				(int)VCU_SENSORS_STATUS_BITS::VCU_DIRECTION_BIT_POS
			);
		}
		// powerboard sends voltage of supplemental battery
		else if (RxHeader.IDE == CAN_ID_STD && RxHeader.StdId == CAN_ID_POWERBOARD)
		{
			dashboardState.supp_batt_0_mv = RxData[POWERBOARD_SUPPLEMENTAL_BATTERY_VOLTAGE_LSB_INDEX];
			dashboardState.supp_batt_1_mv = RxData[POWERBOARD_SUPPLEMENTAL_BATTERY_VOLTAGE_LSB_INDEX+1];
			dashboardState.supp_batt_2_mv = RxData[POWERBOARD_SUPPLEMENTAL_BATTERY_VOLTAGE_LSB_INDEX+2];
			dashboardState.supp_batt_3_mv = RxData[POWERBOARD_SUPPLEMENTAL_BATTERY_VOLTAGE_LSB_INDEX+3];
		}
		// bms sends contactors closed indicator and other power stuff that we are ignoring on dashboard
		else if (RxHeader.IDE == CAN_ID_STD && RxHeader.StdId == CAN_ID_BMS_POWER_CONSUM_INFO)
		{
			dashboardState.bmsStatus = RxData[BMS_CONTACTORS_CLOSED_INDEX];
			dashboardState.dead_bms_message_code = RxData[0];
		}
    } else { // CAN2
		// mitsuba motor sends velocity and other data?
		if (RxHeader.IDE == CAN_ID_EXT && RxHeader.ExtId == CAN_ID_MITSUBA_MOTOR_FRAME_0)
		{
			uint64_t full_data = 0;
			for (int i = 0; i < 8; i++)
			{
				full_data = (full_data << 8) | RxData[7 - i]; // lsb first
			}

			uint16_t motor_rpm = (full_data >> MITSUBA_RPM_VELOCITY_LSB_BIT_INDEX) & ((1 << MITSUBA_RPM_VELOCITY_LEN) - 1);
			uint16_t motor_voltage = (full_data >> MITSUBA_VOLTAGE_LSB_BIT_INDEX) & ((1 << MITSUBA_VOLTAGE_LEN) - 1);
			uint16_t motor_current = (full_data >> MITSUBA_CURRENT_LSB_BIT_INDEX) & ((1 << MITSUBA_CURRENT_LEN) - 1);
			uint8_t motor_current_direction = (full_data >> MITSUBA_BATTERY_CURRENT_DIRECTION_BIT_INDEX) & 0x01;
			dashboardState.motor_rpm_lsb = motor_rpm & 0xFF;
			dashboardState.motor_rpm_msb = (motor_rpm >> 8) & 0xFF;
			dashboardState.motor_voltage_lsb = motor_voltage & 0xFF;
			dashboardState.motor_voltage_msb = (motor_voltage >> 8) & 0xFF;
			dashboardState.motor_current_lsb = motor_current & 0xFF;
			dashboardState.motor_current_msb = (motor_current >> 8) & 0xFF;
			dashboardState.motor_current_direction = motor_current_direction;
		}
    }
}


