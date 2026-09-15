/*
 * SerlinkRelay.hpp
 *
 * Relays SerLink traffic between pairs of Sockets, e.g. a uart socket and a
 * radio socket. Each registered pair is relayed in both directions:
 *
 *   'U' frame received by one socket -> sent by the other socket.
 *
 *   'T' frame received by one socket -> sent (as 'T') by the other socket.
 *       When the destination's ack ('A') is received, a relay ack frame
 *       ('B', Frame::TYPE_RELAY_ACK) is sent back by the source socket,
 *       containing the ack's data length (e.g. ACK_OK) & data.
 *       If no ack is received within SERLINK_RELAY__ACK_TIMEOUT_mS, no 'B'
 *       frame is sent.
 *
 * Relayed frames keep the roll code of the received frame, so the source
 * can match the 'B' frame to the 'T' frame it sent, e.g.
 *   PC    -> LED01T492002A1   (uart socket; the Reader acks it with 'A')
 *   radio -> LED01T492002A1   (radio socket)
 *   radio <- LED01A492900     (far end ack)
 *   PC    <- LED01B492900     (relay ack)
 *
 * Only one 'T' frame per pair is waited on at a time: a new 'T' frame
 * received by either socket of the pair replaces the one being waited on.
 *
 * Both sockets of a pair must have an rx frame and a tx frame. Received
 * frames of a relayed socket are consumed by the relay, so the application
 * must not also call getRxData() on it. run() must be called every main loop
 * iteration.
 */

#ifndef SERLINK_RELAY_HPP_
#define SERLINK_RELAY_HPP_

#include <stdint.h>
#include <stdbool.h>
#include "Socket.hpp"
#include "Frame.hpp"

#define SERLINK_RELAY__MAX_NUM_PAIRS 5
#define SERLINK_RELAY__ACK_TIMEOUT_mS 1500

class SerlinkRelayPair
{
  public:
    SerLink::Socket* socketA;
    SerLink::Socket* socketB;

    // 'T' frame waiting for the destination's ack
    bool ackWait;
    SerLink::Socket* ackSource; // socket the 'T' frame was received by ('B' frame is sent by this)
    SerLink::Socket* ackDest;   // socket the 'T' frame was sent by (ack is received by this)
    uint16_t ackRollCode;
    uint16_t startTick;

    SerlinkRelayPair();
};

class SerlinkRelay
{
  public:
    // workFrame: frame (with its own buffer) used to pass frames between sockets.
    SerlinkRelay(SerLink::Frame* workFrame);

    // Registers a pair of sockets to relay between.
    // Returns false if all SERLINK_RELAY__MAX_NUM_PAIRS pairs are in use.
    bool registerPair(SerLink::Socket* socketA, SerLink::Socket* socketB);

    void run();

  protected:
    SerlinkRelayPair pairs[SERLINK_RELAY__MAX_NUM_PAIRS];
    uint8_t numPairs;
    SerLink::Frame* workFrame;

    // Checks for the destination's ack of a relayed 'T' frame, and if
    // received, sends the 'B' frame.
    void checkAck(SerlinkRelayPair* pair);

    // Relays a 'U' or 'T' frame received by source, to dest.
    void relay(SerlinkRelayPair* pair, SerLink::Socket* source, SerLink::Socket* dest);
};

#endif /* SERLINK_RELAY_HPP_ */
