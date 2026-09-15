// Glue so uart.c can allocate its SCI interrupts through the core's C++
// IRQManager (which owns the RA4M1 interrupt vector slots).

#include "Arduino.h"
#include "IRQManager.h"

extern "C" bool uart_irq_setup(uart_cfg_t* p_cfg)
{
  return IRQManager::getInstance().addPeripheral(IRQ_SCI_UART, p_cfg);
}
