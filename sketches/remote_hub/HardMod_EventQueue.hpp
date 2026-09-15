#ifndef HARDMOD_EVENTQUEUE_HPP_
#define HARDMOD_EVENTQUEUE_HPP_

#include "HardMod_Event.hpp"
#include <stdint.h>
#include <stdbool.h>

namespace HardMod
{
class EventQueue
{
  protected:
    Event** events;
    uint8_t len, head, tail;
    uint8_t nextHead();
    uint8_t nextTail();

  public:
    EventQueue(Event** events, uint8_t len);
    bool isFull();
    bool isEmpty();
    bool put(Event* event);
    bool get(Event* event);
};

} // end namespace HardMod

#endif