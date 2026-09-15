/*
 * LinkInterface.hpp
 *
 * Interface used by SerLink::Reader & SerLink::Writer to access the
 * (newline framed) transport below them, e.g. a uart or a radio. This
 * decouples Reader & Writer from any specific driver:
 *
 *   SerLinkUartAdapter  uart0Adapter;               // uart.h
 *   SerLinkRadioAdapter serLinkRadioAdapter(&radio); // nRF24L01
 *
 *   SerLink::Reader reader0(id, &uart0Adapter, ...);
 *   SerLink::Reader reader1(id, &serLinkRadioAdapter, ...);
 *
 * The same LinkInterface object may be shared by a Reader and a Writer.
 */

#ifndef LINK_INTERFACE_HPP_
#define LINK_INTERFACE_HPP_

#include <stdint.h>

namespace SerLink
{

class LinkInterface
{
  public:
    // write() return codes, same values as UART_STATUS_* in uart.h
    static const uint8_t WRITE_STATUS_OK = 1;
    static const uint8_t WRITE_STATUS_BUSY = 2;
    static const uint8_t WRITE_STATUS_ERROR = 3;

    // Initialises the link, and sets the external rx frame buffer.
    // Returns false if the link could not be opened.
    virtual bool init(char* pRxBuffer, uint8_t rxBufferLen) = 0;
    // Returns true if a frame has been received.
    virtual bool checkFrameRx() = 0;
    // Gets received frame length (and resets rx flag).
    virtual uint8_t getRxLenAndReset() = 0;
    // Writes a '\n' terminated frame. Returns WRITE_STATUS_*.
    virtual uint8_t write(const char* buffer) = 0;
    // Returns whether or not link tx is busy.
    virtual bool getTxBusy() = 0;

  protected:
    // Not virtual: implementations are never deleted through this interface,
    // and a virtual destructor would pull in operator delete.
    ~LinkInterface() = default;
};

} // end namespace SerLink

#endif /* LINK_INTERFACE_HPP_ */
