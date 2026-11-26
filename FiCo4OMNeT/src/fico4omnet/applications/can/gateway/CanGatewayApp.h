#ifndef FICO4OMNET_CANGATEWAYAPP_H_
#define FICO4OMNET_CANGATEWAYAPP_H_

#include "fico4omnet/base/FiCo4OMNeT_Defs.h"
#include "fico4omnet/applications/can/sink/CanTrafficSinkAppBase.h"
#include "fico4omnet/applications/can/source/CanTrafficSourceAppBase.h"

namespace FiCo4OMNeT {

/**
 * @brief Gateway Application for bridging CAN-FD and Standard CAN
 */
class CanGatewayApp : public omnetpp::cSimpleModule {

public:
    CanGatewayApp();
    virtual ~CanGatewayApp();

protected:
    virtual void initialize() override;
    virtual void handleMessage(omnetpp::cMessage *msg) override;

    // Logic to fragment FD frames to Std
    void handleFdFrame(CanDataFrame *frame);

    // Logic to forward Std frames (simple pass-through)
    void handleStdFrame(CanDataFrame *frame);

    // Helper to send frame to specific port
    void sendToStdPort(CanDataFrame *frame);
    void sendToFdPort(CanDataFrame *frame);
    
    // Register reception
    void registerReception();

private:
   omnetpp::simsignal_t fragmentedSignal;
   omnetpp::simsignal_t forwardedSignal;
};

} // namespace FiCo4OMNeT

#endif
