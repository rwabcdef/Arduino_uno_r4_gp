/*
 * SerLinkRadioAdapter.cpp
 */

#include "SerLinkRadioAdapter.hpp"
#include <string.h>

SerLinkRadioAdapter::SerLinkRadioAdapter(Radio* radio) : radio(radio)
{
  this->pRxFramebuffer = nullptr;
  this->rxFrameBufferLen = 0;
  this->rxFlag = false;
  this->rxLen = 0;
}

void SerLinkRadioAdapter::run()
{
  if((this->radio == nullptr) || (this->pRxFramebuffer == nullptr))
  {
    return;
  }

  if(this->rxFlag)
  {
    // The Reader has not taken the previous frame yet - so leave any new
    // frame in the radio.
    return;
  }

  char buffer[RADIO__FRAME_LEN_MAX];
  uint8_t len;
  if(this->radio->hasRxData(buffer, &len))
  {
    this->setRxFrame(buffer, len);
  }
}

void SerLinkRadioAdapter::setRxFrame(char* buffer, uint8_t len)
{
  if(len > this->rxFrameBufferLen)
  {
    // Too long for the external buffer. A truncated frame would claim more
    // data than the buffer holds - so drop it.
    return;
  }

  memset(this->pRxFramebuffer, 0, this->rxFrameBufferLen); // clear external buffer
  memcpy(this->pRxFramebuffer, buffer, len);

  this->rxLen = len;
  this->rxFlag = true;
}

//----------------------------------------------------------------
// LinkInterface

bool SerLinkRadioAdapter::init(char* pRxBuffer, uint8_t rxBufferLen)
{
  this->pRxFramebuffer = pRxBuffer;
  this->rxFrameBufferLen = rxBufferLen;
  this->rxFlag = false;
  this->rxLen = 0;

  return this->radio != nullptr;
}

bool SerLinkRadioAdapter::checkFrameRx()
{
  return this->rxFlag;
}

uint8_t SerLinkRadioAdapter::getRxLenAndReset()
{
  this->rxFlag = false;
  return this->rxLen;
}

uint8_t SerLinkRadioAdapter::write(const char* buffer)
{
  if(this->radio == nullptr)
  {
    return WRITE_STATUS_ERROR;
  }

  return (0 == this->radio->write(buffer)) ? WRITE_STATUS_OK : WRITE_STATUS_ERROR;
}

bool SerLinkRadioAdapter::getTxBusy()
{
  if(this->radio == nullptr)
  {
    return false;
  }

  return this->radio->getTxBusy();
}
