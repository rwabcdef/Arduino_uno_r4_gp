// remote_hub
//
// The ControlHubAA26 Remote Hub: a user IO device for the control hub
// (NUCLEO-F439ZI firmware, ControlHubAA26_V1), over an nRF24L01 radio
// carrying SerLink. Two buttons, two LEDs and a potentiometer:
//
//   button 1  start / stop a Control run (stops a Lift move too)
//   button 2  toggle the direction - the hub accepts it only while Idle
//   pot       the speed of a Control run started here (0..100% of the
//             hub's REMOTE_POT_RPM_MAX); a run the PC started ignores it
//   LED A     the hub's mode: off Idle, on Control, slow flash Lift.
//             Fast flash: no word from the hub for LINK_LOST_MS
//   LED B     the selected direction: on = reverse
//
// The hub decides everything - this sketch only reports the buttons and
// the pot, and shows what the hub says. The protocol contract is
// sockets_summary.txt in the ControlHubAA26_V1 repo.
//
//----------------------------------------------------------------
// SerLink sockets
//
// Radio (writer1 / reader1). Frame data is at most MAX_DATA_LEN (9) chars:
//
//   BTN01  -> hub, 'T'   button presses, ButtonEvent format (Button.hpp):
//                          BTN01T<rrr>0021P   button 1 pressed
//                          BTN01T<rrr>0022P   button 2 pressed
//   POT01  -> hub, 'U'   the pot, PotEvent format (pot.hpp), sent on a
//                        change of POT_MIN_CHANGE or more:
//                          POT01U<rrr>004P050
//   HBT01  -> hub, 'U'   heartbeat, every HEARTBEAT_PERIOD_MS:
//                          HBT01U<rrr>001H
//                        The hub stops a run started here once nothing has
//                        arrived from this end for 2 s.
//   LED01  <- hub, 'U'   LedEvent format (Led.hpp), on a change and every
//                        second: A0 / A1 / AF0002020, B0 / B1
//
// Every frame from this end counts as a heartbeat at the hub, so HBT01
// only matters while the buttons and pot are still.
//
// Uart (writer0 / reader0), for debugging only:
//
//   DBG01  instant handler, the reply is returned in the ack, e.g.
//            PC -> DBG01T156003RPB         (read port B: D8 - D13)
//            PC <- DBG01A156006DDPPII      (hex: direction, output latch, input level)
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
// User IO:
//   button 1  D2 -> push button -> GND  (internal pull-up, active low)
//   button 2  D3 -> push button -> GND  (internal pull-up, active low)
//   LED A     D8 -> resistor (e.g. 330R) -> LED anode, cathode -> GND
//   LED B     D4 -> resistor (e.g. 330R) -> LED anode, cathode -> GND
//   pot       wiper -> A0, ends -> 5V and GND
//
// The built-in LED is on D13, the radio's SPI SCK, so it can't be used.
//----------------------------------------------------------------

#include "uart_wrapper.hpp"
#include "SerLinkUartAdapter.hpp"
#include "Frame.hpp"
#include "Reader.hpp"
#include "Writer.hpp"
#include "Socket.hpp"
#include "Registers.hpp"
#include "timer0.h"
#include "swTimer.h"
#include "Radio.hpp"
#include "SerLinkRadioAdapter.hpp"
#include "Led.hpp"
#include "Button.hpp"
#include "Adc.hpp"
#include "pot.hpp"
#include "hw_gpio.h"
#include <string.h>

const uint8_t CE_PIN = 9;
const uint8_t CSN_PIN = 10;
const byte RADIO_ADDRESS[6] = "00001";

const uint8_t BUTTON1_PIN = 2;   // D2, port D pin 2
const uint8_t BUTTON2_PIN = 3;   // D3, port D pin 3

// The hub's LED01 ids - the same characters it sends.
const char LED_RUN_ID = 'A';
const char LED_DIRECTION_ID = 'B';

const uint16_t HEARTBEAT_PERIOD_MS = 500;   // the hub times out at 2000
const uint16_t LINK_LOST_MS = 3000;         // the hub refreshes LED01 every 1000
const uint8_t POT_MIN_CHANGE = 2;           // percent - hysteresis against ADC jitter

// Frame::toString() clears one byte past the frame's '\n', so every frame
// buffer gets one extra byte.
#define FRAME_BUFF_LEN (UART_BUFF_LEN + 1)

// Largest frame payload that fits in a uart frame (header + data + '\n')
#define MAX_DATA_LEN (UART_BUFF_LEN - SerLink::Frame::LEN_HEADER - 1)

bool debugSockInstantHandler(SerLink::Frame &rxFrame, uint16_t* dataLen, char* data);
void handleLedData(char* data);
void showLinkLost();

//-------------------------------------------------
// reader0 & writer0 (uart, debugging)

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
// debug socket (uart)
char debugSocketRxFrameBuffer[FRAME_BUFF_LEN];
char debugSocketTxFrameBuffer[FRAME_BUFF_LEN];

SerLink::Frame debugSocketRxFrame(debugSocketRxFrameBuffer);
SerLink::Frame debugSocketTxFrame(debugSocketTxFrameBuffer);

SerLink::Socket debugSocket(&writer0, &reader0, (char*)"DBG01", &debugSocketRxFrame, &debugSocketTxFrame,
    nullptr, nullptr, &debugSockInstantHandler);

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
// radio sockets. Send-only sockets have no rx frame and the receive-only
// one no tx frame (Socket.hpp), which saves their buffers.

char buttonSocketTxFrameBuffer[FRAME_BUFF_LEN];
SerLink::Frame buttonSocketTxFrame(buttonSocketTxFrameBuffer);
SerLink::Socket buttonSocket(&writer1, &reader1, (char*)"BTN01", nullptr, &buttonSocketTxFrame);

char potSocketTxFrameBuffer[FRAME_BUFF_LEN];
SerLink::Frame potSocketTxFrame(potSocketTxFrameBuffer);
SerLink::Socket potSocket(&writer1, &reader1, (char*)"POT01", nullptr, &potSocketTxFrame);

char heartbeatSocketTxFrameBuffer[FRAME_BUFF_LEN];
SerLink::Frame heartbeatSocketTxFrame(heartbeatSocketTxFrameBuffer);
SerLink::Socket heartbeatSocket(&writer1, &reader1, (char*)"HBT01", nullptr, &heartbeatSocketTxFrame);

char ledSocketRxFrameBuffer[FRAME_BUFF_LEN];
SerLink::Frame ledSocketRxFrame(ledSocketRxFrameBuffer);
SerLink::Socket ledSocket(&writer1, &reader1, (char*)"LED01", &ledSocketRxFrame, nullptr);

//-------------------------------------------------
// user IO

// Active low: pressed reads 0. The internal pull-ups are switched on in
// setup() - the constructor's pinMode(INPUT) runs before it.
HardMod::Std::Button button1('1', GPIO_REG__PORTD, BUTTON1_PIN, false);
HardMod::Std::Button button2('2', GPIO_REG__PORTD, BUTTON2_PIN, false);
HardMod::Std::ButtonEvent buttonEvent;

HardMod::Std::Led ledRun(LED_RUN_ID, GPIO_REG__PORTB, 0);          // D8
HardMod::Std::Led ledDirection(LED_DIRECTION_ID, GPIO_REG__PORTD, 4); // D4
HardMod::Std::LedEvent ledEvent;   // decodes received LED01 data

HardMod::Std::AdcInput potInput(HardMod::Std::HwModule::ADC0);     // A0
HardMod::Std::AdcInput* adcInputs[] = { &potInput };
HardMod::Std::Adc adc(adcInputs, 1);
HardMod::Std::Pot pot('P', &adc, 0);
HardMod::Std::PotEvent potEvent;

//-------------------------------------------------
// socket data buffers
char socketRxData[FRAME_BUFF_LEN];
char socketTxData[FRAME_BUFF_LEN];
uint16_t socketRxDataLen;

uint16_t heartbeatTick;
uint16_t lastLedTick;      // when LED01 was last heard from the hub
bool linkLost = true;      // nothing heard yet
int16_t potLastSent = -1;  // -1: nothing sent yet

void setup() {
  // Over the constructors' pinMode(INPUT): active low buttons, so pull up.
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);

  // Again here, not only from adc's constructor: that ran before main(),
  // which may be too early for the core's analog setup to stick.
  HardMod::Std::HwModule::Adc_init(HardMod::Std::HwModule::PS_128);

  reader0.init(); // initialises the uart
  reader1.init(); // sets serLinkRadioAdapter's rx frame buffer
  timer0_init();
  radio.init(RADIO_ADDRESS);
  radio.startListening();

  swTimer_tickReset(&heartbeatTick);
  swTimer_tickReset(&lastLedTick);
  showLinkLost();
}

void loop() {
  // Buttons: presses only - each one a 'T' frame, so the hub's ack says it
  // arrived. The hub decides what it means (start/stop, direction).
  if (button1.getEvent(&buttonEvent) || button2.getEvent(&buttonEvent)) {
    if (buttonEvent.getAction() == BUTTONEVENT__PRESSED) {
      uint8_t len = buttonEvent.serialise(socketTxData);   // e.g. "1P"
      buttonSocket.sendData(socketTxData, len, true);
    }
  }

  // Pot: only a move of POT_MIN_CHANGE or more, so ADC jitter between two
  // neighbouring percents doesn't flood the radio.
  if (pot.getEvent(&potEvent)) {
    int16_t percent = potEvent.getPercent();
    if ((potLastSent < 0) || (abs(percent - potLastSent) >= POT_MIN_CHANGE)) {
      potLastSent = percent;
      uint8_t len = potEvent.serialise(socketTxData);      // e.g. "P050"
      potSocket.sendData(socketTxData, len, false);
    }
  }

  if (swTimer_tickCheckTimeout(&heartbeatTick, HEARTBEAT_PERIOD_MS)) {
    heartbeatSocket.sendData((char*)"H", 1, false);
  }

  // LEDs: whatever the hub says. Its refresh every second is also how this
  // end knows the link is up.
  if (ledSocket.getRxData(socketRxData, &socketRxDataLen)) {
    if (socketRxDataLen > MAX_DATA_LEN) {
      socketRxDataLen = MAX_DATA_LEN;
    }
    socketRxData[socketRxDataLen] = '\0';

    swTimer_tickReset(&lastLedTick);
    linkLost = false;
    handleLedData(socketRxData);
  }

  if (!linkLost && swTimer_tickCheckTimeoutNoReset(&lastLedTick, LINK_LOST_MS)) {
    linkLost = true;
    showLinkLost();
  }

  // The debug socket replies from its instant handler (in the ack), so the
  // received data is not used here.
  debugSocket.getRxData(socketRxData, &socketRxDataLen); // DBG01T156003RPB

  // Drop received frames that no socket has claimed (e.g. unknown protocol)
  reader0.clearRxFlag();
  reader1.clearRxFlag();

  writer0.run();
  reader0.run();

  radio.run();
  serLinkRadioAdapter.run();
  writer1.run();
  reader1.run();

  debugSocket.run();
  buttonSocket.run();
  potSocket.run();
  heartbeatSocket.run();
  ledSocket.run();

  button1.run();
  button2.run();
  adc.run();
  pot.run();

  ledRun.run();
  ledDirection.run();
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
// Applies one LED01 frame from the hub - an LedEvent (Led.hpp): <id>1 on,
// <id>0 off, <id>F<nn><on><off><final> flash - to the LED with that id.
void handleLedData(char* data)
{
  if (!ledEvent.deSerialise(data)) {
    return;
  }

  HardMod::Std::LedFlashParams flashParams;
  HardMod::Std::LedEvent::eventTypes type = ledEvent.getType(&flashParams);

  if (ledEvent.getId() == ledRun.getId()) {
    HardMod::Std::LedUtils::setLedEvent(&ledRun, type, &flashParams);
  } else if (ledEvent.getId() == ledDirection.getId()) {
    HardMod::Std::LedUtils::setLedEvent(&ledDirection, type, &flashParams);
  }
}
//-----------------------------------------------------------------------------------------------
// No LED01 from the hub for LINK_LOST_MS (or none yet): LED A flashes fast
// (250 ms on / 250 ms off, until the hub is heard again) and LED B goes
// off, since the direction it showed may be stale.
void showLinkLost()
{
  ledRun.flash(0, 1, 1, true);
  ledDirection.off();
}
//-----------------------------------------------------------------------------------------------
