/*
 * lptimer.h
 *
 *  Created on: Mar 10, 2025
 *      Author: mremi
 */

#ifndef INC_LPTIMER_H_
#define INC_LPTIMER_H_

#include <stm32l475xx.h>

void lptimer_init(LPTIM_TypeDef* timer);
void lptimer_reset(LPTIM_TypeDef* timer);
void lptimer_set_ms(LPTIM_TypeDef* timer, uint16_t period);

#endif /* INC_LPTIMER_H_ */
