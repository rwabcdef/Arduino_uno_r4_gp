#ifndef TIMER0_H
#define TIMER0_H

#include<stdint.h>
#include "env.h"

#ifdef __cplusplus
 extern "C" {
#endif

// UNO R4: the 1ms tick comes from the Arduino core's millis(), so no
// hardware timer is set up here.
void timer0_init();

// Returns the 1ms tick count (wraps at 16 bits).
uint16_t timer0_getTick();

#ifdef __cplusplus
}
#endif

#endif
