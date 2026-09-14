/*
 * Event.hpp
 *
 *  Created on: 4 Jun 2024
 *      Author: rob
 */

#ifndef HARDMOD_EVENT_HPP_
#define HARDMOD_EVENT_HPP_
 
#include <stdint.h>
#include <stdbool.h>

#define EVENT__NONE '-' // code for: no event
 
namespace HardMod
{
 
class Event{

  protected:
    bool ack;  // used when event is sent over SerLink - does not appear in the frame payload
    char action;
    void clr();
 
  public:
 
    Event();

    // set the Event's ack bool
    void setAck(bool ack);

    // get the Event's ack bool
    bool getAck();

    // Sets the Event's action
    void setAction(char action);

    char getAction();

    virtual char getActionClear();
 
    // Convert this event to a string. Returns the string length.
    virtual uint8_t serialise(char* str);
 
    // Parses the input string and sets the variables of this event accordingly.
    virtual bool deSerialise(char* str);
 
    // Copies the variables of this event into the other event (copyEvent).
    virtual void copy(Event* copyEvent);

    // Clear this event
    virtual void clear();
 };

} // end namespace ArdMod



#endif /* ARDMOD_EVENT_HPP_ */