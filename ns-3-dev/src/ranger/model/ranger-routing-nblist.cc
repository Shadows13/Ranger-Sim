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
        m_nbStatus.push_back(nbIns);
    }

}

void
RangerNeighborList::RefreshNeighborNodeStatus(void)
{
    Time CurrTime = Simulator::Now();
    for(auto iter = m_nbStatus.begin(); iter != m_nbStatus.end();) {
        if(CurrTime - iter->refreshTime > m_refreshInterval) {
            iter->lqiBuffer.insert(false);
        } else {
            iter->lqiBuffer.insert(true);
        }
        iter->lqi = iter->lqiBuffer.calLqi();

        // update status
        if(iter->lqi > 170) {
            iter->status = NeighborStatus::STATUS_STABLE;
            iter++;
        } else if(iter->lqi > 75) {
            iter->status = NeighborStatus::STATUS_UNSTABLE;
            iter++;
        } else if (iter->lqi > 0) {
            iter->status = NeighborStatus::STATUS_NONE;
            iter++;
        } else {
            iter = m_nbStatus.erase(iter);
        }
    }
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
RangerNeighborList::GetNeighborNodeInfo(MessageHeader::NodeInfo& header) {
    header.linkNumber = m_nbStatus.size();
    for(std::size_t i = 0; i < m_nbStatus.size(); i++) {
        MessageHeader::NodeInfo::LinkMessage linkMsg;
        linkMsg.neighborAddresses = m_nbStatus[i].neighborMainAddr;
        linkMsg.linkStatus = m_nbStatus[i].status;
        header.linkMessages.push_back(linkMsg);
    }
}

struct reachableNodeElement
{
    Ipv4Address reachableAddr;
    uint8_t oneHopStatus;
    uint8_t twoHopStatus;
};

struct reachableMapElement
{
    Ipv4Address targetAddr;
    std::vector<reachableNodeElement> reachableNode;
};

void
RangerNeighborList::GetForwardAssignNeighbor(Ipv4Address SrcAddress, MessageHeader::AudioData& header) {

}

void
RangerNeighborList::GetSourceAssignNeighbor(MessageHeader::AudioData& header) {
    
}

void
RangerNeighborList::Print(std::ostream& os) const
{
    os << "----------------[" << m_mainAddr << "]----------------" << std::endl;
    for(auto iter = m_nbStatus.begin(); iter != m_nbStatus.end(); iter++) {
        os << "[" << iter->neighborMainAddr << "]";
        os << "(" << (uint16_t)iter->lqi << "-" << (uint16_t)iter->status << "):";
        iter->Print(os);
    }
    os << "----------------[" << m_mainAddr << "]----------------" << std::endl;
}

} // namespace ns3