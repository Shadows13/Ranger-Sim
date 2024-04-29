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
#include "ranger-routing-protocol.h"
#include <ns3/assert.h>
#include <ns3/core-module.h>

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("RangerRoutingProtocol");
NS_OBJECT_ENSURE_REGISTERED(RangerRoutingProtocol);

TypeId
RangerRoutingProtocol::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::RangerRoutingProtocol")
            .SetParent<Object>()
            .SetGroupName("Ranger")
            .AddConstructor<RangerRoutingProtocol>()
            // .AddAttribute("NodeInfoInterval",
            //               "NodeInfo messages emission interval.",
            //               TimeValue(Seconds(1)),
            //               MakeTimeAccessor(&RangerRoutingProtocol::m_nodeInfoInterval),
            //               MakeTimeChecker())
            ;
    return tid;
}

RangerRoutingProtocol::RangerRoutingProtocol()
    : m_nbList(Seconds(1.0)),
      m_audioManagement(),
      m_queuedMessagesTimer(Timer::CANCEL_ON_DESTROY),
      m_nodeInfoTimer(Timer::CANCEL_ON_DESTROY)
{
    NS_LOG_FUNCTION(this);
    m_queuedMessagesInterval = MilliSeconds(1.0);
    m_nodeInfoInterval = Seconds(1.0);
}

RangerRoutingProtocol::~RangerRoutingProtocol()
{
}

void
RangerRoutingProtocol::DoInitialize()
{
    NS_LOG_FUNCTION(this);
    Ptr<UniformRandomVariable> randomTime = CreateObject<UniformRandomVariable>();
    randomTime->SetAttribute("Min", DoubleValue(0.0));
    randomTime->SetAttribute("Max", DoubleValue(1.0)); // 最大值设为1秒

    m_queuedMessagesTimer.SetFunction(&RangerRoutingProtocol::QueuedMessagesTimerExpire, this);
    m_queuedMessagesTimer.Schedule(MilliSeconds(1.0 + randomTime->GetValue()));

    m_nodeInfoTimer.SetFunction(&RangerRoutingProtocol::NodeInfoTimerExpire, this);
    m_nodeInfoTimer.Schedule(Seconds(1.0 + randomTime->GetValue()));
}
void
RangerRoutingProtocol::DoDispose()
{

}

void
RangerRoutingProtocol::SetMainAddress(Ipv4Address mainAddress) {
    m_mainAddr = mainAddress;
    m_nbList.SetMainAddress(mainAddress);
    m_audioManagement.setMainAddr(mainAddress);
}

void
RangerRoutingProtocol::PrintNeighborList(std::ostream& os) {
    m_nbList.Print(os);
}

Ptr<RangerMac>
RangerRoutingProtocol::GetMac() const
{
    NS_LOG_FUNCTION(this);
    return m_mac;
}

void
RangerRoutingProtocol::SetMac(Ptr<RangerMac> mac)
{
    NS_LOG_FUNCTION(this);
    m_mac = mac;
    this->SetSendCallback(MakeCallback(&RangerMac::McpsDataRequest, m_mac));
    m_mac->SetMcpsDataIndicationCallback(MakeCallback(&RangerRoutingProtocol::ReceivePacket, this));
}

void
RangerRoutingProtocol::SendPacket(ranger::McpsDataRequestParams sendParams, Ptr<Packet> p) {
    NS_LOG_FUNCTION(this);
    NS_ASSERT(!m_sendCallback.IsNull());
    m_sendCallback(sendParams, p);
}

void
RangerRoutingProtocol::ReceivePacket(ranger::McpsDataIndicationParams receiveParams, Ptr<Packet> p) {
    NS_LOG_FUNCTION(this);
    MessageHeader msgHdr;
    p->RemoveHeader(msgHdr);
    std::ostringstream oss;
    oss << "[" << m_mainAddr << "](R-AT +" << Simulator::Now().GetMilliSeconds() << "ms)";
    msgHdr.Print(oss); // 将输出重定向到字符串流
    NS_LOG_INFO(oss.str()); // 将捕获的字符串输出到日志
    switch (msgHdr.GetMessageType())
    {
    case MessageHeader::NODEINFO_MESSAGE:
        m_nbList.UpdateNeighborNodeStatus(msgHdr.GetSrcAddress(), msgHdr.GetNodeInfo());
        break;
    
    case MessageHeader::AUDIODATA_MESSAGE: {

        MessageHeader::AudioData audioDataHdr = msgHdr.GetAudioData();
        if(m_audioManagement.isNewSeq(audioDataHdr.OriAddr, audioDataHdr.AudioSeq, Simulator::Now())) {
            // Trace
            m_receiveTraceCallback(m_mainAddr, audioDataHdr.OriAddr, audioDataHdr.AudioSeq, Simulator::Now());
            if(isForwardNode(audioDataHdr) || false) {
                ForwardAudioDataRequest(msgHdr);
            }
        }
        break;
    }
    default:
        break;
    }
}

void
RangerRoutingProtocol::SetSendCallback(RangerRoutingProtocolSendCallback cb) {
    m_sendCallback = cb;
}

void
RangerRoutingProtocol::SetReceiveTraceCallback(RangerRoutingProtocolReceiveTraceCallback cb) {
    m_receiveTraceCallback = cb;
}

void
RangerRoutingProtocol::SetSendTraceCallback(RangerRoutingProtocolSendTraceCallback cb) {
    m_sendTraceCallback = cb;
}

void
RangerRoutingProtocol::SourceAudioDataRequest(const uint32_t audioLen) {
    NS_LOG_FUNCTION(this);
    // Create a new message hdr
    MessageHeader msg;
    msg.SetMessageType(MessageHeader::AUDIODATA_MESSAGE);
    msg.SetSrcAddress(m_mainAddr);
    // Send the message
    
    MessageHeader::AudioData& audiodataHdr = msg.GetAudioData();
    m_nbList.GetSourceAssignNeighbor(audiodataHdr);
    audiodataHdr.OriAddr = m_mainAddr;
    audiodataHdr.AudioSeq = m_audioManagement.getSelfSeq();
    audiodataHdr.AudioSize = audioLen;

    msg.SetMessageLength(msg.GetSerializedSize());

    EnqueueMessage(msg, Seconds(0));
}

void
RangerRoutingProtocol::ForwardAudioDataRequest(const MessageHeader& OriMessageHdr) {
    NS_LOG_FUNCTION(this);
    // Create a new message hdr
    MessageHeader msg;
    msg.SetMessageType(MessageHeader::AUDIODATA_MESSAGE);
    msg.SetSrcAddress(m_mainAddr);
    // Send the message
    
    MessageHeader::AudioData& audioDataHdr = msg.GetAudioData();
    audioDataHdr.OriAddr = OriMessageHdr.GetAudioData().OriAddr;
    audioDataHdr.AudioSeq = OriMessageHdr.GetAudioData().AudioSeq;
    audioDataHdr.AudioSize = OriMessageHdr.GetAudioData().AudioSize;
    m_nbList.GetForwardAssignNeighbor(OriMessageHdr.GetSrcAddress(), audioDataHdr);
    msg.SetMessageLength(msg.GetSerializedSize());

    EnqueueMessage(msg, Seconds(0));
}

bool 
RangerRoutingProtocol::isForwardNode(const MessageHeader::AudioData& assignHdr){
    NS_LOG_FUNCTION(this);
    for(auto assignIter = assignHdr.AssignNeighbor.begin(); assignIter != assignHdr.AssignNeighbor.end(); assignIter++) {
        if(*assignIter == m_mainAddr) {
            return true;
        }
    }
    return false;
}

void
RangerRoutingProtocol::QueuedMessagesTimerExpire() {
    //NS_LOG_FUNCTION(this);
    SendQueuedMessages();
    m_queuedMessagesTimer.Schedule(m_queuedMessagesInterval);
}


void
RangerRoutingProtocol::SendQueuedMessages() {
    //NS_LOG_FUNCTION(this);
    for (auto messageIter = m_queuedMessages.begin(); messageIter != m_queuedMessages.end(); messageIter++) {


        switch (messageIter->GetMessageType()) {
        case MessageHeader::NODEINFO_MESSAGE: {
            std::ostringstream oss;
            oss << "[" << m_mainAddr << "](S-AT +" << Simulator::Now().GetMilliSeconds() << "ms)";
            messageIter->Print(oss); // 将输出重定向到字符串流
            NS_LOG_INFO(oss.str()); // 将捕获的字符串输出到日志

            Ptr<Packet> p = Create<Packet>(0);
            p->AddHeader(*messageIter);
            ranger::McpsDataRequestParams sendParams;
            sendParams.m_dstAddr = Ipv4Address("255.255.255.255");
            sendParams.m_msduHandle = 0;
            sendParams.m_txOptions = 0b010;
            SendPacket(sendParams, p);
            break;
        }
        case MessageHeader::AUDIODATA_MESSAGE: {
            MessageHeader::AudioData tmp = messageIter->GetAudioData();

            if(m_mainAddr == tmp.OriAddr) {
                std::ostringstream oss;
                oss << "[" << m_mainAddr << "](S-AT +" << Simulator::Now().GetMilliSeconds() << "ms)";
                messageIter->Print(oss); // 将输出重定向到字符串流
                NS_LOG_INFO(oss.str()); // 将捕获的字符串输出到日志
            } else {
                std::ostringstream oss;
                oss << "[" << m_mainAddr << "](F-AT +" << Simulator::Now().GetMilliSeconds() << "ms)";
                messageIter->Print(oss); // 将输出重定向到字符串流
                NS_LOG_INFO(oss.str()); // 将捕获的字符串输出到日志
            }
            Ptr<Packet> p = Create<Packet>(tmp.AudioSize);
            p->AddHeader(*messageIter);
            ranger::McpsDataRequestParams sendParams;
            sendParams.m_dstAddr = Ipv4Address("255.255.255.255");
            sendParams.m_msduHandle = 0;
            sendParams.m_txOptions = 0b010;
            SendPacket(sendParams, p);
            // Trace
            m_sendTraceCallback(m_mainAddr, messageIter->GetAudioData().OriAddr, messageIter->GetAudioData().AudioSeq, Simulator::Now());
            break;
        }
        
        default:
            break;
        }
    }
    m_queuedMessages.clear();
}

void
RangerRoutingProtocol::EnqueueMessage(const MessageHeader& messageHdr, Time delay) {
    m_queuedMessages.push_back(messageHdr);
}

void
RangerRoutingProtocol::SendNodeInfo() {
    NS_LOG_FUNCTION(this);
    // Create a new message hdr
    MessageHeader msg;
    msg.SetMessageType(MessageHeader::NODEINFO_MESSAGE);
    msg.SetSrcAddress(m_mainAddr);
    
    MessageHeader::NodeInfo& nodeinfoHdr = msg.GetNodeInfo();
    m_nbList.GetNeighborNodeInfo(nodeinfoHdr);

    msg.SetMessageLength(msg.GetSerializedSize());

    // Send the message
    EnqueueMessage(msg, Seconds(0));

}

void
RangerRoutingProtocol::NodeInfoTimerExpire() {
    m_nbList.RefreshNeighborNodeStatus();
    std::ostringstream oss;
    PrintNeighborList(oss);
    NS_LOG_INFO(oss.str());

    SendNodeInfo();
    m_nodeInfoTimer.Schedule(m_nodeInfoInterval);
}

} // namespace ns3