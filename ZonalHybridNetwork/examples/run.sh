#!/bin/bash
# Run the ZonalHybridNetwork simulation with OMNeT++ 6.0.3
# All libraries are compiled for clang-release

OMNETPP_DIR="${OMNETPP_DIR:-$HOME/omnetpp-6.0.3}"
PROJECT_ROOT="${PROJECT_ROOT:-$(cd "$(dirname "$0")/../.." && pwd)}"

source "$OMNETPP_DIR/setenv"

cd "$(dirname "$0")"

opp_run -u Cmdenv \
  -n "$PROJECT_ROOT/ZonalHybridNetwork/zonal_hybrid" \
  -n "$PROJECT_ROOT/MultiProtocol_Gateway" \
  -n "$PROJECT_ROOT/lin" \
  -n "$PROJECT_ROOT/inet/src" \
  -n "$PROJECT_ROOT/FiCo4OMNeT/src" \
  -l "$PROJECT_ROOT/MultiProtocol_Gateway/MultiProtocol_Gateway" \
  -l "$PROJECT_ROOT/lin/LIN" \
  -l "$PROJECT_ROOT/inet/src/INET" \
  -l "$PROJECT_ROOT/FiCo4OMNeT/src/FiCo4OMNeT" \
  omnetpp.ini "$@"
