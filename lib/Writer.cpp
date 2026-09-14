/*
 * Writer.cpp
 *
 *  Created on: 31 Oct 2023
 *      Author: rw123
 */

#include "Writer.hpp"
#if defined(ENV_CONFIG__SYSTEM_PC)
#include "ATmega328.hpp"
#endif
#include "swTimer.h"
#include <string.h>
#include <stdio.h>

#define IDLE 0
#define TXWAIT 1
#define RXACKWAIT 2

using namespace SerLink;

Writer::Writer(uint8_t id, char* txBuffer, uint8_t bufferLen, Frame* txFrame, Frame* ackRxFrame): id(id), DebugUser()
{
  this->txFlag = false;
  this->ackRxFlag = false;
  this->txBuffer = txBuffer;
  this->bufferLen = bufferLen;
  this->txFrame = txFrame;
  this->ackRxFrame = ackRxFrame;
  this->currentState = IDLE;
  #if defined (ENV_CONFIG__SYSTEM_PC)
  this->debugLevel = DebugPrint_defs::Writer0;
  #endif
  this->status = Writer::STATUS_IDLE;
  this->flag = 0;
}

void Writer::run()
{
  switch(this->currentState)
    {
      case IDLE: { this->currentState = this->idle(); break;}
      case TXWAIT: { this->currentState = this->txWait(); break;}
      case RXACKWAIT: { this->currentState = this->rxAckWait(); break;}
    }
}

// Used to send frame (called from app layer above)
uint8_t Writer::sendFrame(Frame* frame)
{
  if(this->status == Writer::STATUS_BUSY)
  {
    return 1;
  }

  this->txFlag = true;
  frame->copy(this->txFrame);
  this->status = Writer::STATUS_BUSY;
  return 0;
}

// Used to check tx status (called from app layer above)
uint8_t Writer::getStatus()
{
  uint8_t status = this->status;
  if(Writer::STATUS_BUSY == this->status)
  {
    // writer is still busy - do nothing, i.e. don't clear status
  }
  else
  {
    this->status = Writer::STATUS_IDLE; // clear status
  }
  return status;
}

uint8_t Writer::getStatusProtocol(char* protocol)
{
  uint8_t status = this->status;
  if(Writer::STATUS_BUSY == this->status)
  {
    // writer is still busy - do nothing, i.e. don't clear status
  }
  else
  {
    // writer is not busy
    if(strncmp(this->txFrame->protocol, protocol, SerLink::Frame::LEN_PROTOCOL) == 0)
    {
      // the last frame that was sent has the same protocol as the specified one
      this->status = Writer::STATUS_IDLE; // clear status
    }
    else
    {
      // protocol mis-match - so do nothing
    }
  }
  return status;
}

void Writer::getStatusStr(char* str)
{
  switch(this->status)
  {
    case STATUS_IDLE: sprintf(str, "idle"); break;
    case STATUS_BUSY: sprintf(str, "busy"); break;
    case STATUS_OK: sprintf(str, "ok"); break;
    case STATUS_TIMEOUT: sprintf(str, "timeout"); break;
    case STATUS_PROTOCOL_ERROR: sprintf(str, "protocol error"); break;
  }
}

  // Called by a Reader to pass an ack frame to the Writer.
void Writer::setAckFrame(Frame* frame)
{
  frame->copy(this->ackRxFrame);
  this->ackRxFlag = true;
  //this->debugWrite("writer ack");
}

//----------------------------------------------------------------
// start of state methods
uint8_t Writer::idle()
{
  if(this->txFlag)
  {
    this->txFlag = false;

    uint8_t ret;
    this->txFrame->toString((char*)this->txBuffer, &ret);

    //this->status = Writer::STATUS_BUSY;

    this->uartWrite((char*)this->txBuffer);

    return TXWAIT;
  }

  return IDLE;
}
uint8_t Writer::txWait()
{
  if(this->getUartTxBusy())
  {
    return TXWAIT;
  }
  else
  {
    //return IDLE;

    if(this->txFrame->type == Frame::TYPE_UNIDIRECTION)
    {
      this->status = Writer::STATUS_IDLE;
      return IDLE;
    }
    else
    {
#if defined(ENV_CONFIG__SYSTEM_PC)
      //sprintf(this->s, "writer ack wait", 0);
      //this->debugWrite(this->s);
#endif

      swTimer_tickReset(&this->startTick);
      return RXACKWAIT;
    }
  }
}
uint8_t Writer::rxAckWait()
{
  if(true == this->ackRxFlag)
  {
    this->ackRxFlag = false;

#if defined(ENV_CONFIG__SYSTEM_PC)
    sprintf(this->s, "writer rx ack", 0);
    this->debugWrite(this->s);
#endif

    if(0 == strncmp(this->ackRxFrame->protocol, this->txFrame->protocol, Frame::LEN_PROTOCOL))
    {
      this->status = Writer::STATUS_OK;
      return IDLE;
    }
    else
    {
      this->status = Writer::STATUS_PROTOCOL_ERROR;
      return IDLE;
    }
  }

  if(swTimer_tickCheckTimeout(&this->startTick, 1000))
  {
    this->status = Writer::STATUS_TIMEOUT;
    //printf("timeout\n");
    return IDLE;
  }

  return RXACKWAIT;
}
// end of state methods
//----------------------------------------------------------------

uint8_t Writer::uartWrite(char* buffer)
{
#ifdef WRITER_CONFIG__WRITER0

  if(this->id == WRITER_CONFIG__WRITER0_ID)
  {
    return uart_write(buffer);
  }

#endif
  return 0;
}
bool Writer::getUartTxBusy()
{
#ifdef WRITER_CONFIG__WRITER0

  if(this->id == WRITER_CONFIG__WRITER0_ID)
  {
    return uart_getTxBusy();
  }

#endif
  return false;
}
