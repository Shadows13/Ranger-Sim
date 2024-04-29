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
#include "ranger-recorder.h"
#include <iomanip>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RangerRecorder");
NS_OBJECT_ENSURE_REGISTERED(RangerRecorder);

TypeId
RangerRecorder::GetTypeId()
{
    static TypeId tid = TypeId("ns3::RangerRecorder").SetParent<Object>().SetGroupName("Ranger");
    return tid;
}


RangerRecorder::RangerRecorder() : m_receiveList(), m_sendList()
{

}

RangerRecorder::~RangerRecorder()
{
    std::ostringstream oss;
    // PrintReceiveList(oss);
    // PrintSendList(oss);
    // PrintForwardList(oss);
    // PrintReceiveRate(oss);
    PrintGlobalIndicators(oss);
    NS_LOG_UNCOND(oss.str());
}

void
RangerRecorder::recordReceive(Ipv4Address receiver, Ipv4Address sender, uint8_t seq, Time time)
{
    m_receiveList[receiver].m_fromList[sender].m_totalCnt++;
    m_receiveList[receiver].m_fromList[sender].m_lastSeq = seq;
    m_receiveList[receiver].m_fromList[sender].m_lastTime = time;
}

void
RangerRecorder::recordSend(Ipv4Address sender, Ipv4Address origin, uint8_t seq, Time time)
{
    if(sender != origin)
    {
        m_forwardList[sender].m_forwardList[origin].m_totalCnt++;
        m_forwardList[sender].m_forwardList[origin].m_lastSeq = seq;
        m_forwardList[sender].m_forwardList[origin].m_lastTime = time;
    } else {
        m_sendList[sender].m_totalCnt++;
        m_sendList[sender].m_lastSeq = seq;
        m_sendList[sender].m_lastTime = time;
    }
}

void
RangerRecorder::DoCalculation()
{
    // Cal receive rate
    total_receive_cnt = 0;
    total_should_receive_cnt = 0;
    total_receive_rate = 0.0;
    // 遍历所有接受者，把接受者对于发送者的List进行遍历，如果没找到记为0，如果找到了则将这个发送者的包接受数记录
    // 因为receiver可能也在sender中存在，所以如果receiver和sender相同则不计入总数
    // 把一个接受者遍历完后，记入总数
    for(auto& receiver : m_receiveList)
    {
        uint32_t total_receive_cnt_tmp = 0;
        uint32_t total_should_receive_cnt_tmp = 0;
        for(auto& sender : m_sendList)
        {
            uint32_t tmp = 0;
            if(receiver.second.m_fromList.find(sender.first) != receiver.second.m_fromList.end())
                tmp = receiver.second.m_fromList[sender.first].m_totalCnt;
            else 
                tmp = 0;

            if(sender.first != receiver.first) {
                total_receive_cnt_tmp += tmp;
                total_should_receive_cnt_tmp += sender.second.m_totalCnt;
            }
        }
        total_receive_cnt += total_receive_cnt_tmp;
        total_should_receive_cnt += total_should_receive_cnt_tmp;
    }
    total_receive_rate = (double)total_receive_cnt / total_should_receive_cnt;

    // Cal the forward cost
    total_source_cnt = 0;
    total_forward_cnt = 0;
    total_forward_cost = 0.0;
    for(auto& sender : m_sendList)
    {
        total_source_cnt += sender.second.m_totalCnt;
    }
    for(auto& forwarder : m_forwardList)
    {
        for(auto& origin : forwarder.second.m_forwardList)
        {
            total_forward_cnt += origin.second.m_totalCnt;
        }
    }
    total_forward_cost = (double)total_forward_cnt / total_source_cnt;
}

void
RangerRecorder::PrintReceiveList(std::ostream& os)
{
    os << "-----------------ReceiveList----------------" << std::endl;
    for(auto& receiver : m_receiveList)
    {
        os << "Receiver: " << receiver.first << std::endl;
        for(auto& sender : receiver.second.m_fromList)
        {
            os << "---Sender: " << sender.first << " TotalCnt: " << sender.second.m_totalCnt << std::endl;
        }
    }
    os << "-----------------ReceiveList----------------" << std::endl;
}

void
RangerRecorder::PrintSendList(std::ostream& os)
{
    os << "------------------SendList------------------" << std::endl;
    for(auto& sender : m_sendList)
    {
        os << "Sender: " << sender.first << " TotalCnt: " << sender.second.m_totalCnt << std::endl;
    }
    os << "------------------SendList------------------" << std::endl;
}

void
RangerRecorder::PrintForwardList(std::ostream& os)
{
    os << "-----------------ForwardList----------------" << std::endl;
    for(auto& forwarder : m_forwardList)
    {
        os << "Forwarder: " << forwarder.first << std::endl;
        for(auto& origin : forwarder.second.m_forwardList)
        {
            os << "---Origin: " << origin.first << " TotalCnt: " << origin.second.m_totalCnt << std::endl;
        }
    }
    os << "-----------------ForwardList----------------" << std::endl;
}

void
RangerRecorder::PrintReceiveRate(std::ostream& os)
{
    total_receive_cnt = 0;
    total_should_receive_cnt = 0;
    total_receive_rate = 0.0;
    os << "=================ReceiveRate=================" << std::endl;
    for(auto& receiver : m_receiveList)
    {
        uint32_t total_receive_cnt_tmp = 0;
        uint32_t total_send_cnt_tmp = 0;
        os << "Receiver: [" << receiver.first << "]";
        for(auto& sender : m_sendList)
        {
            uint32_t tmp = 0;
            if(receiver.second.m_fromList.find(sender.first) != receiver.second.m_fromList.end())
                tmp = receiver.second.m_fromList[sender.first].m_totalCnt;
            else 
                tmp = 0;

            os << " ---[" << sender.first << "]:(" << std::setfill('0') << std::setw(6) << tmp \
             << "/" << std::setfill('0') << std::setw(6) << sender.second.m_totalCnt \
             << "-" << std::setfill('0') << std::setw(5) << std::fixed << std::setprecision(2) << ((double)tmp / sender.second.m_totalCnt) * 100 << "%)";
            if(sender.first != receiver.first) {
                total_receive_cnt_tmp += tmp;
                total_send_cnt_tmp += sender.second.m_totalCnt;
            }
        }
        total_receive_cnt += total_receive_cnt_tmp;
        total_should_receive_cnt += total_send_cnt_tmp;
        os << " ---Total:(" << std::setfill('0') << std::setw(6) << total_receive_cnt_tmp \
            << "/" << std::setfill('0') << std::setw(6) << total_send_cnt_tmp \
            << "-" << std::setfill('0') << std::setw(5) << std::fixed << std::setprecision(2) << ((double)total_receive_cnt_tmp / total_send_cnt_tmp) * 100 << "%)";
        os << std::endl;
    }
    os << "Average Receive Rate:(" << std::setfill('0') << std::setw(8) << total_receive_cnt \
    << "/" << std::setfill('0') << std::setw(8) << total_should_receive_cnt \
    << "-" << std::setfill('0') << std::setw(5) << std::fixed << std::setprecision(2) << ((double)total_receive_cnt / total_should_receive_cnt) * 100 << "%)";
    os << std::endl;
    os << "=================ReceiveRate=================" << std::endl;
}


void
RangerRecorder::PrintForwardCost(std::ostream& os)
{
    total_source_cnt = 0;
    for(auto& sender : m_sendList)
    {
        total_source_cnt += sender.second.m_totalCnt;
    }
    os << "=================ForwardCost=================" << std::endl;
    total_forward_cnt = 0;
    total_forward_cost = 0.0;
    for(auto& forwarder : m_forwardList)
    {
        for(auto& origin : forwarder.second.m_forwardList)
        {
            total_forward_cnt += origin.second.m_totalCnt;
        }
    }
    total_forward_cost = (double)total_forward_cnt / total_source_cnt;
    os << "Total Forward Cost: " << std::setfill('0') << std::setw(5) << std::fixed << std::setprecision(2) << total_forward_cost << std::endl;
    os << "=================ForwardCost=================" << std::endl;
}

/**
 * 1. Calculate the forward cost
 * 2. Calculate the ratio of receive rate to forward cost
 * 3. Calculate the delay
 * 4. Calculate the jitter
*/
void
RangerRecorder::PrintGlobalIndicators(std::ostream& os)
{
    DoCalculation();
    os << "=================GlobalIndicator=================" << std::endl;
    os << "Total Receive Rate: " << std::setfill('0') << std::setw(5) << std::fixed << std::setprecision(2) << total_receive_rate * 100 << "%" << std::endl;
    os << "Total Forward Cost: " << std::setfill('0') << std::setw(5) << std::fixed << std::setprecision(2) << total_forward_cost << std::endl;
    //os << "Total Receive Rate: " << std::setfill('0') << std::setw(5) << std::fixed << std::setprecision(2) << total_receive_rate * 100 << "%" << std::endl;

    os << "=================GlobalIndicator=================" << std::endl;
}

} // namespace ns3