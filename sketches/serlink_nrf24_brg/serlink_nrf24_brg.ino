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
// Led socket (LED01): receives a frame with a single byte of data, and sets
// LED01U492002A1
//
//----------------------------------------------------------------

#include "Frame.hpp"
#include "Reader.hpp"
#include "Writer.hpp"
#include "Socket.hpp"
#include "Registers.hpp"
#include "timer0.h"
#include "Radio.hpp"
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

SerLink::Writer writer0(WRITER_CONFIG__WRITER0_ID, writerTxBuffer,
    UART_BUFF_LEN, &writerTxFrame, &writerAckFrame);

SerLink::Reader reader0(READER_CONFIG__READER0_ID, readerRxBuffer, readerAckBuffer,
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
char radioTxBuffer[RADIO__FRAME_LEN_MAX];
//-------------------------------------------------
// socket data buffers
char socketRxData[FRAME_BUFF_LEN];
char socketTxData[FRAME_BUFF_LEN];
uint16_t socketRxDataLen;

void setup() {
  reader0.init(); // initialises the uart
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

  // ledSocket traffic is forwarded to the radio, so the received data is not used here.
  if (ledSocket.getRxData(socketRxData, &socketRxDataLen)) { // LED01U492002A1
    // socketRxData only holds the frame's data ("A1"), but Radio::write()
    // needs a whole '\n' terminated frame, so re-serialise the received frame.
    uint8_t ret;
    ledSocketRxFrame.toString(radioTxBuffer, &ret); // "LED01U492002A1\n"
    radio.write(radioTxBuffer);
  }

  // Drop received frames that no socket has claimed (e.g. unknown protocol)
  reader0.clearRxFlag();

  writer0.run();
  reader0.run();
  radio.run();

  echoSocket.run();
  debugSocket.run();
  ledSocket.run();
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
