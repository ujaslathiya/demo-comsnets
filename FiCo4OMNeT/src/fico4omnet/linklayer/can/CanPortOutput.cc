
#include "fico4omnet/linklayer/can/CanPortOutput.h"
#include "fico4omnet/linklayer/can/messages/CanDataFrame_m.h"
#include "fico4omnet/utilities/CanCRC.h"
#include "fico4omnet/utilities/CanFdUtil.h"


namespace FiCo4OMNeT {

Define_Module(CanPortOutput);

CanPortOutput::CanPortOutput() {
  this->bandwidth = 0;
  this->errorperc = 0;
  this->scheduledErrorFrame = nullptr;
  this->errorReceived = false;
}

CanPortOutput::~CanPortOutput() { cancelAndDelete(scheduledErrorFrame); }

void CanPortOutput::handleReceivedErrorFrame() {
  errorReceived = true;
  if (scheduledErrorFrame != nullptr && scheduledErrorFrame->isScheduled()) {
    cancelEvent(scheduledErrorFrame);
    delete scheduledErrorFrame;
    scheduledErrorFrame = nullptr;
  }
}

void CanPortOutput::initialize() {
  cModule *nodePort = getParentModule();
  if (!nodePort)
    throw cRuntimeError("CanPortOutput: No parent module (NodePort)");

  cModule *node = nodePort->getParentModule();
  if (!node)
    throw cRuntimeError("CanPortOutput: No node module");

  cGate *phygate = nodePort->gate("phygate$o");
  if (!phygate)
    throw cRuntimeError("CanPortOutput: NodePort has no gate 'phygate'");

  cGate *pathEnd = phygate->getPathEndGate();
  if (!pathEnd)
    throw cRuntimeError("CanPortOutput: Connection path end not found");

  cModule *busPortOwner = pathEnd->getOwnerModule();
  if (!busPortOwner)
    throw cRuntimeError("CanPortOutput: BusPort owner not found");

  cModule *bus = busPortOwner->getParentModule();
  if (!bus)
    throw cRuntimeError("CanPortOutput: Bus module not found");

  bandwidth = bus->par("bandwidth");

  try {
    if (bus->hasPar("dataBitrate")) {
      dataBitrate = bus->par("dataBitrate");
    } else {
      EV << "CanPortOutput: Bus has no dataBitrate param, using bandwidth"
         << endl;
      dataBitrate = bandwidth;
    }
  } catch (...) {
    dataBitrate = bandwidth;
  }

  errorperc = node->par("errorperc");
  scheduledErrorFrame = new ErrorFrame();
  initializeStatisticValues();
}

void CanPortOutput::initializeStatisticValues() {
  sentDFSignal = registerSignal("txDF");
  sentRFSignal = registerSignal("txRF");
  sendErrorsSignal = registerSignal("txEF");
  receiveErrorsSignal = registerSignal("rxEF");
}

void CanPortOutput::handleMessage(cMessage *msg) {
  if (ErrorFrame *ef = dynamic_cast<ErrorFrame *>(msg)) {
    if (!errorReceived) {
      if (ef->getKind() < 2) {
        emit(sendErrorsSignal, ef);
      } else {
        emit(receiveErrorsSignal, ef);
      }
      colorError();
      send(msg, "out");
      scheduledErrorFrame = nullptr;
    } else {
      delete msg;
    }
  } else {
    CanDataFrame *df;
    colorBusy();
    errorReceived = false;
    if ((df = dynamic_cast<CanDataFrame *>(msg))) {
      if (isBusOff) {
        EV << "Node is Bus Off, dropping frame: " << df->getName() << endl;
        delete df;
        return;
      }

      if (df->isCanFd()) {
        int dlc = df->getDlc();
        int payloadLen = 8;
        if (dlc > 8) {
          switch (dlc) {
          case 9:
            payloadLen = 12;
            break;
          case 10:
            payloadLen = 16;
            break;
          case 11:
            payloadLen = 20;
            break;
          case 12:
            payloadLen = 24;
            break;
          case 13:
            payloadLen = 32;
            break;
          case 14:
            payloadLen = 48;
            break;
          case 15:
            payloadLen = 64;
            break;
          default:
            payloadLen = 64;
            break;
          }
        } else {
          payloadLen = dlc;
        }

        if (df->getPayloadArraySize() != payloadLen) {
          df->setPayloadArraySize(payloadLen);
        }

        if (df->getCrc() == 0) {
          std::vector<uint8_t> bits = CanFdUtil::serializeForCRC(df);
          if (df->isCanFd()) {
            if (df->getPayloadArraySize() < 16)
              df->setCrc(CanCRC::calculateCRC17(bits));
            else
              df->setCrc(CanCRC::calculateCRC21(bits));
          } else {
            df->setCrc(CanCRC::calculateCRC15(bits));
          }
        }
        df->setEsi(isErrorPassive);
      }

      bool errorOccurred = false;
      if (errorperc > 0) {
        int senderr = intuniform(0, 99);
        if (senderr < errorperc) {
          errorOccurred = true;
          tec += 8;
          if (tec > 127 && !isErrorPassive)
            isErrorPassive = true;
          if (tec > 255) {
            isBusOff = true;
            EV << "Node went BUS OFF!" << endl;
          }

          ErrorFrame *errself = new ErrorFrame("senderror");
          int position = intuniform(0, static_cast<int>(df->getBitLength()) -
                                           MAXERRORFRAMESIZE);
          errself->setKind(intuniform(0, 1));
          errself->setCanID(df->getCanID());
          if (position > 0)
            position--;
          errself->setPos(position);
          if (scheduledErrorFrame != nullptr &&
              scheduledErrorFrame->isScheduled()) {
            cancelEvent(scheduledErrorFrame);
            delete (scheduledErrorFrame);
            scheduledErrorFrame = nullptr;
          }
          scheduledErrorFrame = errself;
          scheduleAt(
              (simTime() + calculateScheduleTiming(
                               position, df->isCanFd() ? df->getBrs() : false)),
              scheduledErrorFrame);
        }
      }

      if (!errorOccurred) {
        if (tec > 0)
          tec--;
        if (tec <= 127 && isErrorPassive)
          isErrorPassive = false;
      }
    }

    if (df->getRtr()) {
      emit(sentRFSignal, df);
    } else {
      emit(sentDFSignal, df);
    }

    send(df, "out");
  }
}

double CanPortOutput::calculateScheduleTiming(int length, bool brs) {
  if (!brs) {
    return static_cast<double>(length) / (bandwidth);
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

void CanPortOutput::sendingCompleted() { colorIdle(); }

void CanPortOutput::colorBusy() {
  getParentModule()->getParentModule()->getDisplayString().setTagArg("i", 1,
                                                                     "yellow");
  cGate *extGate = getParentModule()->gate("phygate$i")->getPreviousGate();
  if (extGate) {
    extGate->getDisplayString().setTagArg("ls", 0, "yellow");
    extGate->getDisplayString().setTagArg("ls", 1, "3");
    if (extGate->getPreviousGate()) {
      extGate->getPreviousGate()->getDisplayString().setTagArg("ls", 0,
                                                               "yellow");
      extGate->getPreviousGate()->getDisplayString().setTagArg("ls", 1, "3");
    }
  }
}

void CanPortOutput::colorIdle() {
  getParentModule()->getParentModule()->getDisplayString().setTagArg("i", 1,
                                                                     "");
  cGate *extGate = getParentModule()->gate("phygate$i")->getPreviousGate();
  if (extGate) {
    extGate->getDisplayString().setTagArg("ls", 0, "black");
    extGate->getDisplayString().setTagArg("ls", 1, "1");
    if (extGate->getPreviousGate()) {
      extGate->getPreviousGate()->getDisplayString().setTagArg("ls", 0,
                                                               "black");
      extGate->getPreviousGate()->getDisplayString().setTagArg("ls", 1, "1");
    }
  }
}

void CanPortOutput::colorError() {
  getParentModule()->getParentModule()->getDisplayString().setTagArg("i", 1,
                                                                     "red");
  cGate *extGate = getParentModule()->gate("phygate$i")->getPreviousGate();
  if (extGate) {
    extGate->getDisplayString().setTagArg("ls", 0, "red");
    extGate->getDisplayString().setTagArg("ls", 1, "3");
    if (extGate->getPreviousGate()) {
      extGate->getPreviousGate()->getDisplayString().setTagArg("ls", 0, "red");
      extGate->getPreviousGate()->getDisplayString().setTagArg("ls", 1, "3");
    }
  }
}

void CanPortOutput::receptionSuccessful() {
  if (rec > 0)
    rec--;
  if (rec <= 127 && isErrorPassive)
    isErrorPassive = false;
  EV << "Reception Successful. REC: " << rec << ", Node is "
     << (isErrorPassive ? "Error Passive" : "Error Active") << endl;
}

void CanPortOutput::receptionFailed() {
  rec++;
  if (rec > 127 && !isErrorPassive) {
    isErrorPassive = true;
    EV << "Node transitioned to Error Passive (REC > 127)" << endl;
  }
  EV << "Reception Failed. REC: " << rec << endl;
}

} // namespace FiCo4OMNeT
