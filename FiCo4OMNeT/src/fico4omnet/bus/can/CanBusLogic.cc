#include "fico4omnet/bus/can/CanBusLogic.h"
#include "fico4omnet/buffer/can/CanOutputBuffer.h"
#include "fico4omnet/linklayer/can/messages/ErrorFrame_m.h"
#include "fico4omnet/utilities/CanFdUtil.h"

namespace FiCo4OMNeT {

Define_Module(CanBusLogic);

static inline int stuffedBits(int bits) { return bits; }

CanBusLogic::CanBusLogic() {
  errored = false;
  idle = true;
  errpos = INT_MAX;
  bandwidth = 0;
  currentSendingID = 0;
  sendingNode = nullptr;
  scheduledDataFrame = nullptr;
  numDataFrames = 0;
  numRemoteFrames = 0;
  numErrorFrames = 0;
  numFramesSent = 0;
  numBitsSent = 0;
}

CanBusLogic::~CanBusLogic() {
  if (scheduledDataFrame)
    cancelAndDelete(scheduledDataFrame);
}

void CanBusLogic::initialize() {
  rcvdSignal = registerSignal("rxPk");
  rcvdDFSignal = registerSignal("rxDF");
  rcvdRFSignal = registerSignal("rxRF");
  rcvdEFSignal = registerSignal("rxEF");
  rcvdDFPayloadBytesSignal = registerSignal("rxDFPayloadBytes");
  stateSignal = registerSignal("state");
  arbitrationLengthSignal = registerSignal("arbitrationLength");
  bandwidth = getParentModule()->par("bandwidth");
  bubble("state: idle");
}

int CanBusLogic::getSendingNodeID() {
  return sendingNode ? sendingNode->getId() : -1;
}

void CanBusLogic::handleMessage(cMessage *msg) {
  if (msg->isSelfMessage()) {
    if (dynamic_cast<CanDataFrame *>(msg)) {
      send(msg->dup(), "gate$o");
      sendingCompleted();
    } else if (dynamic_cast<ErrorFrame *>(msg)) {
      colorIdle();
      errored = false;
      eraseids.clear();
    }
    grantSendingPermission();
    delete msg;
    return;
  }

  if (dynamic_cast<CanDataFrame *>(msg)) {
    colorBusy();
    handleDataFrame(msg);
  } else if (dynamic_cast<ErrorFrame *>(msg)) {
    colorError();
    handleErrorFrame(msg);
  }
  delete msg;
}

void CanBusLogic::registerForArbitration(unsigned int canID, cModule *module,
                                         simtime_t signInTime, bool rtr) {
  Enter_Method_Silent();

  CanID *newId = new CanID(canID, module, signInTime, rtr);
  ids.push_back(newId);
  emit(arbitrationLengthSignal, (unsigned long)ids.size());

  if (idle) {
    scheduleAt(simTime() + (1.0 / bandwidth), new cMessage("idle_signin"));
    idle = false;
  }
}

void CanBusLogic::checkoutFromArbitration(unsigned int canID) {
  Enter_Method_Silent();
  for (auto it = ids.begin(); it != ids.end(); ++it) {
    if ((*it)->getCanID() == canID) {
      delete *it;
      ids.erase(it);
      break;
    }
  }
  emit(arbitrationLengthSignal, (unsigned long)ids.size());
}

void CanBusLogic::grantSendingPermission() {
  currentSendingID = UINT_MAX;
  sendingNode = nullptr;

  auto winner_it = ids.end();
  for (auto it = ids.begin(); it != ids.end(); ++it) {
    if (!*it)
      continue;
    if ((*it)->getCanID() < currentSendingID) {
      currentSendingID = (*it)->getCanID();
      sendingNode = (*it)->getNode();
      currsit = (*it)->getSignInTime();
      winner_it = it;
    }
  }

  if (sendingNode) {
    if (winner_it != ids.end()) {
      eraseids.push_back(winner_it);
    }
    auto *ctrl = check_and_cast<CanOutputBuffer *>(sendingNode);
    ctrl->receiveSendingPermission(currentSendingID);
  } else {
    idle = true;
    emit(stateSignal, (int)State::IDLE);
  }
}

void CanBusLogic::sendingCompleted() {
  colorIdle();
  for (auto it : eraseids) {
    delete *it;
    ids.erase(it);
  }
  eraseids.clear();
  errored = false;

  if (scheduledDataFrame)
    cancelEvent(scheduledDataFrame);
  scheduledDataFrame = nullptr;
}

void CanBusLogic::handleDataFrame(cMessage *msg) {
  auto *df = check_and_cast<CanDataFrame *>(msg);

  double nextidle;
  int payloadBits = df->getPayloadArraySize() * 8;

  double dataRate = 0;
  if (df->isCanFd() && df->getBrs()) {
    dataRate = getParentModule()->par("dataBitrate").doubleValue();
  }
  nextidle = CanFdUtil::calculateTransmissionTime(df, bandwidth, dataRate);

  if (scheduledDataFrame)
    cancelAndDelete(scheduledDataFrame);

  scheduledDataFrame = df->dup();
  scheduleAt(simTime() + nextidle, scheduledDataFrame);

  emit(rcvdSignal, df);
  emit(rcvdDFSignal, df);
  emit(rcvdDFPayloadBytesSignal, (long)df->getPayloadArraySize());

  numFramesSent++;
  numBitsSent += stuffedBits(df->getBitLength());
}

void CanBusLogic::handleErrorFrame(cMessage *msg) {
  if (!errored) {
    auto *ef = new ErrorFrame();
    scheduleAt(simTime() + (MAXERRORFRAMESIZE / bandwidth), ef);
    emit(rcvdEFSignal, ef);
    send(msg->dup(), "gate$o");
    errored = true;
    numErrorFrames++;
  }
}

void CanBusLogic::colorBusy() {
  if (!getEnvir()->isGUI())
    return;

  int n = getParentModule()->gate("gate$o", 0)->getVectorSize();
  for (int i = 0; i < n; i++)
    getParentModule()
        ->gate("gate$o", i)
        ->getDisplayString()
        .setTagArg("ls", 0, "yellow");
}

void CanBusLogic::colorIdle() {
  if (!getEnvir()->isGUI())
    return;

  int n = getParentModule()->gate("gate$o", 0)->getVectorSize();
  for (int i = 0; i < n; i++)
    getParentModule()
        ->gate("gate$o", i)
        ->getDisplayString()
        .setTagArg("ls", 0, "black");
}

void CanBusLogic::colorError() {
  if (!getEnvir()->isGUI())
    return;

  int n = getParentModule()->gate("gate$o", 0)->getVectorSize();
  for (int i = 0; i < n; i++)
    getParentModule()
        ->gate("gate$o", i)
        ->getDisplayString()
        .setTagArg("ls", 0, "red");
}

void CanBusLogic::finish() {
  recordScalar("numDataFrames", numDataFrames);
  recordScalar("numRemoteFrames", numRemoteFrames);
  recordScalar("numErrorFrames", numErrorFrames);
  recordScalar("numFramesSent", numFramesSent);
  recordScalar("numBitsSent", numBitsSent);
}

} // namespace FiCo4OMNeT
