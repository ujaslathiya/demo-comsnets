#!/bin/bash
# Open a new terminal and run the RingTopology simulation with Qtenv (visual GUI).
# Qtenv shows the animated network topology with packet flow, statistics panels,
# and an object inspector for all nodes, buses, and switches.
#
# Usage:
#   bash run_gui.sh          # opens Qtenv simulation in a new gnome-terminal
#   bash run_gui.sh cmdenv   # runs headless (Cmdenv, no window) in a new terminal

OMNETPP_DIR="${OMNETPP_DIR:-$HOME/omnetpp-6.0.3}"
PROJECT_ROOT="${PROJECT_ROOT:-$(cd "$(dirname "$0")/../.." && pwd)}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

MODE="Qtenv"
if [[ "$1" == "cmdenv" ]]; then
    MODE="Cmdenv"
fi

# Build the opp_run command
OPP_CMD="source \"$OMNETPP_DIR/setenv\" && \
cd \"$SCRIPT_DIR\" && \
opp_run -u $MODE \
  -n \"$PROJECT_ROOT/RingTopology/ring_topology\" \
  -n \"$PROJECT_ROOT/MultiProtocol_Gateway\" \
  -n \"$PROJECT_ROOT/lin\" \
  -n \"$PROJECT_ROOT/CoRE4INET/src\" \
  -n \"$PROJECT_ROOT/inet/src\" \
  -n \"$PROJECT_ROOT/FiCo4OMNeT/src\" \
  -l \"$PROJECT_ROOT/MultiProtocol_Gateway/MultiProtocol_Gateway\" \
  -l \"$PROJECT_ROOT/lin/LIN\" \
  -l \"$PROJECT_ROOT/CoRE4INET/src/CoRE4INET\" \
  -l \"$PROJECT_ROOT/inet/src/INET\" \
  -l \"$PROJECT_ROOT/FiCo4OMNeT/src/FiCo4OMNeT\" \
  omnetpp.ini ; \
echo ''; echo 'Simulation finished. Press Enter to close.'; read"

echo "Launching RingTopology simulation in a new terminal (mode: $MODE) ..."

gnome-terminal \
    --title="RingTopology OMNeT++ Simulation ($MODE)" \
    --geometry=120x40 \
    -- bash -c "$OPP_CMD"
