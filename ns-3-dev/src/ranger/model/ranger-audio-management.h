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
#ifndef RANGER_AUDIO_MANAGEMENT_H
#define RANGER_AUDIO_MANAGEMENT_H


#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"

namespace ns3
{

struct SeqTableElement
{
    Ipv4Address oriAddr;
	uint8_t	currSeq;
	Time lastTime;
};

typedef std::vector<SeqTableElement> SeqTable;

class RangerAudioManagement
{
  private:
	Ipv4Address m_mainAddr;
    uint8_t 	m_sourceAudioSeq;
	SeqTable	m_seqTable;

  public:
    RangerAudioManagement();
    ~RangerAudioManagement();

	void setMainAddr(Ipv4Address mainAddr);
	uint8_t getSelfSeq();
	bool isNewSeq(Ipv4Address oriAddr, uint8_t seq, Time now);
};


} // namespace ns3

#endif /* RANGER_AUDIO_MANAGEMENT_H */
