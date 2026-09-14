/*
 * Socket.cpp
 *
 *  Created on: 18 Nov 2023
 *      Author: rw123
 */

#include "Socket.hpp"
#include "swTimer.h"
#include "string.h"

SerLink::Socket::Socket(Writer* writer, Reader* reader, char* protocol, Frame *rxFrame, Frame* txFrame, HardMod::Event* event
,HardMod::EventQueue* sendEventQueue,  readHandler instantReadHandler, uint16_t startRollCode):
writer(writer), reader(reader), event(event), sendEventQueue(sendEventQueue) // event(event),
{
  this->active = true;
  this->txDataFlag = false;
  //this->txDataAck = false;
  // this->rxFlag = false;
  //this->txBusy = false;
  //this->txStatus = TX_STATUS_IDLE;
  this->rxFrame = rxFrame;
  this->txFrame = txFrame;
  strncpy(this->protocol, protocol, Frame::LEN_PROTOCOL);
  this->instantReadHandler = instantReadHandler;
  if(this->instantReadHandler != nullptr)
  {
    this->reader->registerInstantCallback(this->protocol, this->instantReadHandler);
  }
  this->txRollCode = startRollCode;
}

// void SerLink::Socket::init(char* protocol, Frame *rxFrame, Frame* txFrame,
//     readHandler instantReadHandler, uint16_t startRollCode)
// {
//   this->active = true;
//   this->txFlag = false;
//   this->rxFlag = false;
//   //this->txBusy = false;
//   this->txStatus = TX_STATUS_IDLE;
//   this->rxFrame = rxFrame;
//   this->txFrame = txFrame;
//   strncpy(this->protocol, protocol, Frame::LEN_PROTOCOL);
//   //this->instantReadHandler = instantReadHandler;
//   this->txRollCode = startRollCode;
// }

//------------------------------------------------------------------------------
// Upper (i.e. application) Interface

bool SerLink::Socket::getRxData(char* data, uint16_t* dataLen)
{
  if(this->rxFrame == nullptr)
  {
    // This is a write only socket (this->rxFrame == nullptr in order to save memory) - so do nothing
    return false;
  }

  if(this->reader->getRxFrameProtocol(this->rxFrame, this->protocol))
  {
    *dataLen = this->rxFrame->dataLen;
    strncpy(data, this->rxFrame->buffer, this->rxFrame->dataLen);
    return true;
  }
  return false;
}

bool SerLink::Socket::getRxEvent(HardMod::Event &event)
{
  if(this->rxFrame == nullptr)
  {
    // This is a write only socket (this->rxFrame == nullptr in order to save memory) - so do nothing
    return false;
  }

  if(this->reader->getRxFrameProtocol(this->rxFrame, this->protocol))
  {
    // de-serialise the event
    if(event.deSerialise(this->rxFrame->buffer))
    {
      return true;
    }
  }
  return false;
}

bool SerLink::Socket::sendData(char* data, uint16_t dataLen, bool ack)
{
  if(this->txFrame == nullptr)
  {
    // This is a read only socket (this->txFrame == nullptr in order to save memory) - so do nothing
    return false;
  }

  if(!this->active)
  {
    // This socket is not acive - so do nothing
    return false;
  }

  this->txDataFlag = true;
  //this->txDataAck = ack;

  // if(this->writer->getStatus() == Writer::STATUS_BUSY)
  // {
  //   // Writer is currently busy - so do nothing
  //   return false;
  // }

  this->txFrame->setProtocol(this->protocol);
  if(ack)
  {
    this->txFrame->type = Frame::TYPE_TRANSMISSION;
  }
  else
  {
    this->txFrame->type = Frame::TYPE_UNIDIRECTION;
  }
  this->txFrame->rollCode = this->txRollCode;
  Frame::incRollCode(&this->txRollCode);
  this->txFrame->dataLen = dataLen;
  strncpy(this->txFrame->buffer, data, dataLen);

  // this->writer->sendFrame(this->txFrame);

  //this->txStatus = TX_STATUS_IDLE;

  return true;
}

bool SerLink::Socket::sendEvent(HardMod::Event &event, char* buffer, bool ack)
{
  if(this->sendEventQueue == nullptr){
    // This is not a buffered socket, i.e., it does not have a sendEventQueue

    this->txDataFlag = true;

    this->txFrame->setProtocol(this->protocol);
    if(event.getAck())
    {
      this->txFrame->type = Frame::TYPE_TRANSMISSION;
    }
    else
    {
      this->txFrame->type = Frame::TYPE_UNIDIRECTION;
    }
    this->txFrame->rollCode = this->txRollCode;
    Frame::incRollCode(&this->txRollCode);

    this->txFrame->dataLen = event.serialise(this->txFrame->buffer);

    return true;
  }
  else
  {
    // This is a buffered socket, i.e., it has a sendEventQueue
    if(this->sendEventQueue->isFull())
    {
      return false;
    }

    event.setAck(ack);

    this->sendEventQueue->put(&event);
    return true;
  }




  // serialise the event
  // uint8_t len = event.serialise(buffer);

  // // send the data
  // return this->sendData(buffer, len, ack);
}

uint8_t SerLink::Socket::getAndClearSendStatus()
{
  return this->writer->getStatusProtocol(this->protocol);

  // if(this->txStatus == TX_STATUS_BUSY)
  // {
  //   return this->txStatus;
  // }
  // else
  // {
  //   uint8_t status = this->txStatus;
  //   this->txStatus = TX_STATUS_IDLE; // clear status
  //   return status;
  // }
}

void SerLink::Socket::run() // char* buffer
{
  if(this->writer->getStatus() == Writer::STATUS_BUSY)
  {
    // writer is busy - so do nothinh
    return;
  }

  if(this->txDataFlag)
  {
    // send data (not event), this->txFrame is already initialised

    this->txDataFlag = false;

    this->writer->sendFrame(this->txFrame);
    return;
  }

  if(this->sendEventQueue == nullptr)
  {
    return;
  }

  if(this->sendEventQueue->isEmpty())
  {
    // There are no events to send
    return;
  }

  //HardMod::Event event;
  // get the first event to be sent from the sendEventQueue
  this->sendEventQueue->get(this->event); // this->event

  this->txFrame->setProtocol(this->protocol);
  if(this->event->getAck())
  {
    this->txFrame->type = Frame::TYPE_TRANSMISSION;
  }
  else
  {
    this->txFrame->type = Frame::TYPE_UNIDIRECTION;
  }
  this->txFrame->rollCode = this->txRollCode;
  Frame::incRollCode(&this->txRollCode);

  this->txFrame->dataLen = this->event->serialise(this->txFrame->buffer);
  //strncpy(this->txFrame->buffer, data, dataLen);

  this->writer->sendFrame(this->txFrame);

  //this->txStatus = TX_STATUS_IDLE;
}

/*
//------------------------------------------------------------------------------
// Lower (i.e. transport) Interface

bool SerLink::Socket::getTxFrame(Frame* txFrame)
{
  if(this->txFrame == nullptr)
  {
    // This is a read only socket (this->txFrame == nullptr in order to save memory) - so do nothing
    return false;
  }

  if(this->txFlag)
  {
    this->txFrame->copy(txFrame);
    this->txFlag = false;         // clear tx flag
    //this->txBusy = true;
    return true;
  }
  else
  {
    return false;
  }
}

void SerLink::Socket::setWriterDoneStatus(uint8_t status)
{
  if(status == Writer::STATUS_IDLE)
  {
    this->txStatus = TX_STATUS_IDLE;
  }
  else if(status >= Writer::STATUS_OK)
  {
    // Writer has finished sending Frame
    this->txStatus = status;
  }
}

void SerLink::Socket::setRxFrame(Frame* rxFrame)
{
  if(this->rxFrame == nullptr)
  {
    // This is a write only socket (this->rxFrame == nullptr in order to save memory) - so do nothing
    return;
  }

  if(!this->active)
  {
    return;
  }

  this->rxFlag = true;
  rxFrame->copy(this->rxFrame);
}
//------------------------------------------------------------------------------
*/
