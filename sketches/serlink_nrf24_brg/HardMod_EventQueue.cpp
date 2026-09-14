
#include "HardMod_EventQueue.hpp"

namespace HardMod
{
EventQueue::EventQueue(Event** events, uint8_t len): events(events), len(len)
{
  this->head = 0;
  this->tail = 0;
}

uint8_t EventQueue::nextHead()
{
  if(this->head >= (this->len - 1)){
    return 0;
  } else {
    return this->head + 1;
  }
}
uint8_t EventQueue::nextTail()
{
  if(this->tail >= (this->len - 1)){
    return 0;
  } else {
    return this->tail + 1;
  }
}


bool EventQueue::isFull()
{
  if(nextHead() == this->tail){
    return true;
  } else {
    return false;
  }
}
bool EventQueue::isEmpty()
{
  if(this->head == this->tail){
    return true;
  } else {
    return false;
  }
}
bool EventQueue::put(Event* event)
{
  if(this->isFull()){
    return false;
  }
  // Copy input event variables into the event pointed to by this queue`s head
  event->copy(this->events[this->head]);
  this->head = this->nextHead();
  return true;
}
bool EventQueue::get(Event* event)
{
  if(this->isEmpty()){
    return false;
  }

  // Copy variables from the event pointed to by this queue`s tail into the output event
  this->events[this->tail]->copy(event);
  this->tail = this->nextTail();
  return true;
}

}  