/*
 * UartInterface.hpp
 *
 * Function pointer table used by SerLink::Reader & SerLink::Writer to access
 * the (newline framed) uart layer below them. This decouples Reader & Writer
 * from any specific uart driver, e.g. for uart.h:
 *
 *   const SerLink::UartInterface uart0Interface = {
 *     uart_init,
 *     uart_checkFrameRx,
 *     uart_getRxLenAndReset,
 *     uart_write,
 *     uart_getTxBusy
 *   };
 *
 * The same interface object may be shared by a Reader and a Writer.
 */

#ifndef UART_INTERFACE_HPP_
#define UART_INTERFACE_HPP_

#include <stdint.h>

namespace SerLink
{

struct UartInterface
{
  // Initialises the uart. Returns false if the uart could not be opened.
  bool (*init)(char* pRxBuffer, uint8_t rxBufferLen);
  // Returns true if a frame has been received.
  bool (*checkFrameRx)();
  // Gets received frame length (and resets rx flag).
  uint8_t (*getRxLenAndReset)();
  // Writes a '\n' terminated frame.
  uint8_t (*write)(const char* buffer);
  // Returns whether or not uart tx is busy.
  bool (*getTxBusy)();
};

} // end namespace SerLink

#endif /* UART_INTERFACE_HPP_ */
