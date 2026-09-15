/*
 * SerLinkUartAdapter.cpp
 */

#include "SerLinkUartAdapter.hpp"
#include "uart_wrapper.hpp"

bool SerLinkUartAdapter::init(char* pRxBuffer, uint8_t rxBufferLen)
{
  return uart_init(pRxBuffer, rxBufferLen);
}

bool SerLinkUartAdapter::checkFrameRx()
{
  return uart_checkFrameRx();
}

uint8_t SerLinkUartAdapter::getRxLenAndReset()
{
  return uart_getRxLenAndReset();
}

uint8_t SerLinkUartAdapter::write(const char* buffer)
{
  return uart_write(buffer);
}

bool SerLinkUartAdapter::getTxBusy()
{
  return uart_getTxBusy();
}
