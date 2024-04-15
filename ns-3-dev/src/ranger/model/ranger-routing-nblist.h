/*
 * Copyright (c) 2004 Francisco J. Ros
 * Copyright (c) 2007 INESC Porto
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
 * Authors: Francisco J. Ros  <fjrm@dif.um.es>
 *          Gustavo J. A. M. Carneiro <gjc@inescporto.pt>
 */

#ifndef RANGER_ROUTING_NEIGHBOR_LIST_H
#define RANGER_ROUTING_NEIGHBOR_LIST_H

#include "ranger-nwk-header.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"

#include <iostream>
#include <set>
#include <vector>

namespace ns3
{

#define MAX_NODEINFO_RECEIVE_RATE_BUFFER (8)
class NodeInfoReceiveRateBuffer {
private:
    std::vector<bool> buffer;
    int head = 0;  // 指向下一个插入位置
    bool isFull = false;
    int capacity;

public:
    explicit NodeInfoReceiveRateBuffer(int capacity) : buffer(capacity, 0), capacity(capacity) {}

    void insert(bool isReceive) {
        buffer[head] = isReceive;
        head = (head + 1) % capacity;

        // 检查是否第一次填满
        if (head == 0 && !isFull) {
            isFull = true;
        }
    }
    bool isBufferFull() const {
        return isFull;
    }
    uint8_t calLqi() const; 
};


struct NeighborStatus
{
    Ipv4Address neighborMainAddr;

    enum Status
    {
        STATUS_NONE = 0,
        STATUS_UNSTABLE = 1,
        STATUS_STABLE = 2,
    };

    Status status;
    uint8_t lqi;
    Time refreshTime;
    NodeInfoReceiveRateBuffer lqiBuffer;
    // from
    std::vector<MessageHeader::NodeInfo::LinkMessage> twoHopNodeInfo;

    explicit NeighborStatus() : lqiBuffer(MAX_NODEINFO_RECEIVE_RATE_BUFFER) {}


    /**
     * Find the target Address. If it exits return <bool>true & put it index into <param>index
     * \param TargetAddr target address.
     * \param index target address's index in m_nbStatus.
     */    
    bool FindTwoHopNeighbor(Ipv4Address TargetAddr, uint8_t& index) {
        for(std::size_t i = 0; i < twoHopNodeInfo.size(); i++) {
            if (twoHopNodeInfo[i].neighborAddresses == TargetAddr)
            {
                index = i;
                return true;
            }
        }
        return false;
    }
    /**
     * This method is used to print the content of m_nbStatus.
     * \param os output stream
     */
    void Print(std::ostream& os) const {
        for(auto i = 0u; i < twoHopNodeInfo.size(); i++) {
            os << " [" << twoHopNodeInfo[i].neighborAddresses << "]";
            os << "(" << (uint16_t)twoHopNodeInfo[i].linkQuality << ")";
        }
        os << std::endl;
    }
};

inline bool
operator==(const NeighborStatus& a, const NeighborStatus& b)
{
    return (a.neighborMainAddr == b.neighborMainAddr);
}

inline std::ostream&
operator<<(std::ostream& os, const NeighborStatus& tuple)
{
    // os << "NeighborTuple(neighborMainAddr=" << tuple.neighborMainAddr
    //    << ", status=" << (tuple.status == NeighborTuple::STATUS_SYM ? "SYM" : "NOT_SYM")
    //    << ", willingness=" << tuple.willingness << ")";
    return os;
}


class RangerNeighborList
{
  public:
    RangerNeighborList(Time refreshInterval);
    ~RangerNeighborList();

  
  private:

    Ipv4Address m_mainAddr;
    std::vector<NeighborStatus> m_nbStatus;
    // to judge whethe nodeinfo packet arrive?
    Time m_refreshInterval;

    // manage neighbor;
  public:
    // self management
    /**
     * Receive a NodeInfo Packet & Update the Neighbor Status.
     * \param nodeinfoHdr the nodeinfo header.
     */
    void UpdateNeighborNodeStatus(Ipv4Address SrcAddress, MessageHeader::NodeInfo nodeinfoHdr);
    /**
     * Judge whether receive the nodeinfo Packet in last interval 
     *  & calculate the lqi & remove unstable node
     * \param nodeinfoHdr the nodeinfo header.
     */
    void RefreshNeighborNodeStatus(void);

    // information request
    /**
     * Find the target Address. If it exits return <bool>true & put it index into <param>index
     * \param TargetAddr target address.
     * \param index target address's index in m_nbStatus.
     */
    bool FindNeighbor(Ipv4Address TargetAddr, uint8_t& index);
    /**
     * Pick up the assign forward node for a forward packet and edit the header
     * \param SrcAddress prev node address.
     * \param header the packet header.
     */
    void SetForwardAssignNeighbor(Ipv4Address SrcAddress, MessageHeader& header);
    /**
     * Pick up the assign forward node for a source packet and edit the header
     * \param header the packet header.
     */
    void SetSourceAssignNeighbor(MessageHeader& header);

    // 
    /**
     * This method is used to print the content of m_nbStatus.
     * \param os output stream
     */
    void Print(std::ostream& os) const;
    void SetMainAddress(Ipv4Address Address) {
        m_mainAddr = Address;
    }
};

typedef std::vector<Ipv4Address> AssignNeighbor;

} // namespace ns3

#endif /* RANGER_ROUTING_NEIGHBOR_LIST_H */
