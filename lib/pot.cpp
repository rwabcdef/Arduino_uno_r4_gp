
#include "pot.hpp"
#include "SerLink_Utils.hpp"
#include "swTimer.h"

namespace HardMod::Std
{

PotEvent::PotEvent() : Event(), VariableIdChar()
{
    this->percent = 0;
}

void PotEvent::setPercent(uint8_t value)
{
    this->percent = value;
}

uint8_t PotEvent::getPercent()
{
    return this->percent;
}
uint8_t PotEvent::serialise(char* str)
{
    uint8_t index = 0;

    str[index++] = this->getId();
    //str[index++] = this->action; // ignore action

    SerLink::Utils::uint16ToStr((uint8_t)this->percent, &str[index], 3);

    index = index + 3;

    return index;
}

void PotEvent::clear()
{
    this->clr(); // base class clr()
    this->percent = 0;
}
void PotEvent::copy(Event* copyEvent)
{
    PotEvent* copy = (PotEvent*)copyEvent;
    copy->setAck(this->ack);
    copy->setId(this->getId());
    copy->setAction(this->action);
    copy->setPercent(this->percent);
}

//------------------------------------------------------------
Pot::Pot(char id, Adc* adc, uint8_t adcIndex) : FixedIdChar(id), adc(adc), adcIndex(adcIndex)
{
    this->bufferIndex = 0;
    this->buffer[0] = 0;
    this->buffer[1] = 0;
    this->percent = -1;
    this->initialState = true;
    swTimer_tickReset(&this->startTick);
}

bool Pot::getEvent(PotEvent* event)
{
    if (this->percent == -1)
    {
        return false;
    }

    event->setId(this->getId());
    event->setAction(POTEVENT__CHANGE); // Pot change action
    event->setPercent((uint8_t)this->percent);

    this->percent = -1; // reset percent to indicate event has been read

    return true;
}

int8_t Pot::getPercent()
{
    return this->percent;
}

void Pot::run()
{
  this->percent = -1; // reset percent to indicate no new event - default value

  if(swTimer_tickCheckTimeout(&this->startTick, 250))
  {
    // read the ADC value
    uint8_t current;

    bool newValue = this->adc->getValue(this->adcIndex, &current);
    if (!newValue){
      return;
    }

    if(this->initialState)
    {
      this->buffer[this->bufferIndex++] = current;
      if(this->bufferIndex >= 2)
      {
        this->initialState = false;
        this->bufferIndex = 0;
      }
    }
    
    uint8_t previous = (this->buffer[0] + this->buffer[1]) >> 1; // average of last two readings

    // store in buffer
    this->buffer[this->bufferIndex] = current;
    this->bufferIndex = (this->bufferIndex + 1) % 2;
    
    current = (this->buffer[0] + this->buffer[1]) >> 1; // average of last two readings
    if (current != previous)
    {
        this->percent = current;
        if(this->percent > 1)
        {
            this->percent--;
        }
    }
  }
}
} // end namespace HardMod::Std