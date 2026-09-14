/*
 * Reader.hpp
 *
 *  Created on: 22 Apr 2023
 *      Author: rw123
 */

#ifndef READER_HPP_
#define READER_HPP_

#include "Reader_config.hpp"
#include "StateMachine.hpp"
#include "Writer.hpp"
#include "Frame.hpp"
#include "DebugUser.hpp"
//#include "Transport.hpp"

#if defined(ENV_CONFIG__SYSTEM_PC)
#include "DebugPrint.hpp"
#endif

namespace SerLink
{
typedef bool (*readHandler)(Frame &rxFrame, uint16_t* dataLen, char* data);
//typedef void (*readHandler)(const TransportData* pRxData, TransportData* pAckData);
/*
typedef void (*reader_uart_init)(char* pRxBuffer, uint8_t rxBufferLen);
typedef bool (*reader_uart_checkFrameRx)();
typedef uint8_t (*reader_uart_getRxLenAndReset)();

class ReaderAdapter
{
public:
	reader_uart_init init;
	reader_uart_checkFrameRx checkFrameRx;
	reader_uart_getRxLenAndReset getRxLenAndReset;
};
*/
class Reader : public StateMachine, public DebugUser
{
private:
  class HandlerRegistration{
  public:
    char protocol[Frame::LEN_PROTOCOL];
    readHandler handler;
  };

	//const uint8_t IDLE = 0;
	//const uint8_t ACKDELAY = 1;
	//const uint8_t TXACKWAIT = 2;
	uint8_t id;
	bool rxFlag;
	Writer* writer;
	char* rxBuffer;
	char* ackBuffer;
	uint8_t bufferLen;
	uint16_t startTick;
	//DebugPrint* debugPrinter;
	//char rxBuffer[UART_BUFF_LEN];
	//char ackBuffer[UART_BUFF_LEN];
#if defined(ENV_CONFIG__SYSTEM_PC)
	char s[64];
#endif
	Frame* rxFrame;
	Frame* ackFrame;
	HandlerRegistration handlerRegistrations[READER_CONFIG__MAX_NUM_INSTANT_HANDLERS];
	uint8_t numInstantHandlers;

	uint8_t idle();
	uint8_t ackDelay();
	uint8_t txAckWait();
  uint8_t rxDelay();

	//-------------------------------------
	// Uart Interface

	// Checks uart layer (below) to see if a frame has been received.
	bool checkUartFrameRx();
	// Gets received frame length (and resets rx flag) from uart layer.
	uint8_t getUartRxLenAndReset();
	// write buffer to uart layer.
	uint8_t uartWrite(char* buffer);
  // returns whether or not uart tx is busy.
	bool getUartTxBusy();
	//-------------------------------------



	readHandler getInstantHandler(char* protocol);

public:
	Reader(uint8_t id, char* rxBuffer, char* ackBuffer, uint8_t bufferLen,
	    Frame* rxFrame, Frame* ackFrame, Writer* writer = nullptr); // , DebugPrint* debugPrint = nullptr
  void init();
	void run();
	bool registerInstantCallback(char* protocol, readHandler handler);
	bool getRxFrame(Frame* rxFrame);
  bool getRxFrameProtocol(Frame* rxFrame, char* protocol);
  void clearRxFlag();

	uint8_t getCurrentState();
	char* getCurrentStateName();
};

}

#endif /* READER_HPP_ */
