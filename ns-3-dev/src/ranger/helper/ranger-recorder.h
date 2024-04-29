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
#ifndef RANGER_RECORDER_H
#define RANGER_RECORDER_H
#include <unordered_map>

#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include <ns3/ranger-routing-nblist.h>
#include <ns3/ranger-routing-protocol.h>

#include <ns3/object.h>


namespace ns3
{
struct RecorderElement
{
    uint32_t m_totalCnt;
    uint8_t m_lastSeq;
    Time m_lastTime;
};

struct ReceiveRecorderMap
{
    std::unordered_map<Ipv4Address, RecorderElement> m_fromList;
};

struct ForwardRecorderMap
{
    std::unordered_map<Ipv4Address, RecorderElement> m_forwardList;
};

class RangerRecorder : public Object
{
  private:
    std::unordered_map<Ipv4Address, ReceiveRecorderMap> m_receiveList;
    std::unordered_map<Ipv4Address, RecorderElement> m_sendList;
    std::unordered_map<Ipv4Address, ForwardRecorderMap> m_forwardList;

    uint64_t total_source_cnt = 0;
    uint64_t total_forward_cnt = 0;
    uint64_t total_receive_cnt = 0;
    uint64_t total_should_receive_cnt = 0;

    double total_receive_rate = 0.0;
    double total_forward_cost = 0.0;
    //double total_ratio_forward_receive = 0.0;
    Time total_delay = Seconds(0.0);
    double total_jitter = 0.0;


    void DoCalculation();

  public:
    RangerRecorder();
    ~RangerRecorder();

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    void recordReceive(Ipv4Address receiver, Ipv4Address sender, uint8_t seq, Time time);
    void recordSend(Ipv4Address sender, Ipv4Address origin, uint8_t seq, Time time);

    void PrintReceiveList(std::ostream& os);
    void PrintSendList(std::ostream& os);
    void PrintForwardList(std::ostream& os);
    void PrintReceiveRate(std::ostream& os);
    void PrintForwardCost(std::ostream& os);
    void PrintGlobalIndicators(std::ostream& os);

};


} // namespace ns3

#endif /* RANGER_RECORDER_H */
