

#ifndef BUTTON_HPP_
#define BUTTON_HPP_

#include <stdint.h>
#include <stdbool.h>
#include "StateMachine.hpp"
#include "idChar.hpp"
#include "HardMod_Event.hpp"
//#include "ButtonEvent.hpp"

#define BUTTONMODULE_THRESHOLD 4
#define BUTTONMODULE_PERIOD_mS 50

namespace HardMod::Std
{

  #include "HardMod_Event.hpp"
  #include "idChar.hpp"
  
  #define BUTTONEVENT__PRESSED 'P'
  #define BUTTONEVENT__LONGPRESS 'L'
  #define BUTTONEVENT__RELEASED 'R'
  #define BUTTONEVENT__STUCK 'S'
  
  class ButtonEvent: public HardMod::Event, public VariableIdChar
  {
    protected:
      uint8_t pressDuration;
  
    public:
      ButtonEvent();
      void setPressDuration(uint8_t pressDuration);
      uint8_t getPressDuration();
  
      //--------------------
      // Over-ridden base class methods
      uint8_t serialise(char* str) override;
  
      void clear() override;
  
      void copy(Event* copyEvent) override;
      //--------------------
  };
  
  //------------------------------------------------------------
  // ButtonConfigEvent
  
  #define BUTTONCONFIGEVENT__LONGPRESS 'L'
  #define BUTTONCONFIGEVENT__RELEASE 'R'
  
  class ButtonConfigEvent: public Event, public VariableIdChar
  {
    protected:
      uint8_t value;
      // uint8_t longPressThreshold;
      // bool enableRelease;
  
    public:
      ButtonConfigEvent();
      
      uint8_t getLongPressThreshold();
  
      bool getEnableRelease();
  
      //--------------------
      // Over-ridden base class methods
      bool deSerialise(char* str);
  
      void clear();
  
      void copy(Event* copyEvent);
      //--------------------
  
  };  

class Button : public StateMachine, public FixedIdChar
{
  public:
    enum eventTypes{
      None = 0,
      Pressed,
      LongPressed,
      Released,
      Stuck
    };

    Button(char id, uint8_t port, uint8_t pin, bool pressedPinState,
      bool releaseActive = false, uint8_t longPressThreshold = 0);
    
    void run();

    //---------------------------------------------------
    // Config interface

    void enableRelease(bool value);

    void setLongPressThreshold(uint8_t value);

    //---------------------------------------------------
    // Basic (flag) interface

    // Returns true if button pressed otherwise returns false.
    bool getPress();

    // Returns length of ButtonModule periods if button has been released,
    // otherwise returns 0.
    uint16_t getRelease();

    eventTypes getEvent(uint8_t* pressDuration);
    //---------------------------------------------------
    // Event object interface

    bool getEvent(ButtonEvent* event = nullptr);
    //---------------------------------------------------

  protected:    
    uint8_t port;
    uint8_t pin;
    //ButtonEvent *buttonEvent;
    uint8_t longPressThreshold;
    bool releaseActive;
    bool pressedPinState;
    bool eventPinState;
    bool currentPinState;
    bool previousPinState;
    bool outEventFlag;
    bool stable;
    uint8_t activeCount;
    uint8_t inActiveCount;
    uint8_t pressedCount;
    //uint16_t duration;
    uint16_t startTick;
    eventTypes eventType;

    // state methods
    uint8_t released();
    uint8_t pressed();    

    enum internalEventTypes{
      NoEvent = 0,
      Edge = 1,
      StableActive,
      StableInactive,
      StableActiveLong
    };

    internalEventTypes eventCheck();
};

} // end namespace HardMod::Std

#endif