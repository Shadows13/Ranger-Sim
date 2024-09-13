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
#ifndef RANGER_AUDIO_APPLICATION_H
#define RANGER_AUDIO_APPLICATION_H
#include <unordered_map>
#include <vector>

#include "ns3/nstime.h"
#include "ns3/ipv4-address.h"
#include <ns3/ranger-routing-nblist.h>
#include <ns3/ranger-routing-protocol.h>
#include <ns3/ranger-net-device.h>

#include <ns3/object.h>


namespace ns3
{

struct SentenceReceiveStatus
{
    Ipv4Address mainAddr;
    uint8_t receiveCnt;
    Time totalDelay;
};


struct RangerAudioAppSentenceElement
{
    Time startTime;
    uint8_t startSeq;
    uint8_t pacNum;
    uint32_t interval_ms;
    Ipv4Address oriAddr;
    std::vector<SentenceReceiveStatus> receiveList;
};

struct RangerAudioAppSentenceRecord
{
    Time startTime;
    Ipv4Address oriAddr;
    float receiveRate;
    Time aveDelay;
};

class RangerAudioApp : public Object
{
  private:
    std::vector<Ptr<RangerNetDevice>> devices;
    std::vector<RangerAudioAppSentenceRecord> records;
    std::vector<RangerAudioAppSentenceElement> currRecords;


  public:
    RangerAudioApp();
    ~RangerAudioApp();

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    void setDevices(std::vector<Ptr<RangerNetDevice>> devices);

    void audioSend(Ipv4Address sender);
    void audioReceive(Ipv4Address receiver, Ipv4Address sender, uint8_t seq, Time time);
    void periodClean();
    void periodPrint();
};


} // namespace ns3

#endif /* RANGER_AUDIO_APPLICATION_H */
