/*
 * Reader.cpp
 *
 *  Created on: 22 Apr 2023
 *      Author: rw123
 */

#include "Reader.hpp"
#if defined(ENV_CONFIG__SYSTEM_PC)
#include "ATmega328.hpp"
#endif
#if defined(ENV_CONFIG__SYSTEM_ARDUINO_UNO)
#include <avr/io.h>
#include "wiring_private.h"
#endif
#include "swTimer.h"
#include <string.h>
#include <stdio.h>

#define IDLE 0
#define ACKDELAY 1
#define TXACKWAIT 2
#define RXDELAY 3

using namespace SerLink;


Reader::Reader(uint8_t id, LinkInterface* link, char* rxBuffer, char* ackBuffer, uint8_t bufferLen,
    Frame* rxFrame, Frame* ackFrame, Writer* writer) : id(id), link(link), writer(writer), DebugUser()
{
	//this->id = id;
	this->rxFlag = false;
	this->rxBuffer = rxBuffer;
	this->ackBuffer = ackBuffer;
  this->bufferLen = bufferLen;
	this->rxFrame = rxFrame;
	this->ackFrame = ackFrame;
	this->currentState = IDLE;
  #if defined(ENV_CONFIG__SYSTEM_PC)
	this->debugLevel = DebugPrint_defs::UartRx;
  #endif

	for(uint8_t i=0; i<READER_CONFIG__MAX_NUM_INSTANT_HANDLERS; i++)
	{
	  this->handlerRegistrations[i].handler = nullptr;
	  memset(this->handlerRegistrations[i].protocol, 0, Frame::LEN_PROTOCOL);
	}
	this->numInstantHandlers = 0;
}

bool Reader::init()
{
  if(this->link != nullptr)
  {
    // Initialise link hardware & driver layer
    return this->link->init((char*) this->rxBuffer, this->bufferLen);
  }
  return false;
}

void Reader::run()
{
	switch(this->currentState)
	{
		case IDLE: { this->currentState = this->idle(); break;}
		case ACKDELAY: { this->currentState = this->ackDelay(); break;}
		case TXACKWAIT: { this->currentState = this->txAckWait(); break;}
		case RXDELAY: { this->currentState = this->rxDelay(); break;}
	}
}

char* Reader::getCurrentStateName()
{
  return (char*)"N/A";
}

bool Reader::registerInstantCallback(char* protocol, readHandler handler)
{
  if(this->numInstantHandlers < (READER_CONFIG__MAX_NUM_INSTANT_HANDLERS - 1))
  {
    strncpy(this->handlerRegistrations[this->numInstantHandlers].protocol, protocol, Frame::LEN_PROTOCOL);
    this->handlerRegistrations[this->numInstantHandlers].handler = handler;
    this->numInstantHandlers++;
    return true;
  }
  else
  {
    // No more registrations are available
    return false;
  }
}

readHandler Reader::getInstantHandler(char* protocol)
{
  for(uint8_t i=0; i<READER_CONFIG__MAX_NUM_INSTANT_HANDLERS; i++)
  {
    if(0 == strncmp(this->handlerRegistrations[i].protocol, protocol, Frame::LEN_PROTOCOL))
    {
      return this->handlerRegistrations[i].handler;
    }
  }
  return nullptr;
}

bool Reader::getRxFrame(Frame* rxFrame)
{
  if(this->rxFlag)
  {
    this->rxFlag = false;

    this->rxFrame->copy(rxFrame);

    return true;
  }
  else
  {
    return false;
  }
}

bool Reader::getRxFrameProtocol(Frame* rxFrame, char* protocol)
{
  if(this->rxFlag)
  {
    if(strncmp(this->rxFrame->protocol, protocol, SerLink::Frame::LEN_PROTOCOL) == 0)
    {
      this->rxFrame->copy(rxFrame);

      rxFrame->buffer[rxFrame->dataLen] = '\0';

      this->rxFlag = false;

      return true;
    }
    else
    {
      return false;
    }
  }
  else
  {
    return false;
  }
}

void Reader::clearRxFlag()
{
  this->rxFlag = false;
}
//-----------------------------------------------------------------
// Start of state methods

uint8_t Reader::idle()
{

	cli();
	if(this->checkUartFrameRx())
	{
		uint8_t rxLen = this->getUartRxLenAndReset();

		// Read frame from uart.
		Frame::fromString((char*) this->rxBuffer, this->rxFrame);

		//this->rxFlag = true;

		if(this->rxFrame->type == Frame::TYPE_TRANSMISSION)
		{

      // Check for: No ack special case (protocol = NOACK)
      if(0 == strncmp(this->rxFrame->protocol, "NOACK", Frame::LEN_PROTOCOL))
      {
        // The received packet is the special case (do not send an ack) - so do nothing
        swTimer_tickReset(&this->startTick);
        sei();
        return IDLE;
      }

			this->rxFrame->copy(this->ackFrame);
			this->ackFrame->type = Frame::TYPE_ACK;
			this->ackFrame->dataLen = Frame::ACK_OK;
			memset(this->ackFrame->buffer, 0, Frame::LEN_HEADER);

			readHandler instantHandler = this->getInstantHandler(this->rxFrame->protocol);
			if(instantHandler == nullptr)
			{
			  // No instant (i.e. piggyback) handler has been found - so do nothing.
			}
			else
			{
			  // An instant (i.e. piggyback) handler has been found for this protocol,
			  // so call it now.
			  // The instant handler callback sets the ackFrame's data & dataLen.
			  bool useReturn = instantHandler(*this->rxFrame, &this->ackFrame->dataLen, this->ackFrame->buffer);

			  if(!useReturn)
			  {
			    // do not use data length and data in ack frame that was set by the instantHandler
			    this->ackFrame->type = Frame::TYPE_ACK;
				  this->ackFrame->dataLen = Frame::ACK_OK;
			  }
			  else
			  {
			    // do nothing - use data length and data in ack frame that was set by the instantHandler
			  }
			}

			swTimer_tickReset(&this->startTick);

//			for(int i=0; i<1000000; i++){
//			  volatile int n = i * 7;
//			}

#if defined(ENV_CONFIG__SYSTEM_PC)
			if(this->debugOn)
			{
				sprintf(this->s, "uart rx: %s", this->rxBuffer);
				this->debugWrite(this->s);
			}
#endif

			sei();
			//return IDLE;
			return ACKDELAY;
		}
		else if(this->rxFrame->type == Frame::TYPE_UNIDIRECTION)
		{
		  // Received Frame is unidirectional - so do nothing

          swTimer_tickReset(&this->startTick);
          sei();
          return RXDELAY;
		}
		else if(this->rxFrame->type == Frame::TYPE_ACK)
		{
      // Received Frame is an ack frame

		  if(this->writer != nullptr)
		  {
		    //sprintf(this->s, "reader set ack", 0);
		    //this->debugWrite(this->s);

		    // Pass received frame onto the associated Writer.
		    this->writer->setAckFrame(this->rxFrame);
		  }
		}
		else
		{
		  // Un-recognised frame type
		}
//
//		swTimer_tickReset(&this->startTick);
//		sei();
//		return ACKDELAY;
	}
	sei();

	return IDLE;
}

uint8_t Reader::ackDelay()
{
	if(swTimer_tickCheckTimeout(&this->startTick, 10))
	{
		//sprintf(this->s, "Reader::ackDelay() done\n\0", 0);

		//this->debugWrite(this->s);

		memset((char*)this->ackBuffer, 0, this->bufferLen);
		uint8_t ret;
		//this->ackFrame.toString((char*)this->ackBuffer, &ret);
		this->ackFrame->toString((char*)this->ackBuffer, &ret);
		//char ackBuffer[64] = {0};
		//sprintf(ackBuffer, "TestAck\n", 0);

#if defined(ENV_CONFIG__SYSTEM_PC)
		if(this->debugOn)
		{
			sprintf(this->s, "ack frame: %s\n\0", this->ackBuffer);
			this->debugWrite(this->s);
		}
#endif

		this->uartWrite((char*)this->ackBuffer);
		//this->write((char*)"hello\n\0");

		return TXACKWAIT;
	}
	return ACKDELAY;
}

uint8_t Reader::txAckWait()
{
	if(!this->getUartTxBusy())
	{
#if defined(ENV_CONFIG__SYSTEM_PC)
		if(this->debugOn)
		{
			sprintf(this->s, "Reader::txAckWait() done\n\0", 0);
			this->debugWrite(this->s);
		}
#endif

		swTimer_tickReset(&this->startTick);
		return RXDELAY;
	}
	return TXACKWAIT;
}

uint8_t Reader::rxDelay()
{
  if(swTimer_tickCheckTimeout(&this->startTick, 20))
  {
    this->rxFlag = true;

    return IDLE;
  }
  return RXDELAY;
}
// end of state methods
//-------------------------------------------------------------
bool Reader::checkUartFrameRx()
{
	if(this->link != nullptr)
	{
		return this->link->checkFrameRx();
	}
	return false;
}
uint8_t Reader::getUartRxLenAndReset()
{
  if(this->link != nullptr)
	{
		return this->link->getRxLenAndReset();
	}
  return 0;
}
uint8_t Reader::uartWrite(char* buffer)
{
	if(this->link != nullptr)
	{
		return this->link->write(buffer);
	}
	return 0;
}
bool Reader::getUartTxBusy()
{
	if(this->link != nullptr)
	{
		return this->link->getTxBusy();
	}
	return false;
}
