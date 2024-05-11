#include <ns3/abort.h>
#include <ns3/callback.h>
#include <ns3/ranger-module.h>
#include <ns3/command-line.h>
#include <ns3/constant-position-mobility-model.h>
#include <ns3/gnuplot.h>
#include <ns3/log.h>
#include <ns3/lr-wpan-error-model.h>
#include <ns3/lr-wpan-mac.h>
#include <ns3/lr-wpan-net-device.h>
#include <ns3/lr-wpan-spectrum-value-helper.h>
#include <ns3/mac16-address.h>
#include <ns3/multi-model-spectrum-channel.h>
#include <ns3/net-device.h>
#include <ns3/node.h>
#include <ns3/nstime.h>
#include <ns3/packet.h>
#include <ns3/propagation-loss-model.h>
#include <ns3/simulator.h>
#include <ns3/single-model-spectrum-channel.h>
#include <ns3/spectrum-value.h>
#include <ns3/test.h>
#include <ns3/uinteger.h>
#include <ns3/ranger-mac.h>
#include "ns3/mobility-module.h"
#include "ns3/core-module.h"
#include "ns3/ranger-mac-recorder.h"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace ns3;


NS_LOG_COMPONENT_DEFINE("RangerErrorDistancePlot");


double
DbmToW(double dbm)
{
    return (pow(10.0, dbm / 10.0) / 1000.0);
}

Ptr<SpectrumValue>
CreateNoisePowerSpectralDensity(uint32_t channel, double rxSensitivity, Ptr<const SpectrumModel> spectrumModel)
{
    Ptr<SpectrumValue> noisePsd = Create<SpectrumValue>(spectrumModel);

    double noiseFactor = 1.0;
    double maxRxSensitivityW = DbmToW(-106.58);
    noiseFactor = DbmToW(rxSensitivity) / maxRxSensitivityW;

    static const double BOLTZMANN = 1.3803e-23;
    // Nt  is the power of thermal noise in W
    double Nt = BOLTZMANN * 290.0;
    // noise Floor (W) which accounts for thermal noise and non-idealities of the receiver
    double noisePowerDensity = noiseFactor * Nt;

    NS_ASSERT_MSG((channel >= 11 && channel <= 26), "Invalid channel numbers");

    (*noisePsd)[2405 + 5 * (channel - 11) - 2400 - 2] = noisePowerDensity;
    (*noisePsd)[2405 + 5 * (channel - 11) - 2400 - 1] = noisePowerDensity;
    (*noisePsd)[2405 + 5 * (channel - 11) - 2400] = noisePowerDensity;
    (*noisePsd)[2405 + 5 * (channel - 11) - 2400 + 1] = noisePowerDensity;
    (*noisePsd)[2405 + 5 * (channel - 11) - 2400 + 2] = noisePowerDensity;

    return noisePsd;
}

int
main(int argc, char* argv[])
{
    std::ostringstream os;
#if 0
    std::ofstream berFile("./src/ranger/branchTest/ranger-psr-distance.plt");
    Gnuplot psrPlot = Gnuplot("ranger-psr-distance.png");
    Gnuplot2dDataset psrDataset("ranger-psr-vs-distance");
    psrPlot.SetTitle(os.str());
    psrPlot.SetLegend("distance (m)", "Packet Success Rate (PSR)");
    psrPlot.SetTerminal("png size 1600,1200 font \"Merriweather, 22\"");
    psrPlot.SetExtra(std::string("set xrange [0:500]\n") +
                                 "set yrange [0:1.1]\n" +
                                 "set grid\n" +
                                 "set linetype 1 lc rgb \"dark-green\" lw 2");
#else
    std::ofstream berFile("./src/ranger/branchTest/ranger-ast-distance.plt");
    Gnuplot astPlot = Gnuplot("ranger-ast-distance.png");
    Gnuplot2dDataset astDataset("ranger-ast-vs-distance");
    astPlot.SetTitle(os.str());
    astPlot.SetLegend("distance (m)", "Average Send Times (AST)");
    astPlot.SetTerminal("png size 1600,1200 font \"Merriweather, 22\"");
    astPlot.SetExtra(std::string("set xrange [0:500]\n") +
                                 "set yrange [0:3.3]\n" +
                                 "set grid\n" +
                                 "set linetype 1 lc rgb \"dark-red\" lw 2");
#endif

    int minDistance = 100;
    int maxDistance = 420; // meters
    int increment = 1;
    int maxPackets = 100;
    int packetSize = 80; // PSDU = 100 bytes (20 bytes MAC header + 80 bytes MSDU )
    double txPower = 30;
    uint32_t channelNumber = 11;
    double rxSensitivity = -93; // dBm

    LogComponentEnable("RangerRoutingProtocol", LOG_LEVEL_INFO);
    LogComponentEnable("RangerMac", LOG_LEVEL_INFO);
    // LogComponentEnable("RangerMac", LOG_LEVEL_FUNCTION);
    // LogComponentEnable("LrWpanPhy", LOG_LEVEL_FUNCTION);

    os << "Packet (MSDU) size = " << packetSize << " bytes; tx power = " << txPower
       << " dBm; channel = " << channelNumber << "; Rx sensitivity = " << rxSensitivity << " dBm";

    // 创建发送节点，设置移动模型
    NodeContainer nodes_send;
    uint8_t nodes_send_cnt = 1;
    nodes_send.Create(nodes_send_cnt);
    MobilityHelper mobility_send;
    mobility_send.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    Ptr<ListPositionAllocator> positionAllocSend = CreateObject<ListPositionAllocator>();
    positionAllocSend->Add(Vector(0, 0, 0));  // 为每个节点设置相同的初始位置
    mobility_send.SetPositionAllocator(positionAllocSend);
    mobility_send.Install(nodes_send);
    
    // 创建接收节点，设置移动模型
    NodeContainer nodes_recv;
    uint8_t nodes_recv_cnt = 1;
    nodes_recv.Create(nodes_recv_cnt);
    MobilityHelper mobility_recv;
    mobility_recv.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    Ptr<ListPositionAllocator> positionAllocRecv = CreateObject<ListPositionAllocator>();
    positionAllocRecv->Add(Vector(0, 0, 0));  // 为每个节点设置相同的初始位置
    mobility_recv.SetPositionAllocator(positionAllocRecv);
    mobility_recv.Install(nodes_recv);

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
        std::string address = "255.255.255." + std::to_string(index);
        char *addr = new char[address.length() + 1];
        std::strcpy(addr, address.c_str());
        dev->SetAddress(Ipv4Address(addr));
        // 绑定channel
        dev->SetChannel(channel);

        // 设置Phy层参数
        LrWpanSpectrumValueHelper svh;
        Ptr<SpectrumValue> txPsd = svh.CreateTxPowerSpectralDensity(txPower, channelNumber);
        dev->GetPhy()->SetRxSensitivity(rxSensitivity);
        dev->GetPhy()->SetTxPowerSpectralDensity(txPsd);
        Ptr<const SpectrumValue> oriNoise = dev->GetPhy()->GetNoisePowerSpectralDensity();
        Ptr<const SpectrumModel> spectrumModel = oriNoise->GetSpectrumModel();
        Ptr<SpectrumValue> noisePsd = CreateNoisePowerSpectralDensity(channelNumber, rxSensitivity, spectrumModel);
        dev->GetPhy()->SetNoisePowerSpectralDensity(noisePsd);

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
        std::string address = "255.255.255." + std::to_string(index);
        char *addr = new char[address.length() + 1];
        std::strcpy(addr, address.c_str());
        dev->SetAddress(Ipv4Address(addr));
        // 绑定channel
        dev->SetChannel(channel);

        // 设置Phy层参数
        LrWpanSpectrumValueHelper svh;
        Ptr<SpectrumValue> txPsd = svh.CreateTxPowerSpectralDensity(txPower, channelNumber);
        dev->GetPhy()->SetRxSensitivity(rxSensitivity);
        dev->GetPhy()->SetTxPowerSpectralDensity(txPsd);
        Ptr<const SpectrumValue> oriNoise = dev->GetPhy()->GetNoisePowerSpectralDensity();
        Ptr<const SpectrumModel> spectrumModel = oriNoise->GetSpectrumModel();
        Ptr<SpectrumValue> noisePsd = CreateNoisePowerSpectralDensity(channelNumber, rxSensitivity, spectrumModel);
        dev->GetPhy()->SetNoisePowerSpectralDensity(noisePsd);

        // 绑定到Recorder
        dev->GetRoutingProtocol()->SetReceiveTraceCallback(MakeCallback(&RangerRecorder::recordReceive, recorder));
        dev->GetRoutingProtocol()->SetSendTraceCallback(MakeCallback(&RangerRecorder::recordSend, recorder));
        dev->GetRoutingProtocol()->GetMac()->SetMacSendPktTraceCallback(MakeCallback(&RangerMacRecorder::SendPkt, macRecorder));
        dev->GetRoutingProtocol()->GetMac()->SetMacSendTimesTraceCallback(MakeCallback(&RangerMacRecorder::SendTimes, macRecorder));

        // 绑定到node
        nodes_recv.Get(i)->AddDevice(dev);

        devices.push_back(dev);
    }

    Ptr<MobilityModel> mob = nodes_recv.Get(0)->GetObject<MobilityModel>();

    for (int j = minDistance; j <= maxDistance; j += increment)
    {
        for (int i = 0; i < maxPackets; i++)
        {
            Simulator::Schedule(Seconds(10 + i*0.05), &RangerRoutingProtocol::SourceAudioDataRequest,
                                    devices[0]->GetRoutingProtocol(),
                                    packetSize);
        }
        Simulator::Stop(Seconds(maxPackets*0.05 + 12));
        Simulator::Run();
#if 0
        recorder->DoCalculation();
        NS_LOG_UNCOND("Received " << recorder->GetTotalReceiveCnt() << " packets for distance " << j);
        psrDataset.Add(j, recorder->GetTotalReceiveRate());
        recorder->Clear();
#else
        macRecorder->DoCalculate();
        astDataset.Add(j, macRecorder->GetAvgSendTimes());
        // NS_LOG_UNCOND("Total send " << macRecorder->GetTotalSendTimesCnt() << " times for distance " << j);
        // NS_LOG_UNCOND("Total send " << macRecorder->GetTotalPktCnt() << " packets for distance " << j);
        NS_LOG_UNCOND("Avg send " << macRecorder->GetAvgSendTimes() << " times for distance " << j);
        macRecorder->Clear();
#endif
        mob->SetPosition(Vector(j+1, 0, 0));
    }

#if 0
    psrPlot.AddDataset(psrDataset);
    psrPlot.GenerateOutput(berFile);
#else
    astPlot.AddDataset(astDataset);
    astPlot.GenerateOutput(berFile);
#endif
    berFile.close();

    Simulator::Destroy();
    return 0;
}
