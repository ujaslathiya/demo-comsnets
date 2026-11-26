#!/bin/bash
# Run the ZonalHybridNetwork (hybrid zonal star) simulation on macOS.
#
# Uses the native build in $OMNET_WS (default ~/omnet_ws), created by
# setup_macos.sh (repository root). OMNeT++ cannot read files below a
# directory whose path contains spaces, so
# the run executes in the workspace: the .ned and .ini files are copied there
# from this repository before every run, and the result files are copied back to
# results/macos/ here afterwards.
#
# Usage:
#   ./run_macos.sh                 # headless (Cmdenv), 10 s simulated time
#   ./run_macos.sh gui             # Qtenv graphical runtime
#   SIM_TIME=60s ./run_macos.sh    # other simulated time
#   ./run_macos.sh cmdenv --seed-set=3   # extra opp_run options are passed through

WS="${OMNET_WS:-$HOME/omnet_ws}"
SIM_TIME="${SIM_TIME:-10s}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

MODE="Cmdenv"
EXTRA=(--cmdenv-express-mode=true)
case "$1" in
    gui|qtenv|Qtenv) MODE="Qtenv"; EXTRA=(); shift ;;
    cmdenv|Cmdenv)   shift ;;
esac

if [ ! -x "$WS/omnetpp-6.0.3/bin/opp_run" ]; then
    echo "OMNeT++ build not found in $WS -- run setup_macos.sh (repository root) first." >&2
    exit 1
fi

[ -f "$WS/venv/bin/activate" ] && source "$WS/venv/bin/activate"
source "$WS/omnetpp-6.0.3/setenv" -f >/dev/null

# Sync the interpreted inputs (NED, ini) from the package into the workspace
RUN_DIR="$WS/ZonalHybridNetwork/examples"
mkdir -p "$RUN_DIR" "$WS/ZonalHybridNetwork/zonal_hybrid"
cp "$SCRIPT_DIR"/*.ini "$RUN_DIR/"
cp "$SCRIPT_DIR"/../zonal_hybrid/*.ned "$WS/ZonalHybridNetwork/zonal_hybrid/"

cd "$RUN_DIR" || exit 1
rm -rf results/macos && mkdir -p results/macos

opp_run -u "$MODE" \
  -n "$WS/ZonalHybridNetwork/zonal_hybrid;$WS/MultiProtocol_Gateway;$WS/lin;$WS/inet/src;$WS/FiCo4OMNeT/src" \
  -l "$WS/MultiProtocol_Gateway/MultiProtocol_Gateway" \
  -l "$WS/lin/LIN" \
  -l "$WS/inet/src/INET" \
  -l "$WS/FiCo4OMNeT/src/FiCo4OMNeT" \
  --sim-time-limit="$SIM_TIME" \
  --result-dir=results/macos \
  "${EXTRA[@]}" \
  omnetpp.ini "$@"
STATUS=$?

# Copy results back into the package
mkdir -p "$SCRIPT_DIR/results/macos"
cp results/macos/* "$SCRIPT_DIR/results/macos/" 2>/dev/null
echo "Results: $SCRIPT_DIR/results/macos/"
exit $STATUS
