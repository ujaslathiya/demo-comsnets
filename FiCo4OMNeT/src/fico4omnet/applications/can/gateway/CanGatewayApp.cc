#include "fico4omnet/applications/can/gateway/CanGatewayApp.h"
#include "fico4omnet/linklayer/can/CanPortInput.h"
#include "fico4omnet/utilities/CanFdUtil.h"
#include <iostream>

using namespace std;

namespace FiCo4OMNeT {

Define_Module(CanGatewayApp);

CanGatewayApp::CanGatewayApp() {}
CanGatewayApp::~CanGatewayApp() {}

void CanGatewayApp::initialize() {
  fragmentedSignal = registerSignal("gatewayFragmented");
  forwardedSignal = registerSignal("gatewayForwarded");

  // Register to receive EVERYTHING on both ports
  // We assume we are inside a CanGateway module which has portStd and portFd
  // ...
}

void CanGatewayApp::handleMessage(omnetpp::cMessage *msg) {
  if (msg->isSelfMessage()) {
    return;
  }

  auto *df = dynamic_cast<CanDataFrame *>(msg);
  if (!df) {
    delete msg;
    return;
  }

  // Port detection logic...

  // Prevent loop: Only handle frames from correct input side
  // Standard -> FD
  if (msg->getArrivalGate()->isName("inStd")) {
    if (!df->isCanFd()) { // Only forward Standard frames from here
      handleStdFrame(df);
    }
  }
  // FD -> Standard
  else if (msg->getArrivalGate()->isName("inFd")) {
    if (df->isCanFd()) {
      handleFdFrame(df);
    } else {
      // Ignore Standard frames on FD input (Prevent Echo Loop)
      // These are frames we just forwarded, coming back to us.
    }
  }

  delete df;
}

void CanGatewayApp::handleFdFrame(CanDataFrame *frame) {
  EV << "Gateway: Received CAN-FD Frame ID " << frame->getCanID() << " Length "
     << frame->getPayloadArraySize() << ". Fragmenting..." << endl;

  unsigned int totalLen = frame->getPayloadArraySize();
  unsigned int fragments = (totalLen + 7) / 8;

  for (unsigned int i = 0; i < fragments; i++) {
    CanDataFrame *frag = new CanDataFrame("CAN_Frag");
    frag->setTimestamp(frame->getTimestamp());
    frag->setCanID(frame->getCanID()); // Keep same ID? Or generic?
    frag->setIsCanFd(false);           // Standard Frame
    frag->setDlc(8);
    frag->setPayloadArraySize(8);

    unsigned int offset = i * 8;
    for (unsigned int b = 0; b < 8; b++) {
      if (offset + b < totalLen)
        frag->setPayload(b, frame->getPayload(offset + b));
      else
        frag->setPayload(b, 0); // Padding
    }

    frag->setBitLength(CanFdUtil::calculateStuffedBitLength(frag));

    // Send to Standard Port
    EV << "Gateway: Sending fragment " << i << "..." << endl;
    // sendToStdPort(frag);
    // Direct send caused Segfaults. Using sendDelayed to serialize.
    sendDelayed(frag, omnetpp::SimTime((double)(i + 1) * 1e-6), "outStd");
    EV << "Gateway: Sent fragment " << i << "." << endl;
  }

  emit(fragmentedSignal, (long)fragments);
}

void CanGatewayApp::handleStdFrame(CanDataFrame *frame) {
  EV << "Gateway: Forwarding Standard Frame ID " << frame->getCanID() << endl;

  CanDataFrame *forward = frame->dup();
  // Potentially upgrade to FD? Or keep as Std?
  // Let's keep as Std for compatibility, FD bus understands Std.

  sendToFdPort(forward);
  emit(forwardedSignal, 1);
}

void CanGatewayApp::sendToStdPort(CanDataFrame *frame) {
  // send(frame, "outStd");
  // Use sendDelayed to serialize processing and avoid Heisenbug
  // Delay based on fragment index (implied by call order, but we don't have
  // index here) Actually, we can pass delay as arg, or just use sendDelayed
  // with 0 for now? 0 delay is same as send. I need to change signature or
  // handle it in handleFdFrame.
  send(frame, "outStd");
}

void CanGatewayApp::sendToFdPort(CanDataFrame *frame) { send(frame, "outFd"); }

} // namespace FiCo4OMNeT
