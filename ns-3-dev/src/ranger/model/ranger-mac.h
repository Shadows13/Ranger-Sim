#ifndef RANGER_MAC_H
#define RANGER_MAC_H

#include "ranger-mac-header.h"
#include "ranger-mac-constants.h"

#include <ns3/lr-wpan-phy.h>
#include <ns3/log.h>
#include <ns3/sequence-number.h>
#include <ns3/random-variable-stream.h>
#include <ns3/packet.h>
#include <ns3/simulator.h>

#include <deque>


namespace ns3
{

/**
 * @ingroup lr-wpan
 *
 * The status of a confirm or an indication primitive as a result of a previous request.
 * Represent the value of status in IEEE 802.15.4-2011 primitives.
 * (Tables 6, 12, 18, 20, 31, 33, 35, 37, 39, 47)
 *
 * Status codes values only appear in IEEE 802.15.4-2006, Table 78
 * See also NXP JN5169 IEEE 802.15.4 Stack User Guide
 * (Table 6: Status Enumerations and Section 6.1.23)
 *
 */
enum class RangerMacStatus : std::uint8_t   // TODO
{
    SUCCESS = 0,              //!< The operation was completed successfully.
    FULL_CAPACITY = 0x01,     //!< PAN at capacity. Association Status field (std. 2006, Table 83)
    ACCESS_DENIED = 0x02,     //!< PAN access denied. Association Status field (std. 2006, Table 83)
    COUNTER_ERROR = 0xdb,     //!< The frame counter of the received frame is invalid.
    IMPROPER_KEY_TYPE = 0xdc, //!< The key is not allowed to be used with that frame type.
    IMPROPER_SECURITY_LEVEL = 0xdd, //!< Insufficient security level expected by the recipient.
    UNSUPPORTED_LEGACY = 0xde,      //!< Deprecated security used in IEEE 802.15.4-2003
    UNSUPPORTED_SECURITY = 0xdf,    //!< The security applied is not supported.
    BEACON_LOSS = 0xe0,             //!< The beacon was lost following a synchronization request.
    CHANNEL_ACCESS_FAILURE = 0xe1,  //!< A Tx could not take place due to activity in the CH.
    DENIED = 0xe2,                  //!< The GTS request has been denied by the PAN coordinator.
    DISABLE_TRX_FAILURE = 0xe3,     //!< The attempt to disable the transceier has failed.
    SECURITY_ERROR = 0xe4,      // Cryptographic process of the frame failed(FAILED_SECURITY_CHECK).
    FRAME_TOO_LONG = 0xe5,      //!< Frame more than aMaxPHYPacketSize or too large for CAP or GTS.
    INVALID_GTS = 0xe6,         //!< Missing GTS transmit or undefined direction.
    INVALID_HANDLE = 0xe7,      //!< When purging from TX queue handle was not found.
    INVALID_PARAMETER = 0xe8,   //!< Primitive parameter not supported or out of range.
    NO_ACK = 0xe9,              //!< No acknowledgment was received after macMaxFrameRetries.
    NO_BEACON = 0xea,           //!< A scan operation failed to find any network beacons.
    NO_DATA = 0xeb,             //!<  No response data were available following a request.
    NO_SHORT_ADDRESS = 0xec,    //!< Failure due to unallocated 16-bit short address.
    OUT_OF_CAP = 0xed,          //!< (Deprecated) See IEEE 802.15.4-2003
    PAN_ID_CONFLICT = 0xee,     //!<  PAN id conflict detected and informed to the coordinator.
    REALIGMENT = 0xef,          //!< A coordinator realigment command has been received.
    TRANSACTION_EXPIRED = 0xf0, //!< The transaction expired and its information discarded.
    TRANSACTION_OVERFLOW = 0xf1,  //!< There is no capacity to store the transaction.
    TX_ACTIVE = 0xf2,             //!< The transceiver was already enabled.
    UNAVAILABLE_KEY = 0xf3,       //!< Unavailable key, unknown or blacklisted.
    UNSUPPORTED_ATTRIBUTE = 0xf4, //!< SET/GET request issued with a non supported ID.
    INVALID_ADDRESS = 0xf5,       //!< Invalid source or destination address.
    ON_TIME_TOO_LONG = 0xf6,      //!< RX enable request fail due to syms. longer than Bcn. interval
    PAST_TIME = 0xf7,             //!< Rx enable request fail due to lack of time in superframe.
    TRACKING_OFF = 0xf8,          //!< This device is currently not tracking beacons.
    INVALID_INDEX = 0xf9,     //!< A MAC PIB write failed because specified index is out of range.
    LIMIT_REACHED = 0xfa,     //!< PAN descriptors stored reached max capacity.
    READ_ONLY = 0xfb,         //!< SET/GET request issued for a read only attribute.
    SCAN_IN_PROGRESS = 0xfc,  //!< Scan failed because already performing another scan.
    SUPERFRAME_OVERLAP = 0xfd //!< Coordinator sperframe and this device superframe tx overlap.
};

namespace ranger
{

/**
 * @ingroup ranger
 *
 * MAC states
 */
enum MacState
{
    MAC_IDLE,               // MAC_IDLE
    MAC_CSMA,               // MAC_CSMA
    MAC_SENDING,            // MAC_SENDING
    CHANNEL_ACCESS_FAILURE, // CHANNEL_ACCESS_FAILURE
    CHANNEL_IDLE,           // CHANNEL_IDLE
    SET_PHY_TX_ON,          // SET_PHY_TX_ON
    MAC_INACTIVE,           // MAC_INACTIVE
};

/**
 * @ingroup ranger
 */
struct McpsDataRequestParams
{
    Ipv4Address m_dstAddr;      // Destination address
    uint8_t m_msduHandle{0};    // MSDU handle
    uint8_t m_txOptions{0};     // Tx Options (bitfield)
};

/**
 * @ingroup ranger
 */
struct McpsDataConfirmParams
{
    uint8_t m_msduHandle{0};                                        // MSDU handle
    RangerMacStatus m_status{RangerMacStatus::INVALID_PARAMETER};   // The status
                                                                    // of the last MSDU transmission
};

/**
 * @ingroup ranger
 */
struct McpsDataIndicationParams
{
    Ipv4Address m_srcAddr;          // Source address
    Ipv4Address m_dstAddr;          // Destination address
    uint8_t m_mpduLinkQuality{0};   // LQI value measured during reception of the MPDU
    uint8_t m_dsn{0};               // 收到的包的MAC层序列号
};

/**
 * @ingroup ranger
 *
 * @brief callback is called after a McpsDataRequest has been called from
 *        the higher layer.  It returns a status of the outcome of the
 *        transmission request.
 */
using McpsDataConfirmCallback = Callback<void, McpsDataConfirmParams>;

/**
 * @ingroup ranger
 *
 * @brief This callback is called after a Mcps has successfully received a
 *        frame and wants to deliver it to the higher layer.
 */
using McpsDataIndicationCallback = Callback<void, McpsDataIndicationParams, Ptr<Packet>>;

}   // namespace ranger

class RangerMac : public Object
{
public:
    /**
     * Get the type ID.
     *
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Default constructor.
     */
    RangerMac();
    ~RangerMac() override;

    /**
     * @brief 通过Mac层请求发包
     * 
     * @param params McpsDataRequestParams
     * @param p packet 
     */
    void McpsDataRequest(ranger::McpsDataRequestParams params, Ptr<Packet> p);

    /**
     * @brief Set the underlying PHY for the MAC.
     *
     * @param phy the PHY
     */
    void SetPhy(Ptr<LrWpanPhy> phy);

    /**
     * Set the address of this MAC.
     *
     * @param address the new address
     */
    void SetAddress(Ipv4Address address);

    /**
     * Get the address of this MAC.
     *
     * @return the address
     */
    Ipv4Address GetAddress() const;

    ////////////////////////////////////
    // Interfaces between MAC and PHY //
    ////////////////////////////////////

    /**
     * PD-DATA.indication
     * @brief Indicates the transfer of an MPDU from PHY to MAC (receiving)
     * @param psduLength number of bytes in the PSDU
     * @param p the packet to be transmitted
     * @param lqi Link quality (LQI) value measured during reception of the PPDU
     */
    void PdDataIndication(uint32_t psduLength, Ptr<Packet> p, uint8_t lqi);

    /**
     * PD-DATA.confirm
     * @brief Confirm the end of transmission of an MPDU to MAC
     * @param status to report to MAC
     */
    void PdDataConfirm(LrWpanPhyEnumeration status);

    /**
     * PLME-SET-TRX-STATE.confirm
     * @brief Confirm the PHY state
     * @param status in RX_ON,TRX_OFF,FORCE_TRX_OFF,TX_ON
     */
    void PlmeSetTRXStateConfirm(LrWpanPhyEnumeration status);

    /**
     * PLME-CCA.confirm status // TODO 修改函数描述
     * 
     * When Phy has completed CCA, it calls back here which in turn execute the final steps
     * of the CSMA-CA algorithm.
     * It checks to see if the Channel is idle, if so check the Contention window  before
     * permitting transmission (step 5). If channel is busy, either backoff and perform CCA again or
     * treat as channel access failure (step 4).
     * 
     * @param status TRX_OFF, BUSY or IDLE
     */
    void PlmeCcaConfirm(LrWpanPhyEnumeration status);

    //////////////////////////
    // set callback methods //
    //////////////////////////

    /**
     * @brief Set the callback for the confirmation of a data transmission request.
     * @param c the callback
     */
    void SetMcpsDataConfirmCallback(ranger::McpsDataConfirmCallback c);

    /**
     * @brief Set the callback for the indication of an incoming data packet.
     * @param c the callback
     */
    void SetMcpsDataIndicationCallback(ranger::McpsDataIndicationCallback c);

//  public

protected:
    // Inherited from Object.
    void DoInitialize() override;
    void DoDispose() override;

// protected

private:
    //////////////////////
    // callback methods //
    //////////////////////
    /**
     * This callback is used to report data transmission request status to the
     * upper layers.
     */
    ranger::McpsDataConfirmCallback m_mcpsDataConfirmCallback;

    /**
     * This callback is used to notify incoming packets to the upper layers.
     */
    ranger::McpsDataIndicationCallback m_mcpsDataIndicationCallback;

    //////////////////////
    // member variables //
    //////////////////////
    /** 
     * The address (32 bit address) used by this MAC.
     */
    Ipv4Address m_address;

    /**
     * Sequence number added to transmitted data, 00-ff.
     */
    SequenceNumber8 m_macDsn;

    /**
     * The PHY associated with this MAC.
     */
    Ptr<LrWpanPhy> m_phy;

    /**
     * The current state of the MAC layer.
     */
    TracedValue<ranger::MacState> m_macState;

    /**
     * The packet which is currently being receive by the MAC layer.
     */
    Ptr<Packet> m_rxPkt;

    /**
     * The packet which is currently being sent by the MAC layer.
     */
    Ptr<Packet> m_txPkt;

    /**
     * The number of already used retransmission for the currently transmitted
     * packet.
     */
    // uint8_t m_retransmission;

    /**
     * The number of CSMA/CA retries used for sending the current packet.
     */
    // uint8_t m_numCsmacaRetry;

    //////////////
    // Tx Queue //
    //////////////

    /**
     * Maximum number of retransmissions for a packet.
     */
    uint8_t m_maxRetryTimes;

    /**
     * Helper structure for managing transmission queue elements.
     */
    struct TxQueueElement : public SimpleRefCount<TxQueueElement>
    {
        uint8_t txQMsduHandle;  // MSDU Handle
        uint8_t retryTimes = 0;     // Number of retries
        Ptr<Packet> txQPkt;     // Queued packet
    };

    /**
     * The transmit queue used by the MAC.
     */
    std::deque<Ptr<TxQueueElement>> m_txQueue;

    /**
     * The maximum size of the transmit queue.
     */
    uint32_t m_maxTxQueueSize;

    /**
     * @brief Add an element to the transmission queue.
     *
     * @param txQElement The element added to the Tx Queue.
     */
    void EnqueueTxQElement(Ptr<TxQueueElement> txQElement);

    /**
     * Check the transmission queue. If there are packets in the transmission
     * queue and the MAC is idle, pick the first one and initiate a packet
     * transmission.
     */
    void CheckQueue();

    /**
     * CheckQueue()的定时器
    */
    void CheckQueuePeriodically(double t);

    /**
     * Remove the tip of the transmission queue, including clean up related to the
     * last packet transmission.
     */
    void RemoveFirstTxQElement();

    /**
     * Send an acknowledgment packet for the given sequence number.
     *
     * @param seqNum the sequence number for the ACK
     */
    void SendAck(uint8_t seqNum);

    ///////////////
    // Mac State //
    ///////////////

    /**
     * Scheduler event for a deferred MAC state change.
     */
    EventId m_setMacState;

    /**
     * @brief Change the current MAC state to the given new state.
     *
     * @param newState the new state
     */
    void ChangeMacState(ranger::MacState newState);

    /** // TODO 修改函数说明
     * @brief CSMA-CA algorithm calls back the MAC after executing channel assessment.
     *
     * @param macState indicate BUSY or IDLE channel condition
     */
    void SetMacState(ranger::MacState macState);

    /////////////
    // CSMA-CA //
    /////////////

    /**
     * Minimum backoff exponent. 0 - macMaxBE, default 3
     */
    uint8_t m_macMinBE;

    /**
     * Maximum backoff exponent. 3 - 8, default 5
     */
    uint8_t m_macMaxBE;

    /**
     * Maximum number of backoffs. 0 - 5, default 4
     */
    uint8_t m_macMaxCSMABackoffs;

    /**
     * Number of backoffs for the current transmission.
     */
    uint8_t m_NB;

    /**
     * Backoff exponent.
     */
    uint8_t m_BE;

    /**
     * Uniform random variable stream.
     */
    Ptr<UniformRandomVariable> m_random;

    /**
     * Count the number of remaining random backoff periods left to delay.
     */
    uint64_t m_randomBackoffPeriodsLeft;

    /**
     * Flag indicating that the PHY is currently running a CCA. Used to prevent
     * reporting the channel status to the MAC while canceling the CSMA algorithm.
     */
    bool m_ccaRequestRunning;

    /**
     * Scheduler event for the start of the next random backoff.
     */
    EventId m_randomBackoffEvent;

    /**
     * Scheduler event when to start the CCA after a random backoff.
     */
    EventId m_requestCcaEvent;

    /**
     * Start CSMA-CA algorithm (step 1), initialize NB, BE for CSMA-CA
     */
    void CSMACAStart();

    /**
     * In step 2 of the CSMA-CA, perform a random backoff in the range of 0 to 2^BE -1
     */
    void RandomBackoffDelay();

    /**
     * Request the Phy to perform CCA (Step 3)
     */
    void RequestCCA();

    ///////////
    // Trace //
    ///////////

    /**
     * The trace source fired when packets come into the "top" of the device
     * at the L3/L2 transition, when being queued for transmission.
     *
     * @see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_macTxEnqueueTrace;

    /**
     * The trace source fired when packets are dequeued from the
     * L3/l2 transmission queue.
     *
     * \see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_macTxDequeueTrace;

    /**
     * The trace source fired when packets are dropped due to missing ACKs or
     * because of transmission failures in L1.
     *
     * @see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_macTxDropTrace;

    /**
     * The trace source fired when packets are considered as successfully sent
     * or the transmission has been given up.
     *
     * The data should represent:
     * packet, number of retries, total number of csma backoffs
     *
     * @see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>, uint8_t, uint8_t> m_sentPktTrace;

    /**
     * A trace source that fires when the Mac changes states.
     * Parameters are the old mac state and the new mac state.
     *
     * @deprecated This TracedCallback is deprecated and will be
     * removed in a future release,  Instead, use the @c MacStateValue
     * TracedValue. //TODO
     */
    TracedCallback<ranger::MacState, ranger::MacState> m_macStateLogger;

    /**
     * The trace source fired when packets where successfully transmitted, that is
     * an acknowledgment was received, if requested, or the packet was
     * successfully sent by L1, if no ACK was requested.
     *
     * @see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_macTxOkTrace;

    /**
     * The trace source fired when packets are being sent down to L1.
     *
     * @see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_macTxTrace;

    /**
     * A trace source that emulates a promiscuous mode protocol sniffer connected
     * to the device.  This trace source fire on packets destined for any host
     * just like your average everyday packet sniffer.
     *
     * On the transmit size, this trace hook will fire after a packet is dequeued
     * from the device queue for transmission.  In Linux, for example, this would
     * correspond to the point just before a device hard_start_xmit where
     * dev_queue_xmit_nit is called to dispatch the packet to the PF_PACKET
     * ETH_P_ALL handlers.
     *
     * On the receive side, this trace hook will fire when a packet is received,
     * just before the receive callback is executed.  In Linux, for example,
     * this would correspond to the point at which the packet is dispatched to
     * packet sniffers in netif_receive_skb.
     *
     * @see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_promiscSnifferTrace;

    /**
     * A trace source that emulates a non-promiscuous protocol sniffer connected
     * to the device.  Unlike your average everyday sniffer, this trace source
     * will not fire on PACKET_OTHERHOST events.
     *
     * On the transmit size, this trace hook will fire after a packet is dequeued
     * from the device queue for transmission.  In Linux, for example, this would
     * correspond to the point just before a device hard_start_xmit where
     * dev_queue_xmit_nit is called to dispatch the packet to the PF_PACKET
     * ETH_P_ALL handlers.
     *
     * On the receive side, this trace hook will fire when a packet is received,
     * just before the receive callback is executed.  In Linux, for example,
     * this would correspond to the point at which the packet is dispatched to
     * packet sniffers in netif_receive_skb.
     *
     * @see class CallBackTraceSource
     */
    TracedCallback<Ptr<const Packet>> m_snifferTrace;

// private

};  // class RangerMac

}   // namespace ns3

#endif  // RANGER_MAC_H
