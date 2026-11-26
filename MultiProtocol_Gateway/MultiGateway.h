#ifndef __MULTIPROTOCOL_GATEWAY_H_
#define __MULTIPROTOCOL_GATEWAY_H_

#include <deque>
#include <map>
#include <queue>
#include <vector>

#include "RoutingTable.h"
#include "fico4omnet/linklayer/can/messages/CanDataFrame_m.h"
#include "inet/linklayer/ethernet/EtherFrame_m.h"
#include "lin/LinDataFrame_m.h"
#include <omnetpp.h>

using namespace omnetpp;

namespace multiprotocol_gateway {

struct CompareCanPriority {
    bool operator()(const FiCo4OMNeT::CanDataFrame *a,
                    const FiCo4OMNeT::CanDataFrame *b) const {
        return a->getCanID() > b->getCanID();
    }
};

// Source protocol tags for queued Ethernet frames
enum class SrcProto { CAN_STD = 0, CAN_FD = 1, LIN = 2, ETH_BACKBONE = 3 };

struct QueuedEthFrame {
    int                  port;
    inet::EtherPhyFrame *frame;
    SrcProto             protocol;
    simtime_t            enqueueTime;
    simtime_t            originCreationTime; // ECU frame creation time for e2e measurement
};

class MultiGateway : public cSimpleModule {
protected:
    virtual void initialize() override;
    virtual ~MultiGateway();
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;

    void processCanFrame(cMessage *msg);
    void processEthFrame(cMessage *msg);
    void processLinFrame(cMessage *msg);

    // protocol arg: which bus the payload came from; originCreationTime = ECU frame birth time
    void packToEth(int destEthPort, const std::vector<uint8_t> &payload, SrcProto protocol,
                   simtime_t originCreationTime = SIMTIME_ZERO);
    void packToCan(int destCanPort, const std::vector<uint8_t> &payload, bool isCanFd,
                   simtime_t originCreationTime = SIMTIME_ZERO, SrcProto sourceProtocol = SrcProto::ETH_BACKBONE);
    void packToLin(int destLinPort, const std::vector<uint8_t> &payload, int linId);

private:
    RoutingTable routingTable;
    bool         enableFragmentation;
    bool         enableAggregation;
    double       processingDelay;
    unsigned int maxQueueSize;

    // Drop counters
    unsigned int droppedCorruptedCount;
    unsigned int bufferOverflowCount;

    // CAN fragmentation priority queue (outbound to CAN bus)
    std::priority_queue<FiCo4OMNeT::CanDataFrame *,
                        std::vector<FiCo4OMNeT::CanDataFrame *>,
                        CompareCanPriority>
        pendingCanFragments;
    cMessage *txTimer;

    // Ethernet output queue (tagged with source protocol + arrival time)
    std::deque<QueuedEthFrame> pendingEthFrames;
    cMessage *ethTxTimer;
    void sendNextEthFrame();

    // Per-protocol pending frame counts (for queue-length signals)
    int canPendingCount;
    int canFdPendingCount;
    int linPendingCount;

    // LIN polling queue
    std::deque<int> pendingLinPolls;
    cMessage       *linTxTimer;
    void sendNextLinPoll();

    // LIN poll timestamps: linId → simTime() when master sent the poll header
    std::map<int, simtime_t> linPollSentTime;

    // Aggregation buffer (CAN → Ethernet aggregation)
    std::map<int, std::vector<uint8_t>> aggregationBuffers;
    std::map<int, SrcProto>             aggregationProtocol;
    std::map<int, simtime_t>            aggregationCreationTime; // first frame's ECU birth time
    cMessage *aggregationTimer;
    void sendAggregatedEth(int port);

    void sendNextFragment();

    // ── Statistics signals ─────────────────────────────────────────────
    simsignal_t canInputQueueLengthSignal;
    simsignal_t canFdInputQueueLengthSignal;
    simsignal_t linInputQueueLengthSignal;
    simsignal_t canQueuingDelaySignal;
    simsignal_t canFdQueuingDelaySignal;
    simsignal_t linQueuingDelaySignal;
    simsignal_t canE2EDelaySignal;    // ECU creation → gateway Ethernet departure (µs)
    simsignal_t canFdE2EDelaySignal;
    simsignal_t linE2EDelaySignal;
    simsignal_t ethE2EDelaySignal;    // ETH node send → gateway receive (µs)
    simsignal_t fullE2EDelaySignal;   // Full path: fieldbus source → fieldbus/ECU destination (µs)
    simsignal_t inputDropCountSignal;
    simsignal_t processingDelaySignal;
    simsignal_t uplinkUtilSignal;

    // ── Full end-to-end delay tracking (across backbone) ────────────────────
    // Track frames by CAN ID: records when they enter gateway and when they exit
    struct FrameTracker {
        simtime_t entryTime;
        SrcProto  sourceProtocol;
        int       sourcePort;
    };
    std::map<unsigned int, FrameTracker> frameTrackingMap; // canId → entry info

    // Uplink utilisation measurement (1-second rolling window)
    cMessage *utilMeasureTimer;
    long      uplinkBytesSent;
    void measureUplinkUtil();
};

} // namespace multiprotocol_gateway
#endif
