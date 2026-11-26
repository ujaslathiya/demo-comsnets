#!/bin/bash
# Run the ZonalTopology (TSN-enabled zonal network) with OMNeT++ 6.0.3
# TSN:  IEEE 802.1Qbv TAS (edge switches) + IEEE 802.1Qav CBS (all switches)
#       via CoRE4INET TSNEtherSwitch / TSNEtherHost

OMNETPP_DIR="${OMNETPP_DIR:-$HOME/omnetpp-6.0.3}"
PROJECT_ROOT="${PROJECT_ROOT:-$(cd "$(dirname "$0")/../.." && pwd)}"

source "$OMNETPP_DIR/setenv"

cd "$(dirname "$0")"

opp_run -u Cmdenv \
  -n "$PROJECT_ROOT/ZonalTopology/zonal_topology" \
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
