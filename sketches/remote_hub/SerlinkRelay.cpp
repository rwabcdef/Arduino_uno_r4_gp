/*
 * SerlinkRelay.cpp
 */

#include "SerlinkRelay.hpp"
#include "swTimer.h"

using namespace SerLink;

SerlinkRelayPair::SerlinkRelayPair()
{
  this->socketA = nullptr;
  this->socketB = nullptr;
  this->ackWait = false;
  this->ackSource = nullptr;
  this->ackDest = nullptr;
  this->ackRollCode = 0;
  this->startTick = 0;
}

SerlinkRelay::SerlinkRelay(Frame* workFrame) : workFrame(workFrame)
{
  this->numPairs = 0;
}

bool SerlinkRelay::registerPair(Socket* socketA, Socket* socketB)
{
  if((socketA == nullptr) || (socketB == nullptr))
  {
    return false;
  }

  if(this->numPairs >= SERLINK_RELAY__MAX_NUM_PAIRS)
  {
    // No more pairs are available
    return false;
  }

  SerlinkRelayPair* pair = &this->pairs[this->numPairs];
  pair->socketA = socketA;
  pair->socketB = socketB;
  pair->ackWait = false;
  this->numPairs++;
  return true;
}

void SerlinkRelay::run()
{
  for(uint8_t i=0; i<this->numPairs; i++)
  {
    SerlinkRelayPair* pair = &this->pairs[i];

    this->checkAck(pair);

    this->relay(pair, pair->socketA, pair->socketB);
    this->relay(pair, pair->socketB, pair->socketA);
  }
}

void SerlinkRelay::checkAck(SerlinkRelayPair* pair)
{
  if(!pair->ackWait)
  {
    return;
  }

  if(pair->ackDest->getAckFrame(this->workFrame) &&
    (this->workFrame->rollCode == pair->ackRollCode))
  {
    // Destination has acked the relayed 'T' frame - so send the relay ack
    // back to the source. The ack's data length (e.g. ACK_OK) & data are kept.
    pair->ackWait = false;
    this->workFrame->type = Frame::TYPE_RELAY_ACK;
    this->workFrame->rollCode = pair->ackRollCode;
    pair->ackSource->sendFrame(this->workFrame);
    return;
  }

  if(swTimer_tickCheckTimeoutNoReset(&pair->startTick, SERLINK_RELAY__ACK_TIMEOUT_mS))
  {
    // No ack from destination - so give up (no relay ack is sent).
    pair->ackWait = false;
  }
}

void SerlinkRelay::relay(SerlinkRelayPair* pair, Socket* source, Socket* dest)
{
  if(!source->getRxFrame(this->workFrame))
  {
    return;
  }

  if(this->workFrame->type == Frame::TYPE_UNIDIRECTION)
  {
    dest->sendFrame(this->workFrame);
  }
  else if(this->workFrame->type == Frame::TYPE_TRANSMISSION)
  {
    if(dest->sendFrame(this->workFrame))
    {
      pair->ackWait = true;
      pair->ackSource = source;
      pair->ackDest = dest;
      pair->ackRollCode = this->workFrame->rollCode;
      swTimer_tickReset(&pair->startTick);

      // Discard any previous ack received by dest, so only the ack of this
      // frame can complete the wait.
      dest->getAckFrame(this->workFrame);
    }
  }
  else
  {
    // Not a relayed frame type - so do nothing
  }
}
