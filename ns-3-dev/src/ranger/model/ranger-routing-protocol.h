/*
 * Copyright (c) 2011 The Boeing Company
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author:
 *  Gary Pei <guangyu.pei@boeing.com>
 *  Sascha Alexander Jopen <jopen@cs.uni-bonn.de>
 */
#ifndef RANGER_ROUTING_PROTOCOL_H
#define RANGER_ROUTING_PROTOCOL_H

#include "ranger-mac.h"
#include "ranger-nwk-header.h"
#include "ranger-routing-nblist.h"
#include "ranger-audio-management.h"

#include <ns3/lr-wpan-module.h>

#include <ns3/object.h>
#include "ns3/ipv4.h"
#include "ns3/node.h"
#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/timer.h"
#include "ns3/traced-callback.h"

namespace ns3
{

typedef Callback<void, ranger::McpsDataRequestParams, Ptr<Packet>> RangerRoutingProtocolSendCallback;
typedef Callback<void, Ipv4Address, Ipv4Address, uint8_t, Time> RangerRoutingProtocolReceiveTraceCallback;
typedef Callback<void, Ipv4Address, Ipv4Address, uint8_t, Time> RangerRoutingProtocolSendTraceCallback;


class RangerRoutingProtocol : public Object
{
  public:
    RangerRoutingProtocol();
    ~RangerRoutingProtocol() override;

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

  protected:
    void DoInitialize() override;
    void DoDispose() override;

  public:
    /**
     * 
     * Properties of ranger routing protocol
     * 
    */
    
    /**
     * \brief Set the RANGER main address to the first address on the indicated interface.
     *
     * \param mainAddress IPv4 interface index
     */
    void SetMainAddress(Ipv4Address mainAddress);

    /**
     * \brief Print the hold neighbor list.
     *
     * \param os std::ostream
     */
    void PrintNeighborList(std::ostream& os);

    /**
     * Set the MAC to be used by this NetDevice.
     *
     * \param mac the MAC to be used
     */
    void SetMac(Ptr<RangerMac> mac);

    /**
     * Get the MAC used by this NetDevice.
     *
     * \return the MAC object
     */
    Ptr<RangerMac> GetMac() const;


  private:
    RangerNeighborList m_nbList; // neighbor management
    Ipv4Address m_mainAddr;
    RangerAudioManagement m_audioManagement;

    Ptr<RangerMac> m_mac;

    // for record
    // RangerRecorder m_record;

  public:
    /**
     * 
     * Send/Receive Management
     * 
    */
    void ReceivePacket(ranger::McpsDataIndicationParams receiveParams, Ptr<Packet> p);  // receive packet
    void SendPacket(ranger::McpsDataRequestParams sendParams, Ptr<Packet> p);     // send packet
    void SetSendCallback(RangerRoutingProtocolSendCallback cb);        // set the send callback

    void SourceAudioDataRequest(const uint32_t audioLen);        // send audio data request
    void ForwardAudioDataRequest(const MessageHeader& OriMessageHdr);        // send audio data request

    bool isForwardNode(const MessageHeader::AudioData& assignHdr);       // check if the node is a forward node
    bool isForwardJudge_Hivemesh(const MessageHeader& hdr);
  private:
    // A list of pending messages which are buffered awaiting for being sent.
    MessageHeaderList m_queuedMessages;
    Time m_queuedMessagesInterval;
    Timer m_queuedMessagesTimer; //!< timer for throttling outgoing messages

    /**
     * \brief Sends a message from the queuedMessages list.
     */
    void QueuedMessagesTimerExpire();     //!< timer callback for sending queued messages
    void SendQueuedMessages();            //!< send all queued messages
    void EnqueueMessage(const MessageHeader& messageHdr, Time delay);   //!< enqueue a message for sending


    RangerRoutingProtocolSendCallback m_sendCallback;
    RangerRoutingProtocolReceiveTraceCallback m_receiveTraceCallback;
    RangerRoutingProtocolSendTraceCallback m_sendTraceCallback;

  public:
    void SetReceiveTraceCallback(RangerRoutingProtocolReceiveTraceCallback cb);
    void SetSendTraceCallback(RangerRoutingProtocolSendTraceCallback cb);

  private:
    /**
     * \brief Sends a NodeInfo message and reschedules the NodeInfo timer.
     */
    Time m_nodeInfoInterval;
    Timer m_nodeInfoTimer;
    void NodeInfoTimerExpire();
    void SendNodeInfo();

    // For Testing
    //Ptr<Packet> m_txPacket;
};



} // namespace ns3

#endif /* RANGER_ROUTING_PROTOCOL_H */
