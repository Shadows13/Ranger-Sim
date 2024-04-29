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

RangerNeighborList::twoHopLinkJudge
RangerNeighborList::JudgeTwoHopLinkStatus(NeighborStatus::Status oneHopStatus, NeighborStatus::Status twoHopStatus) {
    if(oneHopStatus == NeighborStatus::STATUS_STABLE && twoHopStatus == NeighborStatus::STATUS_STABLE) {
        return TWOHOP_LINK_STABLE_STABLE;
    } else if(oneHopStatus == NeighborStatus::STATUS_STABLE && twoHopStatus == NeighborStatus::STATUS_UNSTABLE) {
        return TWOHOP_LINK_STABLE_UNSTABLE;
    } else if(oneHopStatus == NeighborStatus::STATUS_UNSTABLE && twoHopStatus == NeighborStatus::STATUS_STABLE) {
        return TWOHOP_LINK_UNSTABLE_STABLE;
    } else if(oneHopStatus == NeighborStatus::STATUS_UNSTABLE && twoHopStatus == NeighborStatus::STATUS_UNSTABLE) {
        return TWOHOP_LINK_UNSTABLE_UNSTABLE;
    } else {
        return TWOHOP_LINK_INVALID;
    }
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

void
RangerNeighborList::GetForwardAssignNeighbor(Ipv4Address SrcAddress, MessageHeader::AudioData& header) {
    std::unordered_set<Ipv4Address> hiddenNode;
    std::unordered_map<Ipv4Address, std::vector<reachableMapElement>> reachableMap;
    std::unordered_set<Ipv4Address> assignNeighborSet;
    // get all the STABLEorUNSTABLE one hop neighbor, mark as hiddenNode.
    // It means there is no need to forward the audio data to them.
    hiddenNode.insert(m_mainAddr);
    for(auto iter = m_nbStatus.begin(); iter != m_nbStatus.end(); iter++) {
        if(iter->neighborMainAddr == SrcAddress) {
            hiddenNode.insert(iter->neighborMainAddr);
            for(auto secondIter = iter->twoHopNodeInfo.begin(); secondIter != iter->twoHopNodeInfo.end(); secondIter++) {
                hiddenNode.insert(secondIter->neighborAddresses);
            }
        } else if(iter->status == NeighborStatus::STATUS_STABLE || iter->status == NeighborStatus::STATUS_UNSTABLE) {
            hiddenNode.insert(iter->neighborMainAddr);
        }
    }
    // find all the node that can be reached from the source node.
    // if find target, fill it into the reachableMap.
    for(auto oneHopIter = m_nbStatus.begin(); oneHopIter != m_nbStatus.end(); oneHopIter++) {
        for(auto twoHopIter = oneHopIter->twoHopNodeInfo.begin(); twoHopIter != oneHopIter->twoHopNodeInfo.end(); twoHopIter++) {
            if(hiddenNode.find(twoHopIter->neighborAddresses) == hiddenNode.end()) {
                if(reachableMap.find(twoHopIter->neighborAddresses) != reachableMap.end()) {
                    reachableMapElement tmpElem;
                    tmpElem.oneHopAddr = oneHopIter->neighborMainAddr;
                    tmpElem.linkStatus = JudgeTwoHopLinkStatus(oneHopIter->status, (NeighborStatus::Status)twoHopIter->linkStatus);
                    reachableMap[twoHopIter->neighborAddresses].push_back(tmpElem);
                } else {
                    reachableMapElement tmpElem;
                    tmpElem.oneHopAddr = oneHopIter->neighborMainAddr;
                    tmpElem.linkStatus = JudgeTwoHopLinkStatus(oneHopIter->status, (NeighborStatus::Status)twoHopIter->linkStatus);
                    std::vector<reachableMapElement> tmpVec;
                    tmpVec.push_back(tmpElem);
                    reachableMap.insert(std::make_pair(twoHopIter->neighborAddresses, tmpVec));
                }
            }
        }
    }
    // for(auto mapiter = reachableMap.begin(); mapiter != reachableMap.end(); mapiter++) {
    //     std::cout << "targetAddr:" << mapiter->first;
    //     for(auto veciter = mapiter->second.begin(); veciter != mapiter->second.end(); veciter++) {
    //         std::cout << " [" << veciter->oneHopAddr << "]-" << (uint16_t)veciter->linkStatus;
    //     }
    //     std::cout << std::endl;
    // }

    // find all the target node that can be reached from one way, make that onehop node as the assign forward node;

    for(auto mapiter = reachableMap.begin(); mapiter != reachableMap.end(); mapiter++) {
        if(mapiter->second.size() == 1) {
            assignNeighborSet.insert(mapiter->second[0].oneHopAddr);
        }
    }
    // // If there is more than one path to reach a target node, first check whether any of the arriving nodes are already in the assigned node SET
    // // If there is, do not further evaluate; if there isn't, then compare among the multiple paths.

    // todo: 最终的选择并不完备，对于所有有2个选择以上的target，其中间仍然可能存在相同的，在这没有处理，只是按顺序查找。
    for(auto mapiter = reachableMap.begin(); mapiter != reachableMap.end(); mapiter++) {
        if(mapiter->second.size() > 1) {
            bool found = false;
            for(auto veciter = mapiter->second.begin(); veciter != mapiter->second.end(); veciter++) {
                if(assignNeighborSet.find(veciter->oneHopAddr) != assignNeighborSet.end()) {
                    found = true;
                    break;
                }
            }
            if(!found) {
                Ipv4Address winAddr = Ipv4Address("255.255.255.255");
                twoHopLinkJudge winLink = TWOHOP_LINK_INVALID;
                for(auto veciter = mapiter->second.begin(); veciter != mapiter->second.end(); veciter++) {
                    if (veciter->linkStatus < winLink)
                    {
                        winLink = veciter->linkStatus;
                        winAddr = veciter->oneHopAddr;
                    }
                }
                assignNeighborSet.insert(winAddr);
            }
        }
    }

    header.AssignNum = 0;
    for(auto setIter = assignNeighborSet.begin(); setIter != assignNeighborSet.end(); setIter++) {
        header.AssignNeighbor.push_back(*setIter);
        header.AssignNum++;
    }
}

void
RangerNeighborList::GetSourceAssignNeighbor(MessageHeader::AudioData& header) {
    std::unordered_set<Ipv4Address> hiddenNode;
    std::unordered_map<Ipv4Address, std::vector<reachableMapElement>> reachableMap;
    std::unordered_set<Ipv4Address> assignNeighborSet;
    // get all the STABLEorUNSTABLE one hop neighbor, mark as hiddenNode.
    // It means there is no need to forward the audio data to them.
    hiddenNode.insert(m_mainAddr);
    for(auto iter = m_nbStatus.begin(); iter != m_nbStatus.end(); iter++) {
        if(iter->status == NeighborStatus::STATUS_STABLE || iter->status == NeighborStatus::STATUS_UNSTABLE) {
            hiddenNode.insert(iter->neighborMainAddr);
        }
    }
    // find all the node that can be reached from the source node.
    // if find target, fill it into the reachableMap.
    for(auto oneHopIter = m_nbStatus.begin(); oneHopIter != m_nbStatus.end(); oneHopIter++) {
        for(auto twoHopIter = oneHopIter->twoHopNodeInfo.begin(); twoHopIter != oneHopIter->twoHopNodeInfo.end(); twoHopIter++) {
            if(hiddenNode.find(twoHopIter->neighborAddresses) == hiddenNode.end()) {
                if(reachableMap.find(twoHopIter->neighborAddresses) != reachableMap.end()) {
                    reachableMapElement tmpElem;
                    tmpElem.oneHopAddr = oneHopIter->neighborMainAddr;
                    tmpElem.linkStatus = JudgeTwoHopLinkStatus(oneHopIter->status, (NeighborStatus::Status)twoHopIter->linkStatus);
                    reachableMap[twoHopIter->neighborAddresses].push_back(tmpElem);
                } else {
                    reachableMapElement tmpElem;
                    tmpElem.oneHopAddr = oneHopIter->neighborMainAddr;
                    tmpElem.linkStatus = JudgeTwoHopLinkStatus(oneHopIter->status, (NeighborStatus::Status)twoHopIter->linkStatus);
                    std::vector<reachableMapElement> tmpVec;
                    tmpVec.push_back(tmpElem);
                    reachableMap.insert(std::make_pair(twoHopIter->neighborAddresses, tmpVec));
                }
            }
        }
    }
    // for(auto mapiter = reachableMap.begin(); mapiter != reachableMap.end(); mapiter++) {
    //     std::cout << "targetAddr:" << mapiter->first;
    //     for(auto veciter = mapiter->second.begin(); veciter != mapiter->second.end(); veciter++) {
    //         std::cout << " [" << veciter->oneHopAddr << "]-" << (uint16_t)veciter->linkStatus;
    //     }
    //     std::cout << std::endl;
    // }

    // find all the target node that can be reached from one way, make that onehop node as the assign forward node;

    for(auto mapiter = reachableMap.begin(); mapiter != reachableMap.end(); mapiter++) {
        if(mapiter->second.size() == 1) {
            assignNeighborSet.insert(mapiter->second[0].oneHopAddr);
        }
    }
    // // If there is more than one path to reach a target node, first check whether any of the arriving nodes are already in the assigned node SET
    // // If there is, do not further evaluate; if there isn't, then compare among the multiple paths.

    // todo: 最终的选择并不完备，对于所有有2个选择以上的target，其中间仍然可能存在相同的，在这没有处理，只是按顺序查找。
    for(auto mapiter = reachableMap.begin(); mapiter != reachableMap.end(); mapiter++) {
        if(mapiter->second.size() > 1) {
            bool found = false;
            for(auto veciter = mapiter->second.begin(); veciter != mapiter->second.end(); veciter++) {
                if(assignNeighborSet.find(veciter->oneHopAddr) != assignNeighborSet.end()) {
                    found = true;
                    break;
                }
            }
            if(!found) {
                Ipv4Address winAddr = Ipv4Address("255.255.255.255");
                twoHopLinkJudge winLink = TWOHOP_LINK_INVALID;
                for(auto veciter = mapiter->second.begin(); veciter != mapiter->second.end(); veciter++) {
                    if (veciter->linkStatus < winLink)
                    {
                        winLink = veciter->linkStatus;
                        winAddr = veciter->oneHopAddr;
                    }
                }
                assignNeighborSet.insert(winAddr);
            }
        }
    }

    header.AssignNum = 0;
    for(auto setIter = assignNeighborSet.begin(); setIter != assignNeighborSet.end(); setIter++) {
        header.AssignNeighbor.push_back(*setIter);
        header.AssignNum++;
    }
}

void
RangerNeighborList::Print(std::ostream& os) const
{
    os << "----------------[" << m_mainAddr << "]---------------- AT +" << Simulator::Now().GetMilliSeconds() << "ms" << std::endl;
    for(auto iter = m_nbStatus.begin(); iter != m_nbStatus.end(); iter++) {
        os << "[" << iter->neighborMainAddr << "]";
        os << "(" << (uint16_t)iter->lqi << "-" << (uint16_t)iter->status << "):";
        iter->Print(os);
    }
    os << "----------------[" << m_mainAddr << "]---------------- AT +" << Simulator::Now().GetMilliSeconds() << "ms" << std::endl;
}

} // namespace ns3