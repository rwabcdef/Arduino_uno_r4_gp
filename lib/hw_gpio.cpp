/*
 * hw_gpio.cpp
 *
 * UNO R4 version of hw_gpio.c (which uses the ATmega328 DDRx/PORTx/PINx
 * registers). The RA4M1 has no such registers, so each Uno R3 port/pin is
 * mapped to the Arduino pin it was wired to (same mapping as Registers.hpp):
 *   GPIO_REG__PORTB: pins 0 - 5 -> D8 - D13
 *   GPIO_REG__PORTC: pins 0 - 5 -> A0 - A5
 *   GPIO_REG__PORTD: pins 0 - 7 -> D0 - D7
 * Illegal port/pin values are ignored (getPinState returns false).
 *
 * C++ (not C) so it can use the Arduino API; the functions keep C linkage
 * via hw_gpio.h.
 */

#include "hw_gpio.h"
#include <Arduino.h>

static const int INVALID_PIN = -1;

// Returns the Arduino pin number for port & pin, or INVALID_PIN.
static int toArduinoPin(uint8_t port, uint8_t pin)
{
  if(port == GPIO_REG__PORTB)
  {
    return (pin < 6) ? (8 + pin) : INVALID_PIN;
  }
  else if(port == GPIO_REG__PORTC)
  {
    return (pin < 6) ? (A0 + pin) : INVALID_PIN;
  }
  else if(port == GPIO_REG__PORTD)
  {
    return (pin < 8) ? pin : INVALID_PIN;
  }
  return INVALID_PIN;
}

//--------------------------------------------------------------------------------
void gpio_setPinDirection(uint8_t port, uint8_t pin, uint8_t direction)
{
  int arduinoPin = toArduinoPin(port, pin);
  if(arduinoPin == INVALID_PIN)
  {
    return;
  }

  if(direction == GPIO_PIN_DIRECTION__IN)
  {
    pinMode(arduinoPin, INPUT);
  }
  else if(direction == GPIO_PIN_DIRECTION__OUT)
  {
    pinMode(arduinoPin, OUTPUT);
  }
  else{ /* Invalid direction - do nothing */ }
}
//--------------------------------------------------------------------------------
void gpio_setPinHigh(uint8_t port, uint8_t pin)
{
  int arduinoPin = toArduinoPin(port, pin);
  if(arduinoPin != INVALID_PIN)
  {
    digitalWrite(arduinoPin, HIGH);
  }
}
//--------------------------------------------------------------------------------
void gpio_setPinLow(uint8_t port, uint8_t pin)
{
  int arduinoPin = toArduinoPin(port, pin);
  if(arduinoPin != INVALID_PIN)
  {
    digitalWrite(arduinoPin, LOW);
  }
}
//--------------------------------------------------------------------------------
bool gpio_getPinState(uint8_t port, uint8_t pin)
{
  int arduinoPin = toArduinoPin(port, pin);
  if(arduinoPin == INVALID_PIN)
  {
    return false;
  }
  return digitalRead(arduinoPin) == HIGH;
}
//--------------------------------------------------------------------------------
void gpio_setDebugOn(bool on)
{
  (void)on; // PC debug build only
}
//--------------------------------------------------------------------------------
