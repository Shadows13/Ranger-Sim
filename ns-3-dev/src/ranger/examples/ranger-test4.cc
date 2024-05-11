#include <iostream>
#include <ns3/core-module.h>
#include <ns3/lr-wpan-module.h>
#include <ns3/log.h>
#include <ns3/mobility-module.h>
#include <ns3/netanim-module.h>
#include <ns3/network-module.h>
#include <ns3/propagation-delay-model.h>
#include <ns3/propagation-loss-model.h>
#include <ns3/random-variable-stream.h>
#include <ns3/random-walk-2d-mobility-model.h>
#include <ns3/ranger-module.h>
#include <ns3/single-model-spectrum-channel.h>
#include <ns3/simulator.h>
#include <ns3/test.h>
#include <ns3/vector.h>

using namespace ns3;


void AdjustDirection(Ptr<Node> node, Vector averageDirection) {
    Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
    Vector position = mobility->GetPosition();
    Vector newPosition = Vector(position.x + averageDirection.x, position.y + averageDirection.y, 0);
    mobility->SetPosition(newPosition);
}

void CheckDistances(NodeContainer &nodes, double minDistance) {
    Vector posA, posB;
    double distance;

    for (auto itA = nodes.Begin(); itA != nodes.End(); ++itA) {
        Ptr<Node> nodeA = *itA;
        Ptr<MobilityModel> mobA = nodeA->GetObject<MobilityModel>();
        posA = mobA->GetPosition();
        Vector directionSum(0, 0, 0);
        int neighborCount = 0;

        for (auto itB = nodes.Begin(); itB != nodes.End(); ++itB) {
            if (itA == itB) continue;

            Ptr<Node> nodeB = *itB;
            Ptr<MobilityModel> mobB = nodeB->GetObject<MobilityModel>();
            posB = mobB->GetPosition();
            distance = CalculateDistance(posA, posB);

            if (distance < minDistance) {
                directionSum.x += (posB.x - posA.x);
                directionSum.y += (posB.y - posA.y);
                neighborCount++;
            }
        }

        if (neighborCount == 0) {
            // Apply random direction with a slight bias towards the center of the area
            Ptr<UniformRandomVariable> randomVar = CreateObject<UniformRandomVariable>();
            directionSum.x = randomVar->GetValue(-1, 1);
            directionSum.y = randomVar->GetValue(-1, 1);
        } else {
            directionSum.x /= neighborCount;
            directionSum.y /= neighborCount;
        }

        AdjustDirection(nodeA, directionSum);
    }
}

void ChangePosition(Ptr<Node> node, Vector position) {
    Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
    mobility->SetPosition(position);
    NS_LOG_UNCOND("Distance Change!");
}

int main(int argc, char *argv[]) {

    LogComponentEnable("RangerRoutingProtocol", LOG_LEVEL_INFO);
    LogComponentEnable("RangerMac", LOG_LEVEL_INFO);
    LogComponentEnable("RangerMac", LOG_LEVEL_FUNCTION);
    // LogComponentEnable("LrWpanPhy", LOG_LEVEL_FUNCTION);

    SeedManager::SetSeed(1);
    SeedManager::SetRun(2);

    // 配置一些Phy层参数
    double txPower = 30;
    uint32_t channelNumber = 11;
    double rxSensitivity = -93; // dBm

    // 创建节点、设置移动模型
    NodeContainer nodes;
    int node_num = 2;
    nodes.Create(node_num);
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));      // n0的位置
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));      // n1的位置
    // positionAlloc->Add(Vector(0.0, 100.0, 0.0));    // n2的位置
    // positionAlloc->Add(Vector(100.0, 100.0, 0.0));  // n3的位置
    // positionAlloc->Add(Vector(0.0, 200.0, 0.0));    // n4的位置
    // positionAlloc->Add(Vector(100.0, 200.0, 0.0));  // n5的位置
    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(nodes);

    // 创建Channel
    Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel>();
    Ptr<LogDistancePropagationLossModel> propModel =
        CreateObject<LogDistancePropagationLossModel>();
    Ptr<ConstantSpeedPropagationDelayModel> delayModel =
        CreateObject<ConstantSpeedPropagationDelayModel>();
    channel->AddPropagationLossModel(propModel);
    channel->SetPropagationDelayModel(delayModel);

    // 创建网络设备 1、设置地址 2、绑定channel 3、设置Phy层参数 4、绑定到node
    std::vector<Ptr<RangerNetDevice>> devices;
    for(int i = 0; i < node_num; i++) {
        Ptr<RangerNetDevice> dev = CreateObject<RangerNetDevice>();
        // 设置地址
        std::string address = "255.255.255.10" + std::to_string(i);
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

        // 绑定到node
        nodes.Get(i)->AddDevice(dev);

        devices.push_back(dev);
    }

    int time = 0;
    for (int i = 0; i < 500; ++i)
    {
        Simulator::Schedule(Seconds(time),
                            &ChangePosition,
                            nodes.Get(1),
                            Vector(double(i), 0.0, 0.0));
        time += 2;
        for (int j = 0; j < 10; ++j)
        {
            Simulator::Schedule(Seconds(time),
                                &RangerRoutingProtocol::SourceAudioDataRequest,
                                devices[0]->GetRoutingProtocol(),
                                80);
            time += 1;
        }
    }

    // Simulator::Schedule(Seconds(1),
    //                     &ChangePosition,
    //                     nodes.Get(1),
    //                     Vector(350.0, 0.0, 0.0));
    // Simulator::Schedule(Seconds(5),
    //                     &RangerRoutingProtocol::SourceAudioDataRequest,
    //                     devices[0]->GetRoutingProtocol(),
    //                     80);
    // Simulator::Schedule(Seconds(6),
    //                     &RangerRoutingProtocol::SourceAudioDataRequest,
    //                     devices[0]->GetRoutingProtocol(),
    //                     80);
    // Simulator::Schedule(Seconds(7),
    //                     &RangerRoutingProtocol::SourceAudioDataRequest,
    //                     devices[0]->GetRoutingProtocol(),
    //                     80);

    AnimationInterface anim ("./src/ranger/branchTest/animation.xml");
    Simulator::Stop(Seconds(10.0));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
