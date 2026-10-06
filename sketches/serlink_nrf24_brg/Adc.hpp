#ifndef ADC_HPP_
#define ADC_HPP_

#include <stdint.h>
#include <stdbool.h>
#include "HwModule.hpp"

namespace HardMod::Std
{
class AdcInput {
  public:
    HwModule::AdcInputValues input;
    uint8_t percent; // 0 - 100%
    bool newValueFlag;

    AdcInput(HwModule::AdcInputValues input) : input(input), percent(0), newValueFlag(false) {}
};

class Adc {
  protected:
    AdcInput** adcInput;
    uint8_t numInputs;
    uint8_t currentInput;
    HwModule::AdcPrescalerValues psValue;

  public:
    Adc(AdcInput** inputs, uint8_t numInputs, HwModule::AdcPrescalerValues psValue = HwModule::PS_128);
    void run();
    bool getValue(uint8_t inputIndex, uint8_t* percent);
};

} // end namespace HardMod::Std

#endif