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
    Simulator::Schedule(interval, &CheckDistances, std::ref(nodes), maxDistance, interval);
}

int main(int argc, char *argv[]) {

    uint32_t randomSeed = 44;
    uint32_t randomRun = 44;
    // float intervalPacket = 0.1;
    // LogComponentEnable("RangerRoutingProtocol", LOG_LEVEL_INFO);
    // LogComponentEnable("RangerRoutingProtocol", LOG_LEVEL_FUNCTION);
    // LogComponentEnable("RangerMac", LOG_LEVEL_INFO);
    // LogComponentEnable("RangerMac", LOG_LEVEL_FUNCTION);
   
    SeedManager::SetSeed(randomSeed);
    SeedManager::SetRun(randomRun);

    // 配置一些Phy层参数
    double txPower = 30;
    uint32_t channelNumber = 11;
    double rxSensitivity = -93; // dBm

    // 配置移动范围
    uint32_t x_max = 2000;
    uint32_t y_max = 2000;
    double max_distance = 500;

    // 创建发送节点，设置移动模型
    NodeContainer nodes_send;
    uint8_t nodes_send_cnt = 16;
    nodes_send.Create(nodes_send_cnt);
    MobilityHelper mobility_send;
    mobility_send.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    Ptr<ListPositionAllocator> positionAllocSend = CreateObject<ListPositionAllocator>();
    positionAllocSend->Add(Vector(x_max / 2, y_max / 2, 0));  // 为每个节点设置相同的初始位置
    mobility_send.SetPositionAllocator(positionAllocSend);
    mobility_send.Install(nodes_send);
    
    // 创建接收节点，设置移动模型
    NodeContainer nodes_recv;
    uint8_t nodes_recv_cnt = 0;
    nodes_recv.Create(nodes_recv_cnt);
    MobilityHelper mobility_recv;
    mobility_recv.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                              "Mode", StringValue("Time"),
                              "Time", TimeValue(Seconds(10.0)),
                              "Speed", StringValue("ns3::ConstantRandomVariable[Constant=5.0]"),
                              "Bounds", RectangleValue(Rectangle(0, x_max, 0, y_max)));
    Ptr<ListPositionAllocator> positionAllocRecv = CreateObject<ListPositionAllocator>();
    for (uint32_t i = 0; i < nodes_recv_cnt; ++i) {
        positionAllocRecv->Add(Vector(x_max / 2, y_max / 2, 0));  // 为每个节点设置相同的初始位置
    }
    mobility_recv.SetPositionAllocator(positionAllocRecv);
    mobility_recv.Install(nodes_recv);
    Simulator::Schedule(Seconds(5.0), &CheckDistances, std::ref(nodes_recv), max_distance, Seconds(5.0));

    // 创建边界节点
    BoundaryGuards(0, x_max, 0, y_max);

    // 创建Channel
    Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel>();
    Ptr<LogDistancePropagationLossModel> propModel =
        CreateObject<LogDistancePropagationLossModel>();
    Ptr<ConstantSpeedPropagationDelayModel> delayModel =
        CreateObject<ConstantSpeedPropagationDelayModel>();
    channel->AddPropagationLossModel(propModel);
    channel->SetPropagationDelayModel(delayModel);

    // 创建Recorder
    Ptr<RangerRecorder> recorder = CreateObject<RangerRecorder>();
    Ptr<RangerMacRecorder> macRecorder = CreateObject<RangerMacRecorder>();

    // 创建网络设备
    std::vector<Ptr<RangerNetDevice>> devices;
    int index = 0;

    // 1、设置地址 2、绑定channel 3、设置Phy层参数 4、绑定Recorder 5、绑定到node
    for (int i = 0; i < nodes_send_cnt; i++, index++) {
        Ptr<RangerNetDevice> dev = CreateObject<RangerNetDevice>();
        // 设置地址
        std::ostringstream oss;
        oss << std::setw(2) << std::setfill('0') << index;
        std::string addr = "255.255.255.1" + oss.str();
        dev->SetAddress(Ipv4Address(addr.c_str()));
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

        // 绑定到node
        nodes_send.Get(i)->AddDevice(dev);

        devices.push_back(dev);
    }

    for (int i = 0; i < nodes_recv_cnt; i++, index++) {
        Ptr<RangerNetDevice> dev = CreateObject<RangerNetDevice>();
        // 设置地址
        std::ostringstream oss;
        oss << std::setw(2) << std::setfill('0') << index;
        std::string addr = "255.255.255.1" + oss.str();
        dev->SetAddress(Ipv4Address(addr.c_str()));
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

        // 绑定到node
        nodes_recv.Get(i)->AddDevice(dev);

        devices.push_back(dev);
    }

    // int time = 100;
    // for (int i = 0; i < 100; ++i)
    // {
    //     for (int j = 0; j < 20; ++j)
    //     {
    //         Simulator::Schedule(Seconds(time + j * 0.05),
    //                             &RangerRoutingProtocol::SourceAudioDataRequest,
    //                             devices[time % 16]->GetRoutingProtocol(),
    //                             80);
    //     }
    //     time += 1;
    // }

    // Simulator::Schedule(Seconds(210),
    //                     &RangerMacRecorder::PrintAvgSendTimes,
    //                     macRecorder);

    AnimationInterface anim("./src/ranger/branchTest/animation-ack-forward.xml");
    Simulator::Stop(Seconds(2020.0));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
