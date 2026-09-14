// nrf24_receiver
//
// Listens for SerLink frames on an nRF24L01 module (using the Radio class)
// and prints each received frame over serial. Does not transmit.
//
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
// Requires the "RF24" library by TMRh20 (Library Manager).
// Run synclib.py first to copy the modules from lib/ into this folder.
// Must use the same RADIO_ADDRESS as the transmitter.

#include "Radio.hpp"

const uint8_t CE_PIN = 9;
const uint8_t CSN_PIN = 10;
const byte RADIO_ADDRESS[6] = "00001";

Radio radio(CE_PIN, CSN_PIN);

char rxFrame[RADIO__FRAME_LEN_MAX];

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    // wait for native USB serial to connect
  }

  Serial.println("start");

  // The module is brought up by radio.run(), retrying until it answers.
  radio.init(RADIO_ADDRESS);
  radio.startListening();
}

void loop() {
  radio.run();

  uint8_t len;
  if (radio.hasRxData(rxFrame, &len)) {
    Serial.print(rxFrame); // frame already ends in '\n'
  }
}
