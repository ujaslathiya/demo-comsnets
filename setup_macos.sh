#!/bin/bash
# One-time macOS setup for the OMNeT++ simulation stack (tested on macOS 26, Intel,
# Apple clang 17). Builds everything the ZonalTopology (zonal TSN) network needs:
#   OMNeT++ 6.0.3, INET 3.8.5, FiCo4OMNeT, CoRE4INET, lin, MultiProtocol_Gateway
#
# The build happens in a workspace without spaces in its path ($OMNET_WS,
# default ~/omnet_ws), because OMNeT++ makefiles break on paths with spaces.
# Project sources are copied there from this repository.
#
# Re-running is safe: finished steps are skipped, project sources are re-synced
# and rebuilt incrementally.
#
# Requirements: Xcode Command Line Tools (xcode-select --install), python3.
# On Apple Silicon change OMNETPP_TGZ to the aarch64 archive.

set -e

WS="${OMNET_WS:-$HOME/omnet_ws}"
PKG="$(cd "$(dirname "$0")" && pwd)"
JOBS="$(sysctl -n hw.ncpu)"
# Leave room in the dylib headers so OMNeT++'s install_name_tool step can write
# absolute library paths even when the workspace path is long.
export LDFLAGS_EXTRA="-Wl,-headerpad_max_install_names"
OMNETPP_TGZ="omnetpp-6.0.3-macos-x86_64.tgz"
OMNETPP_URL="https://github.com/omnetpp/omnetpp/releases/download/omnetpp-6.0.3/$OMNETPP_TGZ"
INET_URL="https://github.com/inet-framework/inet/releases/download/v3.8.5/inet-3.8.5-src.tgz"

mkdir -p "$WS/dl"
cd "$WS"

# --- Python modules that OMNeT++'s configure checks for ----------------------
if [ ! -d venv ]; then
    python3 -m venv venv
    venv/bin/pip install -q --upgrade pip
    venv/bin/pip install -q numpy pandas matplotlib scipy posix_ipc
fi
source venv/bin/activate

# --- OMNeT++ 6.0.3 (Cmdenv + Qtenv, no OSG) ----------------------------------
if [ ! -x omnetpp-6.0.3/bin/opp_run ]; then
    [ -f "dl/$OMNETPP_TGZ" ] || curl -fL --retry 3 -o "dl/$OMNETPP_TGZ" "$OMNETPP_URL"
    [ -d omnetpp-6.0.3 ] || tar xzf "dl/$OMNETPP_TGZ"
    (cd omnetpp-6.0.3 && source setenv -f && \
        ./configure WITH_OSG=no WITH_OSGEARTH=no && make -j"$JOBS" MODE=release)
fi
source omnetpp-6.0.3/setenv -f >/dev/null

# --- INET 3.8.5 (visualization disabled, as CoRE4INET requires) --------------
if [ ! -f inet/src/libINET.dylib ]; then
    [ -f dl/inet-3.8.5-src.tgz ] || curl -fL --retry 3 -o dl/inet-3.8.5-src.tgz "$INET_URL"
    [ -d inet ] || tar xzf dl/inet-3.8.5-src.tgz
    (cd inet && \
        for f in visualization visualization_examples visualizationtutorial visualizershowcases \
                 wirelessshowcases wirelesstutorial configuratortutorial TCP_lwIP TCP_NSC \
                 packetdrill ExternalInterface ExternalInterface_examples VoIPStream VoIPStream_examples; do
            ./inet_featuretool disable -f "$f" >/dev/null 2>&1 || true
        done && \
        make makefiles && make -j"$JOBS" MODE=release)
fi

# --- Project sources: copy from the package (no Linux binaries, no results) --
rsync -a --exclude 'out/' --exclude '*.so' --exclude '*.a' --exclude '*.so.bak' --exclude '*.o' \
      --exclude '*_m.cc' --exclude '*_m.h' --exclude 'Makefile' \
      --exclude 'results/' --exclude '*.log' --exclude '.DS_Store' \
      "$PKG/CoRE4INET" "$PKG/FiCo4OMNeT" "$PKG/lin" "$PKG/MultiProtocol_Gateway" "$PKG/ZonalTopology" "$WS/"
rsync -a "$PKG/CoRE4INET/Makefile" "$WS/CoRE4INET/Makefile"
rm -f "$WS/MultiProtocol_Gateway/MultiProtocol_Gateway"   # Linux executable, shadows the library name

# Portability fix: cListener::receiveSignal() takes intval_t (int64_t). That is
# 'long' on Linux but 'long long' on macOS, so overrides declared with 'long'
# do not override anything here. Rewrite them to the portable typedefs.
grep -rlE "receiveSignal" FiCo4OMNeT/src CoRE4INET/src --include="*.h" --include="*.cc" | \
    xargs perl -pi -e 'if (/receiveSignal/) { $n = 3 }
        if ($n > 0) { s/\bunsigned long(\s+l\b)/omnetpp::uintval_t$1/g;
                      s/(?<!unsigned )\blong(\s+l\b)/omnetpp::intval_t$1/g; $n-- }
        $n = 0 if eof'

# --- FiCo4OMNeT, lin, CoRE4INET, MultiProtocol_Gateway -----------------------
(cd FiCo4OMNeT/src && opp_makemake --make-so -f --deep -I. -o FiCo4OMNeT && make -j"$JOBS" MODE=release)
(cd lin && opp_makemake --make-so -f -I. -o LIN -X src && make -j"$JOBS" MODE=release)
(cd CoRE4INET && make makefiles && make -j"$JOBS" MODE=release)
(cd MultiProtocol_Gateway && \
    opp_makemake -f --deep --make-so -o MultiProtocol_Gateway \
        -I../inet/src -I../FiCo4OMNeT/src -I.. \
        -L../inet/src -lINET -L../FiCo4OMNeT/src -lFiCo4OMNeT -L../lin -lLIN -DINET_IMPORT && \
    make -j"$JOBS" MODE=release)

echo
echo "Setup complete. Run the zonal TSN simulation with:"
echo "  \"$PKG/ZonalTopology/examples/run_macos.sh\"        # headless"
echo "  \"$PKG/ZonalTopology/examples/run_macos.sh\" gui    # Qtenv"
