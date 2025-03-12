/*
 * i2c.c
 *
 *  Created on: Feb 4, 2025
 *      Author: jingles
 */

#include "i2c.h"
#include <stm32l475xx.h>


void i2c_init()
{
	/* Disabling Peripheral*/
	I2C2->CR1 &= ~I2C_CR1_PE;

	/* Setting up I2C to run using 4MHz System clock */
	RCC->CCIPR &= ~RCC_CCIPR_I2C2SEL;
	RCC->CCIPR |= RCC_CCIPR_I2C2SEL_0;

	/* Enabling clock for I2C peripheral */
	RCC->APB1ENR1 |= RCC_APB1ENR1_I2C2EN;

	/* Enabling clock for GPIOB peripheral */
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;

	/* Setting GPIOB to alternate function mode for I2C lines*/
	GPIOB->MODER &= ~(GPIO_MODER_MODE10 | GPIO_MODER_MODE11);
	GPIOB->MODER |= (GPIO_MODER_MODE10_1 | GPIO_MODER_MODE11_1);


	/* Selecting pull down mode*/
	GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPD10 | GPIO_PUPDR_PUPD11);
	GPIOB->PUPDR |= (GPIO_PUPDR_PUPD10_1 | GPIO_PUPDR_PUPD11_1);

	/* Selecting open drain*/
	GPIOB->OTYPER |= (GPIO_OTYPER_OT10 | GPIO_OTYPER_OT11);

	/* Set to medium speed*/
	GPIOB->OSPEEDR &= ~((GPIO_OSPEEDR_OSPEED10_Pos) | (GPIO_OSPEEDR_OSPEED11_Pos));
	GPIOB->OSPEEDR |= ((0x1 << GPIO_OSPEEDR_OSPEED10_Pos) | (0x1 << GPIO_OSPEEDR_OSPEED11_Pos));

	/* Setting GPIO to alternate function 4*/
	GPIOB->AFR[1] &= ~(GPIO_AFRH_AFSEL10 | GPIO_AFRH_AFSEL11);
	GPIOB->AFR[1] |= (GPIO_AFRH_AFSEL10_2 | GPIO_AFRH_AFSEL11_2);

	/* Set I2C rate: 10Khz */
	I2C2->TIMINGR &= ~(I2C_TIMINGR_PRESC | I2C_TIMINGR_SCLL | I2C_TIMINGR_SCLH |
					I2C_TIMINGR_SDADEL | I2C_TIMINGR_SCLDEL);
	I2C2->TIMINGR |= ((0xC7 << I2C_TIMINGR_SCLL_Pos) | (0xC3 << I2C_TIMINGR_SCLH_Pos) |
					(0x02 << I2C_TIMINGR_SDADEL_Pos) | (0x04 << I2C_TIMINGR_SCLDEL_Pos));

	/* Enabling I2C */
	I2C2->CR1 |= I2C_CR1_PE;

}

uint8_t i2c_transaction(uint8_t address, uint8_t dir, uint8_t* data, uint8_t len)
{

	while(I2C2->ISR & I2C_ISR_BUSY);

	/* Set up primary for a transfer*/
	I2C2->CR2 &= ~(I2C_CR2_SADD | I2C_CR2_RD_WRN | I2C_CR2_ADD10 | I2C_CR2_NBYTES);
	I2C2->CR2 |= ((address << 1) | (len << I2C_CR2_NBYTES_Pos));

	if(!dir) {/*Write data*/

		/* Sending Start bit*/
		I2C2->CR2 |= I2C_CR2_START;

		/* Checking for NACKs or if Transmission Register is ready*/
		while(1) {
			if(I2C2->ISR & I2C_ISR_NACKF){
				return 0;
			}
			if(I2C2->ISR & I2C_ISR_TXIS) {
				break;
			}
		}

		for(int i = 0; i < len; i++) {
			/* Checking for NACKs or if Transmission Register is ready*/
			while(1) {
				if(I2C2->ISR & I2C_ISR_NACKF){
					return 0;
				}
				if(I2C2->ISR & I2C_ISR_TXIS){
					break;
				}
			}
			I2C2->TXDR = data[i];
		}
		while(!(I2C2->ISR & I2C_ISR_TC));

	} else { /*Read data*/

		/* Setting to Read mode and starting transaction*/
		I2C2->CR2 |= (I2C_CR2_RD_WRN | I2C_CR2_START);

		/* Checking for NACKs and if Read Register is ready*/
		while(1) {
			if(I2C2->ISR & I2C_ISR_NACKF){
				return 0;
			}

			if(I2C2->ISR & I2C_ISR_RXNE) {
				break;
			}
		}

		for(int i = 0; i < len; i++) {

			/* Checking for NACKs and if Read Register is ready*/
			while(1) {
				if(I2C2->ISR & I2C_ISR_NACKF){
					return 0;
				}

				if(I2C2->ISR & I2C_ISR_RXNE) {
					break;
				}
			}
			data[i] = I2C2->RXDR;
		}
		while(!(I2C2->ISR & I2C_ISR_TC));
	}


	I2C2->CR2 |= I2C_CR2_STOP;


	return len;

}


