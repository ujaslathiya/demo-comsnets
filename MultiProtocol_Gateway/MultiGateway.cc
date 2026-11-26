#include "MultiGateway.h"
#include "lin/LinUtil.h"
#include "inet/linklayer/ethernet/EtherFrame.h"
#include "inet/applications/ethernet/EtherApp_m.h"

namespace multiprotocol_gateway {

Define_Module(MultiGateway);

// ── Lifecycle ─────────────────────────────────────────────────────────────────

void MultiGateway::initialize() {
    enableFragmentation = par("enableFragmentation").boolValue();
    enableAggregation   = par("enableAggregation").boolValue();
    processingDelay     = par("processingDelay").doubleValue();
    maxQueueSize        = par("maxQueueSize").intValue();

    droppedCorruptedCount = 0;
    bufferOverflowCount   = 0;
    canPendingCount       = 0;
    canFdPendingCount     = 0;
    linPendingCount       = 0;
    uplinkBytesSent       = 0;

    txTimer          = new cMessage("txTimer");
    aggregationTimer = new cMessage("aggregationTimer");
    ethTxTimer       = new cMessage("ethTxTimer");
    linTxTimer       = new cMessage("linTxTimer");
    utilMeasureTimer = new cMessage("utilMeasureTimer");

    // Register statistics signals
    canInputQueueLengthSignal   = registerSignal("canInputQueueLength");
    canFdInputQueueLengthSignal = registerSignal("canFdInputQueueLength");
    linInputQueueLengthSignal   = registerSignal("linInputQueueLength");
    canQueuingDelaySignal       = registerSignal("canQueuingDelay");
    canFdQueuingDelaySignal     = registerSignal("canFdQueuingDelay");
    linQueuingDelaySignal       = registerSignal("linQueuingDelay");
    canE2EDelaySignal           = registerSignal("canE2EDelay");
    canFdE2EDelaySignal         = registerSignal("canFdE2EDelay");
    linE2EDelaySignal           = registerSignal("linE2EDelay");
    ethE2EDelaySignal           = registerSignal("ethE2EDelay");
    fullE2EDelaySignal          = registerSignal("fullE2EDelay");
    inputDropCountSignal        = registerSignal("inputDropCount");
    processingDelaySignal       = registerSignal("processingDelay");
    uplinkUtilSignal            = registerSignal("uplinkUtil");

    // First utilisation measurement at t=1s
    scheduleAt(simTime() + 1.0, utilMeasureTimer);
}

MultiGateway::~MultiGateway() {
    cancelAndDelete(txTimer);
    cancelAndDelete(aggregationTimer);
    cancelAndDelete(ethTxTimer);
    cancelAndDelete(linTxTimer);
    cancelAndDelete(utilMeasureTimer);

    while (!pendingCanFragments.empty()) {
        delete pendingCanFragments.top();
        pendingCanFragments.pop();
    }
    for (auto &qf : pendingEthFrames) delete qf.frame;
}

void MultiGateway::finish() {
    // Emit final drop count scalar
    emit(inputDropCountSignal, (long)(droppedCorruptedCount + bufferOverflowCount));
}

// ── Message dispatch ──────────────────────────────────────────────────────────

void MultiGateway::handleMessage(cMessage *msg) {
    if (msg == txTimer)          { sendNextFragment();  return; }
    if (msg == ethTxTimer)       { sendNextEthFrame();  return; }
    if (msg == linTxTimer)       { sendNextLinPoll();   return; }
    if (msg == utilMeasureTimer) { measureUplinkUtil(); return; }

    if (msg == aggregationTimer) {
        for (auto const& [port, buf] : aggregationBuffers) {
            if (!buf.empty()) sendAggregatedEth(port);
        }
        return;
    }

    if (msg->isSelfMessage()) { delete msg; return; }

    std::string gateName = msg->getArrivalGate()->getBaseName();
    if      (gateName == "canPort" || gateName == "canFdPort") processCanFrame(msg);
    else if (gateName == "ethg")                               processEthFrame(msg);
    else if (gateName == "linPort")                            processLinFrame(msg);
    else                                                       delete msg;
}

// ── CAN / CAN-FD ─────────────────────────────────────────────────────────────

void MultiGateway::processCanFrame(cMessage *msg) {
    FiCo4OMNeT::CanDataFrame *canFrame = dynamic_cast<FiCo4OMNeT::CanDataFrame *>(msg);
    if (!canFrame) { delete msg; return; }

    if (canFrame->hasBitError()) {
        droppedCorruptedCount++;
        emit(inputDropCountSignal, (long)(droppedCorruptedCount + bufferOverflowCount));
        delete msg;
        return;
    }

    EV << "Received CAN frame ID: " << canFrame->getCanID()
       << " (" << (canFrame->isCanFd() ? "FD" : "Std") << ")\n";

    SrcProto srcProto = canFrame->isCanFd() ? SrcProto::CAN_FD : SrcProto::CAN_STD;
    // FiCo4OMNeT sets timestamp = simTime() in frameTransmission() just before send().
    // getCreationTime() is always 0 (pre-allocated in initialize). Use getTimestamp().
    simtime_t frameCreationTime = canFrame->getTimestamp();

    // ── Track this frame for full end-to-end delay measurement ────────────────
    unsigned int frameId = canFrame->getCanID();
    int sourceGateIndex = msg->getArrivalGate()->getIndex();
    frameTrackingMap[frameId] = {simTime(), srcProto, sourceGateIndex};

    RouteEntity route;
    if (routingTable.lookupCanRoute(canFrame->getCanID(), route)) {
        std::vector<uint8_t> payload;
        payload.reserve(canFrame->getPayloadArraySize());
        for (unsigned int i = 0; i < canFrame->getPayloadArraySize(); i++)
            payload.push_back(canFrame->getPayload(i));

        if (route.targetProto == Protocol::ETHERNET) {
            if (enableAggregation) {
                auto &buf = aggregationBuffers[route.targetPort];
                // Record first-arriving protocol and creation time for this aggregation slot
                if (buf.empty()) {
                    aggregationProtocol[route.targetPort]     = srcProto;
                    aggregationCreationTime[route.targetPort] = frameCreationTime;
                }
                buf.insert(buf.end(), payload.begin(), payload.end());

                if (buf.size() >= 64) {
                    sendAggregatedEth(route.targetPort);
                } else if (!aggregationTimer->isScheduled()) {
                    scheduleAt(simTime() + 0.0005, aggregationTimer);
                }
            } else {
                packToEth(route.targetPort, payload, srcProto, frameCreationTime);
            }
        } else if (route.targetProto == Protocol::CAN ||
                   route.targetProto == Protocol::CAN_FD) {
            packToCan(route.targetPort, payload,
                      route.targetProto == Protocol::CAN_FD);
        } else if (route.targetProto == Protocol::LIN) {
            packToLin(0, payload, route.targetId);
        }
    }
    delete msg;
}

// ── Ethernet backbone ─────────────────────────────────────────────────────────

void MultiGateway::processEthFrame(cMessage *msg) {
    EV << "Received Ethernet Frame from Backbone\n";

    // E2E: ETH node send → this gateway receive. The TSN switch re-wraps the frame
    // in a new EtherPhyFrame (creation time = switch TX time), but the inner
    // EthernetIIFrame retains the original source-node creation time.
    simtime_t originTime = msg->getCreationTime();
    SrcProto sourceProtocol = SrcProto::ETH_BACKBONE;

    if (auto *phy = dynamic_cast<inet::EtherPhyFrame *>(msg)) {
        cPacket *inner = phy->getEncapsulatedPacket();
        if (inner) {
            originTime = inner->getCreationTime();
        }
    }
    double ethE2eUs = (simTime() - originTime).dbl() * 1e6;
    if (ethE2eUs >= 0) emit(ethE2EDelaySignal, ethE2eUs);

    std::vector<uint8_t> mockPayload(64, 0xBB);

    if (gate("canPort$o", 0)->isConnected()) {
        EV << " -> Routing to Standard CAN (ID: 110)\n";
        packToCan(0, mockPayload, false, originTime, sourceProtocol);
    }
    if (gate("canFdPort$o", 0)->isConnected()) {
        EV << " -> Routing to CAN-FD (ID: 220)\n";
        packToCan(0, mockPayload, true, originTime, sourceProtocol);
    }
    if (gate("linPort$o", 0)->isConnected()) {
        pendingLinPolls.push_back(0x30);
        pendingLinPolls.push_back(0x31);
        pendingLinPolls.push_back(0x32);
        if (!linTxTimer->isScheduled())
            scheduleAt(simTime(), linTxTimer);
    }
    delete msg;
}

// ── LIN ───────────────────────────────────────────────────────────────────────

void MultiGateway::processLinFrame(cMessage *msg) {
    LinResponse *rsp = dynamic_cast<LinResponse *>(msg);
    if (rsp) {
        EV << "Received LIN Response (ID: 0x" << std::hex << rsp->getFrameId()
           << std::dec << ")\n";
        // Use the time this slot's poll was sent as the origin for e2e measurement.
        // This gives true poll-to-ETH-departure latency; falls back to creation time if missing.
        simtime_t frameCreationTime = rsp->getCreationTime();
        auto pollIt = linPollSentTime.find(rsp->getFrameId());
        if (pollIt != linPollSentTime.end()) {
            frameCreationTime = pollIt->second;
            linPollSentTime.erase(pollIt);
        }
        std::vector<uint8_t> payload;
        for (int i = 0; i < rsp->getDataLength(); i++)
            payload.push_back(rsp->getData(i));
        packToEth(0, payload, SrcProto::LIN, frameCreationTime);
    }
    delete msg;
}

// ── Ethernet output queue ─────────────────────────────────────────────────────

void MultiGateway::packToEth(int destEthPort,
                              const std::vector<uint8_t> &payload,
                              SrcProto protocol,
                              simtime_t originCreationTime) {
    // Buffer-overflow drop
    if (pendingEthFrames.size() >= maxQueueSize) {
        bufferOverflowCount++;
        emit(inputDropCountSignal,
             (long)(droppedCorruptedCount + bufferOverflowCount));
        EV << "[DROP] Gateway Eth queue full, dropping frame\n";
        return;
    }

    inet::EthernetIIFrame *ethFrame = new inet::EthernetIIFrame("EthFwd");
    ethFrame->setSrc(inet::MACAddress("02-00-00-00-00-01"));
    ethFrame->setDest(inet::MACAddress::BROADCAST_ADDRESS);
    ethFrame->setEtherType(0xAAAA);

    inet::EtherAppReq *appReq = new inet::EtherAppReq("GW-to-Eth");
    appReq->setByteLength(payload.size());
    ethFrame->encapsulate(appReq);

    inet::EtherPhyFrame *phyFrame = new inet::EtherPhyFrame("EthPhyFwd");
    phyFrame->setSrcMacFullDuplex(true);
    phyFrame->encapsulate(ethFrame);

    EV << "[GW→ETH] Protocol=" << (int)protocol
       << " payload=" << payload.size() << "B on port " << destEthPort << "\n";

    QueuedEthFrame qf;
    qf.port                = destEthPort;
    qf.frame               = phyFrame;
    qf.protocol            = protocol;
    qf.enqueueTime         = simTime();
    qf.originCreationTime  = originCreationTime;
    pendingEthFrames.push_back(qf);

    // Update per-protocol queue length and emit signal
    switch (protocol) {
        case SrcProto::CAN_STD:
            canPendingCount++;
            emit(canInputQueueLengthSignal, (long)canPendingCount);
            break;
        case SrcProto::CAN_FD:
            canFdPendingCount++;
            emit(canFdInputQueueLengthSignal, (long)canFdPendingCount);
            break;
        case SrcProto::LIN:
            linPendingCount++;
            emit(linInputQueueLengthSignal, (long)linPendingCount);
            break;
        default: break;
    }

    if (!ethTxTimer->isScheduled())
        scheduleAt(simTime(), ethTxTimer);
}

void MultiGateway::sendNextEthFrame() {
    if (pendingEthFrames.empty()) return;

    auto &next = pendingEthFrames.front();
    int              port               = next.port;
    inet::EtherPhyFrame *frame          = next.frame;
    SrcProto         protocol           = next.protocol;
    simtime_t        enqueueTime        = next.enqueueTime;
    simtime_t        originCreationTime = next.originCreationTime;
    int              byteLen            = frame->getByteLength();

    cGate    *outGate   = gate("ethg$o", port);
    cChannel *txChannel = outGate->getTransmissionChannel();
    simtime_t txFin     = txChannel ? txChannel->getTransmissionFinishTime() : simTime();

    if (simTime() < txFin) {
        scheduleAt(txFin, ethTxTimer);
        return;
    }

    pendingEthFrames.pop_front();
    sendDelayed(frame, processingDelay, outGate);

    // Queuing delay = time in queue + processing delay (µs)
    double delayUs = (simTime() - enqueueTime).dbl() * 1e6 + processingDelay * 1e6;
    emit(processingDelaySignal, processingDelay * 1e6);

    // E2E delay = ECU frame transmission start → gateway Ethernet departure (µs)
    double e2eUs = (originCreationTime > SIMTIME_ZERO)
        ? (simTime().dbl() + processingDelay - originCreationTime.dbl()) * 1e6
        : -1.0;

    switch (protocol) {
        case SrcProto::CAN_STD:
            canPendingCount--;
            emit(canInputQueueLengthSignal, (long)canPendingCount);
            emit(canQueuingDelaySignal, delayUs);
            if (e2eUs >= 0) emit(canE2EDelaySignal, e2eUs);
            break;
        case SrcProto::CAN_FD:
            canFdPendingCount--;
            emit(canFdInputQueueLengthSignal, (long)canFdPendingCount);
            emit(canFdQueuingDelaySignal, delayUs);
            if (e2eUs >= 0) emit(canFdE2EDelaySignal, e2eUs);
            break;
        case SrcProto::LIN:
            linPendingCount--;
            emit(linInputQueueLengthSignal, (long)linPendingCount);
            emit(linQueuingDelaySignal, delayUs);
            if (e2eUs >= 0) emit(linE2EDelaySignal, e2eUs);
            break;
        default: break;
    }

    uplinkBytesSent += byteLen;

    if (!pendingEthFrames.empty()) {
        cChannel *ch = outGate->getTransmissionChannel();
        if (ch) scheduleAt(ch->getTransmissionFinishTime() + 1e-9, ethTxTimer);
    }
}

void MultiGateway::sendAggregatedEth(int port) {
    std::vector<uint8_t> payload = aggregationBuffers[port];
    aggregationBuffers[port].clear();

    SrcProto proto = SrcProto::CAN_STD;
    auto it = aggregationProtocol.find(port);
    if (it != aggregationProtocol.end()) {
        proto = it->second;
        aggregationProtocol.erase(it);
    }

    simtime_t ct = SIMTIME_ZERO;
    auto ctIt = aggregationCreationTime.find(port);
    if (ctIt != aggregationCreationTime.end()) {
        ct = ctIt->second;
        aggregationCreationTime.erase(ctIt);
    }

    packToEth(port, payload, proto, ct);
}

// ── Uplink utilisation measurement ────────────────────────────────────────────

void MultiGateway::measureUplinkUtil() {
    double datarate = 100e6; // default 100 Mbps
    if (gate("ethg$o", 0)->getChannel()) {
        cDatarateChannel *ch =
            dynamic_cast<cDatarateChannel *>(gate("ethg$o", 0)->getChannel());
        if (ch) datarate = ch->getDatarate();
    }
    // bytes transmitted in the last 1-second window
    double util = (double)uplinkBytesSent * 8.0 / (1.0 * datarate) * 100.0;
    util = std::min(100.0, util);
    emit(uplinkUtilSignal, util);
    uplinkBytesSent = 0;
    scheduleAt(simTime() + 1.0, utilMeasureTimer);
}

// ── CAN fragmentation (outbound to bus) ───────────────────────────────────────

void MultiGateway::packToCan(int destCanPort,
                              const std::vector<uint8_t> &payload,
                              bool isCanFd,
                              simtime_t originCreationTime,
                              SrcProto sourceProtocol) {
    cGate *outGate = nullptr;
    try {
        outGate = gate(isCanFd ? "canFdPort$o" : "canPort$o", destCanPort);
        if (!outGate->isConnected()) return;
    } catch (std::exception &) { return; }

    unsigned int maxLen   = isCanFd ? 64 : 8;
    unsigned int fragments = (payload.size() + maxLen - 1) / maxLen;

    EV << "[Segmentation] → " << (isCanFd ? "CAN-FD" : "CAN")
       << " port " << destCanPort << " (" << fragments << " fragments)\n";

    // ── Calculate full end-to-end delay (cross-zone) ───────────────────────
    // If this frame came from Ethernet (backbone), calculate total delay
    if (sourceProtocol != SrcProto::ETH_BACKBONE && originCreationTime > SIMTIME_ZERO) {
        double fullE2eUs = (simTime() - originCreationTime).dbl() * 1e6;
        if (fullE2eUs >= 0) {
            emit(fullE2EDelaySignal, fullE2eUs);
            EV << "[FULL-E2E] Source=" << (int)sourceProtocol
               << " Delay=" << fullE2eUs << " µs\n";
        }
    }

    for (unsigned int i = 0; i < payload.size(); i += maxLen) {
        FiCo4OMNeT::CanDataFrame *frag = new FiCo4OMNeT::CanDataFrame("Fragment");
        frag->setCanID(isCanFd ? 220 : 110);
        frag->setIsCanFd(isCanFd);
        frag->setBrs(isCanFd);
        unsigned int len = std::min(maxLen, (unsigned int)payload.size() - i);
        frag->setPayloadArraySize(len);
        for (unsigned int j = 0; j < len; j++)
            frag->setPayload(j, payload[i + j]);
        pendingCanFragments.push(frag);
    }
    if (!txTimer->isScheduled())
        scheduleAt(simTime() + 0.0001, txTimer);
}

void MultiGateway::sendNextFragment() {
    if (pendingCanFragments.empty()) return;

    FiCo4OMNeT::CanDataFrame *fwdMsg = pendingCanFragments.top();
    pendingCanFragments.pop();

    const char *gateName = fwdMsg->isCanFd() ? "canFdPort$o" : "canPort$o";
    if (gate(gateName, 0)->isConnected())
        send(fwdMsg, gateName, 0);
    else
        delete fwdMsg;

    if (!pendingCanFragments.empty())
        scheduleAt(simTime() + 0.001, txTimer);
}

// ── LIN polling (outbound to bus) ─────────────────────────────────────────────

void MultiGateway::packToLin(int destLinPort,
                              const std::vector<uint8_t> &payload,
                              int linId) {
    pendingLinPolls.push_back(linId);
    if (!linTxTimer->isScheduled())
        scheduleAt(simTime(), linTxTimer);
}

void MultiGateway::sendNextLinPoll() {
    if (pendingLinPolls.empty()) return;

    int linId = pendingLinPolls.front();
    pendingLinPolls.pop_front();

    EV << "[LIN MASTER] Polling ID 0x" << std::hex << linId << std::dec << "\n";

    // Record when we sent this poll — used in processLinFrame() for true e2e measurement.
    linPollSentTime[linId] = simTime();

    LinBreakSignal *brk = new LinBreakSignal("LIN_Break");
    brk->setSourceAddr(100);
    brk->setDuration(13.0 / 20000.0);
    send(brk, "linPort$o", 0);

    LinHeader *hdr = new LinHeader("LIN_Header");
    hdr->setSourceAddr(100);
    hdr->setFrameId(linId);
    hdr->setPid(computePID(linId));
    sendDelayed(hdr, 0.001, "linPort$o", 0);

    if (!pendingLinPolls.empty())
        scheduleAt(simTime() + 0.015, linTxTimer);
}

} // namespace multiprotocol_gateway
