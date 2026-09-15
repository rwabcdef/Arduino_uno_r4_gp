/*
 * Radio.hpp
 *
 * SerLink over the nRF24L01, for the Arduino UNO R4 (polled, no RTOS).
 * Wraps the RF24 library (TMRh20) and carries '\n' terminated serialised
 * SerLink frames in both directions. Based on ControlHubAA26_V1
 * Components/Radio, with the FreeRTOS queues replaced by a single tx buffer
 * and a single rx buffer:
 *
 *   write()     --> txBuffer --> run() --> nRF24L01
 *   hasRxData() <-- rxBuffer <-- run() <-- nRF24L01
 *
 * All SPI to the device happens in run(), which must be called from the main
 * loop. The public methods only copy into / out of the buffers and set flags.
 * The nRF24L01 IRQ pin is not used; run() polls the RX FIFO while listening.
 *
 * Over the air
 * ------------
 * A serialised frame (up to RADIO__FRAME_LEN_MAX - 1 chars, '\n' terminated)
 * does not fit in one 32-byte packet, so it is split across as many as it
 * needs:
 *
 *   byte 0       flags: PACKET_FLAG_START on the first packet of a frame
 *   bytes 1..31  frame text; the last packet is zero-padded
 *
 * The receiver collects text from a START packet up to '\n', then stores the
 * frame in rxBuffer. Auto-ack makes each packet reliable and in order. If one
 * still fails (max retries) the sender abandons the rest of the frame, and the
 * far end discards the partial when the next START arrives. SerLink's own ack
 * timeout reports the loss.
 *
 * Both ends use the same address in both directions, so this is a two-node
 * link. Two ends transmitting at the same moment are each out of RX while the
 * other sends, and both frames fail.
 */

#ifndef RADIO_HPP_
#define RADIO_HPP_

#include <stdint.h>
#include <RF24.h>
#include "StateMachine.hpp"

// Size of the tx and rx frame buffers, including the '\n' and the NUL.
// Buffers passed to hasRxData() must be at least this long.
#define RADIO__FRAME_LEN_MAX 80

class Radio : public StateMachine
{
  public:
    static const uint8_t ADDRESS_LEN = 5;

    // Link address used when init() is given none. Must match the far end.
    static const uint8_t DEFAULT_ADDRESS[ADDRESS_LEN];

    // Pins for the nRF24L01 CE and CSN lines (SPI uses D11/D12/D13).
    // Touches no hardware, so fine for a static instance.
    Radio(uint8_t cePin, uint8_t csnPin);

    // Sets the address and enters INIT. The device itself is brought up by
    // run(), retrying every INIT_RETRY_MS until it answers on SPI; the state
    // machine then goes to IDLE (or RX if startListening() was called).
    void init(const uint8_t* address = DEFAULT_ADDRESS);

    // Services the radio. Call every main loop iteration after init().
    // Returns immediately except while a frame is being transmitted.
    void run();

    // Request RX (listening) or IDLE (standby). Applied by run().
    // 0 if accepted, 1 if init() has not been called.
    uint8_t startListening();
    uint8_t stopListening();

    // Copies buffer, a NUL-terminated serialised frame ending in '\n', into
    // the tx buffer for run() to send. 0 if accepted; 1 if init() has not
    // been called, the previous frame has not been sent yet, or the frame is
    // too long / not '\n' terminated.
    uint8_t write(const char* buffer);

    // Returns true while a frame accepted by write() has not been sent yet.
    bool getTxBusy() { return this->txPending; }

    // If a received frame is waiting, copies it (NUL-terminated) into buffer,
    // which must be at least RADIO__FRAME_LEN_MAX long, sets len to its length
    // (including the '\n'), clears it from the rx buffer and returns true.
    // A frame that arrives before the previous one has been read is dropped.
    bool hasRxData(char* buffer, uint8_t* len);

  private:
    enum State : uint8_t
    {
      STATE_INIT = 0,
      STATE_IDLE,
      STATE_RX
    };

    static const uint8_t  PACKET_LEN        = 32;   // nRF24L01 max payload
    static const uint8_t  PACKET_HEADER_LEN = 1;
    static const uint8_t  PACKET_DATA_LEN   = PACKET_LEN - PACKET_HEADER_LEN;
    static const uint8_t  PACKET_FLAG_START = 0x01;
    static const uint8_t  RX_FIFO_DEPTH     = 3;
    static const uint8_t  CHANNEL           = 76;
    static const uint32_t INIT_RETRY_MS     = 1000;

    RF24     nrf;
    uint8_t  address[ADDRESS_LEN];
    bool     initialised;       // init() has been called
    bool     listen;            // requested mode: true = RX, false = IDLE
    bool     initAttempted;
    uint32_t initAttemptMs;

    char     txBuffer[RADIO__FRAME_LEN_MAX];
    bool     txPending;         // txBuffer holds a frame not yet sent

    char     rxFrame[RADIO__FRAME_LEN_MAX];   // frame being reassembled from packets
    uint8_t  rxFrameLen;
    bool     rxSynced;          // a START has been seen for the frame in rxFrame

    char     rxBuffer[RADIO__FRAME_LEN_MAX];  // last complete frame, for hasRxData()
    uint8_t  rxLen;
    bool     rxReady;           // rxBuffer holds a frame not yet read

    uint8_t initDevice();
    uint8_t idle();
    uint8_t rx();

    void    tx(bool listening);
    void    drainRxFifo();
    void    reassemble(const uint8_t* packet);
    void    resetRxFrame();
};

#endif /* RADIO_HPP_ */
