#include "ranger-mac.h"


namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RangerMac");
NS_OBJECT_ENSURE_REGISTERED(RangerMac);

TypeId
RangerMac::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::RangerMac")
            .SetParent<Object>()
            .SetGroupName("Ranger")
            .AddConstructor<RangerMac>();
    return tid;
}   // RangerMac::GetTypeId

RangerMac::RangerMac()
{
    // 初始化MAC地址
    m_address = Ipv4Address("ff.ff.ff.ff");

    // First set the state to a known value, call ChangeMacState to fire trace source.
    m_macState = ranger::MAC_IDLE;
    ChangeMacState(ranger::MAC_IDLE);

    // 创建一个随机数生成器，从0-255中随机选择一个数作为MAC层的包初始序列号
    Ptr<UniformRandomVariable> uniformVar = CreateObject<UniformRandomVariable>();
    uniformVar->SetAttribute("Min", DoubleValue(0));
    uniformVar->SetAttribute("Max", DoubleValue(255));
    m_macDsn = SequenceNumber8(uniformVar->GetValue());

    // 初始化PHY层指针
    m_phy = nullptr;

    // 初始化最大重传次数
    m_maxRetryTimes = 2;
    // 初始化发送队列最大长度
    m_maxTxQueueSize = m_txQueue.max_size();

    // 初始化正在接收的数据包指针
    m_rxPkt = nullptr;
    // 初始化正在发送的数据包指针
    m_txPkt = nullptr;
    // 初始化重传次数
    // m_retransmission = 0;
    // 初始化CSMA-CA重试次数
    // m_numCsmacaRetry = 0;

    // 初始化CSMA退避指数的上下限
    m_macMinBE = 3;
    m_macMaxBE = 5;

    // 初始化CSMA退避次数的上限
    m_macMaxCSMABackoffs = 4;

    // 初始化随机退避时间
    m_random = CreateObject<UniformRandomVariable>();
    m_randomBackoffPeriodsLeft = 0;

    // 重传间隔
    m_resendInterval = Seconds(0.01);

    // 初始化发送队列检查定时器
    CheckQueuePeriodically(0.001);
    
}   // RangerMac::RangerMac

RangerMac::~RangerMac()
{
}   // RangerMac::~RangerMac

void
RangerMac::DoInitialize()
{
    SetMacState(ranger::MAC_IDLE);

    Object::DoInitialize();
}   // RangerMac::DoInitialize

void
RangerMac::DoDispose()
{
    // 清空发送队列
    for (uint32_t i = 0; i < m_txQueue.size(); i++)
    {
        m_txQueue[i]->txQPkt = nullptr;
    }
    m_txQueue.clear();

    // 清空指针
    m_phy = nullptr;
    m_rxPkt = nullptr;
    m_txPkt = nullptr;

    // 清空回调函数
    m_mcpsDataConfirmCallback = MakeNullCallback<void, ranger::McpsDataConfirmParams>();
    m_mcpsDataIndicationCallback = MakeNullCallback<void, ranger::McpsDataIndicationParams, Ptr<Packet>>();
    m_macSendPktTraceCallback = MakeNullCallback<void, Ipv4Address>();
    m_macSendTimesTraceCallback = MakeNullCallback<void, Ipv4Address>();

    Object::DoDispose();
}   // RangerMac::Dispose

void
RangerMac::McpsDataRequest(ranger::McpsDataRequestParams params, Ptr<Packet> p)
{
    NS_LOG_FUNCTION(this << p);

    ranger::McpsDataConfirmParams confirmParams;
    confirmParams.m_msduHandle = params.m_msduHandle;

    // 检查包大小
    if (p->GetSize() > ranger::aMaxPhyPacketSize - ranger::aMPDUOverhead)
    {
        NS_LOG_ERROR(this << "[MAC] packet too big: " << p->GetSize());
        confirmParams.m_status = RangerMacStatus::FRAME_TOO_LONG;
        if (!m_mcpsDataConfirmCallback.IsNull())
        {
            m_mcpsDataConfirmCallback(confirmParams);
        }
        return;
    }

    // 创建MAC头部并设置包序列号
    if (m_macDsn.GetValue() == 0)
    {
        m_macDsn++;
    }
    RangerMacHeader macHdr(RangerMacHeader::RANGER_MAC_RESERVED, m_macDsn.GetValue());
    m_macDsn++;

    // 设置MAC头部中的目的地址和源地址
    macHdr.SetDstAddr(params.m_dstAddr);
    macHdr.SetSrcAddr(m_address);

    // 根据params.m_txOptions设置ACK请求和是否广播
    // 如果是需要回复ACK，m_txOption = 0b'001' = 1
    // 如果是广播包，m_txOption = 0b'010' = 2
    // 如果是广播包且需要回复ACK，m_txOption = 0b'011' = 3
    if (params.m_txOptions & 0b001)
    {
        macHdr.SetAckReq();
    }
    if (params.m_txOptions & 0b010)
    {
        macHdr.SetType(RangerMacHeader::RANGER_MAC_BROADCAST);
    }
    else
    {
        macHdr.SetType(RangerMacHeader::RANGER_MAC_UNICAST);
    }

    // 添加MAC包头
    p->AddHeader(macHdr);
    
    // 定义发送队列元素
    Ptr<TxQueueElement> txQElement = Create<TxQueueElement>();
    // txQElement->txQMsduHandle = params.m_msduHandle;
    txQElement->txQPkt = p;
    // 根据包类型决定包是否重发
    if (!macHdr.IsAckReq())
    {   // 不需要ACK的包不重发
        txQElement->txQMsduHandle = 0;  // 表示不需要重发
        txQElement->retryTimes = m_maxRetryTimes;
    }
    else
    {   // 需要ACK的包进行重发
        txQElement->txQMsduHandle = 1;  // 表示需要重发
        txQElement->retryTimes = 0;
        if (!m_macSendPktTraceCallback.IsNull())
        {
            m_macSendPktTraceCallback(m_address);
        }
    }

    // 将包加入发送队列
    // EnqueueTxQElement(txQElement);
    m_BE = m_macMinBE;
    Time randomBackoff = CalRandomBackoff();
    Simulator::Schedule(randomBackoff, &RangerMac::EnqueueTxQElement, this, txQElement);
    // CheckQueue();
}   // RangerMac::McpsDataRequest

void
RangerMac::PdDataIndication(uint32_t psduLength, Ptr<Packet> p, uint8_t lqi)
{
    NS_ASSERT(m_macState == ranger::MAC_IDLE || m_macState == ranger::MAC_CSMA);
    NS_LOG_FUNCTION(this << psduLength << p << (uint16_t)lqi);

    if (m_macState == ranger::MAC_CSMA)
    {
        NS_LOG_INFO("[MAC][" << m_address << "] Recv packet in CSMA state, drop it");
        return;
    }

    // 保存原始包
    Ptr<Packet> originalPkt = p->Copy();

    // 从包中提取MAC头部
    RangerMacHeader macHdr;
    p->RemoveHeader(macHdr);

    // 检查是否需要回复ACK
    if (macHdr.GetType() == RangerMacHeader::RANGER_MAC_BROADCAST &&
        macHdr.GetDstAddr() == m_address &&
        macHdr.IsAckReq())
    {
        m_rxPkt = originalPkt->Copy();
            SendAck(macHdr.GetSeqNum());
    }

    // 比较包的类型、源地址和序列号，如果已收到则丢弃
    for (auto it = m_rxedPkts.begin(); it != m_rxedPkts.end(); it++)
    {
        if (it->type == macHdr.GetType() &&
            it->srcAddr == macHdr.GetSrcAddr() &&
            it->seq == macHdr.GetSeqNum())
        {
            return;
        }
    }

    // 将包的源地址和序列号加入已收到的包列表
    RxedPkt rxedPkt;
    rxedPkt.type = macHdr.GetType();
    rxedPkt.srcAddr = macHdr.GetSrcAddr();
    rxedPkt.seq = macHdr.GetSeqNum();
    m_rxedPkts.emplace_front(rxedPkt);
    // 如果已收到包队列长度大于50，则删除第一个元素
    if (m_rxedPkts.size() > 50)
    {
        m_rxedPkts.pop_back();
    }

    // 收到ACK包且目的地址是本机
    if (macHdr.GetType() == RangerMacHeader::RANGER_MAC_ACK &&
        macHdr.GetDstAddr() == m_address)
    {
        NS_LOG_INFO("[MAC][" << m_address << "](R-AT +" << Simulator::Now().GetMilliSeconds() << "ms)"
                    << " type: ACK  Src-Addr: [" << macHdr.GetSrcAddr() << "] " 
                    << "[seq]: " << (uint32_t)macHdr.GetSeqNum() << " Lqi: " << (uint32_t)lqi);
        // 移除该ACK包对应的数据包
        DequeueTxQElement(macHdr.GetSeqNum());
    }
    // 收到广播包或目标地址为本机的单播报，向上层传递数据
    else if (macHdr.GetType() == RangerMacHeader::RANGER_MAC_BROADCAST ||
             (macHdr.GetType() == RangerMacHeader::RANGER_MAC_UNICAST && macHdr.GetDstAddr() == m_address))
    {
        NS_LOG_INFO("[MAC][" << m_address << "](R-AT +" << Simulator::Now().GetMilliSeconds() << "ms)"
                    << " type: DATA Src-Addr: [" << macHdr.GetSrcAddr() << "] " 
                    << "[seq]: " << (uint32_t)macHdr.GetSeqNum() << " Lqi: " << (uint32_t)lqi);

        // 记录接收到的数据包的相关参数
        ranger::McpsDataIndicationParams indicationParams;
        indicationParams.m_srcAddr = macHdr.GetSrcAddr();
        indicationParams.m_dstAddr = macHdr.GetDstAddr();
        indicationParams.m_mpduLinkQuality = lqi;
        indicationParams.m_dsn = macHdr.GetSeqNum();

        if (!m_mcpsDataIndicationCallback.IsNull())
        {   // 通知上层数据接收
            m_mcpsDataIndicationCallback(indicationParams, p);
        }
    }
    else
    {
        // 如果不是广播包也不是目标地址为自己的单播包，丢弃
        NS_LOG_INFO("[MAC] packet not for me: "
                    << "[src]: " << macHdr.GetSrcAddr()
                    << "[dst]: " << macHdr.GetDstAddr());
    }
}   // RangerMac::PdDataIndication

void
RangerMac::PdDataConfirm(LrWpanPhyEnumeration status)
{
    NS_ASSERT(m_macState == ranger::MAC_SENDING);
    NS_ASSERT_MSG(!m_txQueue.empty(), "TxQSize = 0");
    NS_LOG_FUNCTION(this << status << m_txQueue.size());

    // 获取正在发送的数据包的头部
    RangerMacHeader macHdr;
    m_txPkt->PeekHeader(macHdr);

    if (macHdr.IsAck()) // The packet sent was a ACK
    {   
        if (status == IEEE_802_15_4_PHY_SUCCESS)
        {
            NS_LOG_INFO("[MAC][" << m_address << "](S-AT +" << Simulator::Now().GetMilliSeconds() << "ms)"
                        << " type: ACK  Dst-Addr: [" << macHdr.GetDstAddr() << "] " 
                        << "[seq]: " << (uint32_t)macHdr.GetSeqNum());
        }
        else
        {
            NS_LOG_INFO("[MAC][" << m_address << "](S-AT +" << Simulator::Now().GetMilliSeconds() << "ms)"
                        << " type: ACK  Dst-Addr: [" << macHdr.GetDstAddr() << "] " 
                        << "[seq]: " << (uint32_t)macHdr.GetSeqNum() << " Failed");
        }
        // 将此次发送的ACK包从发送队列中移除
        DequeueTxQElement();
    }
    else
    {
        // 从发送队列中取出本次发送的包
        Ptr<TxQueueElement> txQElement = Create<TxQueueElement>();
        for (auto it = m_txQueue.begin(); it != m_txQueue.end(); it++)
        {
            if ((*it)->txNow == true)
            {
                txQElement = *it;
                break;
            }
        }
        // 成功发送，打印相关信息
        if (status == IEEE_802_15_4_PHY_SUCCESS)
        {
            // 发送的是一个心跳包
            if (txQElement->txQMsduHandle == 0)
            {
                NS_LOG_INFO("[MAC][" << m_address << "](S-AT +" << Simulator::Now().GetMilliSeconds() << "ms)"
                            << " type: HRTB Dst-Addr: [" << macHdr.GetDstAddr() << "] " 
                            << "[seq]: " << (uint32_t)macHdr.GetSeqNum());
            }
            else    // 发送的是一个数据包
            {
                NS_LOG_INFO("[MAC][" << m_address << "](S-AT +" << Simulator::Now().GetMilliSeconds() << "ms)"
                            << " type: DATA Dst-Addr: [" << macHdr.GetDstAddr() << "] " 
                            << "[seq]: " << (uint32_t)macHdr.GetSeqNum());
            }
        }
        // 通知上层数据包发送情况
        if (!m_mcpsDataConfirmCallback.IsNull())
        {
            ranger::McpsDataConfirmParams confirmParams;
            confirmParams.m_msduHandle = txQElement->txQMsduHandle;
            if (status == IEEE_802_15_4_PHY_SUCCESS)
            {
                // 记录发送成功的数据包
                m_macTxOkTrace(m_txPkt);
                confirmParams.m_status = RangerMacStatus::SUCCESS;
                confirmParams.sendTimes = txQElement->retryTimes + 1;
            }
            else if (status == IEEE_802_15_4_PHY_UNSPECIFIED)
            {
                confirmParams.m_status = RangerMacStatus::FRAME_TOO_LONG;   // TODO
            }
            else
            {
                NS_LOG_ERROR("Transmission attempt failed with PHY status " << status);
                confirmParams.m_status = RangerMacStatus::TRANSACTION_OVERFLOW;   // TODO
            }
            m_mcpsDataConfirmCallback(confirmParams);
        }
        // 将发包信息发送给mac-recorder
        if (!m_macSendTimesTraceCallback.IsNull() &&
            status == IEEE_802_15_4_PHY_SUCCESS &&
            txQElement->txQMsduHandle == 1)
        {
            m_macSendTimesTraceCallback(m_address);
        }
        // 将此次发送的数据包从发送队列中移除
        RangerMacHeader hdr;
        txQElement->txQPkt->PeekHeader(hdr);
        DequeueTxQElement(hdr.GetSeqNum());
    }
    // 清空发送数据包的指针
    m_txPkt = nullptr;
    // 将MAC层状态设置为IDLE
    m_setMacState.Cancel();
    m_setMacState = Simulator::ScheduleNow(&RangerMac::SetMacState, this, ranger::MAC_IDLE);
}   // RangerMac::PdDataConfirm

void
RangerMac::PlmeSetTRXStateConfirm(LrWpanPhyEnumeration status)
{
    NS_LOG_FUNCTION(this << status);
    NS_LOG_FUNCTION("At: " << Simulator::Now()
                    << " [MAC_ADDR]: " << m_address
                    << " Received Set TRX Confirm: " << status);

    if (m_macState == ranger::MAC_SENDING &&
        (status == IEEE_802_15_4_PHY_TX_ON || status == IEEE_802_15_4_PHY_SUCCESS))
    {
        NS_ASSERT(m_txPkt);

        // Start sending if we are in state SENDING and the PHY transmitter was enabled.
        m_promiscSnifferTrace(m_txPkt);
        m_snifferTrace(m_txPkt);
        m_macTxTrace(m_txPkt);
        m_phy->PdDataRequest(m_txPkt->GetSize(), m_txPkt);
    }
    else if (m_macState == ranger::MAC_CSMA &&
             (status == IEEE_802_15_4_PHY_RX_ON || status == IEEE_802_15_4_PHY_SUCCESS))
    {
        // Start the CSMA algorithm as soon as the receiver is enabled.
        CSMACAStart();
    }
    else if (m_macState == ranger::MAC_IDLE)
    {
        NS_ASSERT(status == IEEE_802_15_4_PHY_RX_ON || status == IEEE_802_15_4_PHY_SUCCESS ||
                  status == IEEE_802_15_4_PHY_TRX_OFF);

        // if (status == IEEE_802_15_4_PHY_RX_ON && m_scanEnergyEvent.IsRunning())
        // {
        //     // Kick start Energy Detection Scan
        //     m_phy->PlmeEdRequest();
        // }
        if (status == IEEE_802_15_4_PHY_RX_ON || status == IEEE_802_15_4_PHY_SUCCESS)
        {
            // Check if there is not messages to transmit when going idle
            // CheckQueue();
        }
    }
    else
    {
        // TODO: What to do when we receive an error?
        // If we want to transmit a packet, but switching the transceiver on results
        // in an error, we have to recover somehow (and start sending again).
        NS_FATAL_ERROR("Error changing transceiver state");
    }
}   // RangerMac::PlmeSetTRXStateConfirm

void
RangerMac::PlmeCcaConfirm(LrWpanPhyEnumeration status)
{
    NS_LOG_FUNCTION(this << status);

    // Only react on this event, if we are actually waiting for a CCA.
    // If the CSMA algorithm was canceled, we could still receive this event from
    // the PHY. In this case we ignore the event.
    if (!m_ccaRequestRunning)
    {
        return;
    }
    m_ccaRequestRunning = false;
    if (status == IEEE_802_15_4_PHY_IDLE)
    {
        SetMacState(ranger::CHANNEL_IDLE);
    }
    else
    {
        m_BE = std::min(static_cast<uint16_t>(m_BE + 1), static_cast<uint16_t>(m_macMaxBE));
        m_NB++;
        if (m_NB > m_macMaxCSMABackoffs)
        {
            // no channel found so cannot send pkt
            NS_LOG_ERROR("[MAC][" << m_address << "] Channel access failure");
            SetMacState(ranger::CHANNEL_ACCESS_FAILURE);
        }
        else
        {
            NS_LOG_FUNCTION("Perform another backoff; m_NB = " << static_cast<uint16_t>(m_NB));
            m_randomBackoffEvent =
                Simulator::ScheduleNow(&RangerMac::RandomBackoffDelay,
                                        this); // Perform another backoff (step 2)
        }
    }
}   // RangerMac::PlmeCcaConfirm

void
RangerMac::SetPhy(Ptr<LrWpanPhy> phy)
{
    NS_LOG_FUNCTION(this << phy);
    m_phy = phy;

    // 设置PHY层的Callback
    m_phy->SetPlmeSetTRXStateConfirmCallback(MakeCallback(&RangerMac::PlmeSetTRXStateConfirm, this));
    m_phy->SetPlmeCcaConfirmCallback(MakeCallback(&RangerMac::PlmeCcaConfirm, this));
    m_phy->SetPdDataConfirmCallback(MakeCallback(&RangerMac::PdDataConfirm, this));
    m_phy->SetPdDataIndicationCallback(MakeCallback(&RangerMac::PdDataIndication, this));

    // 通过初始化MAC层状态初始化PHY层状态
    SetMacState(ranger::MAC_IDLE);

    // m_phy->SetPlmeEdConfirmCallback(MakeCallback(&LrWpanMac::PlmeEdConfirm, m_mac));
    // m_phy->SetPlmeGetAttributeConfirmCallback(MakeCallback(&LrWpanMac::PlmeGetAttributeConfirm, m_mac));
    // m_phy->SetPlmeSetAttributeConfirmCallback(MakeCallback(&LrWpanMac::PlmeSetAttributeConfirm, m_mac));
}   // RangerMac::SetPhy

void
RangerMac::SetAddress(Ipv4Address address)
{
    m_address = address;
}   // RangerMac::SetAddress

Ipv4Address
RangerMac::GetAddress() const
{
    return m_address;
}   // RangerMac::GetAddress

void
RangerMac::SetMcpsDataConfirmCallback(ranger::McpsDataConfirmCallback c)
{
    m_mcpsDataConfirmCallback = c;
}   // RangerMac::SetMcpsDataConfirmCallback

void
RangerMac::SetMcpsDataIndicationCallback(ranger::McpsDataIndicationCallback c)
{
    m_mcpsDataIndicationCallback = c;
}   // RangerMac::SetMcpsDataIndicationCallback

void
RangerMac::SetMacSendPktTraceCallback(MacSendPktTraceCallback c)
{
    m_macSendPktTraceCallback = c;
}   // RangerMac::SetMacSendTraceCallback

void
RangerMac::SetMacSendTimesTraceCallback(MacSendTimesTraceCallback c)
{
    m_macSendTimesTraceCallback = c;
}   // RangerMac::SetMacSendTimesTraceCallback

void
RangerMac::EnqueueTxQElement(Ptr<TxQueueElement> txQElement)
{
    NS_LOG_FUNCTION(this);

    // 如果发送队列未满，则将数据包加入发送队列尾部
    if (m_txQueue.size() < m_maxTxQueueSize)
    {
        m_txQueue.emplace_back(txQElement);
        m_macTxEnqueueTrace(txQElement->txQPkt);
    }
    else    // 如果发送队列已满，则丢弃数据包
    {
        if (!m_mcpsDataConfirmCallback.IsNull())
        {
            ranger::McpsDataConfirmParams confirmParams;
            confirmParams.m_msduHandle = txQElement->txQMsduHandle;
            confirmParams.m_status = RangerMacStatus::TRANSACTION_OVERFLOW;
            m_mcpsDataConfirmCallback(confirmParams);
        }
        NS_LOG_ERROR("TX Queue with size " << m_txQueue.size() << " is full, dropping packet");
        m_macTxDropTrace(txQElement->txQPkt);
    }
}   // RangerMac::EnqueueTxQElement

void
RangerMac::DequeueTxQElement()
{
    NS_LOG_FUNCTION(this << "ACK");

    m_txQueue.pop_front();
}

void
RangerMac::DequeueTxQElement(uint8_t seqNum)
{
    NS_LOG_FUNCTION(this << "DATA");

    for (auto it = m_txQueue.begin(); it != m_txQueue.end(); it++)
    {
        RangerMacHeader hdr;
        (*it)->txQPkt->PeekHeader(hdr);
        if (hdr.GetSeqNum() == seqNum)
        {
            m_txQueue.erase(it);
            break;
        }
    }
}   // RangerMac::DequeueTxQElement

void
RangerMac::CheckQueue()
{
    if (m_txQueue.empty() ||                // 如果发送队列为空，return
        m_macState != ranger::MAC_IDLE ||   // 如果MAC状态不是IDLE，return
        m_setMacState.IsRunning())          // 如果正在改变MAC状态，return
    {
        return;
    }

    NS_LOG_FUNCTION(this);

    // 遍历发送队列
    for (auto it = m_txQueue.begin(); it != m_txQueue.end(); it++)
    {
        // 如果还未到达这个包的重发时间，跳过
        if ((*it)->lastTxTime + m_resendInterval > Simulator::Now())
        {
            continue;
        }
        
        // 满足发送条件，开始发包
        m_txPkt = (*it)->txQPkt;
        (*it)->txNow = true;
        m_setMacState = Simulator::ScheduleNow(&RangerMac::SetMacState, this, ranger::MAC_CSMA);
        // 检查此包是否能够重发，若能够重发则再次进入队列
        if ((*it)->retryTimes < m_maxRetryTimes)
        {
            Ptr<TxQueueElement> copyElement = Create<TxQueueElement>();
            copyElement->txQMsduHandle = (*it)->txQMsduHandle;
            copyElement->lastTxTime = Simulator::Now();
            copyElement->retryTimes = (*it)->retryTimes + 1;
            copyElement->txQPkt = (*it)->txQPkt->Copy();
            EnqueueTxQElement(copyElement);
        }
        break;
    }
}   // RangerMac::CheckQueue

void
RangerMac::CheckQueuePeriodically(double t)
{
    CheckQueue();
    Simulator::Schedule(Seconds(t), &RangerMac::CheckQueuePeriodically, this, t);
}   // RangerMac::CheckQueuePeriodically

void
RangerMac::SendAck(uint8_t seqNum)
{
    NS_LOG_FUNCTION(this << static_cast<uint32_t>(seqNum));

    Ptr<Packet> p = m_rxPkt->Copy();

    RangerMacHeader hdr;
    p->RemoveHeader(hdr);

    // Generate a corresponding ACK Frame.
    RangerMacHeader macHdr(RangerMacHeader::RANGER_MAC_ACK, seqNum);
    macHdr.SetDstAddr(hdr.GetSrcAddr());
    macHdr.SetSrcAddr(m_address);

    Ptr<Packet> ackPacket = Create<Packet>(0);
    ackPacket->AddHeader(macHdr);

    // 将ACK包放入发送队列队首
    Ptr<TxQueueElement> txQElement = Create<TxQueueElement>();
    txQElement->txQPkt = ackPacket;
    txQElement->retryTimes = m_maxRetryTimes;
    // 如果发送队列未满，则将数据包加入发送队列头部
    if (m_txQueue.size() < m_maxTxQueueSize)
    {
        m_txQueue.emplace_front(txQElement);
    }
    else    // 如果发送队列已满，则丢弃数据包
    {
        NS_LOG_ERROR("TX Queue with size " << m_txQueue.size() << " is full, dropping ACK packet");
        m_macTxDropTrace(txQElement->txQPkt);
    }
}   // RangerMac::SendAck

void
RangerMac::ChangeMacState(ranger::MacState newState)
{
    NS_LOG_FUNCTION(this << " change mac state from " << m_macState
                    << " to " << newState);
    m_macStateLogger(m_macState, newState);
    m_macState = newState;
}   // RangerMac::ChangeMacState

void
RangerMac::SetMacState(ranger::MacState macState)
{
    NS_LOG_FUNCTION(this << "mac state = " << macState);

    if (macState == ranger::MAC_IDLE)
    {
        // MAC层状态变为IDLE时，将PHY层状态设置为接收状态
        ChangeMacState(ranger::MAC_IDLE);
        m_phy->PlmeSetTRXStateRequest(IEEE_802_15_4_PHY_RX_ON);
    }
    else if (macState == ranger::MAC_CSMA)
    {
        // MAC层状态变为CSMA时，将PHY层状态设置为接收状态
        NS_ASSERT(m_macState == ranger::MAC_IDLE);
        ChangeMacState(ranger::MAC_CSMA);
        m_phy->PlmeSetTRXStateRequest(IEEE_802_15_4_PHY_RX_ON);
    }
    else if (m_macState == ranger::MAC_CSMA && macState == ranger::CHANNEL_IDLE)
    {
        // Channel is idle, set transmitter to TX_ON
        ChangeMacState(ranger::MAC_SENDING);
        m_phy->PlmeSetTRXStateRequest(IEEE_802_15_4_PHY_TX_ON);
    }
    else if (m_macState == ranger::MAC_CSMA && macState == ranger::CHANNEL_ACCESS_FAILURE)
    {
        NS_ASSERT(m_txPkt);

        // Cannot find a clear channel, drop the current packet
        // and send the proper confirm/indication according to the packet type
        NS_LOG_ERROR("[MAC][" << m_address << "] Can't find clear channel");

        m_macTxDropTrace(m_txPkt);

        Ptr<Packet> pkt = m_txPkt->Copy();
        RangerMacHeader macHdr;
        pkt->PeekHeader(macHdr);

        if (!m_mcpsDataConfirmCallback.IsNull())
        {
            ranger::McpsDataConfirmParams confirmParams;
            confirmParams.m_msduHandle = m_txQueue.front()->txQMsduHandle;
            confirmParams.m_status = RangerMacStatus::CHANNEL_ACCESS_FAILURE;
            m_mcpsDataConfirmCallback(confirmParams);
        }
        // remove the copy of the packet that was just sent
        DequeueTxQElement(macHdr.GetSeqNum());
        
        ChangeMacState(ranger::MAC_IDLE);
        m_phy->PlmeSetTRXStateRequest(IEEE_802_15_4_PHY_RX_ON);
    }
}   // RangerMac::SetMacState

void
RangerMac::CSMACAStart()
{
    NS_LOG_FUNCTION(this);
    // 设置CSMA-CA的初始参数
    m_NB = 0;
    m_BE = m_macMinBE;
    // 进行随机退避
    m_randomBackoffEvent = Simulator::ScheduleNow(&RangerMac::RandomBackoffDelay, this);
}   // RangerMac::CSMACAStart

Time
RangerMac::CalRandomBackoff()
{
    NS_LOG_FUNCTION(this);

    uint64_t upperBound = (uint64_t)pow(2, m_BE) - 1;
    Time randomBackoff;
    uint64_t symbolRate;
    Time timeLeftInCap;

    symbolRate = (uint64_t)m_phy->GetDataOrSymbolRate(false); // symbols per second

    // 计算随机退避时间
    m_randomBackoffPeriodsLeft = (uint64_t)m_random->GetValue(0, upperBound + 1);

    randomBackoff =
        Seconds((double)(m_randomBackoffPeriodsLeft * ranger::aUnitBackoffPeriod) / symbolRate);

    return randomBackoff;
}   // RangerMac::CalRandomBackoff

void
RangerMac::RandomBackoffDelay()
{
    NS_LOG_FUNCTION(this);

    Time randomBackoff = CalRandomBackoff();

    NS_LOG_FUNCTION("Unslotted CSMA-CA: requesting CCA after backoff of "
                    << m_randomBackoffPeriodsLeft << " periods (" << randomBackoff.As(Time::S)
                    << ")");
    // 请求信道空闲检测
    m_requestCcaEvent = Simulator::Schedule(randomBackoff, &RangerMac::RequestCCA, this);
}   // RangerMac::RandomBackoffDelay

void
RangerMac::RequestCCA()
{
    NS_LOG_FUNCTION(this);

    // 请求信道清空检测
    m_ccaRequestRunning = true;
    m_phy->PlmeCcaRequest();
}   // RangerMac::RequestCCA

}   // namespace ns3