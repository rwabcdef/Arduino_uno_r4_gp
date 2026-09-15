/*
 * SerLinkUartAdapter.hpp
 *
 * Lets a SerLink::Reader & SerLink::Writer run over the uart module (uart.h),
 * by implementing SerLink::LinkInterface.
 *
 * uart.h has a single set of global state, so only one SerLinkUartAdapter
 * should be used.
 */

#ifndef SERLINK_UART_ADAPTER_HPP_
#define SERLINK_UART_ADAPTER_HPP_

#include "LinkInterface.hpp"

class SerLinkUartAdapter : public SerLink::LinkInterface
{
  public:
    bool init(char* pRxBuffer, uint8_t rxBufferLen) override;
    bool checkFrameRx() override;
    uint8_t getRxLenAndReset() override;
    uint8_t write(const char* buffer) override;
    bool getTxBusy() override;
};

#endif /* SERLINK_UART_ADAPTER_HPP_ */
