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
 *  Tom Henderson <thomas.r.henderson@boeing.com>
 *  Tommaso Pecorella <tommaso.pecorella@unifi.it>
 *  Margherita Filippetti <morag87@gmail.com>
 */
#include "ranger-net-device.h"

#include <ns3/abort.h>
#include <ns3/boolean.h>
#include <ns3/log.h>
#include <ns3/node.h>
#include <ns3/packet.h>
#include <ns3/pointer.h>
#include <ns3/spectrum-channel.h>

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("RangerNetDevice");

NS_OBJECT_ENSURE_REGISTERED(RangerNetDevice);

// ----------------------From NetDevice----------------------
TypeId
RangerNetDevice::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::RangerNetDevice")
            .SetParent<NetDevice>()
            .SetGroupName("Ranger")
            .AddConstructor<RangerNetDevice>()
            ;
    return tid;
}

RangerNetDevice::RangerNetDevice()
{
    NS_LOG_FUNCTION(this);
    // m_mac = CreateObject<LrWpanMac>();
    m_phy = CreateObject<LrWpanPhy>();
    //m_csmaca = CreateObject<LrWpanCsmaCa>();
}

RangerNetDevice::~RangerNetDevice()
{
    NS_LOG_FUNCTION(this);
}

void
RangerNetDevice::SetIfIndex(const uint32_t index)
{
    NS_LOG_FUNCTION(this << index);
    m_ifIndex = index;
}

uint32_t
RangerNetDevice::GetIfIndex() const
{
    NS_LOG_FUNCTION(this);
    return m_ifIndex;
}

Ptr<Channel>
RangerNetDevice::GetChannel() const
{
    NS_LOG_FUNCTION(this);
    return m_phy->GetChannel();
}

void
RangerNetDevice::SetAddress(Address address)
{
    NS_LOG_FUNCTION(this);
    // if (Mac16Address::IsMatchingType(address))
    // {
    //     m_mac->SetShortAddress(Mac16Address::ConvertFrom(address));
    // }
    // else if (Mac64Address::IsMatchingType(address))
    // {
    //     m_mac->SetExtendedAddress(Mac64Address::ConvertFrom(address));
    // }
    // else if (Mac48Address::IsMatchingType(address))
    // {
    //     uint8_t buf[6];
    //     Mac48Address addr = Mac48Address::ConvertFrom(address);
    //     addr.CopyTo(buf);
    //     Mac16Address addr16;
    //     addr16.CopyFrom(buf + 4);
    //     m_mac->SetShortAddress(addr16);
    //     uint16_t panId;
    //     panId = buf[0];
    //     panId <<= 8;
    //     panId |= buf[1];
    //     m_mac->SetPanId(panId);
    // }
    // else
    // {
    //     NS_ABORT_MSG("LrWpanNetDevice::SetAddress - address is not of a compatible type");
    // }
    m_neighborList.SetMainAddress(Ipv4Address::ConvertFrom(address));
}

Address
RangerNetDevice::GetAddress() const
{
    NS_LOG_FUNCTION(this);

    // if (m_mac->GetShortAddress() == Mac16Address("00:00"))
    // {
    //     return m_mac->GetExtendedAddress();
    // }

    Mac48Address pseudoAddress = 0;
        // BuildPseudoMacAddress(m_mac->GetPanId(), m_mac->GetShortAddress());

    return pseudoAddress;
}
bool
RangerNetDevice::SetMtu(const uint16_t mtu)
{
    NS_ABORT_MSG("Unsupported");
    return false;
}

uint16_t
RangerNetDevice::GetMtu() const
{
    NS_LOG_FUNCTION(this);
    // Maximum payload size is: max psdu - frame control - seqno - addressing - security - fcs
    //                        = 127      - 2             - 1     - (2+2+2+2)  - 0        - 2
    //                        = 114
    // assuming no security and addressing with only 16 bit addresses without pan id compression.
    return 114;
}

bool
RangerNetDevice::IsLinkUp() const
{
    NS_LOG_FUNCTION(this);
    return m_phy && m_linkUp;
}

void
RangerNetDevice::LinkUp()
{
    NS_LOG_FUNCTION(this);
    m_linkUp = true;
    m_linkChanges();
}

void
RangerNetDevice::LinkDown()
{
    NS_LOG_FUNCTION(this);
    m_linkUp = false;
    m_linkChanges();
}

void
RangerNetDevice::AddLinkChangeCallback(Callback<void> callback)
{
    NS_LOG_FUNCTION(this);
    m_linkChanges.ConnectWithoutContext(callback);
}

bool
RangerNetDevice::IsBroadcast() const
{
    NS_LOG_FUNCTION(this);
    return true;
}

Address
RangerNetDevice::GetBroadcast() const
{
    NS_LOG_FUNCTION(this);

    Mac48Address pseudoAddress = 0;
        // BuildPseudoMacAddress(m_mac->GetPanId(), Mac16Address::GetBroadcast());

    return pseudoAddress;
}

bool
RangerNetDevice::IsMulticast() const
{
    NS_LOG_FUNCTION(this);
    return true;
}

Address
RangerNetDevice::GetMulticast(Ipv4Address multicastGroup) const
{
    NS_ABORT_MSG("Unsupported");
    return Address();
}

Address
RangerNetDevice::GetMulticast(Ipv6Address addr) const
{
    NS_LOG_FUNCTION(this << addr);

    Mac48Address pseudoAddress = 0;
        // BuildPseudoMacAddress(m_mac->GetPanId(), Mac16Address::GetMulticast(addr));

    return pseudoAddress;
}

bool
RangerNetDevice::IsBridge() const
{
    NS_LOG_FUNCTION(this);
    return false;
}

bool
RangerNetDevice::IsPointToPoint() const
{
    NS_LOG_FUNCTION(this);
    return false;
}

bool
RangerNetDevice::Send(Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber)
{
    // This method basically assumes an 802.3-compliant device, but a raw
    // 802.15.4 device does not have an ethertype, and requires specific
    // McpsDataRequest parameters.
    // For further study:  how to support these methods somehow, such as
    // inventing a fake ethertype and packet tag for McpsDataRequest
    NS_LOG_FUNCTION(this << packet << dest << protocolNumber);

    if (packet->GetSize() > GetMtu())
    {
        NS_LOG_ERROR("Fragmentation is needed for this packet, drop the packet ");
        return false;
    }

    // McpsDataRequestParams m_mcpsDataRequestParams;

    // Mac16Address dst16;
    // if (Mac48Address::IsMatchingType(dest))
    // {
    //     uint8_t buf[6];
    //     dest.CopyTo(buf);
    //     dst16.CopyFrom(buf + 4);
    // }
    // else
    // {
    //     dst16 = Mac16Address::ConvertFrom(dest);
    // }
    // m_mcpsDataRequestParams.m_dstAddr = dst16;
    // m_mcpsDataRequestParams.m_dstAddrMode = SHORT_ADDR;
    // m_mcpsDataRequestParams.m_dstPanId = m_mac->GetPanId();
    // m_mcpsDataRequestParams.m_srcAddrMode = SHORT_ADDR;
    // // Using ACK requests for broadcast destinations is ok here. They are disabled
    // // by the MAC.
    // if (m_useAcks)
    // {
    //     m_mcpsDataRequestParams.m_txOptions = TX_OPTION_ACK;
    // }
    // m_mcpsDataRequestParams.m_msduHandle = 0;
    // m_mac->McpsDataRequest(m_mcpsDataRequestParams, packet);
    return true;
}

bool
RangerNetDevice::SendFrom(Ptr<Packet> packet,
                          const Address& source,
                          const Address& dest,
                          uint16_t protocolNumber)
{
    NS_ABORT_MSG("Unsupported");
    // TODO: To support SendFrom, the MACs McpsDataRequest has to use the provided source address,
    // instead of to local one.
    return false;
}

Ptr<Node>
RangerNetDevice::GetNode() const
{
    NS_LOG_FUNCTION(this);
    return m_node;
}

void
RangerNetDevice::SetNode(Ptr<Node> node)
{
    NS_LOG_FUNCTION(this);
    m_node = node;
    CompleteConfig();
}

bool
RangerNetDevice::NeedsArp() const
{
    NS_LOG_FUNCTION(this);
    return true;
}

void
RangerNetDevice::SetReceiveCallback(ReceiveCallback cb)
{
    NS_LOG_FUNCTION(this);
    m_receiveCallback = cb;
}

void
RangerNetDevice::SetPromiscReceiveCallback(PromiscReceiveCallback cb)
{
    // This method basically assumes an 802.3-compliant device, but a raw
    // 802.15.4 device does not have an ethertype, and requires specific
    // McpsDataIndication parameters.
    // For further study:  how to support these methods somehow, such as
    // inventing a fake ethertype and packet tag for McpsDataRequest
    NS_LOG_WARN("Unsupported; use LrWpan MAC APIs instead");
}

bool
RangerNetDevice::SupportsSendFrom() const
{
    NS_LOG_FUNCTION_NOARGS();
    return false;
}

void
RangerNetDevice::DoDispose()
{
    NS_LOG_FUNCTION(this);
    // m_mac->Dispose();
    m_phy->Dispose();
    // m_csmaca->Dispose();
    m_phy = nullptr;
    // m_mac = nullptr;
    // m_csmaca = nullptr;
    m_node = nullptr;
    // chain up.
    NetDevice::DoDispose();
}

void
RangerNetDevice::DoInitialize()
{
    NS_LOG_FUNCTION(this);
    m_phy->Initialize();
    // m_mac->Initialize();
    NetDevice::DoInitialize();
}
// ----------------------NetDevice END----------------------

void
RangerNetDevice::CompleteConfig()
{
    NS_LOG_FUNCTION(this);
    // if (!m_mac || !m_phy || !m_csmaca || !m_node || m_configComplete)
    // {
    //     return;
    // }
    // m_mac->SetPhy(m_phy);
    // m_mac->SetCsmaCa(m_csmaca);
    // m_mac->SetMcpsDataIndicationCallback(MakeCallback(&LrWpanNetDevice::McpsDataIndication, this));
    // m_csmaca->SetMac(m_mac);

    // Ptr<LrWpanErrorModel> model = CreateObject<LrWpanErrorModel>();
    // m_phy->SetErrorModel(model);
    // m_phy->SetDevice(this);

    // m_phy->SetPdDataIndicationCallback(MakeCallback(&LrWpanMac::PdDataIndication, m_mac));
    // m_phy->SetPdDataConfirmCallback(MakeCallback(&LrWpanMac::PdDataConfirm, m_mac));
    // m_phy->SetPlmeEdConfirmCallback(MakeCallback(&LrWpanMac::PlmeEdConfirm, m_mac));
    // m_phy->SetPlmeGetAttributeConfirmCallback(
    //     MakeCallback(&LrWpanMac::PlmeGetAttributeConfirm, m_mac));
    // m_phy->SetPlmeSetTRXStateConfirmCallback(
    //     MakeCallback(&LrWpanMac::PlmeSetTRXStateConfirm, m_mac));
    // m_phy->SetPlmeSetAttributeConfirmCallback(
    //     MakeCallback(&LrWpanMac::PlmeSetAttributeConfirm, m_mac));

    // m_csmaca->SetLrWpanMacStateCallback(MakeCallback(&LrWpanMac::SetLrWpanMacState, m_mac));
    // m_phy->SetPlmeCcaConfirmCallback(MakeCallback(&LrWpanCsmaCa::PlmeCcaConfirm, m_csmaca));
    m_configComplete = true;
}

Ptr<LrWpanPhy>
RangerNetDevice::GetPhy() const
{
    NS_LOG_FUNCTION(this);
    return m_phy;
}

void
RangerNetDevice::SetPhy(Ptr<LrWpanPhy> phy)
{
    NS_LOG_FUNCTION(this);
    m_phy = phy;
    CompleteConfig();
}


void
RangerNetDevice::SetChannel(Ptr<SpectrumChannel> channel)
{
    NS_LOG_FUNCTION(this << channel);
    m_phy->SetChannel(channel);
    channel->AddRx(m_phy);
    CompleteConfig();
}

} // namespace ns3
