#include "ranger-mac-recorder.h"


namespace ns3
{

RangerMacRecorder::RangerMacRecorder()
{
    m_totalPktCnt = 0;
    m_totalSendTimesCnt = 0;
    m_avgSendTimes = 0.0;
}   // RangerMacRecorder

RangerMacRecorder::~RangerMacRecorder()
{

}   // ~RangerMacRecorder

TypeId
RangerMacRecorder::GetTypeId()
{
    static TypeId tid = TypeId("ns3::RangerMacRecorder").SetParent<Object>().SetGroupName("Ranger");
    return tid;
}   // GetTypeId

void
RangerMacRecorder::SendPkt(Ipv4Address srcAddr)
{
    if (m_sendList.find(srcAddr) == m_sendList.end())
    {
        MacRecorderElement m_macRecorderElement;
        m_sendList.emplace(srcAddr, m_macRecorderElement);
    }

    m_sendList[srcAddr].m_pktCnt++;
}   // SendPkt

void
RangerMacRecorder::SendTimes(Ipv4Address srcAddr)
{
    m_sendList[srcAddr].m_sendTimesCnt++;
}   // SendTimes

void
RangerMacRecorder::DoCalculate()
{
    for (auto it = m_sendList.begin(); it != m_sendList.end(); it++)
    {
        m_totalPktCnt += it->second.m_pktCnt;
        m_totalSendTimesCnt += it->second.m_sendTimesCnt;
    }

    m_avgSendTimes = (double)m_totalSendTimesCnt / (double)m_totalPktCnt;
}   // DoCalculate

void
RangerMacRecorder::Clear()
{
    m_sendList.clear();
    m_totalPktCnt = 0;
    m_totalSendTimesCnt = 0;
    m_avgSendTimes = 0.0;
}   // Clear

void
RangerMacRecorder::PrintAvgSendTimes()
{
    DoCalculate();
    NS_LOG_UNCOND("Avg Send Times: " << m_avgSendTimes);
}   // PrintAvgSendTimes

uint64_t
RangerMacRecorder::GetTotalPktCnt()
{
    return m_totalPktCnt;
}   // GetTotalPktCnt

uint64_t
RangerMacRecorder::GetTotalSendTimesCnt()
{
    return m_totalSendTimesCnt;
}   // GetTotalSendTimesCnt

double
RangerMacRecorder::GetAvgSendTimes()
{
    return m_avgSendTimes;
}   // GetAvgSendTimes

} // namespace ns3