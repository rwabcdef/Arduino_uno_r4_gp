/*
 * gpio.h
 *
 *  Created on: 6 Jul 2024
 *      Author: rw123
 */

#ifndef HW_GPIO_H_
#define HW_GPIO_H_

#include<stdint.h>
#include<stdbool.h>
#include "env.h"

#define GPIO_REG__PORTB 1
#define GPIO_REG__PORTC 2
#define GPIO_REG__PORTD 3

#define GPIO_PIN_DIRECTION__IN 1
#define GPIO_PIN_DIRECTION__OUT 2


#ifdef __cplusplus
extern "C" {
#endif


//--------------------------------------------------------------------------------
void gpio_setPinDirection(uint8_t port, uint8_t pin, uint8_t direction);
//--------------------------------------------------------------------------------
void gpio_setPinHigh(uint8_t port, uint8_t pin);
//--------------------------------------------------------------------------------
void gpio_setPinLow(uint8_t port, uint8_t pin);
//--------------------------------------------------------------------------------
bool gpio_getPinState(uint8_t port, uint8_t pin);
//--------------------------------------------------------------------------------
void gpio_setDebugOn(bool on);
//--------------------------------------------------------------------------------


#ifdef __cplusplus
}
#endif


#endif /* HW_GPIO_H_ */
