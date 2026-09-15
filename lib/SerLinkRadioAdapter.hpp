/*
 * SerLinkRadioAdapter.hpp
 *
 * Lets a SerLink::Reader & SerLink::Writer run over the nRF24L01 (Radio)
 * instead of the uart, by implementing SerLink::LinkInterface:
 *
 *   Writer --> write() --> Radio::write()
 *   Reader <-- external rx frame buffer <-- run() <-- Radio::hasRxData()
 *
 * Usage:
 *   SerLinkRadioAdapter serLinkRadioAdapter(&radio);
 *   SerLink::Writer writer1(id, &serLinkRadioAdapter, ...);
 *   SerLink::Reader reader1(id, &serLinkRadioAdapter, ...);
 *
 *   setup(): reader1.init(); // sets the external rx frame buffer
 *   loop():  radio.run(); serLinkRadioAdapter.run(); ...
 */

#ifndef SERLINK_RADIO_ADAPTER_HPP_
#define SERLINK_RADIO_ADAPTER_HPP_

#include <stdint.h>
#include "LinkInterface.hpp"
#include "Radio.hpp"

class SerLinkRadioAdapter : public SerLink::LinkInterface
{
  public:
    // Touches no hardware, so fine for a static instance.
    // radio->run() is not called by this class, it must be called externally.
    SerLinkRadioAdapter(Radio* radio);

    // Checks radio for a received frame, if so, reads it and loads it into
    // the external rx frame buffer. Call every main loop iteration.
    void run();

    //-------------------------------------
    // LinkInterface
    bool init(char* pRxBuffer, uint8_t rxBufferLen) override;
    bool checkFrameRx() override;
    uint8_t getRxLenAndReset() override;
    uint8_t write(const char* buffer) override;
    bool getTxBusy() override;
    //-------------------------------------

  protected:
    Radio* radio;
    char* pRxFramebuffer;     // external buffer
    uint8_t rxFrameBufferLen; // external buffer length
    bool rxFlag;
    uint8_t rxLen;

    // Loads the external rx frame buffer with a frame read from the radio.
    // The frame is dropped if it does not fit in the external buffer.
    void setRxFrame(char* buffer, uint8_t len);
};

#endif /* SERLINK_RADIO_ADAPTER_HPP_ */
