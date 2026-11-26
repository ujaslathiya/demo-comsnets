#!/bin/bash
# ─────────────────────────────────────────────────────────────────────────────
# run_gui.sh  —  Launch ZonalHybridNetwork in OMNeT++ Qtenv (GUI mode)
#
# Usage:
#   ./run_gui.sh              # opens Qtenv GUI, use File > Record to capture
#   ./run_gui.sh --demo       # loads demo ini (5s sim, fast animation for recording)
#   ./run_gui.sh --record     # auto-records via ffmpeg (requires ffmpeg installed)
# ─────────────────────────────────────────────────────────────────────────────

OMNETPP_DIR="${OMNETPP_DIR:-$HOME/omnetpp-6.0.3}"
PROJECT_ROOT="${PROJECT_ROOT:-$(cd "$(dirname "$0")/../.." && pwd)}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

source "$OMNETPP_DIR/setenv"
export DISPLAY="${DISPLAY:-:1}"

cd "$SCRIPT_DIR"

# Choose ini file
INI="omnetpp.ini"
if [[ "$1" == "--demo" || "$1" == "--record" ]]; then
    INI="omnetpp_demo.ini"
fi

OPP_CMD="opp_run -u Qtenv
  -n $PROJECT_ROOT/ZonalHybridNetwork/zonal_hybrid
  -n $PROJECT_ROOT/MultiProtocol_Gateway
  -n $PROJECT_ROOT/lin
  -n $PROJECT_ROOT/inet/src
  -n $PROJECT_ROOT/FiCo4OMNeT/src
  -l $PROJECT_ROOT/MultiProtocol_Gateway/MultiProtocol_Gateway
  -l $PROJECT_ROOT/lin/LIN
  -l $PROJECT_ROOT/inet/src/INET
  -l $PROJECT_ROOT/FiCo4OMNeT/src/FiCo4OMNeT
  $INI"

if [[ "$1" == "--record" ]]; then
    # ── Auto-record with ffmpeg ──────────────────────────────────────────────
    if ! command -v ffmpeg &>/dev/null; then
        echo "ERROR: ffmpeg not found. Run:  sudo apt install -y ffmpeg"
        exit 1
    fi

    OUTPUT="./zonal_topology_recording.mp4"
    FRAMERATE=30

    echo "Recording to: $OUTPUT"
    echo "Starting OMNeT++ in background …"

    # Launch sim in background
    eval $OPP_CMD &
    SIM_PID=$!

    sleep 3   # wait for Qtenv window to open

    # Detect window geometry (find OMNeT++ window)
    GEOM=$(xdotool search --name "ZonalHybridNetwork" getwindowgeometry 2>/dev/null \
           | grep -oP 'Geometry: \K[0-9]+x[0-9]+' | head -1)
    GEOM="${GEOM:-1280x720}"
    echo "Recording geometry: $GEOM"

    # Record screen via ffmpeg x11grab
    ffmpeg -y \
        -f x11grab -r $FRAMERATE -s $GEOM -i $DISPLAY \
        -vcodec libx264 -preset ultrafast -crf 23 \
        -pix_fmt yuv420p \
        "$OUTPUT" &
    FFMPEG_PID=$!

    echo ""
    echo "═══════════════════════════════════════════════════════"
    echo "  Recording started!"
    echo "  → Run the simulation in the OMNeT++ window"
    echo "  → Press Ctrl+C here when done to stop recording"
    echo "═══════════════════════════════════════════════════════"

    # Wait for sim to finish or user Ctrl+C
    wait $SIM_PID 2>/dev/null
    sleep 1
    kill $FFMPEG_PID 2>/dev/null
    wait $FFMPEG_PID 2>/dev/null
    echo "Saved: $OUTPUT"
else
    # ── Normal GUI launch ────────────────────────────────────────────────────
    echo "Launching OMNeT++ Qtenv GUI …"
    echo "DISPLAY=$DISPLAY"
    echo ""
    echo "📽  To record inside OMNeT++:"
    echo "    File → Record Animation  (saves frame-by-frame PNG sequence)"
    echo ""
    echo "📽  To record with ffmpeg (external):"
    echo "    Open another terminal and run:"
    echo "    ffmpeg -f x11grab -r 30 -s 1280x720 -i :1 -vcodec libx264 output.mp4"
    echo ""
    eval $OPP_CMD
fi
