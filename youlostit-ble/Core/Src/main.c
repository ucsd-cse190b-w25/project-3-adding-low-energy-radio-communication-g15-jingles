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
//#include "leds.h"
//#include "timer.h"
#include "lptimer.h"
#include "i2c.h"
#include "lsm6dsl.h"
#include "ble.h"
#include <stdlib.h>

#define SEND_BLE	        10
#define XL_DEAD_BAND        5000
#define MINUTE				60

#define DISCOVERABLE        1
#define NONDISCOVERABLE     0

void handleState();
int isMoving();
void lostMessage();
void sleep();
void turnOffPeriph();
void toggleClkSpeed();
void disableUnnecessaryInterrupts(void);
void disableAllEXTI(void);
void ClearPendingInterrupts(void);

void SystemClock_Config(void);
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
uint8_t moving = 0;
uint32_t EXTI_IMR1 = 0;
uint32_t EXTI_IMR2 = 0;


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
  SystemClock_Config();

  turnOffPeriph();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI3_Init();

  //RESET BLE MODULE
  HAL_GPIO_WritePin(BLE_RESET_GPIO_Port,BLE_RESET_Pin,GPIO_PIN_RESET);
  HAL_Delay(10);
  HAL_GPIO_WritePin(BLE_RESET_GPIO_Port,BLE_RESET_Pin,GPIO_PIN_SET);

  ble_init();

//  leds_init();
  lptimer_init(LPTIM1);
  lptimer_set_ms(LPTIM1, 2500);
  i2c_init();
  lsm6dsl_init();
  setDiscoverability(NONDISCOVERABLE);
  disableI2C();
  // disableUnnecessaryInterrupts();
//  DisableAllEXTI();
  toggleClkSpeed();
  HAL_Delay(10);


  while (1)
  {
	  // Wait for interrupt, only uncomment if low power is needed
	  //__WFI();

	  x_prev = x;
	  y_prev = y;
	  z_prev = z;
	  toggleClkSpeed();
	  if(!nonDiscoverable && HAL_GPIO_ReadPin(BLE_INT_GPIO_Port,BLE_INT_Pin)){
	  	catchBLE();
	  }
	  enableI2C();
	  lsm6dsl_read_xyz(&x, &y, &z);
	  disableI2C();
	  toggleClkSpeed();
	  moving = isMoving();

	  handleState();
	  sleep();
  }
}

void disableUnnecessaryInterrupts(void) {
    for (int irq = WWDG_IRQn; irq <= RNG_IRQn; irq++) {
        if (irq != LPTIM1_IRQn && irq != EXTI9_5_IRQn && irq != SPI3_IRQn) {
            NVIC_DisableIRQ((IRQn_Type)irq);
        }
    }
}

void disableAllEXTI(void) {
    EXTI->IMR1 = 0x00000000;  // Mask EXTI lines 0-31
    EXTI->IMR2 = 0x00000000;  // Mask EXTI lines 32+
}

void enableAllEXTI(void) {
    EXTI->IMR1 = EXTI_IMR1;  // Mask EXTI lines 0-31
    EXTI->IMR2 = EXTI_IMR2;  // Mask EXTI lines 32+
}


void toggleClkSpeed() {

	// if MIS Range is not 100kH (0000), then change to 0, else change to 8Mhz
	if(RCC->CR & RCC_CR_MSIRANGE) {
		RCC->CR &= ~RCC_CR_MSIRANGE;
	}
		else {
		RCC->CR |= RCC_CR_MSIRANGE_7;
	}
}

void ClearPendingInterrupts(void) {
    for (int irq = WWDG_IRQn; irq <= RNG_IRQn; irq++) {
        if (irq != LPTIM1_IRQn) {
            NVIC_ClearPendingIRQ((IRQn_Type)irq);
        }
    }
}

void turnOffPeriph() {
	// Disabling PPL to save power
	RCC->PLLCFGR &= ~RCC_PLLCFGR_PLLPEN;
	RCC->PLLSAI1CFGR &= ~RCC_PLLSAI1CFGR_PLLSAI1PEN;
	RCC->PLLSAI2CFGR &= ~RCC_PLLSAI2CFGR_PLLSAI2PEN;

	// Might need to turn back on
	RCC->AHB1ENR &= ~RCC_AHB1ENR_CRCEN;
	RCC->AHB1SMENR &= ~RCC_AHB1SMENR_CRCSMEN;


	RCC->AHB1SMENR &= ~(RCC_AHB1SMENR_DMA1SMEN | RCC_AHB1SMENR_DMA2SMEN | RCC_AHB1SMENR_TSCSMEN);
	RCC->AHB2SMENR &= ~(RCC_AHB2SMENR_GPIOFSMEN | RCC_AHB2SMENR_GPIOGSMEN | RCC_AHB2SMENR_GPIOHSMEN
						| RCC_AHB2SMENR_ADCSMEN);
	RCC->AHB3SMENR &= ~(RCC_AHB3SMENR_QSPISMEN | RCC_AHB3SMENR_FMCSMEN);
	RCC->APB1SMENR1 = RCC_APB1SMENR1_LPTIM1SMEN;
	RCC->APB1SMENR2 = 0;
	RCC->APB2SMENR = RCC_APB2SMENR_SYSCFGSMEN;

}

void sleep() {
	EXTI_IMR1 = EXTI->IMR1;
	EXTI_IMR2 = EXTI->IMR2;

//	disableAllEXTI();
	HAL_SuspendTick();
	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
	HAL_PWREx_EnterSTOP2Mode(PWR_SLEEPENTRY_WFI);
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
	HAL_ResumeTick();
//	enableAllEXTI();

}

/**
  * @brief System Clock Configuration
  * @attention This changes the System clock frequency, make sure you reflect that change in your timer
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
  // This lines changes system clock frequency
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_7;
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

void LPTIM1_IRQHandler(void)
{
//	leds_toggle();
	if (LPTIM1->ISR & LPTIM_ISR_ARRM) {
		LPTIM1->ICR |= LPTIM_ICR_ARRMCF;
		// while((LPTIM1->ISR & LPTIM_ISR_ARROK) == 0);

		secondsLost += 10;

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
			if(moving) {
				secondsLost = 0;
			}
			/* Move to LOST state*/
			else if (secondsLost >= MINUTE && !moving) {
				currentState = LOST;

				toggleClkSpeed();
				setDiscoverability(DISCOVERABLE);
				toggleClkSpeed();
//				leds_set(3);
			}
			break;

		case LOST:

			if(sendMessage){
				toggleClkSpeed();
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
				toggleClkSpeed();
			}
			/* Move to Found State*/
			if (moving) {
				secondsLost = 0;
				sendMessage = 0;
				currentState = FOUND;

				toggleClkSpeed();
				setDiscoverability(NONDISCOVERABLE);
				disconnectBLE();
				standbyBle();
				toggleClkSpeed();

//				leds_set(0);
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

void lostMessage() {

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
