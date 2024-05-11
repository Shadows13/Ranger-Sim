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
#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/random-walk-2d-mobility-model.h"
#include <ns3/propagation-delay-model.h>
#include <ns3/propagation-loss-model.h>
#include <ns3/netanim-module.h>
#include <ns3/ranger-module.h>
#include <ns3/lr-wpan-module.h>
#include <ns3/log.h>

#include <ns3/random-variable-stream.h>
#include <ns3/single-model-spectrum-channel.h>
#include <ns3/simulator.h>
#include <ns3/test.h>
#include <iostream>
#include <sstream>
#include <ns3/vector.h>
#include <ns3/gnuplot.h>

using namespace ns3;

void CalAST(Ptr<RangerMacRecorder> macRecorder, Ptr<RangerNetDevice> sender, Ptr<RangerNetDevice> receiver , uint32_t distance) {
    macRecorder->DoCalculate();
    std::cout << "Distance: " << distance << ",AvgSendTimes: " << macRecorder->GetAvgSendTimes() \
    << ",Lqi: " << (uint16_t)sender->GetRoutingProtocol()->GetNeighborLqi(receiver->GetRoutingProtocol()->GetMainAddress()) << std::endl;
    macRecorder->Clear();
}

int main(int argc, char *argv[]) {
    CommandLine cmd;
    cmd.Parse(argc, argv);
    // LogComponentEnable("RangerRoutingProtocol", LOG_LEVEL_INFO);
    // LogComponentEnable("RangerMac", LOG_LEVEL_INFO);
    // LogComponentEnable("RangerMac", LOG_LEVEL_FUNCTION);

    // plot
    std::ostringstream os;
    std::ofstream berFile("ranger-linkquality.plt");
    Gnuplot astPlot = Gnuplot("ranger-linkquality.png");
    Gnuplot2dDataset astDataset("ranger-linkquality");
    astPlot.SetTitle(os.str());
    astPlot.SetLegend("distance (m)", "Average Send Times (AST)");
    astPlot.SetTerminal("png size 1600,1200 font \"Merriweather, 22\"");
    astPlot.SetExtra(std::string("set xrange [0:500]\n") +
                                 "set yrange [0:3.3]\n" +
                                 "set grid\n" +
                                 "set linetype 1 lc rgb \"dark-red\" lw 2");


    // 配置一些Phy层参数
    double txPower = 30;
    uint32_t channelNumber = 11;
    double rxSensitivity = -93; // dBm

    uint8_t node_cnt = 2;
    NodeContainer nodes;
    nodes.Create(node_cnt);

    // 创建Channel
    Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel>();
    Ptr<LogDistancePropagationLossModel> propModel =
        CreateObject<LogDistancePropagationLossModel>();
    Ptr<ConstantSpeedPropagationDelayModel> delayModel =
        CreateObject<ConstantSpeedPropagationDelayModel>();
    channel->AddPropagationLossModel(propModel);
    channel->SetPropagationDelayModel(delayModel);

    // 创建网络设备 1、设置地址 2、绑定channel 3、设置Phy层参数 4、绑定Recorder 5、绑定移动模型 6、绑定到node
    std::vector<Ptr<RangerNetDevice>> devices;
    std::vector<Ptr<ConstantPositionMobilityModel>> mobilities;
    Ptr<RangerRecorder> recorder = CreateObject<RangerRecorder>();
    Ptr<RangerMacRecorder> macRecorder = CreateObject<RangerMacRecorder>();
    for(int i = 0; i < node_cnt; i++) {
        Ptr<RangerNetDevice> dev = CreateObject<RangerNetDevice>();
        Ptr<ConstantPositionMobilityModel> mob = CreateObject<ConstantPositionMobilityModel>();
        mob->SetPosition(Vector(0.0, 0.0, 0.0));
        // 设置地址
        std::string address = "255.255.255." + std::to_string(i);
        char *addr = new char[address.length() + 1];
        std::strcpy(addr, address.c_str());
        dev->SetAddress(Ipv4Address(addr));
        // 绑定channel
        dev->SetChannel(channel);

        // 设置Phy层参数
        LrWpanSpectrumValueHelper svh;
        Ptr<SpectrumValue> psd = svh.CreateTxPowerSpectralDensity(txPower, channelNumber);
        dev->GetPhy()->SetRxSensitivity(rxSensitivity);
        dev->GetPhy()->SetTxPowerSpectralDensity(psd);

        // 绑定到Recorder
        dev->GetRoutingProtocol()->SetReceiveTraceCallback(MakeCallback(&RangerRecorder::recordReceive, recorder));
        dev->GetRoutingProtocol()->SetSendTraceCallback(MakeCallback(&RangerRecorder::recordSend, recorder));
        dev->GetRoutingProtocol()->GetMac()->SetMacSendPktTraceCallback(MakeCallback(&RangerMacRecorder::SendPkt, macRecorder));
        dev->GetRoutingProtocol()->GetMac()->SetMacSendTimesTraceCallback(MakeCallback(&RangerMacRecorder::SendTimes, macRecorder));

        // 绑定移动模型
        nodes.Get(i)->AggregateObject(mob);

        // 绑定到node
        nodes.Get(i)->AddDevice(dev);

        mobilities.push_back(mob);
        devices.push_back(dev);
    }

    uint32_t WaitTime = 10;
    uint32_t WorkTime = 100;
    uint32_t min_distance = 300;
    uint32_t max_distance = 500;
    double intervalPacket = 0.05;

    for (int i = min_distance; i < max_distance; i += 1) {
        mobilities[1]->SetPosition(Vector(i, 0.0, 0.0));
        uint32_t tmpTime = 0;
        for(int j = 0; j < WorkTime / intervalPacket; j++) {
            Simulator::ScheduleWithContext(1,
                                        Seconds(WaitTime + intervalPacket * j),  // 每0.05秒触发一次
                                        &RangerRoutingProtocol::SourceAudioDataRequest,
                                        devices[0]->GetRoutingProtocol(),
                                        80);
        }
        for(int j = 0; j < WorkTime; j++) {
            Simulator::ScheduleWithContext(1,
                                        Seconds(WaitTime + j),  // 每0.05秒触发一次
                                        &CalAST,
                                        macRecorder,
                                        devices[0],
                                        devices[1],
                                        i);
        }
        Simulator::Stop(Seconds(WaitTime + WorkTime));
        Simulator::Run();
    }

    // AnimationInterface anim ("animation0430-S12-R12.xml");
    astPlot.AddDataset(astDataset);
    astPlot.GenerateOutput(berFile);
    berFile.close();

    Simulator::Destroy();
    return 0;
}

