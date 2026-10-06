/*
 * HwModule.cpp
 *
 * UNO R4 version of HwModule.cpp (which drives the ATmega328 ADCSRA/ADMUX/ADC
 * registers directly). The RA4M1 has no such registers, so the ADC is driven
 * via the Arduino API instead:
 *   - ADC0 - ADC5 map to A0 - A5
 *   - analogRead() is blocking, so Adc_startConversion() performs the whole
 *     conversion and Adc_isConversionComplete() always returns true
 *   - the prescaler value is ignored (the core sets the ADC clock)
 *   - resolution is set to 10 bits so results match the Uno R3 (0 - 1023)
 */

#include "HwModule.hpp"
#include <Arduino.h>

namespace HardMod::Std
{

static uint8_t currentPin = A0;
static uint16_t result = 0;

void HwModule::Adc_init(AdcPrescalerValues psValue)
{
  (void)psValue; // not used on the R4

  analogReadResolution(10);

  // AVCC reference (as REFS0 on the Uno R3)
  analogReference(AR_DEFAULT);
}

void HwModule::Adc_enable(bool enable)
{
  (void)enable; // ADC is enabled by the core
}

void HwModule::Adc_setInput(AdcInputValues input)
{
  uint8_t value = (uint8_t) input;
  if(value <= ADC5)
  {
    currentPin = A0 + value;
  }
}

void HwModule::Adc_startConversion()
{
  result = (uint16_t) analogRead(currentPin);
}

bool HwModule::Adc_isConversionComplete()
{
  // analogRead() is blocking - the conversion is always complete
  return true;
}

uint16_t HwModule::Adc_getResultRaw()
{
  return result;
}

uint8_t HwModule::Adc_getResultPercent()
{
  uint32_t res = result * 3;
  res += (res >> 4);
  res = (res >> 5);
  return (uint8_t) res;
}

} // end namespace HardMod::Std
