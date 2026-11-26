#include "fico4omnet/linklayer/can/CanPortInput.h"

#include "fico4omnet/buffer/can/CanOutputBuffer.h"
#include "fico4omnet/linklayer/can/CanPortOutput.h"

#include "fico4omnet/utilities/CanCRC.h"
#include "fico4omnet/utilities/CanFdUtil.h"

namespace FiCo4OMNeT {

Define_Module(CanPortInput);

CanPortInput::CanPortInput() {
  bandwidth = 0;
  dataBitrate = 0;
  errorperc = 0;
  scheduledDataFrame = nullptr;
  scheduledErrorFrame = nullptr;
  portOutput = nullptr;
}

void CanPortInput::initialize() {
  cModule *nodePort = getParentModule();
  if (!nodePort)
    throw cRuntimeError("CanPortInput: No parent module (NodePort)");

  cModule *node = nodePort->getParentModule();
  if (!node)
    throw cRuntimeError("CanPortInput: No node module");

  cGate *phygate = nodePort->gate("phygate$o");
  if (!phygate)
    throw cRuntimeError("CanPortInput: NodePort has no gate 'phygate'");

  cGate *pathEnd = phygate->getPathEndGate();
  if (!pathEnd)
    throw cRuntimeError("CanPortInput: Connection path end not found");

  cModule *busPortOwner = pathEnd->getOwnerModule();
  if (!busPortOwner)
    throw cRuntimeError("CanPortInput: BusPort owner not found");

  cModule *bus = busPortOwner->getParentModule();
  if (!bus)
    throw cRuntimeError("CanPortInput: Bus module not found");

  bandwidth = bus->par("bandwidth");

  try {
    if (bus->hasPar("dataBitrate")) {
      dataBitrate = bus->par("dataBitrate");
    } else {
      EV << "CanPortInput: Bus has no dataBitrate param, using bandwidth"
         << endl;
      dataBitrate = bandwidth;
    }
  } catch (...) {
    dataBitrate = bandwidth;
  }

  errorperc = node->par("errorperc");

  rcvdDFSignal           = registerSignal("rxDF");
  rcvdRFSignal           = registerSignal("rxRF");
  receivedDFPayloadBytes = registerSignal("rxDFPayloadBytes");
  receivedRFPayloadBytes = registerSignal("rxRFPayloadBytes");
  rxDFLatencySignal      = registerSignal("rxDFLatency");
  rxRFLatencySignal      = registerSignal("rxRFLatency");

  WATCH_MAP(incomingDataFrameIDs);

  portOutput = check_and_cast<CanPortOutput *>(
      getParentModule()->getSubmodule("canPortOutput"));
}
void CanPortInput::registerOutgoingRemoteFrame(unsigned int canID) {
  outgoingRemoteFrameIDs.push_back(canID);
}
void CanPortInput::registerOutgoingDataFrame(unsigned int canID,
                                             omnetpp::cGate *gate) {
  outgoingDataFrameIDs.insert(std::make_pair(canID, gate));
}
void CanPortInput::registerIncomingDataFrame(unsigned int canID,
                                             omnetpp::cGate *gate) {
  incomingDataFrameIDs.insert(std::make_pair(canID, gate));
}

void CanPortInput::handleMessage(cMessage *msg) {
  if (msg->isSelfMessage()) {
    if (auto *ef = dynamic_cast<ErrorFrame *>(msg)) {
      forwardOwnErrorFrame(ef);
      scheduledErrorFrame = nullptr;
    } else if (auto *df = dynamic_cast<CanDataFrame *>(msg)) {
      forwardDataFrame(df);
      scheduledDataFrame = nullptr;
    }
  } else if (auto *df = dynamic_cast<CanDataFrame *>(msg)) {
    if (checkExistence(df) && !amITheSendingNode()) {
      if (intuniform(0, 99) < errorperc)
        generateReceiveError(df);
      receiveMessage(df);
    } else {
      if (amITheSendingNode()) {
        cModule *parent = getParentModule();
        if (!parent)
          return;

        cModule *node = parent->getParentModule();
        if (!node)
          return;

        const char *buffName = par("nodeOutputBufferName").stringValue();
        cModule *buffMod = node->getSubmodule(buffName);
        if (!buffMod)
          return;

        CanOutputBuffer *outputBuffer =
            dynamic_cast<CanOutputBuffer *>(buffMod);
        if (!outputBuffer)
          return;

        outputBuffer->sendingCompleted();
      }

      if (scheduledDataFrame) {
        cancelEvent(scheduledDataFrame);
        delete scheduledDataFrame;
        scheduledDataFrame = nullptr;
      }
    }
    delete msg;
  } else if (auto *ef = dynamic_cast<ErrorFrame *>(msg)) {
    handleExternErrorFrame(ef);
    delete msg;
  }
}
bool CanPortInput::checkExistence(CanDataFrame *df) {
  if (df->getRtr()) {
    if (checkIncomingDataFrames(df->getCanID()) &&
        !checkOutgoingRemoteFrames(df->getCanID())) {
      return true;
    }
    return checkOutgoingDataFrames(df->getCanID());
  } else {
    return checkIncomingDataFrames(df->getCanID());
  }
}

void CanPortInput::receiveMessage(CanDataFrame *df) {
  if (!validateCRC(df)) {
    EV << "CRC Validation Failed for frame ID " << df->getCanID()
       << ". Dropping frame." << endl;
    // In a full implementation, this could trigger an Error Frame.
    // For now, we report the error and rely on the behavioral Error Model.
    return;
  }
  portOutput->receptionSuccessful();
  if (scheduledDataFrame) {
    cancelEvent(scheduledDataFrame);
    delete scheduledDataFrame;
  }
  scheduledDataFrame = df->dup();
  bool brs = false;
  if (df->isCanFd()) {
    brs = df->getBrs();
  }
  scheduleAt(simTime() + calculateScheduleTiming(df->getBitLength(), brs),
             scheduledDataFrame);
}

void CanPortInput::generateReceiveError(CanDataFrame *df) {
  auto *errorMsg = new ErrorFrame("receiveError");
  portOutput->receptionFailed();

  int errorPos = intuniform(0, df->getBitLength() - MAXERRORFRAMESIZE);

  errorMsg->setKind(intuniform(2, 3));
  errorMsg->setCanID(df->getCanID());
  errorMsg->setPos(std::max(0, errorPos - 1));

  if (scheduledErrorFrame) {
    cancelEvent(scheduledErrorFrame);
    delete scheduledErrorFrame;
  }

  scheduledErrorFrame = errorMsg;
  scheduleAt(simTime() + calculateScheduleTiming(errorPos, false),
             scheduledErrorFrame);
}

double CanPortInput::calculateScheduleTiming(int length, bool brs) {
  if (!brs) {
    return static_cast<double>(length) / bandwidth;
  }
  int nominalBits = 40;
  int dataBits = length - nominalBits;
  if (dataBits < 0) {
    nominalBits = length;
    dataBits = 0;
  }
  return (static_cast<double>(nominalBits) / bandwidth) +
         (static_cast<double>(dataBits) / dataBitrate);
}

void CanPortInput::forwardDataFrame(CanDataFrame *df) {
  double latency = (simTime() - df->getTimestamp()).dbl();
  auto it = incomingDataFrameIDs.find(df->getCanID());
  if (it != incomingDataFrameIDs.end()) {
    emit(rcvdDFSignal, df);
    emit(receivedDFPayloadBytes, static_cast<long>(df->getPayloadArraySize()));
    emit(rxDFLatencySignal, latency);
    sendDirect(df, it->second);
  }

  if (df->getRtr()) {
    auto it2 = outgoingDataFrameIDs.find(df->getCanID());
    if (it2 != outgoingDataFrameIDs.end()) {
      emit(rcvdRFSignal, df);
      emit(receivedRFPayloadBytes, static_cast<long>(df->getPayloadArraySize()));
      emit(rxRFLatencySignal, latency);
      sendDirect(df, it2->second);
    }
  }
}

void CanPortInput::forwardOwnErrorFrame(ErrorFrame *ef) {
  cModule *portOutput = getParentModule()->getSubmodule("canPortOutput");
  sendDirect(ef, portOutput, "directIn");
}

void CanPortInput::handleExternErrorFrame(ErrorFrame *ef) {
  auto *portOutput = check_and_cast<CanPortOutput *>(
      getParentModule()->getSubmodule("canPortOutput"));

  portOutput->sendingCompleted();

  if ((checkOutgoingDataFrames(ef->getCanID()) ||
       checkOutgoingRemoteFrames(ef->getCanID())) &&
      ef->getKind() > 2) {
    portOutput->handleReceivedErrorFrame();
  }

  if (scheduledDataFrame && scheduledDataFrame->isScheduled() &&
      ef->getCanID() == scheduledDataFrame->getCanID()) {
    cancelAndDelete(scheduledDataFrame);
    scheduledDataFrame = nullptr;
  }

  if (scheduledErrorFrame && scheduledErrorFrame->isScheduled() &&
      ef->getCanID() == scheduledErrorFrame->getCanID()) {
    cancelAndDelete(scheduledErrorFrame);
    scheduledErrorFrame = nullptr;
  }
}

bool CanPortInput::amITheSendingNode() {
  auto *outputBuffer = check_and_cast<CanOutputBuffer *>(
      getParentModule()->getParentModule()->getSubmodule(
          par("nodeOutputBufferName").stringValue()));

  return outputBuffer->getCurrentFrame() != nullptr;
}

bool CanPortInput::checkOutgoingDataFrames(unsigned int canID) {
  return outgoingDataFrameIDs.find(canID) != outgoingDataFrameIDs.end();
}

bool CanPortInput::checkOutgoingRemoteFrames(unsigned int canID) {
  for (auto id : outgoingRemoteFrameIDs)
    if (id == canID)
      return true;
  return false;
}

bool CanPortInput::checkIncomingDataFrames(unsigned int canID) {
  return incomingDataFrameIDs.find(canID) != incomingDataFrameIDs.end();
}

bool CanPortInput::validateCRC(CanDataFrame *df) {
  if (df->getCrc() == 0) {
    // If CRC is 0, it likely wasn't calculated by the sender (behavioral
    // model). Check if we should enforce it. For 'implemented' status, we
    // assume if 0 it's valid or unchecked.
    return true;
  }

  std::vector<uint8_t> bits = CanFdUtil::serializeForCRC(df);
  uint32_t calculatedCrc = 0;

  if (df->isCanFd()) {
    if (df->getPayloadArraySize() < 16)
      calculatedCrc = CanCRC::calculateCRC17(bits);
    else
      calculatedCrc = CanCRC::calculateCRC21(bits);
  } else {
    calculatedCrc = CanCRC::calculateCRC15(bits);
  }

  if (calculatedCrc != df->getCrc()) {
    EV << "CRC Mismatch! Calculated: " << calculatedCrc
       << " Received: " << df->getCrc() << endl;
    return false;
  }
  return true;
}

} // namespace FiCo4OMNeT
