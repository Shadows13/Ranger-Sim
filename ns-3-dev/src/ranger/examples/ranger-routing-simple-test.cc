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
 * Author:  Tom Henderson <thomas.r.henderson@boeing.com>
 */

/*
 * Try to send data end-to-end through a LrWpanMac <-> LrWpanPhy <->
 * SpectrumChannel <-> LrWpanPhy <-> LrWpanMac chain
 *
 * Trace Phy state changes, and Mac DataIndication and DataConfirm events
 * to stdout
 */
#include <ns3/constant-position-mobility-model.h>
#include <ns3/core-module.h>
#include <ns3/log.h>
#include <ns3/ranger-module.h>
#include <ns3/lr-wpan-module.h>
#include <ns3/packet.h>
#include <ns3/propagation-delay-model.h>
#include <ns3/propagation-loss-model.h>
#include <ns3/simulator.h>
#include <ns3/single-model-spectrum-channel.h>

#include <ns3/test.h>

#include <iostream>

using namespace ns3;

void
ReceivePdDataIndication(uint32_t psduLength, Ptr<Packet> p, uint8_t lqi)
{
    // NS_LOG_UNCOND("At: " << Simulator::Now() << " Received frame size: " << psduLength
    //                      << " LQI: " << (uint16_t)lqi << std::endl);
    MessageHeader msgHdr;
    p->RemoveHeader(msgHdr);
    std::ostringstream oss;
    msgHdr.Print(oss); // 将输出重定向到字符串流
    NS_LOG_UNCOND(oss.str()); // 将捕获的字符串输出到日志
}

int
main(int argc, char* argv[])
{
    bool verbose = false;
    bool extended = false;

    CommandLine cmd(__FILE__);

    cmd.AddValue("verbose", "turn on all log components", verbose);
    cmd.AddValue("extended", "use extended addressing", extended);

    cmd.Parse(argc, argv);
    // LogComponentEnable("RangerNetDevice", LOG_LEVEL_FUNCTION);
    //LogComponentEnable("LrWpanPhy", LOG_LEVEL_FUNCTION);
    LogComponentEnable("RangerRoutingProtocol", LOG_LEVEL_INFO);

    // Enable calculation of FCS in the trailers. Only necessary when interacting with real devices
    // or wireshark. GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

    // Create 2 nodes, and a NetDevice for each one
    Ptr<Node> n0 = CreateObject<Node>();
    Ptr<Node> n1 = CreateObject<Node>();

    Ptr<RangerNetDevice> dev0 = CreateObject<RangerNetDevice>();
    Ptr<RangerNetDevice> dev1 = CreateObject<RangerNetDevice>();

    // Ptr<LrWpanErrorModel> model = CreateObject<LrWpanErrorModel>();
    // dev0->GetPhy()->SetErrorModel(model);
    // dev1->GetPhy()->SetErrorModel(model);

    dev0->SetAddress(Ipv4Address("255.255.255.255"));
    dev1->SetAddress(Ipv4Address("255.255.255.254"));

    // Each device must be attached to the same channel
    Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel>();
    Ptr<LogDistancePropagationLossModel> propModel =
        CreateObject<LogDistancePropagationLossModel>();
    Ptr<ConstantSpeedPropagationDelayModel> delayModel =
        CreateObject<ConstantSpeedPropagationDelayModel>();
    channel->AddPropagationLossModel(propModel);
    channel->SetPropagationDelayModel(delayModel);

    dev0->SetChannel(channel);
    dev1->SetChannel(channel);

    // To complete configuration, a LrWpanNetDevice must be added to a node
    n0->AddDevice(dev0);
    n1->AddDevice(dev1);

    // // Trace state changes in the phy
    // dev0->GetPhy()->TraceConnect("TrxState",
    //                              std::string("phy0"),
    //                              MakeCallback(&StateChangeNotification));
    // dev1->GetPhy()->TraceConnect("TrxState",
    //                              std::string("phy1"),
    //                              MakeCallback(&StateChangeNotification));

    Ptr<ConstantPositionMobilityModel> sender0Mobility =
        CreateObject<ConstantPositionMobilityModel>();
    sender0Mobility->SetPosition(Vector(0, 0, 0));
    dev0->GetPhy()->SetMobility(sender0Mobility);
    Ptr<ConstantPositionMobilityModel> sender1Mobility =
        CreateObject<ConstantPositionMobilityModel>();
    // Configure position 10 m distance
    sender1Mobility->SetPosition(Vector(0, 10, 0));
    dev1->GetPhy()->SetMobility(sender1Mobility);

    // AsciiTraceHelper ascii;
    // Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream("lr-wpan-data.tr");

    // The below should trigger two callbacks when end-to-end data is working
    // 1) DataConfirm callback is called
    // 2) DataIndication callback is called with value of 50
    // Ptr<Packet> p0 = Create<Packet>(0); // 50 bytes of dummy data

    // MessageHeader msgHdr;
    // msgHdr.SetMessageType(MessageHeader::NODEINFO_MESSAGE);
    // msgHdr.SetSrcAddress(Ipv4Address("255.255.255.255"));

    // MessageHeader::NodeInfo& nodeinfoHdr = msgHdr.GetNodeInfo();
    // nodeinfoHdr.linkNumber = 0;
    // // MessageHeader::NodeInfo::LinkMessage lm;
    // // lm.linkQuality = 13;
    // // lm.neighborAddresses = Ipv4Address("0.0.0.0");
    // // nodeinfoHdr.linkMessages.push_back(lm);
    // // lm.linkQuality = 12;
    // // lm.neighborAddresses = Ipv4Address("0.1.0.0");
    // // nodeinfoHdr.linkMessages.push_back(lm);
    
    // msgHdr.SetMessageLength(msgHdr.GetSerializedSize());

    // p0->AddHeader(msgHdr);

    dev0->GetPhy()->PlmeSetTRXStateRequest(IEEE_802_15_4_PHY_TX_ON);
    dev1->GetPhy()->PlmeSetTRXStateRequest(IEEE_802_15_4_PHY_RX_ON);
    // Simulator::ScheduleWithContext(1,
    //                             Seconds(0.1),
    //                             &LrWpanPhy::PdDataRequest,
    //                             dev0->GetPhy(),
    //                             p0->GetSize(),
    //                             p0);

    // dev1->GetPhy()->SetPdDataIndicationCallback(MakeCallback(&ReceivePdDataIndication));
    // // Send a packet back at time 2 seconds
    // Ptr<Packet> p2 = Create<Packet>(60); // 60 bytes of dummy data
    // if (!extended)
    // {
    //     params.m_dstAddr = Mac16Address("00:01");
    // }
    // else
    // {
    //     params.m_dstExtAddr = Mac64Address("00:00:00:00:00:00:00:01");
    // }
    // Simulator::ScheduleWithContext(2,
    //                                Seconds(2.0),
    //                                &LrWpanMac::McpsDataRequest,
    //                                dev1->GetMac(),
    //                                params,
    //                                p2);

    Simulator::Run();

    Simulator::Destroy();
    return 0;
}
