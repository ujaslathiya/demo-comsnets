//Copyright (c) 2014, CoRE Research Group, Hamburg University of Applied Sciences
//All rights reserved.
//
//Redistribution and use in source and binary forms, with or without modification,
//are permitted provided that the following conditions are met:
//
//1. Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
//2. Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
//3. Neither the name of the copyright holder nor the names of its contributors
//   may be used to endorse or promote products derived from this software without
//   specific prior written permission.
//
//THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
//ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
//ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
//(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
//ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "fico4omnet/buffer/can/CanOutputBuffer.h"

#include "fico4omnet/bus/can/CanBusLogic.h"
#include "fico4omnet/linklayer/can/CanPortOutput.h"

namespace FiCo4OMNeT {

Define_Module(CanOutputBuffer);

CanOutputBuffer::~CanOutputBuffer(){
    for (std::list<cMessage*>::iterator it =  frames.begin(); it != frames.end(); ++it)
    {
            cancelAndDelete((*it));
    }
    frames.clear();
}

void CanOutputBuffer::putFrame(cMessage* msg) {
    if (!msg) {
         EV << "CRITICAL: putFrame received NULL message!" << endl;
         return;
    }
    EV << "DEBUG: CanOutputBuffer::putFrame - Start. Msg Name: " << msg->getName() << endl;
    CanDataFrame *frame = dynamic_cast<CanDataFrame *>(msg);
    if (!frame) {
        EV << "CRITICAL: msg is not a CanDataFrame! Kind: " << msg->getKind() << endl;
        return;
    }
    // Removed crashing debug print
    
    if (MOB == true) {
        EV << "DEBUG: MOB mode active." << endl;
        if (getFrame(frame->getCanID()) != nullptr) {
            checkoutFromArbitration(frame->getCanID());
            deleteFrame(frame->getCanID());
        }
        frames.push_back(frame);
        emit(queueLengthSignal, static_cast<unsigned long>(frames.size()));
        queueSize += static_cast<size_t>(frame->getByteLength());
        emit(queueSizeSignal, static_cast<unsigned long>(queueSize));
        registerForArbitration(frame->getCanID(), frame->getRtr());
    } else {
        EV << "DEBUG: FIFO mode active. Pushing back." << endl;
        frames.push_back(frame);
        emit(queueLengthSignal, static_cast<unsigned long>(frames.size()));
        queueSize += static_cast<size_t>(frame->getByteLength());
        emit(queueSizeSignal, static_cast<unsigned long>(queueSize));
        EV << "DEBUG: Registering for arbitration..." << endl;
        registerForArbitration(frame->getCanID(), frame->getRtr());
        EV << "DEBUG: Registration complete." << endl;
    }
    EV << "DEBUG: CanOutputBuffer::putFrame - End" << endl;
}
void CanOutputBuffer::registerForArbitration(unsigned int canID, bool rtr) {
    EV << "DEBUG: CanOutputBuffer::registerForArbitration - Start. ID: " << canID << endl;
    cModule* nodePort = getParentModule()->getSubmodule(par("nodePortName").stringValue());
    if (!nodePort) {
        EV << "CRITICAL: NodePort '" << par("nodePortName").stringValue() << "' not found!" << endl;
        throw cRuntimeError("CanOutputBuffer: NodePort not found");
    }
    EV << "DEBUG: NodePort: " << nodePort->getFullPath() << endl;
    
    cGate* phyGate = nodePort->gate("phygate$o");
    if (!phyGate) {
        EV << "CRITICAL: phygate$o not found on NodePort!" << endl;
        throw cRuntimeError("CanOutputBuffer: phygate$o not found");
    }
    EV << "DEBUG: phyGate: " << phyGate->getFullPath() << endl;
    
    cGate* endGate = phyGate->getPathEndGate();
    if (!endGate) {
        EV << "CRITICAL: phygate$o is not connected!" << endl;
        throw cRuntimeError("CanOutputBuffer: phygate$o is not connected");
    }
    EV << "DEBUG: EndGate: " << endGate->getFullPath() << endl;
    
    cModule* busModule = endGate->getOwnerModule()->getParentModule();
    if (!busModule) {
        EV << "CRITICAL: Bus module not found!" << endl;
        throw cRuntimeError("CanOutputBuffer: Bus module not found");
    }
    EV << "DEBUG: BusModule: " << busModule->getFullPath() << endl;

    CanBusLogic *canBusLogic = dynamic_cast<CanBusLogic*>(busModule->getSubmodule("canBusLogic"));
    if (!canBusLogic) {
        EV << "CRITICAL: canBusLogic not found in bus!" << endl;
        throw cRuntimeError("CanOutputBuffer: canBusLogic not found");
    }
    EV << "DEBUG: CanBusLogic: " << canBusLogic->getFullPath() << endl;
    
    canBusLogic->registerForArbitration(canID, this, simTime(), rtr);
    EV << "DEBUG: CanOutputBuffer::registerForArbitration - End" << endl;
}

void CanOutputBuffer::checkoutFromArbitration(unsigned int canID) {
    cModule* nodePort = getParentModule()->getSubmodule(par("nodePortName").stringValue());
    if (!nodePort) throw cRuntimeError("CanOutputBuffer: NodePort not found");

    CanBusLogic *canBusLogic =
            dynamic_cast<CanBusLogic*> (nodePort->gate("phygate$o")->getPathEndGate()->getOwnerModule()->getParentModule()->getSubmodule(
                    "canBusLogic"));
    if (canBusLogic->getCurrentSendingId() != canID && canBusLogic->getSendingNodeID() != this->getId()) {
        canBusLogic->checkoutFromArbitration(canID);
        deleteFrame(canID);
    }
}

void CanOutputBuffer::receiveSendingPermission(unsigned int canID) {
    Enter_Method_Silent
    ();
    EV << "DEBUG: CanOutputBuffer::receiveSendingPermission - CAN ID: " << canID << " CurrentFrame: " << (currentFrame ? "BUSY" : "NULL") << endl;
    // Guard: If we are already sending a frame, ignore this permission (duplicate grant)
    if (currentFrame != nullptr) {
        EV << "DEBUG: Ignoring permission, already busy." << endl;
        return;
    }
    deliverFrame(canID);
}

void CanOutputBuffer::sendingCompleted() {
    Enter_Method_Silent
    ();
    EV << "DEBUG: CanOutputBuffer::sendingCompleted - Start" << endl;
    if (currentFrame == nullptr) {
        EV << "DEBUG: currentFrame is NULL! Returning." << endl;
        return;
    }
    EV << "DEBUG: Deleting currentFrame: " << currentFrame << endl;
    deleteFrame(currentFrame);
    currentFrame = nullptr;
    EV << "DEBUG: Frame deleted. Notifying PortOutput." << endl;

    cModule* nodePort = getParentModule()->getSubmodule(par("nodePortName").stringValue());
    if (!nodePort) {
        EV << "CRITICAL: NodePort not found!" << endl;
        return;
    }
    CanPortOutput* portOutput = check_and_cast<CanPortOutput*>(nodePort->getSubmodule("canPortOutput"));
    EV << "DEBUG: calling portOutput->sendingCompleted()" << endl;
    portOutput->sendingCompleted();
    EV << "DEBUG: CanOutputBuffer::sendingCompleted - End" << endl;
}

}
