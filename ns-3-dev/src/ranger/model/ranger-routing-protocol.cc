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

    switch (msgHdr.GetMessageType())
    {
    case MessageHeader::NODEINFO_MESSAGE:
    {
        std::ostringstream oss;
        oss << "[NWK][" << m_mainAddr << "](R-AT +" << Simulator::Now().GetMilliSeconds() << "ms)";
        msgHdr.Print(oss); // 将输出重定向到字符串流
        NS_LOG_INFO(oss.str()); // 将捕获的字符串输出到日志
        m_nbList.UpdateNeighborNodeStatus(msgHdr.GetSrcAddress(), msgHdr.GetNodeInfo());
        break;
    }
    case MessageHeader::AUDIODATA_MESSAGE: {

        MessageHeader::AudioData audioDataHdr = msgHdr.GetAudioData();
        if(m_audioManagement.isNewSeq(audioDataHdr.OriAddr, audioDataHdr.AudioSeq, Simulator::Now())) {
            std::ostringstream oss;
            oss << "[NWK][" << m_mainAddr << "](R-AT +" << Simulator::Now().GetMilliSeconds() << "ms)";
            msgHdr.Print(oss); // 将输出重定向到字符串流
            NS_LOG_INFO(oss.str()); // 将捕获的字符串输出到日志
            // Trace
            m_receiveTraceCallback(m_mainAddr, audioDataHdr.OriAddr, audioDataHdr.AudioSeq, Simulator::Now());
            //NS_LOG_UNCOND("--------------");
            // if(false || isForwardNode(audioDataHdr)) {
            //     ForwardAudioDataRequest(msgHdr);
            //     //NS_LOG_UNCOND("Ranger Judge: Forward");
            // }
            // if(isForwardJudge_Hivemesh(msgHdr)) {
            //     ForwardAudioDataRequest(msgHdr);
            // }
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
    MessageHeaderElement payload;

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

    ranger::McpsDataRequestParams sendParams;
    // sendParams.m_dstAddr = Ipv4Address("255.255.255.255");
    sendParams.m_dstAddr = m_nbList.GetAckNeighbor(Ipv4Address("255.255.255.255"));
    sendParams.m_msduHandle = 0;
    sendParams.m_txOptions = 0b011;

    payload.hdr = msg;
    payload.params = sendParams;

    EnqueueMessage(payload, Seconds(0));
}

void
RangerRoutingProtocol::ForwardAudioDataRequest(const MessageHeader& OriMessageHdr) {
    NS_LOG_FUNCTION(this);
    MessageHeaderElement payload;

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

    ranger::McpsDataRequestParams sendParams;
    // sendParams.m_dstAddr = OriMessageHdr.GetSrcAddress();
    sendParams.m_dstAddr = m_nbList.GetAckNeighbor(OriMessageHdr.GetSrcAddress());
    sendParams.m_msduHandle = 0;
    sendParams.m_txOptions = 0b011;

    payload.hdr = msg;
    payload.params = sendParams;

    EnqueueMessage(payload, Seconds(0));
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

bool 
RangerRoutingProtocol::isForwardJudge_Hivemesh(const MessageHeader& hdr){
    NS_LOG_FUNCTION(this);
    // std::ostringstream oss;
    // m_nbList.Print(oss);
    // NS_LOG_UNCOND(oss.str());
    // NS_LOG_UNCOND("");

    uint8_t from_idx = 0;
    // 在邻居列表中找到目标地址，若没找到则直接返回true
    if(!m_nbList.FindNeighbor(hdr.GetSrcAddress(), from_idx)) {
        //NS_LOG_UNCOND("Not Found Target Address");
        return true;
    }
    //Ipv4Address from_addr = hdr.GetSrcAddress();
    Ipv4Address origin_addr = hdr.GetAudioData().OriAddr;
    //NS_LOG_UNCOND("From : " << from_addr << " Origin : " << origin_addr);

    // 若找到目标地址，取出其二跳表，备用
    std::vector<MessageHeader::NodeInfo::LinkMessage> fromTwoHopList = m_nbList.GetTwoHopList(from_idx);
    // 取出本地一跳表
    std::vector<MessageHeader::NodeInfo::LinkMessage> localOneHopList = m_nbList.GetOneHopList();
    // 若本地一跳节点为空，直接返回true
    if(m_nbList.isEmpty()) {
        //NS_LOG_UNCOND("Empty Local Node List");
        return true;
    }
    // 对比local_addr和from_addr的周围节点信息表，得到
    // public公共节点列表(双方都可以触达)
    // forward转发节点列表(仅本地节点可以触达)
    std::vector<MessageHeader::NodeInfo::LinkMessage> forward_node_table;
    std::vector<MessageHeader::NodeInfo::LinkMessage> public_node_table;

    for(auto localIter = localOneHopList.begin(); localIter != localOneHopList.end(); localIter++) {
        // 如果本地一跳表中的节点是origin_addr，跳过
        if(origin_addr == localIter->neighborAddresses) {
            continue;
        }

        // 本地节点表存在，发送节点表不存在的节点，为转发目标节点
        // 本地节点表存在，发送节点表也存在的节点，根据LQI判断是否为转发目标节点
        // -- 发送节点中 该节点LQI <= SAFE_LQI，则  是转发目标节点
        // -- 发送节点中 该节点LQI >  SAFE_LQI，则不是转发目标节点

        // 本地节点表不存在，发送节点表存在的节点，忽略

        // 遍历对比发送节点
        uint8_t j = 0;

        for(; j < fromTwoHopList.size(); j++) {
            if(localIter->neighborAddresses != fromTwoHopList[j].neighborAddresses) {
                continue;
            }
            // 本地节点表存在，发送节点表也存在的节点，根据LQI判断是否为转发目标节点
            if(fromTwoHopList[j].linkStatus < NeighborStatus::Status::STATUS_UNSTABLE) {
                forward_node_table.push_back(*localIter);
            } else {
                public_node_table.push_back(*localIter);
            }
            break;
        }

        if(j == fromTwoHopList.size()) {
            forward_node_table.push_back(*localIter);
        }

    }

    // 从本地周围节点信息表中，取出每个重复节点的周围节点信息表
    uint8_t finish_forward_node_num = forward_node_table.size();
    // 获取每个公共节点的周围节点信息表
    for(auto publicIter = public_node_table.begin(); publicIter != public_node_table.end(); publicIter++) {
        // 从本地周围节点信息表中，取出每个重复节点的周围节点信息表
        uint8_t publicIndex = 0;
        m_nbList.FindNeighbor(publicIter->neighborAddresses, publicIndex);
        std::vector<MessageHeader::NodeInfo::LinkMessage> publicTwoHopList = m_nbList.GetTwoHopList(publicIndex);
        // 获取每个转发目标节点，在公共节点的周围节点信息表中进行对比，判断是否为更优转发选择（检查哪个公共节点为最优转发选择）
        // 如果本地节点不是最优转发选择，则从转发列表里删除
        for(auto forwardIter = forward_node_table.begin(); forwardIter != forward_node_table.end();) {
            bool isErase = false;
            for(auto publicTwoHopIter = publicTwoHopList.begin(); publicTwoHopIter != publicTwoHopList.end(); publicTwoHopIter++) {
                if(forwardIter->neighborAddresses != publicTwoHopIter->neighborAddresses) {
                    continue;
                }
                if(forwardIter->linkStatus < publicTwoHopIter->linkStatus) {
                    if(publicTwoHopIter->linkStatus >= NeighborStatus::Status::STATUS_UNSTABLE) {
                        finish_forward_node_num--;
                        forward_node_table.erase(forwardIter);
                        isErase = true;
                    }
                } else if(forwardIter->linkStatus == publicTwoHopIter->linkStatus) {
                    if(m_mainAddr.Get() < publicIter->neighborAddresses.Get()) {
                        finish_forward_node_num--;
                        forward_node_table.erase(forwardIter);
                        isErase = true;
                    }
                } else {

                }
                
                break;
            }

            if (!isErase)
            {
                forwardIter++;
            }
        }
    }

    bool forward_result = (finish_forward_node_num > 0) ? true : false;

    
    //NS_LOG_UNCOND("JUDGE RESULT: " << forward_result);
    return forward_result;
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


        switch (messageIter->hdr.GetMessageType()) {
        case MessageHeader::NODEINFO_MESSAGE: {
            std::ostringstream oss;
            oss << "[NWK][" << m_mainAddr << "](S-AT +" << Simulator::Now().GetMilliSeconds() << "ms)";
            messageIter->hdr.Print(oss); // 将输出重定向到字符串流
            NS_LOG_INFO(oss.str()); // 将捕获的字符串输出到日志

            Ptr<Packet> p = Create<Packet>(0);
            p->AddHeader(messageIter->hdr);
            SendPacket(messageIter->params, p);
            break;
        }
        case MessageHeader::AUDIODATA_MESSAGE: {
            MessageHeader::AudioData tmp = messageIter->hdr.GetAudioData();

            if(m_mainAddr == tmp.OriAddr) {
                std::ostringstream oss;
                oss << "[NWK][" << m_mainAddr << "](S-AT +" << Simulator::Now().GetMilliSeconds() << "ms)";
                messageIter->hdr.Print(oss); // 将输出重定向到字符串流
                NS_LOG_INFO(oss.str()); // 将捕获的字符串输出到日志
            } else {
                std::ostringstream oss;
                oss << "[NWK][" << m_mainAddr << "](F-AT +" << Simulator::Now().GetMilliSeconds() << "ms)";
                messageIter->hdr.Print(oss); // 将输出重定向到字符串流
                NS_LOG_INFO(oss.str()); // 将捕获的字符串输出到日志
            }
            Ptr<Packet> p = Create<Packet>(tmp.AudioSize);
            p->AddHeader(messageIter->hdr);
            SendPacket(messageIter->params, p);
            // Trace
            m_sendTraceCallback(m_mainAddr, messageIter->hdr.GetAudioData().OriAddr, messageIter->hdr.GetAudioData().AudioSeq, Simulator::Now());
            break;
        }
        
        default:
            break;
        }
    }
    m_queuedMessages.clear();
}

void
RangerRoutingProtocol::EnqueueMessage(const MessageHeaderElement payload, Time delay) {
    m_queuedMessages.push_back(payload);
}

void
RangerRoutingProtocol::SendNodeInfo() {
    NS_LOG_FUNCTION(this);
    MessageHeaderElement payload;

    // Create a new message hdr
    MessageHeader msg;
    msg.SetMessageType(MessageHeader::NODEINFO_MESSAGE);
    msg.SetSrcAddress(m_mainAddr);
    
    MessageHeader::NodeInfo& nodeinfoHdr = msg.GetNodeInfo();
    m_nbList.GetNeighborNodeInfo(nodeinfoHdr);

    msg.SetMessageLength(msg.GetSerializedSize());

    ranger::McpsDataRequestParams sendParams;
    sendParams.m_dstAddr = Ipv4Address("255.255.255.255");
    sendParams.m_msduHandle = 0;
    sendParams.m_txOptions = 0b010;

    payload.hdr = msg;
    payload.params = sendParams;

    // Send the message
    EnqueueMessage(payload, Seconds(0));
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