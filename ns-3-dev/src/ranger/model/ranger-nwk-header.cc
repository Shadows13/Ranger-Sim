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
#include "ranger-nwk-header.h"

#include "ns3/assert.h"
#include "ns3/log.h"

#define IPV4_ADDRESS_SIZE 4
#define RANGER_MSG_HEADER_SIZE 6

namespace ns3
{


// ---------------- OLSR Message -------------------------------

NS_OBJECT_ENSURE_REGISTERED(MessageHeader);

MessageHeader::MessageHeader()
    : m_messageType(MessageHeader::MessageType(0))
{
}

MessageHeader::~MessageHeader()
{
}

TypeId
MessageHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::ranger::MessageHeader")
                            .SetParent<Header>()
                            .SetGroupName("Ranger")
                            .AddConstructor<MessageHeader>();
    return tid;
}

TypeId
MessageHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
MessageHeader::GetSerializedSize() const
{
    uint32_t size = RANGER_MSG_HEADER_SIZE;
    switch (m_messageType)
    {
    case NODEINFO_MESSAGE:
        size += m_message.nodeInfo.GetSerializedSize();
        break;
    case AUDIODATA_MESSAGE:
        size += m_message.audioData.GetSerializedSize();
        break;
    case MEMBERHEARTBEAT_MESSAGE:
        size += m_message.memberHeartbeat.GetSerializedSize();
        break;
    default:
        NS_ASSERT(false);
    }
    return size;
}

void
MessageHeader::Print(std::ostream& os) const
{
    switch (m_messageType)
    {
    case NODEINFO_MESSAGE:
        os << " type: NODE-INFO";
        break;
    case AUDIODATA_MESSAGE:
        os << " type: AUDIODATA";
        break;
    case MEMBERHEARTBEAT_MESSAGE:
        os << " type: MEMBERHEARTBEAT";
        break;
    }

    os << " Src: [" << m_srcAddress << "]";
    os << " Length: " << (uint16_t)m_messageLength;

    switch (m_messageType)
    {
    case NODEINFO_MESSAGE:
        m_message.nodeInfo.Print(os);
        break;
    case AUDIODATA_MESSAGE:
        m_message.audioData.Print(os);
        break;
    case MEMBERHEARTBEAT_MESSAGE:
        m_message.memberHeartbeat.Print(os);
        break;
    default:
        NS_ASSERT(false);
    }
}

void
MessageHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator iter = start;
    iter.WriteU8(m_messageType);
    iter.WriteHtonU32(m_srcAddress.Get());
    iter.WriteU8(m_messageLength);

    switch (m_messageType)
    {
    case NODEINFO_MESSAGE:
        m_message.nodeInfo.Serialize(iter);
        break;
    case AUDIODATA_MESSAGE:
        m_message.audioData.Serialize(iter);
        break;
    case MEMBERHEARTBEAT_MESSAGE:
        m_message.memberHeartbeat.Serialize(iter);
        break;
    default:
        NS_ASSERT(false);
    }
}

uint32_t
MessageHeader::Deserialize(Buffer::Iterator start)
{
    uint32_t size;
    Buffer::Iterator iter = start;
    
    m_messageType = (MessageType)iter.ReadU8();
    m_srcAddress = Ipv4Address(iter.ReadNtohU32());
    m_messageLength = iter.ReadU8();
    size = RANGER_MSG_HEADER_SIZE;
    NS_ASSERT(m_messageType == NODEINFO_MESSAGE || m_messageType == AUDIODATA_MESSAGE || m_messageType == MEMBERHEARTBEAT_MESSAGE);

    switch (m_messageType)
    {
    case NODEINFO_MESSAGE:
        size += m_message.nodeInfo.Deserialize(iter, m_messageLength - RANGER_MSG_HEADER_SIZE);
        break;
    case AUDIODATA_MESSAGE:
        size += m_message.audioData.Deserialize(iter, m_messageLength - RANGER_MSG_HEADER_SIZE);
        break;
    case MEMBERHEARTBEAT_MESSAGE:
        size += m_message.memberHeartbeat.Deserialize(iter, m_messageLength - RANGER_MSG_HEADER_SIZE);
        break;
    default:
        NS_ASSERT(false);
    }
    return size;
}


// ---------------- RANGER NODEINFO Message -------------------------------

uint32_t
MessageHeader::NodeInfo::GetSerializedSize() const
{
    uint32_t size = 1; // linkNumber For 1 Bytes
    size += linkNumber * (IPV4_ADDRESS_SIZE + 1);

    return size;
}

void
MessageHeader::NodeInfo::Print(std::ostream& os) const
{
    os << " LinkNumber: (" << (uint16_t)linkNumber << ")";
    for(auto iter = linkMessages.begin(); iter != linkMessages.end(); iter++) {
        os << " [" << iter->neighborAddresses << "-" << (uint16_t)iter->linkStatus << "]";
    }
}

void
MessageHeader::NodeInfo::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator iter = start;

    iter.WriteU8(linkNumber); 
    for (auto i = this->linkMessages.begin(); i != this->linkMessages.end(); i++)
    {
        const LinkMessage& lm = *i;

        iter.WriteU8(lm.linkStatus);
        iter.WriteHtonU32(lm.neighborAddresses.Get());
    }
}

uint32_t
MessageHeader::NodeInfo::Deserialize(Buffer::Iterator start, uint32_t messageSize)
{
    Buffer::Iterator iter = start;

    NS_ASSERT(messageSize >= 0);

    this->linkMessages.clear();

    this->linkNumber = iter.ReadU8();

    for(int i = 0; i < linkNumber; i++) {
        LinkMessage lm;
        lm.linkStatus = iter.ReadU8();
        lm.neighborAddresses = Ipv4Address(iter.ReadNtohU32());
        this->linkMessages.push_back(lm);
    }


    return messageSize;
}

// ---------------- RANGER AUDIODATA Message -------------------------------

uint32_t
MessageHeader::AudioData::GetSerializedSize() const
{
    uint32_t size = 7; // OriAddr For 4 Bytes/AudioSeq For 1 Bytes/AudioSize For 1 Bytes/AssignNum For 1 Bytes
    size += AssignNum * (IPV4_ADDRESS_SIZE);

    return size;
}

void
MessageHeader::AudioData::Print(std::ostream& os) const
{
    os<<" OriAddr: [" << OriAddr << "](" << (uint16_t)AudioSeq << "-" << (uint16_t)AudioSize << ")";
    os << " Assign: (" << (uint16_t)AssignNum << ")";
    for(auto iter = AssignNeighbor.begin(); iter != AssignNeighbor.end(); iter++) {
        os << " [" << *iter << "]";
    }
}

void
MessageHeader::AudioData::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator iter = start;
    iter.WriteHtonU32(OriAddr.Get());
    iter.WriteU8(AudioSeq); 
    iter.WriteU8(AudioSize); 
    iter.WriteU8(AssignNum); 
    for (auto i = this->AssignNeighbor.begin(); i != this->AssignNeighbor.end(); i++)
    {   
        Ipv4Address Address = *i;
        iter.WriteHtonU32(Address.Get());
    }
}

uint32_t
MessageHeader::AudioData::Deserialize(Buffer::Iterator start, uint32_t messageSize)
{
    Buffer::Iterator iter = start;

    NS_ASSERT(messageSize >= 0);

    this->AssignNeighbor.clear();

    this->OriAddr = Ipv4Address(iter.ReadNtohU32());
    this->AudioSeq = iter.ReadU8();
    this->AudioSize = iter.ReadU8();
    this->AssignNum = iter.ReadU8();

    for(int i = 0; i < AssignNum; i++) {
        this->AssignNeighbor.push_back(Ipv4Address(iter.ReadNtohU32()));
    }

    return messageSize;
}


// ---------------- RANGER MEMBERHEARTBEAT Message -------------------------------

uint32_t
MessageHeader::MemberHeartbeat::GetSerializedSize() const
{
    uint32_t size = 5; // mainAddr For 4 Bytes/roadNum For 1 Bytes
    size += roadNum * (IPV4_ADDRESS_SIZE);

    return size;
}

void
MessageHeader::MemberHeartbeat::Print(std::ostream& os) const
{
    os<<" MainAddr: [" << mainAddr << "]";
    os << " Road: (" << (uint16_t)roadNum << ")";
    for(auto iter = roadAddr.begin(); iter != roadAddr.end(); iter++) {
        os << " [" << *iter << "]";
    }
}

void
MessageHeader::MemberHeartbeat::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator iter = start;
    iter.WriteHtonU32(mainAddr.Get());
    iter.WriteU8(roadNum); 
    for (auto i = this->roadAddr.begin(); i != this->roadAddr.end(); i++)
    {   
        Ipv4Address Address = *i;
        iter.WriteHtonU32(Address.Get());
    }
}

uint32_t
MessageHeader::MemberHeartbeat::Deserialize(Buffer::Iterator start, uint32_t messageSize)
{
    Buffer::Iterator iter = start;

    NS_ASSERT(messageSize >= 0);

    this->roadAddr.clear();

    this->mainAddr = Ipv4Address(iter.ReadNtohU32());
    this->roadNum = iter.ReadU8();

    for(int i = 0; i < roadNum; i++) {
        this->roadAddr.push_back(Ipv4Address(iter.ReadNtohU32()));
    }

    return messageSize;
}

} // namespace ns3