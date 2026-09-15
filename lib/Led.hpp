#ifndef LED_HPP
#define LED_HPP

#include <stdint.h>
#include <stdbool.h>
#include "StateMachine.hpp"
#include "idChar.hpp"
#include "HardMod_Event.hpp"

#define LED_PERIOD_mS 250

#define LEDEVENT__ON '1' // input event - led on
#define LEDEVENT__OFF '0' // input event - led off
#define LEDEVENT__FLASH 'F' // input event - led flash
#define LEDEVENT__FLASH_SET_END_EVENT 'S' // input event - set (i.e. enable) flash end event
#define LEDEVENT__FLASH_CLR_END_EVENT 'C' // input event - clear (i.e. disable) flash end event
#define LEDEVENT__FLASH_END 'E' // output event - indicates end of flash sequence

namespace HardMod::Std
{

class LedFlashParams{
  public:
    uint8_t numFlashes;
    uint8_t onPeriods;
    uint8_t offPeriods;
    bool finalFlashOff; // if true, the final flash will end with the LED off. If false, the final flash will end with the LED on.
};

class LedEvent: public Event, public VariableIdChar {
  
  public:
    // Input event types
    enum eventTypes {
      None = 0,
      On,
      Off,
      Flash,
      FlashEndEnable,
      FlashEndDisable
    };

    LedEvent();
    eventTypes getType(LedFlashParams* flashParams = nullptr);

    //--------------------
    // Over-ridden base class methods

    bool deSerialise(char* str) override;

    uint8_t serialise(char* str) override;

    void clear() override;

    void copy(Event* copyEvent) override;
    //--------------------
  protected:
    eventTypes type;
    uint8_t numFlashes;
    uint8_t onPeriods;
    uint8_t offPeriods;
    bool finalFlashOff;
};
//------------------------------------------------------------------------------------------------------

class Led : public StateMachine, public FixedIdChar {
  
  public:
    Led(char id, uint8_t port, uint8_t pin, bool flashEndEventEnabled = false);
    void on();
    void off();
    void flash(uint8_t numFlashes, uint8_t onPeriods, uint8_t offPeriods, bool finalFlashOff);
    void run();
    void setFlashEndEventEnabled(bool enabled);

    //---------------------------------------------------
    // Basic (flag) interface
    bool getFlashEnd();

    //---------------------------------------------------
    // Event object interface
    bool getEvent(LedEvent* event = nullptr);
    //---------------------------------------------------

  protected:
    uint8_t port;
    uint8_t pin;
    LedEvent::eventTypes type;
    uint8_t numFlashes;
    uint8_t flashCount;
    int8_t periodCount;
    uint8_t onPeriods;
    uint8_t offPeriods;
    bool finalFlashOff;
    uint16_t startTick;
    bool flashEndFlag;
    bool flashEndEventEnabled;

    uint8_t onState();
    uint8_t offState();
    uint8_t flashOnState();
    uint8_t flashOffState();
    void common();
}; // End class Led

//------------------------------------------------------------------------------------------------------

class LedUtils {
  public:
    static void setLedEvent(Led* led, LedEvent::eventTypes type, LedFlashParams* flashParams = nullptr);
};

} // namespace HardMod::Std


#endif