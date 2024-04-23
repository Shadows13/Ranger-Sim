#include "ranger-mac-header.h"


namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RangerMacHeader");
NS_OBJECT_ENSURE_REGISTERED(RangerMacHeader);

TypeId
RangerMacHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::RangerMacHeader")
                            .SetParent<Header>()
                            .SetGroupName("Ranger")
                            .AddConstructor<RangerMacHeader>();
    return tid;
}

TypeId
RangerMacHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

RangerMacHeader::RangerMacHeader()
{
    SetNoAckReq();
}

RangerMacHeader::RangerMacHeader(RangerMacType type, uint8_t seqNum)    
{
    SetType(type);
    SetSeqNum(seqNum);
    SetNoAckReq();
}

RangerMacHeader::~RangerMacHeader()
{
}

RangerMacHeader::RangerMacType
RangerMacHeader::GetType() const
{
    switch (m_fcFrmType)
    {
    case 0:
        return RANGER_MAC_UNICAST;
    case 1:
        return RANGER_MAC_BROADCAST;
    case 2:
        return RANGER_MAC_ACK;
    default:
        return RANGER_MAC_RESERVED;
    }
}

bool
RangerMacHeader::IsUnicast() const
{
    return m_fcFrmType == 0;
}

bool
RangerMacHeader::IsBroadcast() const
{
    return m_fcFrmType == 1;
}

bool
RangerMacHeader::IsAck() const
{
    return m_fcFrmType == 2;
}

bool
RangerMacHeader::IsAckReq() const
{
    return m_fcAckReq == 1;
}

uint8_t
RangerMacHeader::GetSeqNum() const
{
    return m_SeqNum;
}

Ipv4Address
RangerMacHeader::GetDstAddr() const
{
    return m_DstAddr;
}

Ipv4Address
RangerMacHeader::GetSrcAddr() const
{
    return m_SrcAddr;
}

void
RangerMacHeader::SetFrameControl(uint8_t fc)
{
    // 获取fc的第0-3位
    m_fcFrmType = (fc >> 5) & 0x07;
    // 获取fc的第4位
    m_fcAckReq = (fc >> 4) & 0x01;
}

void
RangerMacHeader::SetType(RangerMacType type)
{
    m_fcFrmType = type;
}

void
RangerMacHeader::SetAckReq()
{
    m_fcAckReq = 1;
}

void
RangerMacHeader::SetNoAckReq()
{
    m_fcAckReq = 0;
}

void
RangerMacHeader::SetSeqNum(uint8_t seqNum)
{
    m_SeqNum = seqNum;
}

void
RangerMacHeader::SetDstAddr(Ipv4Address addr)
{
    m_DstAddr = addr;
}

void
RangerMacHeader::SetSrcAddr(Ipv4Address addr)
{
    m_SrcAddr = addr;
}

void
RangerMacHeader::Print(std::ostream& os) const
{
    os << "Type: ";
    switch (m_fcFrmType)
    {
    case 0:
        os << "Unicast";
        break;
    case 1:
        os << "Broadcast";
        break;
    case 2:
        os << "ACK";
        break;
    }
    os << ", AckReq: " << (m_fcAckReq ? "Yes" : "No")
        << ", SeqNum: " << static_cast<uint32_t>(m_SeqNum)
        << ", DstAddr: " << m_DstAddr
        << ", SrcAddr: " << m_SrcAddr;
}

uint32_t
RangerMacHeader::GetSerializedSize() const
{
    /*
        * MAC Header 结构:
        * 1. 帧控制: 1 byte
        * 2. 序列号: 1 byte
        * 3. 目的地址: 4 bytes
        * 4. 源地址: 4 bytes
        * 总共: 10 bytes
        */
    return 10;
}

void
RangerMacHeader::Serialize(Buffer::Iterator start) const
{
    // 序列化帧控制
    uint8_t fc = (m_fcFrmType << 5) | (m_fcAckReq << 4);
    start.WriteU8(fc);
    // 序列化序列号
    start.WriteU8(m_SeqNum);
    // 序列化目的地址
    WriteTo(start, m_DstAddr);
    // 序列化源地址
    WriteTo(start, m_SrcAddr);
}

uint32_t
RangerMacHeader::Deserialize(Buffer::Iterator start)
{
    // 反序列化帧控制
    uint8_t fc = start.ReadU8();
    m_fcFrmType = (fc >> 5) & 0x07;
    m_fcAckReq = (fc >> 4) & 0x01;
    // 反序列化序列号
    m_SeqNum = start.ReadU8();
    // 反序列化目的地址
    ReadFrom(start, m_DstAddr);
    // 反序列化源地址
    ReadFrom(start, m_SrcAddr);
    return 10;
}

};  // namespace ns3
