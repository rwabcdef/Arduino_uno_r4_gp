/*
 * swTimer.h
 *
 *  Created on: 10 Nov 2016
 *      Author: rob
 */

#ifndef SWTIMER_H_
#define SWTIMER_H_

#include<stdbool.h>
#include "timer0.h"


#define BASE_TICK timer0_getTick()
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

static inline void swTimer_tickReset(uint16_t* tickCount)
{
  *tickCount = BASE_TICK;
}
//------------------------------------------------------------------------------

static inline uint32_t swTimer_tickElapsed(uint16_t startTick)
{
  // cast keeps 16 bit wrap-around arithmetic (int is 32 bits on the R4)
  return (uint16_t)(BASE_TICK - startTick);
}

//------------------------------------------------------------------------------

static inline bool swTimer_tickCheckTimeout(uint16_t* tickCount, uint16_t timeoutTicks)
{
  if(swTimer_tickElapsed(*tickCount) >= timeoutTicks)
  {
    swTimer_tickReset(tickCount);
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------

static inline bool swTimer_tickCheckTimeoutNoReset(uint16_t* tickCount, uint16_t timeoutTicks)
{
  if(swTimer_tickElapsed(*tickCount) >= timeoutTicks)
  {
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------


#endif /* SWTIMER_H_ */
