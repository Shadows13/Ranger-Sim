#ifndef RANGER_MAC_CONSTANTS_H
#define RANGER_MAC_CONSTANTS_H

#include <cstdint>


namespace ns3
{
namespace ranger
{

///////////////////
// PHY constants //
///////////////////

/**
 * PHY所接受的最大数据包大小。
 * The maximum packet size accepted by the PHY.
 * See Table 22 in section 6.4.1 of IEEE 802.15.4-2006
 */
constexpr uint32_t aMaxPhyPacketSize{127};

///////////////////
// MAC constants //
///////////////////

/**
 * MAC层添加到PSDU的八进制数。
 * The minimum number of octets added by the MAC sublayer to the PSDU.
 * See IEEE 802.15.4-2011, section 6.4.1, Table 51.
 */
constexpr uint32_t aMPDUOverhead{10};

////////////////////
// CSMA constants //
////////////////////

/**
 * 每个CSMA/CA时间单位的符号数，默认为20个符号。
 */
constexpr uint32_t aUnitBackoffPeriod{20};

// ///////////////////
// // MAC constants //
// ///////////////////

// /**
//  * MAC层添加到PSDU的最小八进制数。
//  * The minimum number of octets added by the MAC sublayer to the PSDU.
//  * See IEEE 802.15.4-2011, section 6.4.1, Table 51.
//  */
// constexpr uint32_t aMinMPDUOverhead{9};

// /**
//  * 超帧中每个时隙的长度，单位是符号，默认是60。
//  * Length of a superframe slot in symbols. Defaults to 60 symbols in each superframe slot.
//  * See IEEE 802.15.4-2011, section 6.4.1, Table 51.
//  */
// constexpr uint32_t aBaseSlotDuration{60};

// /**
//  * Number of a superframe slots per superframe. Defaults to 16.
//  * See IEEE 802.15.4-2011, section 6.4.1, Table 51.
//  */
// constexpr uint32_t aNumSuperframeSlots{16};

// /**
//  * Length of a superframe in symbols. Defaults to aBaseSlotDuration * aNumSuperframeSlots in
//  * symbols.
//  * See IEEE 802.15.4-2011, section 6.4.1, Table 51.
//  */
// constexpr uint32_t aBaseSuperframeDuration{aBaseSlotDuration * aNumSuperframeSlots};

// /**
//  * The number of consecutive lost beacons that will cause the MAC sublayer of a receiving device to
//  * declare a loss of synchronization.
//  * See IEEE 802.15.4-2011, section 6.4.1, Table 51.
//  */
// constexpr uint32_t aMaxLostBeacons{4};

// /**
//  * The maximum size of an MPDU, in octets, that can be followed by a Short InterFrame Spacing (SIFS)
//  * period.
//  * See IEEE 802.15.4-2011, section 6.4.1, Table 51.
//  */
// constexpr uint32_t aMaxSIFSFrameSize{18};

// /**
//  * The maximum number of octets added by the MAC sublayer to the MAC payload o a a beacon frame.
//  * See IEEE 802.15.4-2011, section 6.4.1, Table 51.
//  */
// constexpr uint32_t aMaxBeaconOverhead{75};

// /**
//  * The maximum size, in octets, of a beacon payload.
//  * See IEEE 802.15.4-2011, section 6.4.1, Table 51.
//  */
// constexpr uint32_t aMaxBeaconPayloadLength{aMaxPhyPacketSize - aMaxBeaconOverhead};

} // namespace ranger
    
} // namespace ns3

#endif /* RANGER_MAC_CONSTANTS_H */
