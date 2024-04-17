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
#ifndef RANGER_NWK_HEADER_H
#define RANGER_NWK_HEADER_H


#include "ns3/header.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"

#include <stdint.h>
#include <vector>

namespace ns3
{


/**
 * MessageHeader contain NodeInfo, AudioData
*/
class MessageHeader : public Header
{
  public:
    /**
     * Message type
     */
    enum MessageType
    {
        NODEINFO_MESSAGE = 1,
        AUDIODATA_MESSAGE = 2,
    };

    MessageHeader();
    ~MessageHeader() override;

    /**
     * Set the message type.
     * \param messageType The message type.
     */
    void SetMessageType(MessageType messageType)
    {
        m_messageType = messageType;
    }

    /**
     * Get the message type.
     * \return The message type.
     */
    MessageType GetMessageType() const
    {
        return m_messageType;
    }

    /**
     * Set the srcAddress.
     * \param srcAddress The srcAddress.
     */
    void SetSrcAddress(Ipv4Address srcAddress)
    {
        m_srcAddress = srcAddress;
    }

    /**
     * Get the srcAddress.
     * \return The srcAddress.
     */
    Ipv4Address GetSrcAddress() const
    {
        return m_srcAddress;
    }

    /**
     * Set the message length.
     * \param MessageLength The message length.
     */
    void SetMessageLength(uint8_t messageLength)
    {
        m_messageLength = messageLength;
    }

    /**
     * Get the message length.
     * \return The message length.
     */
    uint8_t GetMessageLength() const
    {
        return m_messageLength;
    }

  private:
    MessageType m_messageType;          //!< The message type
    Ipv4Address m_srcAddress;           //!< The srcAddress.
    uint8_t     m_messageLength;        //!< The message lenghth

  public:
    /**
     * \brief Get the type ID.
     * \return The object TypeId.
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

    /**
     * \ingroup ranger
     * NodeInfo Message Format
    */
    struct NodeInfo
    {
        /**
         * Link message item
         */
        struct LinkMessage
        {
            uint8_t linkStatus; //!< Link code
            Ipv4Address neighborAddresses; //!< Neighbor interface address container.
        };
        uint8_t linkNumber; //!< Link messages container.
        std::vector<LinkMessage> linkMessages; //!< Link messages container.

        /**
         * This method is used to print the content of a Hello message.
         * \param os output stream
         */
        void Print(std::ostream& os) const;
        /**
         * Returns the expected size of the header.
         * \returns the expected size of the header.
         */
        uint32_t GetSerializedSize() const;
        /**
         * This method is used by Packet::AddHeader to
         * store a header into the byte buffer of a packet.
         *
         * \param start an iterator which points to where the header should
         *        be written.
         */
        void Serialize(Buffer::Iterator start) const;
        /**
         * This method is used by Packet::RemoveHeader to
         * re-create a header from the byte buffer of a packet.
         *
         * \param start an iterator which points to where the header should
         *        read from.
         * \param messageSize the message size.
         * \returns the number of bytes read.
         */
        uint32_t Deserialize(Buffer::Iterator start, uint32_t messageSize);
    };

    /**
     * \ingroup ranger
     * NodeInfo Message Format
    */
    struct AudioData
    {
        /**
         * audio message item
         */
        Ipv4Address OriAddr;
        uint8_t AudioSeq;
        uint8_t AudioSize;
        uint8_t AssignNum; //!< Assign node messages container.
        std::vector<Ipv4Address> AssignNeighbor; //!< Assign node messages container.

        /**
         * This method is used to print the content of a Hello message.
         * \param os output stream
         */
        void Print(std::ostream& os) const;
        /**
         * Returns the expected size of the header.
         * \returns the expected size of the header.
         */
        uint32_t GetSerializedSize() const;
        /**
         * This method is used by Packet::AddHeader to
         * store a header into the byte buffer of a packet.
         *
         * \param start an iterator which points to where the header should
         *        be written.
         */
        void Serialize(Buffer::Iterator start) const;
        /**
         * This method is used by Packet::RemoveHeader to
         * re-create a header from the byte buffer of a packet.
         *
         * \param start an iterator which points to where the header should
         *        read from.
         * \param messageSize the message size.
         * \returns the number of bytes read.
         */
        uint32_t Deserialize(Buffer::Iterator start, uint32_t messageSize);
    };

  private:
    /**
     * Structure holding the message content.
     */
    struct
    {
        NodeInfo nodeInfo;     //!< NodeInfo message (optional).
        AudioData audioData;    //!< AudioData message (optional).
    } m_message;     //!< The actual message being carried.

  public:
    /**
     * Set the message type to NodeInfo and return the message content.
     * \returns The NodeInfo message.
     */
    NodeInfo& GetNodeInfo()
    {
        if (m_messageType == 0)
        {
            m_messageType = NODEINFO_MESSAGE;
        }
        else
        {
            NS_ASSERT(m_messageType == NODEINFO_MESSAGE);
        }
        return m_message.nodeInfo;
    }

    /**
     * Get the HELLO message.
     * \returns The HELLO message.
     */
    const NodeInfo& GetNodeInfo() const
    {
        NS_ASSERT(m_messageType == NODEINFO_MESSAGE);
        return m_message.nodeInfo;
    }

    /**
     * Set the message type to AudioData and return the message content.
     * \returns The AudioData message.
     */
    AudioData& GetAudioData()
    {
        if (m_messageType == 0)
        {
            m_messageType = AUDIODATA_MESSAGE;
        }
        else
        {
            NS_ASSERT(m_messageType == AUDIODATA_MESSAGE);
        }
        return m_message.audioData;
    }

    /**
     * Get the HELLO message.
     * \returns The HELLO message.
     */
    const AudioData& GetAudioData() const
    {
        NS_ASSERT(m_messageType == AUDIODATA_MESSAGE);
        return m_message.audioData;
    }

};

typedef std::vector<MessageHeader> MessageHeaderList;

inline std::ostream&
operator<<(std::ostream& os, const MessageHeaderList& messages)
{
    os << "[";
    for (auto messageIter = messages.begin(); messageIter != messages.end(); messageIter++)
    {
        messageIter->Print(os);
        if (messageIter + 1 != messages.end())
        {
            os << ", ";
        }
    }
    os << "]";
    return os;
}

} // namespace ns3

#endif /* RANGER_NWK_HEADER_H */
