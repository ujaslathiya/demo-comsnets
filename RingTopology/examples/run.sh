#!/bin/bash
# Run the RingTopology (multi-hop backbone) with OMNeT++ 6.0.3
# Topology: 4 zones (CAN_FD_A, CAN_FD_B, CAN_Std, LIN) on 1Gbps Ethernet ring
# KEY: ring8 link (SW_B→SW_A) shared by CAN_FD_B + CAN_Std zones

OMNETPP_DIR="${OMNETPP_DIR:-$HOME/omnetpp-6.0.3}"
PROJECT_ROOT="${PROJECT_ROOT:-$(cd "$(dirname "$0")/../.." && pwd)}"

source "$OMNETPP_DIR/setenv"

cd "$(dirname "$0")"

opp_run -u Cmdenv \
  -n "$PROJECT_ROOT/RingTopology/ring_topology" \
  -n "$PROJECT_ROOT/MultiProtocol_Gateway" \
  -n "$PROJECT_ROOT/lin" \
  -n "$PROJECT_ROOT/CoRE4INET/src" \
  -n "$PROJECT_ROOT/inet/src" \
  -n "$PROJECT_ROOT/FiCo4OMNeT/src" \
  -l "$PROJECT_ROOT/MultiProtocol_Gateway/MultiProtocol_Gateway" \
  -l "$PROJECT_ROOT/lin/LIN" \
  -l "$PROJECT_ROOT/CoRE4INET/src/CoRE4INET" \
  -l "$PROJECT_ROOT/inet/src/INET" \
  -l "$PROJECT_ROOT/FiCo4OMNeT/src/FiCo4OMNeT" \
  omnetpp.ini "$@"
