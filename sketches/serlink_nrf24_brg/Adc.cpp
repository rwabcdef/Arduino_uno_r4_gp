#include "Adc.hpp"

namespace HardMod::Std
{
Adc::Adc(AdcInput** inputs, uint8_t numInputs, HwModule::AdcPrescalerValues psValue)
: adcInput(inputs), numInputs(numInputs), currentInput(0), psValue(psValue)
{
  for(uint8_t i = 0; i < numInputs; i++)
  {
    adcInput[i]->percent = 0;
    adcInput[i]->newValueFlag = false;
  }

  HwModule::Adc_init(this->psValue);
  HwModule::Adc_setInput(adcInput[0]->input);
  HwModule::Adc_startConversion();
}

void Adc::run(){
  if(HwModule::Adc_isConversionComplete())
  {
    uint8_t percent = HwModule::Adc_getResultPercent();

    adcInput[currentInput]->percent = percent;
    adcInput[currentInput]->newValueFlag = true;

    currentInput++;
    if(currentInput >= numInputs)
    {
      currentInput = 0;
    }

    HwModule::Adc_setInput(adcInput[currentInput]->input);
    HwModule::Adc_startConversion();
  }
}

bool Adc::getValue(uint8_t inputIndex, uint8_t* percent)
{
  if(inputIndex >= numInputs)
  {
    return false;
  }

  if(adcInput[inputIndex]->newValueFlag)
  {
    *percent = adcInput[inputIndex]->percent;
    adcInput[inputIndex]->newValueFlag = false;
    return true;
  }
  else
  {
    return false;
  }

}

} // end namespace HardMod::Std
