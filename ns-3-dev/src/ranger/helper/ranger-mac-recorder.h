#ifndef RANGER_MAC_RECORDER_H
#define RANGER_MAC_RECORDER_H

#include "ns3/ranger-mac.h"
#include <unordered_map>
#include <ns3/ipv4-address.h>

#include <unordered_map>

#include "ns3/nstime.h"
#include <ns3/ranger-routing-nblist.h>
#include <ns3/ranger-routing-protocol.h>

#include <ns3/object.h>


namespace ns3
{
struct MacRecorderElement
{
    uint32_t m_pktCnt = 0;
    uint32_t m_sendTimesCnt = 0;
};

class RangerMacRecorder : public Object
{
    public:
    RangerMacRecorder();
    ~RangerMacRecorder();

    static TypeId GetTypeId();

    void SendPkt(Ipv4Address srcAddr);
    void SendTimes(Ipv4Address srcAddr);

    void DoCalculate();
    void Clear();

    void PrintAvgSendTimes();

    uint64_t GetTotalPktCnt();
    uint64_t GetTotalSendTimesCnt();
    double GetAvgSendTimes();

    private:
    std::unordered_map<Ipv4Address, MacRecorderElement> m_sendList;

    uint64_t m_totalPktCnt;
    uint64_t m_totalSendTimesCnt;

    // 平均发包次数
    double m_avgSendTimes;
};


}   // namespace ns3

#endif