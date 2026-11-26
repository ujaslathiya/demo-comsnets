#include "fico4omnet/applications/can/source/CanTrafficSourceAppBase.h"
#include "fico4omnet/scheduler/can/CanClock.h"
#include "fico4omnet/linklayer/can/CanPortInput.h"
#include "fico4omnet/utilities/CanFdUtil.h"

namespace FiCo4OMNeT {

Define_Module(CanTrafficSourceAppBase);

CanTrafficSourceAppBase::CanTrafficSourceAppBase() {
    bitStuffingPercentage = 0;
    currentDrift = 0;
}

CanTrafficSourceAppBase::~CanTrafficSourceAppBase() {
    for (auto df : outgoingDataFrames)
        cancelAndDelete(df);
    outgoingDataFrames.clear();
}

void CanTrafficSourceAppBase::initialize(int stage) {
    if (stage == 0) {
        canVersion =
            getParentModule()->gate("gate$o")->getPathEndGate()
                ->getOwnerModule()->getParentModule()
                ->par("version").stdstringValue();

        bitStuffingPercentage =
            getParentModule()->gate("gate$o")->getPathEndGate()
                ->getOwnerModule()->getParentModule()
                ->par("bitStuffingPercentage");

        sentDFSignal = registerSignal("txDF");
        sentRFSignal = registerSignal("txRF");

        checkParameterValues();
    }
    else if (stage == 2) {
        auto *canClock =
            dynamic_cast<CanClock*>(getParentModule()->getSubmodule("canClock"));
        currentDrift = canClock->getCurrentDrift();

        initialDataFrameCreation();
        initialRemoteFrameCreation();
    }
}

void CanTrafficSourceAppBase::checkParameterValues() {
    if (bitStuffingPercentage < 0 || bitStuffingPercentage > 1)
        throw cRuntimeError("bitStuffingPercentage must be in [0,1]");

    if (canVersion != "2.0A" && canVersion != "2.0B")
        throw cRuntimeError("canVersion must be 2.0A or 2.0B");
}

void CanTrafficSourceAppBase::handleMessage(cMessage *msg) {
    auto *df = check_and_cast<CanDataFrame *>(msg);
    frameTransmission(df);
}

void CanTrafficSourceAppBase::initialRemoteFrameCreation() {
    if (par("idRemoteFrames").stdstringValue() != "0")
        initialFrameCreation("remote", "idRemoteFrames",
                             "periodicityRemoteFrames",
                             "dataLengthRemoteFrames",
                             "initialRemoteFrameOffset");
}

void CanTrafficSourceAppBase::initialDataFrameCreation() {
    if (par("idDataFrames").stdstringValue() != "0")
        initialFrameCreation("data", "idDataFrames",
                             "periodicityDataFrames",
                             "dataLengthDataFrames",
                             "initialDataFrameOffset");
}

void CanTrafficSourceAppBase::initialFrameCreation(
        const char *type,
        const char *frameIDsParName,
        const char *framesPeriodicityParName,
        const char *dataLengthFramesParName,
        const char *initialFrameOffsetParName) {

    cStringTokenizer ids(par(frameIDsParName), ",");
    cStringTokenizer periods(par(framesPeriodicityParName), ",");
    cStringTokenizer lengths(par(dataLengthFramesParName), ",");
    cStringTokenizer offsets(par(initialFrameOffsetParName), ",");

    std::vector<int> frameIDs = ids.asIntVector();

    for (unsigned int i = 0; i < frameIDs.size(); i++) {

        if (!lengths.hasMoreTokens() || !periods.hasMoreTokens())
            throw cRuntimeError("Frame parameter mismatch in ini file");

        unsigned int payloadLen =
            static_cast<unsigned int>(atoi(lengths.nextToken()));

        const char* msgName;
        if (std::strcmp(type, "data") == 0) {
            msgName = (payloadLen > 8) ? "CAN_FD" : "CAN_STD";
        } else {
            msgName = "remoteFrame";
        }

        CanDataFrame *df = new CanDataFrame(msgName);

        df->setCanID(checkAndReturnID(frameIDs[i]));
        df->setRtr(std::strcmp(type, "remote") == 0);

        bool isFd = payloadLen > 8;
        df->setIsCanFd(isFd);
        df->setBrs(isFd);

        int dlc = CanFdUtil::lengthToDlc(payloadLen);
        df->setDlc(dlc);

        df->setPayloadArraySize(payloadLen);
        for (unsigned int j = 0; j < payloadLen; j++)
            df->setPayload(j, static_cast<uint8_t>(j & 0xFF));
        
        // Removed visual coloring (setKind) as requested
        
        EV << "Created Frame ID: " << df->getCanID() 
           << " Name: " << msgName
           << " isFd: " << (isFd ? "TRUE" : "FALSE") << endl;

    //         df->setDisplayString(





        df->setPeriod(atof(periods.nextToken()));
        
        // precise ISO 11898-1 bit length (including dynamic stuffing)
        df->setBitLength(CanFdUtil::calculateStuffedBitLength(df)); 

        if (!df->getRtr()) {
            outgoingDataFrames.push_back(df);
            registerDataFrameAtPort(df->getCanID());
        } else {
            registerRemoteFrameAtPort(df->getCanID());
        }

        double offset = offsets.hasMoreTokens()
                        ? atof(offsets.nextToken()) : 0;

        auto *canClock =
            dynamic_cast<CanClock*>(getParentModule()->getSubmodule("canClock"));
        currentDrift = canClock->getCurrentDrift();

        simtime_t t = simTime()
            + SimTime(offset)
            + SimTime(par("periodInaccurracy").doubleValue() + currentDrift);

        scheduleAt(t < SIMTIME_ZERO ? simTime() : t, df);
    }
}

void CanTrafficSourceAppBase::registerRemoteFrameAtPort(unsigned int canID) {
    auto *port =
        dynamic_cast<CanPortInput*>(
            getParentModule()->getSubmodule("canNodePort")
                ->getSubmodule("canPortInput"));
    port->registerOutgoingRemoteFrame(canID);
}

void CanTrafficSourceAppBase::registerDataFrameAtPort(unsigned int canID) {
    auto *port =
        dynamic_cast<CanPortInput*>(
            getParentModule()->getSubmodule("canNodePort")
                ->getSubmodule("canPortInput"));
    port->registerOutgoingDataFrame(canID, gate("remoteIn"));
}

unsigned int CanTrafficSourceAppBase::checkAndReturnID(unsigned int canID) {
    if ((canVersion == "2.0A" && canID > VERSIONAMAX) ||
        (canVersion == "2.0B" && canID > VERSIONBMAX)) {
        EV << "Invalid CAN ID " << canID << endl;
        endSimulation();
    }
    return canID;
}

unsigned int CanTrafficSourceAppBase::calculateLength(unsigned int payloadLen) {
    unsigned int arbLen = (canVersion == "2.0B") ? ARBITRATIONFIELD29BIT : 0;
    return arbLen + DATAFRAMECONTROLBITS
         + calculateStuffingBits(payloadLen, arbLen);
}

unsigned int CanTrafficSourceAppBase::calculateStuffingBits(
        unsigned int payloadLen, unsigned int arbLen) {
    return static_cast<unsigned int>(
        ((CONTROLBITSFORBITSTUFFING + arbLen + payloadLen * 8 - 1) / 4)
        * bitStuffingPercentage);
}

void CanTrafficSourceAppBase::frameTransmission(CanDataFrame *df) {
    CanDataFrame *out = nullptr;

    emit(df->getRtr() ? sentRFSignal : sentDFSignal, df);

    if (df->isSelfMessage()) {
        out = df->dup();

        auto *canClock =
            dynamic_cast<CanClock*>(getParentModule()->getSubmodule("canClock"));
        currentDrift = canClock->getCurrentDrift();

        scheduleAt(simTime() + df->getPeriod()
                   + SimTime(par("periodInaccurracy").doubleValue() + currentDrift),
                   df);
    }
    else if (df->arrivedOn("remoteIn")) {
        // Fix: Ignore own Data Frames (Echoes). Only respond to Remote Frames.
        if (!df->getRtr()) {
            delete df;
            return;
        }

        for (auto tmp : outgoingDataFrames) {
            if (tmp->getCanID() == df->getCanID()) {
                out = tmp->dup();
                break;
            }
        }
        delete df;
    }
    else {
        throw cRuntimeError("Invalid message received by CanTrafficSourceApp");
    }

    out->setTimestamp(simTime());
    out->setTimestamp(simTime());
    send(out, "out");
}

} // namespace FiCo4OMNeT

