#ifndef UART_H
#define UART_H

// Interrupt driven, newline ('\n') framed uart for the Arduino UNO R4 Minima
// (Renesas RA4M1). The port is selected with UART_PORT below:
//
//   UART_PORT_USB   - usb-uart (the programming usb cable, native USB CDC).
//                     Baud rate is set by the PC and ignored here. The PC
//                     terminal must assert DTR (the Arduino serial monitor
//                     does). This module consumes all USB serial input, so
//                     do not also read from Serial.
//
//   UART_PORT_D0_D1 - SCI2 on pins D0 (RX) / D1 (TX), UART_BAUD_RATE 8N1.
//                     Do not use Serial1 alongside it.
//                     Requires uart_irq.cpp (glue to the core's IRQManager).

#include<stdbool.h>
#include<stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UART_PORT_USB 1
#define UART_PORT_D0_D1 2

#ifndef UART_PORT
#define UART_PORT UART_PORT_USB
#endif

#define UART_BUFF_LEN 22
#define UART_BAUD_RATE 19200 // UART_PORT_D0_D1 only

//--------------------------------------------------------------------------------
// Returns false if the uart could not be opened.
bool uart_init(char* pRxBuffer, uint8_t rxBufferLen);
//--------------------------------------------------------------------------------
bool uart_checkFrameRx();
//--------------------------------------------------------------------------------
uint8_t uart_getRxLenAndReset();
//--------------------------------------------------------------------------------
#define UART_STATUS_OK 1
#define UART_STATUS_BUSY 2
#define UART_STATUS_ERROR 3 // UART_PORT_USB: also returned if no terminal is connected

// Writes buffer up to and including the first '\n' (max UART_BUFF_LEN chars).
// If no '\n' is found the frame is truncated and terminated with '\n'.
uint8_t uart_write(const char* buffer);
//--------------------------------------------------------------------------------
bool uart_getTxBusy();
//--------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif
