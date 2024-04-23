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
 * Author: Gary Pei <guangyu.pei@boeing.com>
 */
#include <ns3/command-line.h>
#include <ns3/constant-position-mobility-model.h>
#include <ns3/log.h>
#include <ns3/lr-wpan-phy.h>
#include <ns3/ranger-mac.h>
#include <ns3/packet.h>
#include <ns3/simulator.h>
#include <ns3/single-model-spectrum-channel.h>
#include <ns3/test.h>
#include <ns3/node.h>

#include <sstream>

using namespace ns3;


void
McpsDataIndication(ranger::McpsDataIndicationParams params, Ptr<Packet> p)
{
    // NS_LOG_UNCOND("At: " << Simulator::Now() << " Received Mcps Data Indication: " << params.m_dsn);

    // 输出packet的内容，每个字节用16进制表示
    uint8_t buffer[5] = {0};
    p->CopyData(buffer, 5);
    std::ostringstream oss;
    for (int i = 0; i < 5; i++)
    {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)buffer[i];
    }
    // NS_LOG_UNCOND("Packet content: " << oss.str());
}

/**
 * Send one packet
 * \param phy_sender phy_sender PHY
 * \param phy_receiver phy_receiver PHY
 */
void
SendOnePacket(Ptr<RangerMac> mac_sender, Ptr<RangerMac> mac_receiver)
{
    // NS_LOG_UNCOND("SendOnePacket");
    // 创建一个packet，里面的内容是"0011223344"
    uint32_t n = 10;
    uint8_t buffer[5] = {0x00, 0x11, 0x22, 0x33, 0x44};
    Ptr<Packet> p = Create<Packet>(buffer, n);

    ranger::McpsDataRequestParams params;
    // params.m_dstAddr = Ipv4Address("ff.ff.ff.ff");
    params.m_dstAddr = Ipv4Address("22.22.22.22");
    params.m_txOptions = 0x011;

    mac_sender->McpsDataRequest(params, p);
}

/**
 * 让sender和receiver互相连续的发送和接收数据
 * 用来验证CSMACA是否正常工作
*/
void
SendAndReceive(Ptr<RangerMac> mac_sender, Ptr<RangerMac> mac_receiver)
{
    // NS_LOG_UNCOND("SendAndReceive");
    SendOnePacket(mac_sender, mac_receiver);
    Simulator::Schedule(Seconds(1.0), &SendAndReceive, mac_sender, mac_receiver);
}

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    // LogComponentEnableAll(LOG_PREFIX_FUNC);
    // LogComponentEnable("LrWpanPhy", LOG_LEVEL_ALL);
    // LogComponentEnable("RangerMac", LOG_LEVEL_INFO);
    // LogComponentEnable("SingleModelSpectrumChannel", LOG_LEVEL_ALL);

    Ptr<LrWpanPhy> phy_sender = CreateObject<LrWpanPhy>();
    Ptr<LrWpanPhy> phy_receiver = CreateObject<LrWpanPhy>();

    Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel>();
    phy_sender->SetChannel(channel);
    phy_receiver->SetChannel(channel);
    channel->AddRx(phy_sender);
    channel->AddRx(phy_receiver);

    // CONFIGURE MOBILITY
    Ptr<ConstantPositionMobilityModel> senderMobility =
        CreateObject<ConstantPositionMobilityModel>();
    phy_sender->SetMobility(senderMobility);
    Ptr<ConstantPositionMobilityModel> receiverMobility =
        CreateObject<ConstantPositionMobilityModel>();
    phy_receiver->SetMobility(receiverMobility);

    Ptr<RangerMac> mac_sender = CreateObject<RangerMac>();
    mac_sender->SetAddress(Ipv4Address("11.11.11.11"));
    Ptr<RangerMac> mac_receiver = CreateObject<RangerMac>();
    mac_receiver->SetAddress(Ipv4Address("22.22.22.22"));

    mac_sender->SetPhy(phy_sender);
    mac_receiver->SetPhy(phy_receiver);

    phy_sender->SetPlmeSetTRXStateConfirmCallback(MakeCallback(&RangerMac::PlmeSetTRXStateConfirm, mac_sender));
    phy_receiver->SetPlmeSetTRXStateConfirmCallback(MakeCallback(&RangerMac::PlmeSetTRXStateConfirm, mac_receiver));

    phy_sender->SetPlmeCcaConfirmCallback(MakeCallback(&RangerMac::PlmeCcaConfirm, mac_sender));
    phy_receiver->SetPlmeCcaConfirmCallback(MakeCallback(&RangerMac::PlmeCcaConfirm, mac_receiver));

    phy_sender->SetPdDataConfirmCallback(MakeCallback(&RangerMac::PdDataConfirm, mac_sender));
    phy_receiver->SetPdDataConfirmCallback(MakeCallback(&RangerMac::PdDataConfirm, mac_receiver));

    phy_sender->SetPdDataIndicationCallback(MakeCallback(&RangerMac::PdDataIndication, mac_sender));
    phy_receiver->SetPdDataIndicationCallback(MakeCallback(&RangerMac::PdDataIndication, mac_receiver));

    mac_sender->SetMcpsDataIndicationCallback(MakeCallback(&McpsDataIndication));
    mac_receiver->SetMcpsDataIndicationCallback(MakeCallback(&McpsDataIndication));

    phy_sender->PlmeSetTRXStateRequest(IEEE_802_15_4_PHY_RX_ON);
    phy_receiver->PlmeSetTRXStateRequest(IEEE_802_15_4_PHY_RX_ON);

    Simulator::Schedule(Seconds(1.0), &SendOnePacket, mac_sender, mac_receiver);
    Simulator::Schedule(Seconds(2.0), &SendOnePacket, mac_sender, mac_receiver);
    Simulator::Schedule(Seconds(3.0), &SendOnePacket, mac_sender, mac_receiver);

    // Simulator::Schedule(Seconds(1.0), &SendAndReceive, mac_sender, mac_receiver);
    // Simulator::Schedule(Seconds(1.0), &SendAndReceive, mac_receiver, mac_sender);

    Simulator::Stop(Seconds(10.0));

    Simulator::Run();

    Simulator::Destroy();

    return 0;
}
