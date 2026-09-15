// serlink_nrf24_brg
//
// SerLink sockets:
//
// echoSocket (ECHO1): sends received data back (unacknowledged) prefixed
// with "echo ", e.g.
//   PC -> ECHO1T076004abcd
//   PC <- ECHO1A076900            (ack)
//   PC <- ECHO1U000009echo abcd
//
// debugSocket (DBG01): instant handler, the reply is returned in the ack, e.g.
//   PC -> DBG01T156003RPB         (read port B: D8 - D13)
//   PC <- DBG01A156006DDPPII      (hex: direction, output latch, input level)
//
// The uart port (usb-uart or D0/D1) is selected by UART_PORT in uart.h.
// Run synclib.py first to copy the modules from lib/ into this folder.
//
//----------------------------------------------------------------
// Wiring (nRF24L01 -> Arduino Uno R4 Minima):
//   VCC  -> 3V3   (NOT 5V -- a decoupling cap, e.g. 10uF, across VCC/GND
//                  right at the module is recommended; the module is
//                  sensitive to power noise)
//   GND  -> GND
//   CE   -> D9
//   CSN  -> D10
//   SCK  -> D13
//   MOSI -> D11
//   MISO -> D12
//   IRQ  -> not connected
//
//----------------------------------------------------------------
// Led sockets (LED01): relay (SerlinkRelay) between ledSocket (uart) and
// ledRadioSocket (radio, via writer1 / reader1 / serLinkRadioAdapter), in both
// directions. Relayed frames keep their roll code, e.g.
//   PC -> LED01U492002A1          (uart)
//   radio -> LED01U492002A1
//
// For a 'T' frame, the far end's ack is returned as a relay ack ('B'), e.g.
//   PC -> LED01T492002A1          (uart)
//   PC <- LED01A492900            (ack from this device's reader0)
//   radio -> LED01T492002A1
//   radio <- LED01A492900         (ack from the far end)
//   PC <- LED01B492900            (relay ack)
//
//----------------------------------------------------------------

#include "uart_wrapper.hpp"
#include "SerLinkUartAdapter.hpp"
#include "Frame.hpp"
#include "Reader.hpp"
#include "Writer.hpp"
#include "Socket.hpp"
#include "Registers.hpp"
#include "timer0.h"
#include "Radio.hpp"
#include "SerLinkRadioAdapter.hpp"
#include "SerlinkRelay.hpp"
#include <string.h>

const uint8_t CE_PIN = 9;
const uint8_t CSN_PIN = 10;
const byte RADIO_ADDRESS[6] = "00001";

// Frame::toString() clears one byte past the frame's '\n', so every frame
// buffer gets one extra byte.
#define FRAME_BUFF_LEN (UART_BUFF_LEN + 1)

// Largest frame payload that fits in a uart frame (header + data + '\n')
#define MAX_DATA_LEN (UART_BUFF_LEN - SerLink::Frame::LEN_HEADER - 1)

bool debugSockInstantHandler(SerLink::Frame &rxFrame, uint16_t* dataLen, char* data);

//-------------------------------------------------
// reader0 & writer0

char readerRxBuffer[FRAME_BUFF_LEN];
char readerAckBuffer[FRAME_BUFF_LEN];
char writerTxBuffer[FRAME_BUFF_LEN];

char writerTxFrameBuffer[FRAME_BUFF_LEN];
char writerAckFrameBuffer[FRAME_BUFF_LEN];
SerLink::Frame writerTxFrame(writerTxFrameBuffer);
SerLink::Frame writerAckFrame(writerAckFrameBuffer);

char readerRxFrameBuffer[FRAME_BUFF_LEN];
char readerAckFrameBuffer[FRAME_BUFF_LEN];
SerLink::Frame readerRxFrame(readerRxFrameBuffer);
SerLink::Frame readerAckFrame(readerAckFrameBuffer);

// uart link used by reader0 & writer0 (see uart.h)
SerLinkUartAdapter uart0Adapter;

SerLink::Writer writer0(WRITER_CONFIG__WRITER0_ID, &uart0Adapter, writerTxBuffer,
    UART_BUFF_LEN, &writerTxFrame, &writerAckFrame);

SerLink::Reader reader0(READER_CONFIG__READER0_ID, &uart0Adapter, readerRxBuffer, readerAckBuffer,
    UART_BUFF_LEN, &readerRxFrame, &readerAckFrame, &writer0);

//-------------------------------------------------
// echo socket
char echoSocketRxFrameBuffer[FRAME_BUFF_LEN];
char echoSocketTxFrameBuffer[FRAME_BUFF_LEN];

SerLink::Frame echoSocketRxFrame(echoSocketRxFrameBuffer);
SerLink::Frame echoSocketTxFrame(echoSocketTxFrameBuffer);

SerLink::Socket echoSocket(&writer0, &reader0, (char*)"ECHO1", &echoSocketRxFrame, &echoSocketTxFrame);

//-------------------------------------------------
// debug socket
char debugSocketRxFrameBuffer[FRAME_BUFF_LEN];
char debugSocketTxFrameBuffer[FRAME_BUFF_LEN];

SerLink::Frame debugSocketRxFrame(debugSocketRxFrameBuffer);
SerLink::Frame debugSocketTxFrame(debugSocketTxFrameBuffer);

SerLink::Socket debugSocket(&writer0, &reader0, (char*)"DBG01", &debugSocketRxFrame, &debugSocketTxFrame,
    nullptr, nullptr, &debugSockInstantHandler);
//-------------------------------------------------
// led socket
char ledSocketRxFrameBuffer[FRAME_BUFF_LEN];
char ledSocketTxFrameBuffer[FRAME_BUFF_LEN];

SerLink::Frame ledSocketRxFrame(ledSocketRxFrameBuffer);
SerLink::Frame ledSocketTxFrame(ledSocketTxFrameBuffer);

SerLink::Socket ledSocket(&writer0, &reader0, (char*)"LED01", &ledSocketRxFrame, &ledSocketTxFrame);

//-------------------------------------------------
Radio radio(CE_PIN, CSN_PIN);
SerLinkRadioAdapter serLinkRadioAdapter(&radio);
//-------------------------------------------------
// reader1 & writer1 (over the radio)

char reader1RxBuffer[FRAME_BUFF_LEN];
char reader1AckBuffer[FRAME_BUFF_LEN];
char writer1TxBuffer[FRAME_BUFF_LEN];

char writer1TxFrameBuffer[FRAME_BUFF_LEN];
char writer1AckFrameBuffer[FRAME_BUFF_LEN];
SerLink::Frame writer1TxFrame(writer1TxFrameBuffer);
SerLink::Frame writer1AckFrame(writer1AckFrameBuffer);

char reader1RxFrameBuffer[FRAME_BUFF_LEN];
char reader1AckFrameBuffer[FRAME_BUFF_LEN];
SerLink::Frame reader1RxFrame(reader1RxFrameBuffer);
SerLink::Frame reader1AckFrame(reader1AckFrameBuffer);

SerLink::Writer writer1(WRITER_CONFIG__WRITER1_ID, &serLinkRadioAdapter, writer1TxBuffer,
    UART_BUFF_LEN, &writer1TxFrame, &writer1AckFrame);

SerLink::Reader reader1(READER_CONFIG__READER1_ID, &serLinkRadioAdapter, reader1RxBuffer, reader1AckBuffer,
    UART_BUFF_LEN, &reader1RxFrame, &reader1AckFrame, &writer1);

//-------------------------------------------------
// led radio socket
char ledRadioSocketRxFrameBuffer[FRAME_BUFF_LEN];
char ledRadioSocketTxFrameBuffer[FRAME_BUFF_LEN];

SerLink::Frame ledRadioSocketRxFrame(ledRadioSocketRxFrameBuffer);
SerLink::Frame ledRadioSocketTxFrame(ledRadioSocketTxFrameBuffer);

SerLink::Socket ledRadioSocket(&writer1, &reader1, (char*)"LED01", &ledRadioSocketRxFrame, &ledRadioSocketTxFrame);

//-------------------------------------------------
// relay (ledSocket <-> ledRadioSocket)
char relayWorkFrameBuffer[FRAME_BUFF_LEN];
SerLink::Frame relayWorkFrame(relayWorkFrameBuffer);

SerlinkRelay relay(&relayWorkFrame);

//-------------------------------------------------
// socket data buffers
char socketRxData[FRAME_BUFF_LEN];
char socketTxData[FRAME_BUFF_LEN];
uint16_t socketRxDataLen;

void setup() {
  reader0.init(); // initialises the uart
  reader1.init(); // sets serLinkRadioAdapter's rx frame buffer
  relay.registerPair(&ledSocket, &ledRadioSocket);
  timer0_init();
  radio.init(RADIO_ADDRESS);
  radio.startListening();
}

void loop() {
  if (echoSocket.getRxData(socketRxData, &socketRxDataLen)) { // ECHO1T076004abcd
    if (socketRxDataLen > MAX_DATA_LEN) {
      socketRxDataLen = MAX_DATA_LEN;
    }
    socketRxData[socketRxDataLen] = '\0';

    // "echo " + data, truncated to what fits in a frame
    int len = snprintf(socketTxData, sizeof(socketTxData), "echo %s", socketRxData);
    if (len > MAX_DATA_LEN) {
      len = MAX_DATA_LEN;
    }
    echoSocket.sendData(socketTxData, len, false);
  }

  // The debug socket replies from its instant handler (in the ack), so the
  // received data is not used here.
  debugSocket.getRxData(socketRxData, &socketRxDataLen); // DBG01T156003RPB

  // Relays LED01 frames between ledSocket (uart) and ledRadioSocket (radio).
  // Must run before the readers' rx flags are cleared below.
  relay.run();

  // Drop received frames that no socket has claimed (e.g. unknown protocol)
  reader0.clearRxFlag();
  reader1.clearRxFlag();

  writer0.run();
  reader0.run();

  radio.run();
  serLinkRadioAdapter.run();
  writer1.run();
  reader1.run();

  echoSocket.run();
  debugSocket.run();
  ledSocket.run();
  ledRadioSocket.run();
}
//-----------------------------------------------------------------------------------------------
// Called by reader0 when a DBG01 frame is received. Returns true if data/dataLen
// have been set, in which case they are sent back in the ack frame.
bool debugSockInstantHandler(SerLink::Frame &rxFrame, uint16_t* dataLen, char* data) // DBG01T156003RPB
{
  uint8_t index = 0;
  if(rxFrame.buffer[index++] == 'R')
  {
    if(rxFrame.buffer[index++] == 'P')
    {
      if(rxFrame.buffer[index++] == 'B')
      {
        // read port B
        memset(data, 0, 10); // clear outgoing buffer
        *dataLen = Registers::PortB::Read(data);
        return true;
      }
    }
  }
  return false;
}
//-----------------------------------------------------------------------------------------------
