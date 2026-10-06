#ifndef HWMODULE_HPP_
#define HWMODULE_HPP_

#include <stdint.h>
#include <stdbool.h>

namespace HardMod::Std
{
class HwModule
{
  public:
    enum AdcPrescalerValues{
      PS_2 = 0,
      PS_4,
      PS_8,
      PS_16,
      PS_32,
      PS_64,
      PS_128
    };

    enum AdcInputValues{
      ADC0 = 0,
      ADC1,
      ADC2,
      ADC3,
      ADC4,
      ADC5

    };
    
    static void Adc_init(AdcPrescalerValues psValue);

    static void Adc_enable(bool enable);

    static void Adc_startConversion();

    static bool Adc_isConversionComplete();

    static void Adc_setInput(AdcInputValues input);

    static uint16_t Adc_getResultRaw();

    static uint8_t Adc_getResultPercent();
};
} // end namespace HardMod::Std

#endif