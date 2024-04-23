#ifndef RANGER_MAC_HEADER_H
#define RANGER_MAC_HEADER_H

#include <ns3/header.h>
#include <ns3/address-utils.h>


namespace ns3
{

class RangerMacHeader : public Header
{
  public:
    // MAC帧类型
    enum RangerMacType
    {
        RANGER_MAC_UNICAST = 0,     // 单播
        RANGER_MAC_BROADCAST = 1,   // 广播
        RANGER_MAC_ACK = 2,         // ACK
        RANGER_MAC_RESERVED = 3,    // 保留
    };

    // 获取TypeId
    static TypeId GetTypeId ();
    TypeId GetInstanceTypeId() const override;
    // 构造函数和析构函数
    RangerMacHeader();
    RangerMacHeader(RangerMacType type, uint8_t seqNum);
    ~RangerMacHeader();

    /**
     * @brief 获取帧类型
     * @return 帧类型
     */
    RangerMacType GetType() const;

    /**
     * @brief 是否为单播包
     * @return true 如果是单播包
     */
    bool IsUnicast() const;

    /**
     * @brief 是否为广播包
     * @return true 如果是广播包
     */
    bool IsBroadcast() const;

    /**
     * @brief 是否为ACK包
     * @return true 如果是ACK包
     */
    bool IsAck() const;

    /**
     * @brief 是否需要ACK
     * @return true 如果需要ACK
     */
    bool IsAckReq() const;

    /**
     * @brief 获取序列号
     * @return 序列号
     */
    uint8_t GetSeqNum() const;

    /**
     * @brief 获取目标地址
     * @return 目标地址
     */
    Ipv4Address GetDstAddr() const;

    /**
     * @brief 获取源地址
     * @return 源地址
     */
    Ipv4Address GetSrcAddr() const;

    /**
     * @brief 设置Frame Control
     * @param frameControl Frame Control
     */
    void SetFrameControl(uint8_t frameControl);

    /**
     * @brief 设置帧类型
     * @param type 帧类型
     */
    void SetType(RangerMacType type);

    /**
     * @brief 设置为需要ACK
     */
    void SetAckReq();

    /**
     * @brief 设置为不需要ACK
     */
    void SetNoAckReq();

    /**
     * @brief 设置序列号
     * @param seqNum 序列号
     */
    void SetSeqNum(uint8_t seqNum);

    /**
     * @brief 设置目标地址
     * @param dstAddr 目标地址
     */
    void SetDstAddr(Ipv4Address dstAddr);

    /**
     * @brief 设置源地址
     * @param srcAddr 源地址
     */
    void SetSrcAddr(Ipv4Address srcAddr);

    /**
     * @brief 打印MAC帧头部信息
      */
    void Print(std::ostream& os) const override;

    /**
     * @brief 获取序列化(封包)后的大小
     * @return 序列化后的大小(字节byte)
     */
    uint32_t GetSerializedSize() const override;

    /**
     * @brief 序列化(封包)
     * @param start 序列化的起始位置
     */
    void Serialize(Buffer::Iterator start) const override;

    /**
     * @brief 反序列化(解包)
     * @param start 反序列化的起始位置
     * @return 反序列化后的大小
     */
    uint32_t Deserialize(Buffer::Iterator start) override;

  private:
    /* Frame Control 1 Octet */
    uint8_t m_fcFrmType;  // Frame Control Bit 0-2 = 0 - Unicast, 1 - Broadcast, 2 - ACK
    uint8_t m_fcAckReq;   // Frame Control Bit 1   = 0 - No ACK, 1 - ACK
    // uint8_t m_fcReserved; // Frame Control Bit 2-7 = Reserved
    /* 序列号 1 Octet */
    uint8_t m_SeqNum;
    /* 目的地址 4 Octets */
    Ipv4Address m_DstAddr;
    /* 源地址 4 Octets */
    Ipv4Address m_SrcAddr;

};  // RangerMacHeader

};  // namespace ns3

#endif  /* RANGER_MAC_HEADER_H */
