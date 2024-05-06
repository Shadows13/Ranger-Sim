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

using namespace ns3;

void BoundaryGuards(uint32_t x_min, uint32_t x_max, uint32_t y_min, uint32_t y_max) {
    NodeContainer boundaryNodes;
    boundaryNodes.Create(4);

    Ptr<ListPositionAllocator> boundaryPositions = CreateObject<ListPositionAllocator>();
    boundaryPositions->Add(Vector(x_min, y_min, 0));    // 左下角
    boundaryPositions->Add(Vector(x_max, y_min, 0));    // 右下角
    boundaryPositions->Add(Vector(x_min, y_max, 0));    // 左上角
    boundaryPositions->Add(Vector(x_max, y_max, 0));    // 右上角

    MobilityHelper boundaryMobility;
    boundaryMobility.SetPositionAllocator(boundaryPositions);
    boundaryMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    boundaryMobility.Install(boundaryNodes);
}


// 输入节点集合和当前逃逸的节点的索引，调整逃逸节点的方向，朝向所有节点中心点运动
double AdjustDirection(NodeContainer &nodes, uint8_t nodeIndex) {
    Vector pos_ave = Vector(0, 0, 0);
    for(uint8_t i = 0; i < nodes.GetN(); i++) {
        if(i == nodeIndex) continue;
        Ptr<Node> node = nodes.Get(i);
        Vector pos_i = node->GetObject<MobilityModel>()->GetPosition();
        pos_ave.x += pos_i.x;
        pos_ave.y += pos_i.y;
        pos_ave.z += pos_i.z;
    }
    pos_ave.x /= (nodes.GetN() - 1);
    pos_ave.y /= (nodes.GetN() - 1);
    pos_ave.z /= (nodes.GetN() - 1);

    Vector pos_target = nodes.Get(nodeIndex)->GetObject<MobilityModel>()->GetPosition();
    double angle = atan2(pos_ave.y - pos_target.y, pos_ave.x - pos_target.x);
    if (angle < 0) {
        angle += 2 * M_PI;
    }
    return angle;
}

void CheckDistances(NodeContainer &nodes, double maxDistance, Time interval) {
    for (uint32_t i = 0; i < nodes.GetN(); ++i) {
        Ptr<Node> node = nodes.Get(i);
        Vector pos_i = node->GetObject<MobilityModel>()->GetPosition();
        bool foundCloseNeighbor = false;

        for (uint32_t j = 0; j < nodes.GetN(); ++j) {
            if (i == j) continue;
            Ptr<Node> other = nodes.Get(j);
            Vector pos_j = other->GetObject<MobilityModel>()->GetPosition();
            double distance = CalculateDistance(pos_i, pos_j);
            if (distance < maxDistance) {
                foundCloseNeighbor = true;
                break;
            }
        }

        if (!foundCloseNeighbor) {
            // 如果没有找到近邻，重新设置移动模型的方向和速度
            double angle = AdjustDirection(nodes, i);
            Ptr<RandomWalk2dMobilityModel> mobility = node->GetObject<RandomWalk2dMobilityModel>();
            mobility->SetAttribute("Mode", StringValue("Time"));
            mobility->SetAttribute("Time", TimeValue(Seconds(10.0)));
            mobility->SetAttribute("Speed", StringValue("ns3::ConstantRandomVariable[Constant=5.0]"));
            Ptr<ConstantRandomVariable> directionVar = CreateObject<ConstantRandomVariable>();
            directionVar->SetAttribute("Constant", DoubleValue(angle));
            mobility->SetAttribute("Direction", PointerValue(directionVar));
        } else {
            Ptr<RandomWalk2dMobilityModel> mobility = node->GetObject<RandomWalk2dMobilityModel>();
            mobility->SetAttribute("Mode", StringValue("Time"));
            mobility->SetAttribute("Time", TimeValue(Seconds(10.0)));
            mobility->SetAttribute("Speed", StringValue("ns3::ConstantRandomVariable[Constant=5.0]"));
            mobility->SetAttribute("Direction", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=6.283184]"));
        }
    }
    Simulator::Schedule(interval, &CheckDistances, std::ref(nodes), 300.0, interval);
}

int main(int argc, char *argv[]) {
    CommandLine cmd;
    uint8_t nodeCnt = 0;
    uint32_t randomSeed = 0;
    uint32_t randomRun = 0;
    float intervalPacket = 0.0;
    cmd.AddValue("nodeCnt", "Number of nodes", nodeCnt);
    cmd.AddValue("randomSeed", "Random seed", randomSeed);
    cmd.AddValue("randomRun", "Random run", randomRun);
    cmd.AddValue("intervalPacket", "Interval between packets", intervalPacket);
    cmd.Parse(argc, argv);
    // LogComponentEnable("RangerRoutingProtocol", LOG_LEVEL_INFO);
    NS_LOG_UNCOND("nodeCnt: " << (int)nodeCnt << ", randomSeed: " << randomSeed << ", randomRun: " << randomRun << ", intervalPacket: " << intervalPacket);

    SeedManager::SetSeed(randomSeed);
    SeedManager::SetRun(randomRun);

    // 配置一些Phy层参数
    double txPower = 30;
    uint32_t channelNumber = 11;
    double rxSensitivity = -93; // dBm

    uint32_t x_max = 2000;
    uint32_t y_max = 2000;
    uint8_t node_cnt = nodeCnt;


    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    for (uint32_t i = 0; i < node_cnt; ++i) {
        positionAlloc->Add(Vector(x_max / 2, y_max / 2, 0));  // 为每个节点设置相同的初始位置
    }

    // 创建节点、设置移动模型
    NodeContainer nodes;
    nodes.Create(node_cnt);
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                              "Mode", StringValue("Time"),
                              "Time", TimeValue(Seconds(10.0)),
                              "Speed", StringValue("ns3::ConstantRandomVariable[Constant=5.0]"),
                              "Bounds", RectangleValue(Rectangle(0, x_max, 0, y_max)));
    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(nodes);
    Simulator::Schedule(Seconds(5.0), &CheckDistances, std::ref(nodes), 300.0, Seconds(5.0));

    // 创建边界节点
    BoundaryGuards(0, x_max, 0, y_max);

    // 创建Channle
    Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel>();
    Ptr<LogDistancePropagationLossModel> propModel =
        CreateObject<LogDistancePropagationLossModel>();
    Ptr<ConstantSpeedPropagationDelayModel> delayModel =
        CreateObject<ConstantSpeedPropagationDelayModel>();
    channel->AddPropagationLossModel(propModel);
    channel->SetPropagationDelayModel(delayModel);

    // 创建网络设备 1、设置地址 2、绑定channel 3、设置Phy层参数 4、绑定Recorder 5、绑定到node
    std::vector<Ptr<RangerNetDevice>> devices;
    Ptr<RangerRecorder> recorder = CreateObject<RangerRecorder>();
    for(int i = 0; i < node_cnt; i++) {
        Ptr<RangerNetDevice> dev = CreateObject<RangerNetDevice>();
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

        // 绑定到node
        nodes.Get(i)->AddDevice(dev);

        devices.push_back(dev);
    }

    float PacketNum = (float)(1000 - 100) / intervalPacket;
    for(int i = 0; i < PacketNum; i++) {
        Simulator::ScheduleWithContext(1,
                                    Seconds(10 + intervalPacket * i),  // 每0.5秒触发一次
                                    &RangerRoutingProtocol::SourceAudioDataRequest,
                                    devices[0]->GetRoutingProtocol(),
                                    80);
    }
    for(int i = 0; i < 10000; i++) {
        Simulator::ScheduleWithContext(1,
                                    Seconds(10 + intervalPacket * i),  // 每0.5秒触发一次
                                    &RangerRoutingProtocol::SourceAudioDataRequest,
                                    devices[1]->GetRoutingProtocol(),
                                    80);
    }
    for(int i = 0; i < 10000; i++) {
        Simulator::ScheduleWithContext(1,
                                    Seconds(10 + intervalPacket * i),  // 每0.5秒触发一次
                                    &RangerRoutingProtocol::SourceAudioDataRequest,
                                    devices[2]->GetRoutingProtocol(),
                                    80);
    }

    AnimationInterface anim ("animation0430-S12-R12.xml");
    Simulator::Stop(Seconds(1000.0));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}

