#include "fico4omnet/applications/can/sink/CanTrafficSinkAppBase.h"

#include "fico4omnet/linklayer/can/messages/CanDataFrame_m.h"
#include "fico4omnet/buffer/can/CanInputBuffer.h"

namespace FiCo4OMNeT {

Define_Module(CanTrafficSinkAppBase);

void CanTrafficSinkAppBase::initialize() {
    idle = true;
    currentFrameID = 0;
    bufferMessageCounter = 0;

    rxDFSignal           = registerSignal("rxDF");
    rxRFSignal           = registerSignal("rxRF");
    rxDFPayloadBytesSignal = registerSignal("rxDFPayloadBytes");
    rxRFPayloadBytesSignal = registerSignal("rxRFPayloadBytes");
    // Numeric latency: replaces timestampAge(rxDF) which was removed in OMNeT++ 6
    rxDFLatencySignal    = registerSignal("rxDFLatency");
    rxRFLatencySignal    = registerSignal("rxRFLatency");
}

void CanTrafficSinkAppBase::handleMessage(cMessage *msg) {
    if (msg->arrivedOn("controllerIn")) {
        bufferMessageCounter++;
        if (idle) {
            requestFrame();
        }
    }
    else if (auto *frame = dynamic_cast<CanDataFrame *>(msg)) {
        currentFrameID = frame->getCanID();
        bufferMessageCounter--;

        int payloadBytes = frame->getPayloadArraySize();

        double latency = (simTime() - frame->getTimestamp()).dbl();
        if (frame->getRtr()) {
            emit(rxRFSignal, frame);
            emit(rxRFPayloadBytesSignal, payloadBytes);
            emit(rxRFLatencySignal, latency);
        } else {
            emit(rxDFSignal, frame);
            emit(rxDFPayloadBytesSignal, payloadBytes);
            emit(rxDFLatencySignal, latency);
        }

        startWorkOnFrame(0);
    }
    else if (msg->isSelfMessage()) {
        auto *buffer =
            check_and_cast<CanInputBuffer*>(
                getParentModule()->getSubmodule("bufferIn"));

        buffer->deleteFrame(currentFrameID);

        if (bufferMessageCounter > 0) {
            requestFrame();
        } else {
            idle = true;
        }
    }

    delete msg;
}

void CanTrafficSinkAppBase::requestFrame() {
    auto *buffer =
        check_and_cast<CanInputBuffer*>(
            getParentModule()->getSubmodule("bufferIn"));

    buffer->deliverNextFrame();
    idle = false;
}

void CanTrafficSinkAppBase::startWorkOnFrame(float workTime) {
    (void)workTime;
}

} // namespace FiCo4OMNeT

