/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
//#include "ble_commands.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Include memory map of our MCU */
#include <stm32l475xx.h>

/* Include LED driver */
#include "leds.h"
#include "lptimer.h"
#include "i2c.h"
#include "lsm6dsl.h"
#include "ble.h"
#include <stdlib.h>

#define XL_DEAD_BAND        5000
#define MINUTE				10 //60
#define SEND_BLE            10

#define DISCOVERABLE        1
#define NONDISCOVERABLE     0

#define FAST                1
#define SLOW                0

void handleState();
int isMoving();
void lostMessage();
void sleep();

void SystemClock_Config(int sysSpeed);
static void MX_GPIO_Init(void);
static void MX_SPI3_Init(void);

#if !defined(__SOFT_FP__) && defined(__ARM_FP)
  #warning "FPU is not initialized, but the project is compiling for an FPU. Please initialize the FPU before use."
#endif

enum state{
	FOUND,
	LOST
};
enum state currentState = FOUND;

volatile uint8_t secondsLost = 0;
volatile uint8_t sendMessage = 0;
volatile uint8_t nonDiscoverable = 0;
uint8_t convSeconds = 0;

/* XL Axis Data*/
int16_t x = 0;
int16_t y = 0;
int16_t z = 0;
int16_t x_prev = 0;
int16_t y_prev = 0;
int16_t z_prev = 0;
int16_t x_diff;
int16_t y_diff;
int16_t z_diff;

int dataAvailable = 0;

SPI_HandleTypeDef hspi3;

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config(FAST);

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI3_Init();

  //RESET BLE MODULE
  HAL_GPIO_WritePin(BLE_RESET_GPIO_Port,BLE_RESET_Pin,GPIO_PIN_RESET);
  HAL_Delay(10);
  HAL_GPIO_WritePin(BLE_RESET_GPIO_Port,BLE_RESET_Pin,GPIO_PIN_SET);

  ble_init();
  setDiscoverability(NONDISCOVERABLE);

  leds_init();
//  timer_init(TIM2);
//  timer_set_ms(TIM2, 1000);
  lptimer_init(LPTIM1);
  lptimer_set_ms(LPTIM1, 1000);
  i2c_init();
  lsm6dsl_init();
  SystemClock_Config(SLOW);

  HAL_Delay(10);

  while (1)
  {

	  if(!nonDiscoverable && HAL_GPIO_ReadPin(BLE_INT_GPIO_Port,BLE_INT_Pin)){
	  	catchBLE();
	  }

	  x_prev = x;
	  y_prev = y;
	  z_prev = z;

	  SystemClock_Config(FAST);
	  lsm6dsl_read_xyz(&x, &y, &z);
	  SystemClock_Config(SLOW);
	  handleState();

//	  sleep();
  }
}

void sleep() {
	// Testing out sleep mode

	// Setting LPR bit for Low-power sleep mode (page 166 on reference manual)
	PWR->CR1 |= PWR_CR1_LPR;

	HAL_SuspendTick();
	__ASM volatile("wfi");
	HAL_ResumeTick();

	// Changing RRS to keep SRAM
//	PWR->CR3 |= PWR_CR3_RRS_Pos;
}

/**
  * @brief System Clock Configuration
  * @attention This changes the System clock frequency, make sure you reflect that change in your timer
  * @retval None
  */
void SystemClock_Config(int sysSpeed)
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
  // This lines changes system clock frequency
  if(sysSpeed == FAST) {
	  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_7;
  } else {
	  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_0;
  }

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
  * @brief SPI3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI3_Init(void)
{

  /* USER CODE BEGIN SPI3_Init 0 */

  /* USER CODE END SPI3_Init 0 */

  /* USER CODE BEGIN SPI3_Init 1 */

  /* USER CODE END SPI3_Init 1 */
  /* SPI3 parameter configuration*/
  hspi3.Instance = SPI3;
  hspi3.Init.Mode = SPI_MODE_MASTER;
  hspi3.Init.Direction = SPI_DIRECTION_2LINES;
  hspi3.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi3.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi3.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi3.Init.NSS = SPI_NSS_SOFT;
  hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi3.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi3.Init.CRCPolynomial = 7;
  hspi3.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi3.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI3_Init 2 */

  /* USER CODE END SPI3_Init 2 */

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
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIO_LED1_GPIO_Port, GPIO_LED1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(BLE_CS_GPIO_Port, BLE_CS_Pin, GPIO_PIN_SET);


  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(BLE_RESET_GPIO_Port, BLE_RESET_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : BLE_INT_Pin */
  GPIO_InitStruct.Pin = BLE_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BLE_INT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : GPIO_LED1_Pin BLE_RESET_Pin */
  GPIO_InitStruct.Pin = GPIO_LED1_Pin|BLE_RESET_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : BLE_CS_Pin */
  GPIO_InitStruct.Pin = BLE_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(BLE_CS_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

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

void TIM2_IRQHandler(void)
{
	LPTIM1->ISR &= ~TIM_SR_UIF;

	secondsLost++;

	/*
	 * Sends the Bluetooth message after 10 seconds when lost
	 * */
	if (secondsLost >= MINUTE && secondsLost % SEND_BLE == 0){
		sendMessage = 1;
	}
}

void LPTIM1_IRQHandler(void)
{
	if (LPTIM1->ISR & LPTIM_ISR_ARRM) {
		LPTIM1->ICR |= LPTIM_ICR_ARRMCF;

		secondsLost++;

		/*
		 * Sends the Bluetooth message after 10 seconds when lost
		 * */
		if (secondsLost >= MINUTE && secondsLost % SEND_BLE == 0){
			sendMessage = 1;
		}
	}

}

// Redefine the libc _write() function so you can use printf in your code
int _write(int file, char *ptr, int len) {
    int i = 0;
    for (i = 0; i < len; i++) {
        ITM_SendChar(*ptr++);
    }
    return len;
}

void handleState() {
	switch(currentState) {
		case FOUND:
			/* If the minutes lost is greater than 0 and the XL is not moving, go to lost state*/
			if(isMoving()) {
				secondsLost = 0;
			}
			/* Move to LOST state*/
			else if (secondsLost >= MINUTE && !isMoving()) {
				currentState = LOST;

				SystemClock_Config(FAST);
				setDiscoverability(DISCOVERABLE);
				SystemClock_Config(SLOW);


				leds_set(3);
			}
			break;

		case LOST:

			if(sendMessage){
				SystemClock_Config(FAST);
				HAL_Delay(1000);

				unsigned char text[20] = "Jingles Lost: ";

				convSeconds = secondsLost - MINUTE;
				int i = 14;

				if (convSeconds >= 100) {
					text[i++] = (convSeconds / 100) % 10 + '0';
				}

				if (convSeconds >= 10) {
					text[i++] = (convSeconds / 10) % 10 + '0';
				}
				text[i++] = '0';

				text[i] = '\0';


				unsigned char message[20];  // Be careful changing this number!

				memcpy(message, text, sizeof(text));
				updateCharValue(NORDIC_UART_SERVICE_HANDLE, READ_CHAR_HANDLE, 0, sizeof(message) - 1, message);
				sendMessage = 0;
				SystemClock_Config(SLOW);
			}
			/* Move to Found State*/
			if (isMoving()) {
				secondsLost = 0;
				sendMessage = 0;
				currentState = FOUND;

				SystemClock_Config(FAST);
				setDiscoverability(NONDISCOVERABLE);
				disconnectBLE();
				SystemClock_Config(SLOW);

				leds_set(0);
			}
			break;
	}
}

int isMoving() {
	x_diff = abs(x - x_prev);
	y_diff = abs(y - y_prev);
	z_diff = abs(z - z_prev);

	return ((x_diff > XL_DEAD_BAND) || (y_diff > XL_DEAD_BAND) || (z_diff > XL_DEAD_BAND));
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
