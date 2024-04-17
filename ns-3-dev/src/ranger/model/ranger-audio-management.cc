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
#include "ranger-audio-management.h"

namespace ns3
{

#define UINT8_SIZE (256)
static uint8_t SeqElap(uint8_t newSeq, uint8_t nowSeq) {
    return (newSeq > nowSeq) ? newSeq - nowSeq : UINT8_SIZE - (nowSeq - newSeq);
}

RangerAudioManagement::RangerAudioManagement() {
    m_sourceAudioSeq = 0;
}

RangerAudioManagement::~RangerAudioManagement() {

}

void
RangerAudioManagement::setMainAddr(Ipv4Address mainAddr) {
    m_mainAddr = mainAddr;
    m_sourceAudioSeq = 0;
}

uint8_t
RangerAudioManagement::getSelfSeq() {
    return m_sourceAudioSeq++;
}

bool
RangerAudioManagement::isNewSeq(Ipv4Address oriAddr, uint8_t seq, Time now) {
    if(oriAddr == m_mainAddr)
    {
        return false;
    }

    for(auto iter = m_seqTable.begin(); iter != m_seqTable.end(); iter++) {
        if (iter->oriAddr == oriAddr)
        {
            if(now - (iter->lastTime) >= Seconds(1.0) || SeqElap(seq, (iter->currSeq)) < 10) {
                iter->currSeq = seq;
                iter->lastTime = now;
                return true;
            } else {
                return false;
            }
        }
    }
    SeqTableElement tmp;
    tmp.oriAddr = oriAddr;
    tmp.currSeq = seq;
    tmp.lastTime = now;
    m_seqTable.push_back(tmp);
    return true;
}

} // namespace ns3