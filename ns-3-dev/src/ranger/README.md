# 2024.04.23 MAC层接口定义说明

## 上层请求MAC层发包

```C
/**
 * @brief 通过Mac层请求发包
 * 
 * @param params McpsDataRequestParams
 * @param p packet 
 */
void ranger::McpsDataRequest(ranger::McpsDataRequestParams params, Ptr<Packet> p);

/**
 * @ingroup ranger
 */
struct ranger::McpsDataRequestParams
{
    Ipv4Address m_dstAddr;      // Destination address
    uint8_t m_msduHandle{0};    // MSDU handle
    uint8_t m_txOptions{0};     // Tx Options
};
```

如果是需要回复ACK，m_txOption = 0b'001' = 1
如果是广播包，m_txOption = 0b'010' = 2
如果是广播包且需要回复ACK，m_txOption = 0b'011' = 3

## MAC层将发包情况反馈给上层
```C
/**
 * @ingroup ranger
 *
 * @brief callback is called after a McpsDataRequest has been called from
 *        the higher layer.  It returns a status of the outcome of the
 *        transmission request.
 */
using ranger::McpsDataConfirmCallback = Callback<void, McpsDataConfirmParams>;

/**
 * @ingroup ranger
 */
struct ranger::McpsDataConfirmParams
{
    uint8_t m_msduHandle{0};                                        // MSDU handle
    RangerMacStatus m_status{RangerMacStatus::INVALID_PARAMETER};   // The status
                                                                    // of the last MSDU transmission
};
```

发送成功: m_status = RangerMacStatus::SUCCESS
发送队列满了: m_status = RangerMacStatus::TRANSACTION_OVERFLOW
CSMA失败: m_status = RangerMacStatus::CHANNEL_ACCESS_FAILURE
包太大了: m_status = RangerMacStatus::FRAME_TOO_LONG

### 设置方式
```C
void McpsDataConfirm(ranger::McpsDataConfirmParams params);
m_mac->SetMcpsDataConfirmCallback(MakeCallback(&McpsDataConfirm))
```

## MAC层将收到的包传递给上层

```C
/**
 * @ingroup ranger
 *
 * @brief This callback is called after a Mcps has successfully received a
 *        frame and wants to deliver it to the higher layer.
 */
using ranger::McpsDataIndicationCallback = Callback<void, McpsDataIndicationParams, Ptr<Packet>>;

/**
 * @ingroup ranger
 */
struct ranger::McpsDataIndicationParams
{
    Ipv4Address m_srcAddr;          // Source address
    Ipv4Address m_dstAddr;          // Destination address
    uint8_t m_mpduLinkQuality{0};   // LQI value measured during reception of the MPDU
    uint8_t m_dsn{0};               // 收到的包的MAC层序列号
};
```

### 设置方式
```C
void McpsDataIndication(ranger::McpsDataIndicationParams params, Ptr<Packet> p);
m_mac->SetMcpsDataIndicationCallback(MakeCallback(&McpsDataIndication))
```