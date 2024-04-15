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

#include "ranger-nwk-header.h"
#include "ranger-routing-nblist.h"


#include <ns3/object.h>
#include "ns3/ipv4.h"
#include "ns3/node.h"
#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/timer.h"
#include "ns3/traced-callback.h"

namespace ns3
{
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
    void SetMainIAddress(uint32_t mainAddress);

    /**
     * \brief Print the hold neighbor list.
     *
     * \param os std::ostream
     */
    void PrintNeighborList(std::ostream& os);

  private:
    RangerNeighborList m_nbList; // neighbor management
    Ipv4Address m_mainAddr;

    /**
     * 
     * Send/Receive Management
     * 
    */
  public:
    
  private:
    Time m_NodeInfoInterval;
    Timer m_NodeInfoTimer;
    /**
     * \brief Sends a NodeInfo message and reschedules the HELLO timer.
     */
    void HelloTimerExpire();



};




} // namespace ns3

#endif /* RANGER_ROUTING_PROTOCOL_H */
