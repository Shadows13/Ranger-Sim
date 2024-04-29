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
#include <ns3/mobility-module.h>
#include <ns3/core-module.h>
#include <ns3/ranger-module.h>
#include <ns3/lr-wpan-module.h>
#include <ns3/netanim-module.h>
#include <ns3/log.h>
#include <ns3/packet.h>
#include <ns3/single-model-spectrum-channel.h>
#include <ns3/multi-model-spectrum-channel.h>
#include <ns3/propagation-delay-model.h>
#include <ns3/propagation-loss-model.h>
#include <ns3/simulator.h>


#include <ns3/test.h>

#include <iostream>

using namespace ns3;


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
    // LogComponentEnable("LrWpanPhy", LOG_LEVEL_FUNCTION);
    LogComponentEnable("RangerRoutingProtocol", LOG_LEVEL_INFO);
    // LogComponentEnable("RangerMac", LOG_LEVEL_FUNCTION);
    // LogComponentEnable("RangerMac", LOG_LEVEL_INFO);


    double txPower = 30;
    uint32_t channelNumber = 11;
    double rxSensitivity = -93; // dBm
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

    dev0->SetAddress(Ipv4Address("255.255.255.0"));
    dev1->SetAddress(Ipv4Address("255.255.255.1"));
    
    // NS_LOG_UNCOND("Rx Sensitivity: " << dev0->GetPhy()->GetRxSensitivity() << " dBm" << std::endl);
    // NS_LOG_UNCOND("Rx Sensitivity: " << dev1->GetPhy()->GetRxSensitivity() << " dBm" << std::endl);

    // Each device must be attached to the same channel
    Ptr<MultiModelSpectrumChannel> channel = CreateObject<MultiModelSpectrumChannel>();
    Ptr<LogDistancePropagationLossModel> propModel =
        CreateObject<LogDistancePropagationLossModel>();
    // Ptr<ConstantSpeedPropagationDelayModel> delayModel =
    //     CreateObject<ConstantSpeedPropagationDelayModel>();
    channel->AddPropagationLossModel(propModel);
    // channel->SetPropagationDelayModel(delayModel);

    dev0->SetChannel(channel);
    dev1->SetChannel(channel);

    // To complete configuration, a LrWpanNetDevice must be added to a node
    n0->AddDevice(dev0);
    n1->AddDevice(dev1);


    Ptr<ConstantPositionMobilityModel> mob0 = CreateObject<ConstantPositionMobilityModel>();
    Ptr<ConstantPositionMobilityModel> mob1 = CreateObject<ConstantPositionMobilityModel>();
    
    mob0->SetPosition(Vector(0.0, 0.0, 0.0));
    mob1->SetPosition(Vector(0.0, 400.0, 0.0));


    n0->AggregateObject(mob0);
    n1->AggregateObject(mob1);


    LrWpanSpectrumValueHelper svh;
    Ptr<SpectrumValue> psd = svh.CreateTxPowerSpectralDensity(txPower, channelNumber);
    dev0->GetPhy()->SetRxSensitivity(rxSensitivity);
    dev1->GetPhy()->SetRxSensitivity(rxSensitivity);
    dev0->GetPhy()->SetTxPowerSpectralDensity(psd);
    dev1->GetPhy()->SetTxPowerSpectralDensity(psd);


    // for(int i = 0; i < 100; i++) {
    //     Simulator::ScheduleWithContext(1,
    //                                 Seconds(5 + 0.1 * i),  // 每0.5秒触发一次
    //                                 &RangerRoutingProtocol::SourceAudioDataRequest,
    //                                 dev0->GetRoutingProtocol(),
    //                                 80);
    // }

    // for (int j = 10; j < 800; j += 10)
    // {
    //     Simulator::ScheduleWithContext(1,
    //                                 Seconds(10.0 * (j / 10)),  // 每0.5秒触发一次
    //                                 &MobilityModel::SetPosition,
    //                                 mob1,
    //                                 Vector(0.0, 100, 0.0));
    //     //mob1->SetPosition(Vector(0, j, 0));
    // }
    // Schedule a data request
    AnimationInterface anim ("animation.xml");
    Simulator::Stop(Seconds(10.0));
    Simulator::Run();

    Simulator::Destroy();
    return 0;
}
