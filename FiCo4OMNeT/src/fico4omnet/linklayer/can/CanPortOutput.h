

#ifndef FICO4OMNET_CANPORTOUTPUT_H_
#define FICO4OMNET_CANPORTOUTPUT_H_

//FiCo4OMNeT
#include "fico4omnet/base/FiCo4OMNeT_Defs.h"

//Auto-generated messages
#include "fico4omnet/linklayer/can/messages/ErrorFrame_m.h"

namespace FiCo4OMNeT {

using namespace omnetpp;

/**
 * @brief Outgoing messages are handled in this module.
 *
 * @ingroup Port
 *
 * @author Stefan Buschmann
 */
class CanPortOutput: public cSimpleModule {

public:
    /**
     * @brief Constructor
     */
    CanPortOutput();
    
    /**
     * @brief Destructor
     */
    virtual ~CanPortOutput();

    /**
     * @brief Handles received error frames so that scheduled error events can be canceled.
     */
    virtual void handleReceivedErrorFrame();

    /**
     * @brief Is called when the transmission of a frame is completed.
     */
    virtual void sendingCompleted();

    // ISO 11898-1 Compliance: Receiver Error Handling
    virtual void receptionSuccessful();
    virtual void receptionFailed();

protected:
    /**
     * @brief Initialization of the module.
     */
    virtual void initialize();

    /**
     * @brief Handles all received messages
     *
     * @param msg the incoming message.
     */
    virtual void handleMessage(cMessage *msg);

private:
    /**
     * @brief Maximum size of an error frame.
     */
    static const int MAXERRORFRAMESIZE = 12;

    /**
     * @brief Simsignal for sent data frames.
     */
    simsignal_t sentDFSignal;

    /**
     * @brief Simsignal for sent remote frames.
     */
    simsignal_t sentRFSignal;

    /**
     * @brief Simsignal for receive errors.
     */
    simsignal_t receiveErrorsSignal;

    /**
     * @brief Simsignal for send errors.
     */
    simsignal_t sendErrorsSignal;

    /**
     * @brief Bandwidth of the bus in Mbps.
     */
    double bandwidth;

    /**
     * @brief Data phase bitrate in Mbps (for CAN-FD BRS).
     */
    double dataBitrate;

    /**
     * @brief Probability that an error for the received frame will occur.
     *
     */
    int errorperc;

    /**
     * @brief The currently scheduled error frame.
     */
    ErrorFrame *scheduledErrorFrame;

    /**
     * @brief True if an error frame for the current data frame is already received.
     */
    /**
     * @brief True if an error frame for the current data frame is already received.
     */
    bool errorReceived;

    // Error Handling State
    int tec = 0; // Transmit Error Counter
    int rec = 0; // Receive Error Counter
    bool isBusOff = false;
    bool isErrorPassive = false;

    /**
     * @brief Initializes the values needed for the stats collection.
     */
    virtual void initializeStatisticValues();

    /**
     * @brief Calculates when the frame is ready to be forwarded based on the number of bits.
     *
     * @param length length of the frame in bit
     *
     * @return the duration in seconds until frame transmission is completed
     */
    virtual double calculateScheduleTiming(int length) { return calculateScheduleTiming(length, false); }
    virtual double calculateScheduleTiming(int length, bool brs);

    /**
     * @brief Colors the connection to the bus to represent it is busy.
     */
    virtual void colorBusy();

    /**
     * @brief Colors the connection to the bus to represent it is idle.
     */
    virtual void colorIdle();

    /**
     * @brief Colors the connections to the bus to represent a transmission of an error frame.
     */
    virtual void colorError();
};

}

#endif /* CANPORTOUTPUT_H_ */
