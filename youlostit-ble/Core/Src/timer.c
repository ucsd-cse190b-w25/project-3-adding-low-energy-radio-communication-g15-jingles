/*
 * timer.c
 *
 *  Created on: Oct 5, 2023
 *      Author: schulman
 */

#include "timer.h"


void timer_init(TIM_TypeDef* timer)
{

	// Configuring clock to use default MSI clock
	RCC->CFGR &= RCC_CFGR_SW_MSI;

	// Enables timer 2 to use the MSI clock
	RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;

	// Disabling timer count register
	timer->CR1 &= ~TIM_CR1_CEN;

	// Resetting timer count
	timer->CNT = 0;

	// Clearing timer state
	timer->SR &= ~TIM_SR_UIF;

	// Setting max reload time. (Division factor is PSC + 1)
	timer->PSC = 7999;

	// Enabling interrupt update bit
	timer->DIER |= TIM_DIER_UIE;

	// Enabling interrupt in the NVIC
	NVIC_EnableIRQ(TIM2_IRQn);
	// Setting the priority for TIM2
	NVIC_SetPriority(TIM2_IRQn, 0);

	// Enabling timer count register
	timer->CR1 |= TIM_CR1_CEN;


}

void timer_reset(TIM_TypeDef* timer)
{

	timer->CNT = 0;

}

void timer_set_ms(TIM_TypeDef* timer, uint16_t period_ms)
{
	timer->ARR = period_ms - 1;

}

