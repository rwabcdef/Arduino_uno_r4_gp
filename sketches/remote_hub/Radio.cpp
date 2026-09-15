/*
 * Radio.cpp
 */

#include "Radio.hpp"
#include <Arduino.h>
#include <string.h>

const uint8_t Radio::DEFAULT_ADDRESS[Radio::ADDRESS_LEN] = { 'S', 'L', 'N', 'K', '1' };

Radio::Radio(uint8_t cePin, uint8_t csnPin)
  : nrf(cePin, csnPin)
{
  this->currentState  = STATE_INIT;
  this->initialised   = false;
  this->listen        = false;
  this->initAttempted = false;
  this->initAttemptMs = 0;
  this->txPending     = false;
  this->rxReady       = false;
  this->rxLen         = 0;
  this->rxSynced      = false;
  memset(this->address, 0, ADDRESS_LEN);
  memset(this->txBuffer, 0, RADIO__FRAME_LEN_MAX);
  memset(this->rxBuffer, 0, RADIO__FRAME_LEN_MAX);
  this->resetRxFrame();
}

void Radio::init(const uint8_t* address)
{
  memcpy(this->address, address, ADDRESS_LEN);

  this->currentState  = STATE_INIT;
  this->initAttempted = false;
  this->txPending     = false;
  this->rxReady       = false;

  this->resetRxFrame();
  this->rxSynced = false;

  this->initialised = true;
}

void Radio::run()
{
  switch(this->currentState)
  {
    case STATE_INIT: { this->currentState = this->initDevice(); break; }
    case STATE_IDLE: { this->currentState = this->idle(); break; }
    case STATE_RX:   { this->currentState = this->rx(); break; }
  }
}

uint8_t Radio::startListening()
{
  if(!this->initialised)
  {
    return 1;
  }
  this->listen = true;
  return 0;
}

uint8_t Radio::stopListening()
{
  if(!this->initialised)
  {
    return 1;
  }
  this->listen = false;
  return 0;
}

uint8_t Radio::write(const char* buffer)
{
  size_t len;

  if(!this->initialised || this->txPending || (buffer == nullptr))
  {
    return 1;
  }

  /* Reject rather than truncate: a frame cut short loses its '\n' and can
     never be parsed at the far end. */
  len = strnlen(buffer, RADIO__FRAME_LEN_MAX);
  if((len == 0) || (len >= RADIO__FRAME_LEN_MAX) || (buffer[len - 1] != '\n'))
  {
    return 1;
  }

  memset(this->txBuffer, 0, RADIO__FRAME_LEN_MAX);
  memcpy(this->txBuffer, buffer, len);
  this->txPending = true;
  return 0;
}

bool Radio::hasRxData(char* buffer, uint8_t* len)
{
  if(!this->rxReady)
  {
    return false;
  }

  memcpy(buffer, this->rxBuffer, this->rxLen + 1);   // + NUL
  *len = this->rxLen;
  this->rxReady = false;
  return true;
}

//----------------------------------------------------------------
// start of state methods

uint8_t Radio::initDevice()
{
  if(!this->initialised)
  {
    return STATE_INIT;
  }

  if(this->initAttempted && ((millis() - this->initAttemptMs) < INIT_RETRY_MS))
  {
    return STATE_INIT;
  }
  this->initAttempted = true;
  this->initAttemptMs = millis();

  if(!this->nrf.begin())
  {
    /* Not answering on SPI - usually wiring or power on a plug-in module,
       which can be fixed without a reset. */
    return STATE_INIT;
  }

  this->nrf.setChannel(CHANNEL);
  this->nrf.setPayloadSize(PACKET_LEN);   // before the pipes, which are sized from it
  this->nrf.openReadingPipe(1, this->address);

  return STATE_IDLE;
}

// Powered up in standby, not receiving.
uint8_t Radio::idle()
{
  if(this->txPending)
  {
    this->tx(false);
  }

  if(this->listen)
  {
    this->nrf.startListening();
    return STATE_RX;
  }

  return STATE_IDLE;
}

// Listening.
uint8_t Radio::rx()
{
  if(this->txPending)
  {
    this->tx(true);
  }

  if(!this->listen)
  {
    this->nrf.stopListening();
    this->drainRxFifo();   // deliver whatever arrived before listening stopped
    return STATE_IDLE;
  }

  this->drainRxFifo();
  return STATE_RX;
}

// end of state methods
//----------------------------------------------------------------

// Sends txBuffer split into packets. Blocks while the packets are sent.
void Radio::tx(bool listening)
{
  uint8_t packet[PACKET_LEN];
  uint8_t frameLen = (uint8_t)strnlen(this->txBuffer, RADIO__FRAME_LEN_MAX);
  uint8_t offset   = 0;

  if(listening)
  {
    this->nrf.stopListening();
  }

  /* Every time, not once: startListening() closes pipe 0, and write() needs
     pipe 0 open on the TX address to hear the auto-ack. */
  this->nrf.openWritingPipe(this->address);

  while(offset < frameLen)
  {
    uint8_t chunkLen = frameLen - offset;
    if(chunkLen > PACKET_DATA_LEN)
    {
      chunkLen = PACKET_DATA_LEN;
    }

    memset(packet, 0, PACKET_LEN);   // zero-pads the last packet
    packet[0] = (offset == 0) ? PACKET_FLAG_START : 0;
    memcpy(&packet[PACKET_HEADER_LEN], &this->txBuffer[offset], chunkLen);

    if(!this->nrf.write(packet, PACKET_LEN))
    {
      break;   // not acked: the rest of the frame is useless without this part
    }

    offset += chunkLen;
  }

  this->txPending = false;

  if(listening)
  {
    this->nrf.startListening();
    this->drainRxFifo();
  }
}

void Radio::drainRxFifo()
{
  uint8_t packet[PACKET_LEN];

  /* Bounded to the FIFO depth: with the module unplugged, SPI reads back 0
     and available() would report data forever. */
  for(uint8_t i = 0; (i < RX_FIFO_DEPTH) && this->nrf.available(); i++)
  {
    this->nrf.read(packet, PACKET_LEN);
    this->reassemble(packet);
  }
}

void Radio::reassemble(const uint8_t* packet)
{
  if((packet[0] & PACKET_FLAG_START) != 0)
  {
    /* Anything already collected belongs to a frame the sender gave up on
       part way through. */
    this->resetRxFrame();
    this->rxSynced = true;
  }

  if(!this->rxSynced)
  {
    return;   // the middle of a frame whose start was never seen
  }

  for(uint8_t i = PACKET_HEADER_LEN; i < PACKET_LEN; i++)
  {
    char c = (char)packet[i];

    if(c == '\0')
    {
      break;   // zero padding: the rest of this packet is empty
    }

    if(this->rxFrameLen >= (RADIO__FRAME_LEN_MAX - 1))
    {
      /* No '\n' within a buffer's worth: drop it, and ignore everything up to
         the next START. */
      this->resetRxFrame();
      this->rxSynced = false;
      return;
    }

    this->rxFrame[this->rxFrameLen++] = c;

    if(c == '\n')
    {
      if(!this->rxReady)
      {
        // Dropped if the previous frame has not been read yet.
        memcpy(this->rxBuffer, this->rxFrame, RADIO__FRAME_LEN_MAX);
        this->rxLen   = this->rxFrameLen;
        this->rxReady = true;
      }

      this->resetRxFrame();
      this->rxSynced = false;   // one frame per START
      return;
    }
  }
}

// Zero-filled, not just len = 0: the frame is handed out as a string.
void Radio::resetRxFrame()
{
  memset(this->rxFrame, 0, RADIO__FRAME_LEN_MAX);
  this->rxFrameLen = 0;
}
