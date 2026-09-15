#include "Led.hpp"
#include "swTimer.h"
#include "hw_gpio.h"
#include "SerLink_Utils.hpp"

namespace HardMod::Std
{

LedEvent::LedEvent(): Event()
{
  this->type = None;
}

LedEvent::eventTypes LedEvent::getType(LedFlashParams* flashParams)
{
  if(flashParams != nullptr && this->type == Flash)
  {
    flashParams->numFlashes = this->numFlashes;
    flashParams->onPeriods = this->onPeriods;
    flashParams->offPeriods = this->offPeriods;
    flashParams->finalFlashOff = this->finalFlashOff;
  }
  return this->type;
}

bool LedEvent::deSerialise(char* str)
{
  this->clr();  // base class clr()

  this->setId(str[0]);
  this->setAction(str[1]);

  switch(this->action)
  {
    case LEDEVENT__ON:
      this->type = On;
      break;
    case LEDEVENT__OFF:
      this->type = Off;
      break;
    case LEDEVENT__FLASH:
      this->type = Flash;
      this->numFlashes = SerLink::Utils::strToUint8(&str[2], 2);
      this->onPeriods = SerLink::Utils::strToUint8(&str[4], 2);
      this->offPeriods = SerLink::Utils::strToUint8(&str[6], 2);
      if(str[8] == '1')
      {
        this->finalFlashOff = true;
      }
      else
      {
        this->finalFlashOff = false;
      }
      break;
    case LEDEVENT__FLASH_SET_END_EVENT:
      this->type = FlashEndEnable;
      break;
    case LEDEVENT__FLASH_CLR_END_EVENT:
      this->type = FlashEndDisable;
      break;
    default:
      this->type = None;
      return false;
  }
  return true;
}

uint8_t LedEvent::serialise(char* str)
{
  uint8_t index = 0;
  
  str[index++] = this->getId();
  str[index++] = this->action;

  return index;
}

void LedEvent::clear()
{
  this->clr();         // base class clr()
  this->type = None;
  this->numFlashes = 0;
  this->onPeriods = 0;
  this->offPeriods = 0;
}

void LedEvent::copy(Event* copyEvent)
{
  LedEvent* copy = (LedEvent*) copyEvent;
  copy->setAck(this->ack);
  copy->setId(this->getId());
  copy->setAction(this->action);
  copy->type = this->type;
  copy->numFlashes = this->numFlashes;
  copy->onPeriods = this->onPeriods;
  copy->offPeriods = this->offPeriods;
}
//-----------------------------------------------------------------------------------

#define STATE_OFF 0
#define STATE_ON 1
#define STATE_FLASH_ON 2
#define STATE_FLASH_OFF 3

Led::Led(char id, uint8_t port, uint8_t pin, bool flashEndEventEnabled)
: FixedIdChar(id), port(port), pin(pin), flashEndEventEnabled(flashEndEventEnabled),
  numFlashes(0), onPeriods(0), offPeriods(0)
{
  this->type = LedEvent::None;
  this->currentState = STATE_OFF;
  this->flashEndFlag = false;
  gpio_setPinDirection(this->port, this->pin, GPIO_PIN_DIRECTION__OUT);
  swTimer_tickReset(&this->startTick);
}

void Led::on()
{
  this->type = LedEvent::On;
  //gpio_setPinState(this->port, this->pin, GPIO_PIN_STATE__HIGH);
}

void Led::off()
{
  this->type = LedEvent::Off;
}

void Led::flash(uint8_t numFlashes, uint8_t onPeriods, uint8_t offPeriods, bool finalFlashOff)
{
  this->type = LedEvent::Flash;
  this->numFlashes = numFlashes;
  this->onPeriods = onPeriods;
  this->offPeriods = offPeriods;
  this->finalFlashOff = finalFlashOff;
  //swTimer_tickReset(&this->startTick);
}

void Led::setFlashEndEventEnabled(bool enabled)
{
  this->flashEndEventEnabled = enabled;
}

bool Led::getFlashEnd()
{
  if(this->flashEndFlag)
  {
    this->flashEndFlag = false;
    return true;
  }
  return false;
}

bool Led::getEvent(LedEvent* event)
{
  if(this->flashEndFlag)
  {
    this->flashEndFlag = false;
    if(event != nullptr)
    {
      event->setId(this->getId());
      event->setAction(LEDEVENT__FLASH_END);
    }
    return true;
  }
  return false;
}

void Led::run()
{
  if(swTimer_tickCheckTimeout(&this->startTick, LED_PERIOD_mS))
  {
    this->common();
    switch(this->currentState)
    {
      case STATE_OFF:
        this->currentState = this->offState();
        break;
      case STATE_ON:
        this->currentState = this->onState();
        break;
      case STATE_FLASH_ON:
        this->currentState = this->flashOnState();
        break;
      case STATE_FLASH_OFF:
        this->currentState = this->flashOffState();
        break;
    }
  }
}

void Led::common()
{
  if(this->type == LedEvent::On)
  {
    this->currentState = STATE_ON;
    gpio_setPinHigh(this->port, this->pin);
  }
  else if(this->type == LedEvent::Off)
  {
    this->currentState = STATE_OFF;
    gpio_setPinLow(this->port, this->pin);
  }
  else if(this->type == LedEvent::Flash)
  {
    this->currentState = STATE_FLASH_ON;
    this->flashCount = 0;
    this->periodCount = 0;
    gpio_setPinHigh(this->port, this->pin);
  }
  this->type = LedEvent::None; // clear event type
}

uint8_t Led::onState(){
  return STATE_ON;
}

uint8_t Led::offState(){
  return STATE_OFF;
}

uint8_t Led::flashOnState(){
  //this->periodCount++;
  if(this->periodCount >= this->onPeriods)
  {
    this->periodCount = 0;

    if(this->finalFlashOff == true)
    {
      // The sequence of flashes will end with the LED on, so we need to check
      // if we are at the end of the flash sequence before turning off the LED

      uint8_t count = this->flashCount;
      if(++count >= this->numFlashes && this->numFlashes != 0)
      {

        // Final flash has just ended - so end of flash sequence
        gpio_setPinLow(this->port, this->pin);

        if(this->flashEndEventEnabled)
        {
          this->flashEndFlag = true;
        }

        return STATE_OFF;
      }
    }

    gpio_setPinLow(this->port, this->pin);
    return STATE_FLASH_OFF;
  }
  else
  {
    this->periodCount++;
    return STATE_FLASH_ON;
  }
}

uint8_t Led::flashOffState(){
  if(this->periodCount >= this->offPeriods)
  {
    this->periodCount = 0;
    this->flashCount++;
    if(this->numFlashes == 0)
    {
      // special case - infinite flashes
      gpio_setPinHigh(this->port, this->pin);
      return STATE_FLASH_ON;
    }
    if(this->flashCount >= this->numFlashes)
    {
      // End of flash sequence
      gpio_setPinLow(this->port, this->pin);

      if(this->flashEndEventEnabled)
      {
        this->flashEndFlag = true;
      }

      return STATE_OFF;
    }
    else
    {
      gpio_setPinHigh(this->port, this->pin);
      return STATE_FLASH_ON;
    }
  }
  else
  {
    this->periodCount++;
    return STATE_FLASH_OFF;
  }
}

void LedUtils::setLedEvent(Led* led, LedEvent::eventTypes type, LedFlashParams* flashParams)
{
  switch(type)
  {
    case LedEvent::On:
      led->on();
      break;
    case LedEvent::Off:
      led->off();
      break;
    case LedEvent::Flash:
      if(flashParams != nullptr)
      {
        led->flash(flashParams->numFlashes, flashParams->onPeriods, flashParams->offPeriods, flashParams->finalFlashOff);
      }
      break;
    case LedEvent::FlashEndEnable:
      led->setFlashEndEventEnabled(true);
      break;
    case LedEvent::FlashEndDisable:
      led->setFlashEndEventEnabled(false);
      break;
    default:
      // do nothing
      break;
  }
}

} // namespace HardMod::Std