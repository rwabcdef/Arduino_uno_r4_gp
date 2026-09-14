#ifndef REGISTERS_HPP
#define REGISTERS_HPP

// UNO R4 version of the Uno R3 port register reads.
//
// The RA4M1 has no DDRx/PORTx/PINx registers, so each Uno R3 port is emulated
// from the Arduino pins it was wired to (bit 0 = first pin):
//   PORTB: D8 - D13
//   PORTC: A0 - A5
//   PORTD: D0 - D7
// Each Read() returns the same string format as on the R3:
// "DDPPII" (hex) = direction (1 = output), output latch, input level.

#include <Arduino.h>
#include <stdint.h>
#include <stdio.h>

class Registers
{
public:
  // --------------------------------------------------
  // Generic readPort dispatcher — automatically selects the port
  // --------------------------------------------------
  static uint8_t readPort(char port, char *str)
  {
    switch (port)
    {
    case 'B':
    case 'b':
      return PortB::Read(str);
    case 'C':
    case 'c':
      return PortC::Read(str);
    case 'D':
    case 'd':
      return PortD::Read(str);
    default:
      str[0] = '\0';
      return 0;
    }
  }

  // ======================================================
  // PORT B
  // ======================================================
  class PortB
  {
  public:
    static uint8_t Read(char *str) { return Registers::readPins(str, 8, 6); }
  };

  // ======================================================
  // PORT C
  // ======================================================
  class PortC
  {
  public:
    static uint8_t Read(char *str) { return Registers::readPins(str, A0, 6); }
  };

  // ======================================================
  // PORT D
  // ======================================================
  class PortD
  {
  public:
    static uint8_t Read(char *str) { return Registers::readPins(str, 0, 8); }
  };

private:
  // Builds R3 style DDR/PORT/PIN bytes from numPins Arduino pins starting at firstPin.
  static uint8_t readPins(char *str, uint8_t firstPin, uint8_t numPins)
  {
    uint8_t ddr = 0, port = 0, pin = 0;

    for (uint8_t i = 0; i < numPins; i++)
    {
      bsp_io_port_pin_t bspPin = digitalPinToBspPin(firstPin + i);
      const volatile R_PFS_PORT_PIN_Type *pfs = &R_PFS->PORT[bspPin >> 8].PIN[bspPin & 0xFF];

      if (pfs->PmnPFS_b.PDR)  { ddr  |= (1 << i); }
      if (pfs->PmnPFS_b.PODR) { port |= (1 << i); }
      if (pfs->PmnPFS_b.PIDR) { pin  |= (1 << i); }
    }

    sprintf(str, "%02X%02X%02X", ddr, port, pin);
    str[6] = '\0';
    return 6;
  }
};

#endif // REGISTERS_HPP
