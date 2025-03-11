/*
 * lptimer.c
 *
 *  Created on: Mar 10, 2025
 *      Author: mremi
 */


#include "timer.h"


void lptimer_init(LPTIM_TypeDef* timer)
{

	// Configuring clock to use default LSI clock
	RCC->CSR |= RCC_CSR_LSION;
	while ((RCC->CSR & RCC_CSR_LSIRDY) == 0);

	// Enables lptimer 1 to use the LSI clock
	RCC->APB1ENR1 |= RCC_APB1ENR1_LPTIM1EN;

	// Disabling timer count register
	timer->CR &= ~LPTIM_CR_ENABLE;

	// Resetting timer count
	timer->CNT = 0;

	// Clearing timer state
	timer->ICR |= LPTIM_ICR_ARRMCF;

	// Setting prescalar to 32,
	timer->CFGR &= ~LPTIM_CFGR_PRESC;
	timer->CFGR |= (LPTIM_CFGR_PRESC_0 | LPTIM_CFGR_PRESC_2);

	// Enabling interrupt update bit
	timer->IER |= LPTIM_IER_ARRMIE;

	// Enabling interrupt in the NVIC
	NVIC_EnableIRQ(LPTIM1_IRQn);
	// Setting the priority for TIM2
	NVIC_SetPriority(LPTIM1_IRQn, 0);

	// Enabling timer count register
	timer->CR |= LPTIM_CR_ENABLE;


}

void lptimer_reset(LPTIM_TypeDef* timer)
{

	timer->CNT = 0;

}

void lptimer_set_ms(LPTIM_TypeDef* timer, uint16_t period_ms)
{
	timer->ARR = period_ms - 1;

}

