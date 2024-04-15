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
#include "ranger-routing-nblist.h"
#include <ns3/log.h>
#include <ns3/simulator.h>

namespace ns3
{

#define CONTINUE_SCORE (2.0)
#define NO_CONTINUE_SCORE (1.0)
#define UINT8_T_LIMIT (255)
uint8_t
NodeInfoReceiveRateBuffer::calLqi() const
{
    uint8_t total_weight = 0;
    uint8_t lqi_cal = 0;

    if(!isBufferFull()) {
        total_weight = 0;
        lqi_cal = 0;
        for(int j = 0; j < capacity; j++) {
            if(j == capacity - 1) {
                lqi_cal += NO_CONTINUE_SCORE;
                total_weight += NO_CONTINUE_SCORE;
            } else {
                if(head - 1 - j > 0) {
                    if(buffer[head - 1 - j] == false) {
                        if(buffer[head - 1 - j - 1] == false) {
                            lqi_cal += CONTINUE_SCORE;
                            total_weight += CONTINUE_SCORE;
                        } else {
                            lqi_cal += NO_CONTINUE_SCORE;
                            total_weight += NO_CONTINUE_SCORE;
                        }
                    } else {
                        total_weight += NO_CONTINUE_SCORE;
                    }
                }
                else if(head - 1 - j == 0) {
                    if(buffer[head - 1 - j] == false) {
                        lqi_cal += NO_CONTINUE_SCORE;
                        total_weight += NO_CONTINUE_SCORE;
                    } else {
                        //lqi_cal += NO_CONTINUE_SCORE;
                        total_weight += NO_CONTINUE_SCORE;
                    }
                }
                else {
                    lqi_cal += NO_CONTINUE_SCORE;
                    total_weight += NO_CONTINUE_SCORE;
                }
            }
        }
        return (uint8_t)((1.0 - ((float)lqi_cal / (float)total_weight)) * (float)UINT8_T_LIMIT);
    } else {
        total_weight = 0;
        lqi_cal = 0;
        for(int j = 0; j < capacity; j++) {
            if(j == capacity - 1) {
                if(buffer[head - 1 - j + capacity] == false) {
                    lqi_cal += NO_CONTINUE_SCORE;
                    total_weight += NO_CONTINUE_SCORE;
                } else {
                    total_weight += NO_CONTINUE_SCORE;
                }
            } else {
                if(head - 1 - j > 0) {
                    if(buffer[head - 1 - j] == false) {
                        if(buffer[head - 1 - j - 1] == false) {
                            lqi_cal += CONTINUE_SCORE;
                            total_weight += CONTINUE_SCORE;
                        } else {
                            lqi_cal += NO_CONTINUE_SCORE;
                            total_weight += NO_CONTINUE_SCORE;
                        }
                    } else {
                        total_weight += NO_CONTINUE_SCORE;
                    }
                } else if(head - 1 - j == 0) {
                    if(buffer[head - 1 - j] == false) {
                        if(buffer[head - 1 - j - 1 + capacity] == false) {
                            lqi_cal += CONTINUE_SCORE;
                            total_weight += CONTINUE_SCORE;
                        } else {
                            lqi_cal += NO_CONTINUE_SCORE;
                            total_weight += NO_CONTINUE_SCORE;
                        }
                    } else {
                        total_weight += NO_CONTINUE_SCORE;
                    }
                } else {
                    if(buffer[head - 1 - j + capacity] == false) {
                        if(buffer[head - 1 - j - 1 + capacity] == false) {
                            lqi_cal += CONTINUE_SCORE;
                            total_weight += CONTINUE_SCORE;
                        } else {
                            lqi_cal += NO_CONTINUE_SCORE;
                            total_weight += NO_CONTINUE_SCORE;
                        }
                    } else {
                        total_weight += NO_CONTINUE_SCORE;
                    }
                }
            }
        }
        return (uint8_t)((1.0 - ((float)lqi_cal / (float)total_weight)) * (float)UINT8_T_LIMIT);
    }
}

RangerNeighborList::RangerNeighborList(Time refreshInterval)
{
    m_refreshInterval = refreshInterval;
}
RangerNeighborList::~RangerNeighborList()
{

}

void
RangerNeighborList::UpdateNeighborNodeStatus(Ipv4Address SrcAddress, MessageHeader::NodeInfo nodeinfoHdr)
{
    uint8_t index = 0;
    if(FindNeighbor(SrcAddress, index)) {
        m_nbStatus[index].refreshTime = Simulator::Now();
        m_nbStatus[index].twoHopNodeInfo = nodeinfoHdr.linkMessages;
    } else {
        NeighborStatus nbIns = NeighborStatus();
        nbIns.neighborMainAddr = SrcAddress;
        nbIns.status = NeighborStatus::STATUS_NONE;
        nbIns.lqi = 255;
        nbIns.refreshTime = Simulator::Now();
        nbIns.twoHopNodeInfo = nodeinfoHdr.linkMessages;
    }

}

void
RangerNeighborList::RefreshNeighborNodeStatus(void)
{
    Time CurrTime = Simulator::Now();
    for(std::size_t i = 0; i < m_nbStatus.size(); i++) {
        // > : too long not to get a new nodeinfo packet, mark as none into laiBuffer
        if(CurrTime - m_nbStatus[i].refreshTime > m_refreshInterval) {
            m_nbStatus[i].lqiBuffer.insert(false);
        } else {
            m_nbStatus[i].lqiBuffer.insert(true);
        }
        m_nbStatus[i].lqi = m_nbStatus[i].lqiBuffer.calLqi();
    }
    std::ostringstream oss;
    Print(oss); // 将输出重定向到字符串流
    NS_LOG_UNCOND(oss.str()); // 将捕获的字符串输出到日志
}

bool
RangerNeighborList::FindNeighbor(Ipv4Address TargetAddr, uint8_t& index)
{
    for(std::size_t i = 0; i < m_nbStatus.size(); i++) {
        if (m_nbStatus[i].neighborMainAddr == TargetAddr)
        {
            index = i;
            return true;
        }
    }

    return false;
}

void
RangerNeighborList::Print(std::ostream& os) const
{
    os << "----------------[" << m_mainAddr << "]----------------" << std::endl;
    for(auto iter = m_nbStatus.begin(); iter != m_nbStatus.end(); iter++) {
        os << "[" << iter->neighborMainAddr << "]";
        os << "(" << iter->lqi << "):";
        iter->Print(os);
    }
    os << "----------------[" << m_mainAddr << "]----------------" << std::endl;
}

} // namespace ns3