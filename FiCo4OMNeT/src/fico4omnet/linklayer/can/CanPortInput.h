#ifndef FICO4OMNET_CANPORTINPUT_H_
#define FICO4OMNET_CANPORTINPUT_H_

#include "fico4omnet/base/FiCo4OMNeT_Defs.h"
#include "fico4omnet/linklayer/can/messages/CanDataFrame_m.h"
#include "fico4omnet/linklayer/can/messages/ErrorFrame_m.h"

namespace FiCo4OMNeT {

class CanPortOutput;

class CanPortInput : public omnetpp::cSimpleModule {
public:
  CanPortInput();

  void registerOutgoingDataFrame(unsigned int canID, omnetpp::cGate *gate);
  void registerOutgoingRemoteFrame(unsigned int canID);
  void registerIncomingDataFrame(unsigned int canID, omnetpp::cGate *gate);

protected:
  void initialize() override;
  void handleMessage(omnetpp::cMessage *msg) override;

private:
  static const int MAXERRORFRAMESIZE = 12;

  omnetpp::simsignal_t rcvdDFSignal;
  omnetpp::simsignal_t rcvdRFSignal;
  omnetpp::simsignal_t receivedDFPayloadBytes;
  omnetpp::simsignal_t receivedRFPayloadBytes;
  // Numeric latency: (simTime - frame.timestamp) in seconds, replaces timestampAge() removed in OMNeT++ 6
  omnetpp::simsignal_t rxDFLatencySignal;
  omnetpp::simsignal_t rxRFLatencySignal;

  std::map<unsigned int, omnetpp::cGate *> incomingDataFrameIDs;
  std::map<unsigned int, omnetpp::cGate *> outgoingDataFrameIDs;
  std::vector<unsigned int> outgoingRemoteFrameIDs;

  double bandwidth;
  double dataBitrate;
  int errorperc;

  CanDataFrame *scheduledDataFrame;
  ErrorFrame *scheduledErrorFrame;
  CanPortOutput *portOutput;

  void receiveMessage(CanDataFrame *msg);
  void generateReceiveError(CanDataFrame *df);

  bool checkExistence(CanDataFrame *df);
  bool checkOutgoingDataFrames(unsigned int canID);
  bool checkOutgoingRemoteFrames(unsigned int canID);
  bool checkIncomingDataFrames(unsigned int canID);

  double calculateScheduleTiming(int length) {
    return calculateScheduleTiming(length, false);
  }
  double calculateScheduleTiming(int length, bool brs);

  void forwardDataFrame(CanDataFrame *msg);
  void forwardOwnErrorFrame(ErrorFrame *ef);
  void handleExternErrorFrame(ErrorFrame *ef);

  // ISO 11898-1 CRC Validation
  bool validateCRC(CanDataFrame *df);

  bool amITheSendingNode();
};

} // namespace FiCo4OMNeT

#endif
