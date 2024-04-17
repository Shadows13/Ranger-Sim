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
#include "ns3/vector.h"
#include <ns3/netanim-module.h>

using namespace ns3;

void CheckDistances(NodeContainer &nodes, double maxDistance) {
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
            Ptr<RandomWalk2dMobilityModel> mobility = node->GetObject<RandomWalk2dMobilityModel>();
            mobility->SetAttribute("Mode", StringValue("Time"));
            mobility->SetAttribute("Time", TimeValue(Seconds(1.0)));
            mobility->SetAttribute("Speed", StringValue("ns3::ConstantRandomVariable[Constant=1.0]"));
            mobility->SetAttribute("Direction", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=6.283184]"));
        }
    }
}

int main(int argc, char *argv[]) {
    CommandLine cmd;
    cmd.Parse(argc, argv);

    NodeContainer nodes;
    nodes.Create(16);

    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                              "Mode", StringValue("Time"),
                              "Time", TimeValue(Seconds(5.0)),
                              "Speed", StringValue("ns3::ConstantRandomVariable[Constant=5.0]"),
                              "Bounds", RectangleValue(Rectangle(-500, 500, -500, 500)));
    mobility.Install(nodes);

    Simulator::Schedule(Seconds(1.0), &CheckDistances, std::ref(nodes), 300.0);

    AnimationInterface anim ("animation.xml");
    Simulator::Stop(Seconds(1000.0));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}

