#ifndef FICO4OMNET_CANBUSLOGIC_H_
#define FICO4OMNET_CANBUSLOGIC_H_

#include "fico4omnet/base/FiCo4OMNeT_Defs.h"
#include "fico4omnet/bus/can/CanID.h"
#include "fico4omnet/linklayer/can/messages/CanDataFrame_m.h"

namespace FiCo4OMNeT {

class CanBusLogic : public cSimpleModule {

public:
    CanBusLogic();
    virtual ~CanBusLogic();
    
    virtual void registerForArbitration(unsigned int canID, cModule *module,
                                       simtime_t signInTime, bool rtr);
    virtual void checkoutFromArbitration(unsigned int canID);
    
    unsigned int getCurrentSendingId() const { return currentSendingID; }
    int getSendingNodeID();

protected:
    enum class State { IDLE = 0, TRANSMITTING = 1 };
    
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;

private:
    static const int MAXERRORFRAMESIZE = 12;
    
    simsignal_t rcvdSignal;
    simsignal_t rcvdDFSignal;
    simsignal_t rcvdRFSignal;
    simsignal_t rcvdEFSignal;
    simsignal_t rcvdDFPayloadBytesSignal;
    simsignal_t stateSignal;
    simsignal_t arbitrationLengthSignal;
    
    simtime_t currsit;
    bool errored;
    bool idle;
    int errpos;
    double bandwidth;
    
    std::list<CanID*> ids;
    std::vector<std::list<CanID*>::iterator> eraseids;
    
    long numDataFrames;
    long numRemoteFrames;
    long numErrorFrames;
    unsigned long numFramesSent;
    unsigned long numBitsSent;
    unsigned int currentSendingID;
    
    CanDataFrame *scheduledDataFrame;
    cModule *sendingNode;
    
    void grantSendingPermission();
    void sendingCompleted();
    void handleDataFrame(cMessage *msg);
    void handleErrorFrame(cMessage *msg);
    void colorBusy();
    void colorIdle();
    void colorError();
};

} // namespace FiCo4OMNeT

#endif

