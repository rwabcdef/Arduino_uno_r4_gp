/*
 * Writer.hpp
 *
 *  Created on: 12 Jul 2023
 *      Author: rw123
 */

#ifndef WRITER_WRITER_HPP_
#define WRITER_WRITER_HPP_

#include <stdint.h>
#include "Writer_config.hpp"
#include "StateMachine.hpp"
#include "Frame.hpp"
#include "DebugUser.hpp"
//#include "Transport.hpp"
#if defined(ENV_CONFIG__SYSTEM_PC)
#include "DebugPrint.hpp"

#endif

namespace SerLink
{
class Writer : public StateMachine, public DebugUser
{
private:
  uint8_t id;
  uint16_t startTick;
  bool txFlag;
  volatile bool ackRxFlag;
  //char txBuffer[UART_BUFF_LEN];
  char* txBuffer;
#if defined(ENV_CONFIG__SYSTEM_PC)
  char s[64];
#endif
  uint8_t status;
  uint8_t bufferLen;
  Frame* txFrame;
  Frame* ackRxFrame;

  uint8_t flag;

  uint8_t idle();
  uint8_t txWait();
  uint8_t rxAckWait();

public:

  static const uint8_t STATUS_IDLE = 20;
  static const uint8_t STATUS_BUSY = 21;
  static const uint8_t STATUS_OK = 50;
  static const uint8_t STATUS_TIMEOUT = 51;
  static const uint8_t STATUS_PROTOCOL_ERROR = 52;

  Writer(uint8_t id, char* txBuffer, uint8_t bufferLen, Frame* txFrame, Frame* ackRxFrame);
  void run();

  // Used to send frame
  uint8_t sendFrame(Frame* frame);

  uint8_t getStatus();

  // Returns send status with respect to the specified protocol
  uint8_t getStatusProtocol(char* protocol);

  void getStatusStr(char* str);

  // Called by a Reader to pass an ack frame to the Writer.
  void setAckFrame(Frame* frame);

  uint8_t uartWrite(char* buffer);
  bool getUartTxBusy();
};

} // end namespace SerLink

#endif /* WRITER_WRITER_HPP_ */
