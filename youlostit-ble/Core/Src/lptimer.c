/*
 * lptimer.c
 *
 *  Created on: Mar 10, 2025
 *      Author: mremi
 */


#include "lptimer.h"


void lptimer_init(LPTIM_TypeDef* timer)
{

	RCC->CIER |= RCC_CIER_LSIRDYIE;

	// Configuring timer to use LSI clock and waiting for it to be stable
	RCC->CSR |= RCC_CSR_LSION;
	while (!(RCC->CSR & RCC_CSR_LSIRDY));

	// Selects LSI as LPTIM1 clock source
	RCC->CCIPR &= ~RCC_CCIPR_LPTIM1SEL;
	RCC->CCIPR |= RCC_CCIPR_LPTIM1SEL_0;

	// Enables lptimer 1 to use the LSI clock
	RCC->APB1ENR1 |= RCC_APB1ENR1_LPTIM1EN;

	// Enables lptimer to be used in sleep mode
	RCC->APB1SMENR1 |= RCC_APB1SMENR1_LPTIM1SMEN;

	// Disabling timer count register
	timer->CR &= ~LPTIM_CR_ENABLE;
	while ((timer->CR & LPTIM_CR_ENABLE));

	// Enabling internal clock selection
	timer->CFGR &= ~LPTIM_CFGR_CKSEL;
	timer->CFGR &= ~LPTIM_CFGR_COUNTMODE;

	// Resetting timer count
	timer->CNT = 0;

	// Setting prescalar to 32,
	timer->CFGR &= ~LPTIM_CFGR_PRESC;
	timer->CFGR |= (LPTIM_CFGR_PRESC_0 | LPTIM_CFGR_PRESC_2);
	timer->CFGR &= ~LPTIM_CFGR_TRIGEN;

	timer->ICR = LPTIM_ICR_ARRMCF | LPTIM_ICR_CMPMCF | LPTIM_ICR_EXTTRIGCF |
	                LPTIM_ICR_ARRMCF | LPTIM_ICR_CMPMCF;

	// Enabling interrupt update bit
	timer->IER |= LPTIM_IER_ARRMIE;

	// Enabling interrupt in the NVIC
	NVIC_EnableIRQ(LPTIM1_IRQn);
	// Setting the priority for LPTIM1
	NVIC_SetPriority(LPTIM1_IRQn, 0);

	timer->CR |= LPTIM_CR_ENABLE;
	while (!(timer->CR & LPTIM_CR_ENABLE));

	timer->CR |= LPTIM_CR_CNTSTRT;

}

void lptimer_reset(LPTIM_TypeDef* timer)
{

	timer->CNT = 0;

}

void lptimer_set_ms(LPTIM_TypeDef* timer, uint16_t period_ms)
{
	timer->ARR = period_ms - 1;
//	while((LPTIM1->ISR & LPTIM_ISR_ARROK) == 0);

}

