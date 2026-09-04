// nrf24_transmitter
//
// Sends a packet with payload "count X" every 3 seconds over an nRF24L01
// module, with X incrementing from 0. Reports send success/failure over
// serial.
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
// Must use the same RADIO_ADDRESS as the receiver.

#include <SPI.h>
#include <RF24.h>

const uint8_t CE_PIN = 9;
const uint8_t CSN_PIN = 10;
const byte RADIO_ADDRESS[6] = "00001";

RF24 radio(CE_PIN, CSN_PIN);

unsigned long lastSendMs = 0;
const unsigned long sendIntervalMs = 3000;
unsigned int count = 0;

void setup() {
  Serial.begin(115200);

  if (!radio.begin()) {
    Serial.println("nRF24L01 not found");
    while (true) {
      // halt
    }
  }

  radio.setPALevel(RF24_PA_LOW);
  radio.openWritingPipe(RADIO_ADDRESS);
  radio.stopListening();
}

void loop() {
  unsigned long now = millis();
  if (now - lastSendMs >= sendIntervalMs) {
    lastSendMs = now;

    char payload[32];
    snprintf(payload, sizeof(payload), "count %u", count);

    bool ok = radio.write(&payload, sizeof(payload));
    Serial.print("Sent: ");
    Serial.print(payload);
    Serial.println(ok ? " (ack)" : " (failed)");

    count++;
  }
}
