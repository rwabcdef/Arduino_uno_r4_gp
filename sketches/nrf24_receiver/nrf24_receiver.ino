// nrf24_receiver
//
// Listens for packets on an nRF24L01 module and prints each received
// payload over serial. Does not transmit.
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
// Must use the same RADIO_ADDRESS as the transmitter.

#include <SPI.h>
#include <RF24.h>

const uint8_t CE_PIN = 9;
const uint8_t CSN_PIN = 10;
const byte RADIO_ADDRESS[6] = "00001";

RF24 radio(CE_PIN, CSN_PIN);

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    // wait for native USB serial to connect
  }

  Serial.println("start");

  if (!radio.begin()) {
    Serial.println("nRF24L01 not found");
    while (true) {
      // halt
    }
  }

  radio.setPALevel(RF24_PA_LOW);
  radio.openReadingPipe(0, RADIO_ADDRESS);
  radio.startListening();

  Serial.println("nRF24L01 in Rx");
}

void loop() {
  if (radio.available()) {
    char payload[32] = {0};
    radio.read(&payload, sizeof(payload));
    Serial.println(payload);
  }
}
