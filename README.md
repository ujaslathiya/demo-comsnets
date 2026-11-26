# MultiGateway: Heterogeneous In-Vehicle Network Simulation in OMNeT++

OMNeT++ simulation of a zonal in-vehicle network with a multi-protocol zonal gateway, the
**MultiGateway**, that terminates Classical CAN, CAN-FD, LIN, and Automotive Ethernet in one
module. The gateway implements:

- **Upstream aggregation** (fieldbus to Ethernet) with a 64-byte size threshold and a 0.5 ms timeout
- **Downstream segmentation** (Ethernet to fieldbus) to 8-byte (CAN) or 64-byte (CAN-FD) frames,
  released in CAN-identifier order with 1 ms pacing
- **LIN master polling** (15 ms poll interval)
- **Thirteen statistics signals**: per-protocol queue length, queuing delay and end-to-end delay,
  plus Ethernet gateway-to-gateway delay, drop count, processing delay and uplink utilisation

This repository accompanies the demo paper *"A Heterogeneous In-Vehicle Network and
Multi-Protocol Zonal Gateway Simulation in OMNeT++"*.

## Repository layout

| Path | Content |
|---|---|
| `MultiProtocol_Gateway/` | The MultiGateway module (`MultiGateway.cc/.h/.ned`, `RoutingTable.cc/.h`) |
| `lin/` | LIN bus and node model, developed within this project and used by the MultiGateway (not a contribution of the demo paper) |
| `FiCo4OMNeT/` | FiCo4OMNeT CAN model, **extended with CAN-FD** (dual bit rate, 64-byte payloads) |
| `CoRE4INET/` | CoRE4INET real-time Ethernet / TSN models (IEEE 802.1Qbv, 802.1Qav) |
| `SignalsAndGateways/` | CAN–Ethernet–CAN reference network used for the 9,000-run campaign |
| `ZonalHybridNetwork/` | Topology 1: hybrid zonal star, INET switches, one ECU per bus |
| `ZonalTopology/` | Topology 2: zonal star with TSN backbone, three ECUs per bus |
| `RingTopology/` | Topology 3: multi-hop backbone, four TSN switches in a chain (SW_C–SW_B–SW_A–SW_D) joined by 1 Gbit/s links |
| `scripts/` | Batch drivers, result parser, and `vci_stats.py` |
| `data/` | Wall-clock times of the 9,000-run campaign |
| `setup_macos.sh` | One-step build script for macOS |

Each topology folder has the network in `<name>/*.ned`, the configuration in
`examples/omnetpp.ini`, run scripts in `examples/`, and the recorded results used in the
paper in `examples/results/`.

## Requirements

- OMNeT++ 6.0.3
- INET 3.8.5
- A C++17 compiler (GCC 9+ or Clang 10+), Python 3 with `pandas` and `numpy` for the scripts

## Build

### macOS

```bash
./setup_macos.sh
```

The script downloads and builds OMNeT++ 6.0.3 and INET 3.8.5, then builds FiCo4OMNeT, lin,
CoRE4INET and the MultiGateway in a workspace (`$OMNET_WS`, default `~/omnet_ws`). It uses a
workspace because OMNeT++ makefiles do not support paths that contain spaces. Re-running it is
safe; finished steps are skipped. On Apple Silicon, change `OMNETPP_TGZ` in the script to the
`aarch64` archive.

### Linux

Install OMNeT++ 6.0.3 and build INET 3.8.5 into `./inet` (repository root), then:

```bash
source $OMNETPP_DIR/setenv
(cd FiCo4OMNeT/src && opp_makemake --make-so -f --deep -I. -o FiCo4OMNeT && make MODE=release)
(cd lin && opp_makemake --make-so -f -I. -o LIN && make MODE=release)
(cd CoRE4INET && make makefiles && make MODE=release)
(cd MultiProtocol_Gateway && \
  opp_makemake -f --deep --make-so -o MultiProtocol_Gateway \
    -I../inet/src -I../FiCo4OMNeT/src -I.. \
    -L../inet/src -lINET -L../FiCo4OMNeT/src -lFiCo4OMNeT -L../lin -lLIN -DINET_IMPORT && \
  make MODE=release)
```

The batch drivers in `scripts/` also need SignalsAndGateways (Linux, clang build; they look for
libraries under `out/clang-release/`):

```bash
(cd SignalsAndGateways && make makefiles && make MODE=release)
```

## Run

```bash
# macOS (uses the workspace built by setup_macos.sh)
ZonalTopology/examples/run_macos.sh          # headless, 10 s simulated time
ZonalTopology/examples/run_macos.sh gui      # OMNeT++ Qtenv graphical runtime
ZonalHybridNetwork/examples/run_macos.sh gui

# Linux (set OMNETPP_DIR if OMNeT++ is not in ~/omnetpp-6.0.3)
ZonalTopology/examples/run.sh                # headless
ZonalTopology/examples/run_gui.sh            # Qtenv
RingTopology/examples/run.sh
ZonalHybridNetwork/examples/run.sh
```

Gateway parameters can be changed in each `omnetpp.ini`, for example (ZonalTopology):

```ini
*.Gateway_*.processingDelay   = 0.0001s   # d_proc
*.Gateway_*.enableAggregation = false     # forward every payload immediately
```

## Reproducing the numbers in the paper

The result folders contain the OMNeT++ vector index files (`.vci`). Each index line stores
count, minimum, maximum, sum, and sum of squares for one recorded signal, which is enough to
recover the statistics reported in the paper:

```bash
python3 scripts/vci_stats.py ZonalTopology/examples/results/General-0.vci \
  'E2EDelay|QueuingDelay|InputQueueLength|inputDropCount'
```

| Topology | Result file | Run |
|---|---|---|
| Hybrid zonal star | `ZonalHybridNetwork/examples/results/run_*/General-#0.vci` | 25 runs, 10 s, distinct seeds |
| Zonal star with TSN | `ZonalTopology/examples/results/General-0.vci` | 10 s |
| Multi-hop backbone | `RingTopology/examples/results/General-0.vci` | 10 s |

Expected values (all in µs; minimum, mean and maximum coincide unless a range is given):

| Paper quantity | Topology | Module and signal | Value |
|---|---|---|---|
| CAN fieldbus-to-backbone delay | Hybrid | `Gateway_1` `canE2EDelay` | 1740 |
| CAN-FD fieldbus-to-backbone delay | Hybrid | `Gateway_2` `canFdE2EDelay` | 1126.8 |
| LIN fieldbus-to-backbone delay | Hybrid | `Gateway_4` `linE2EDelay` | 8029.17 |
| Gateway-to-gateway, INET switch | Hybrid | `Gateway_2` `ethE2EDelay` | 1008.68 |
| CAN fieldbus-to-backbone delay | Zonal TSN | `Gateway_1` `canE2EDelay` | 852 |
| CAN-FD fieldbus-to-backbone delay | Zonal TSN | `Gateway_2` `canFdE2EDelay` | 286 |
| LIN fieldbus-to-backbone delay | Zonal TSN | `Gateway_4` `linE2EDelay` | 7129.17 |
| Gateway-to-gateway, 1 TSN switch | Zonal TSN | `Gateway_2` `ethE2EDelay` | 116.68 |
| Gateway-to-gateway, 2–3 TSN switches | Multi-hop | `GW_A`, `GW_B` `ethE2EDelay` | 125.456–134.232 |
| Gateway-to-gateway, 4 TSN switches | Multi-hop | `GW_C`, `GW_D` `ethE2EDelay` | 143.008 |

The LIN bus runs at 19.2 kbit/s (`*.LIN_Bus.logic.baudRate`).

The batch drivers in `scripts/` read `OMNETPP_DIR` and `PROJECT_ROOT` from the environment
(defaults: `~/omnetpp-6.0.3` and the repository root):

```bash
python3 scripts/run_multitopo_omnet_suite.py   # multi-topology suite
python3 scripts/run_9000_omnet_60s.py          # 9,000 runs on the reference network
```

## Third-party components

`FiCo4OMNeT/`, `CoRE4INET/` and `SignalsAndGateways/` are open-source frameworks from the
CoRE Research Group (HAW Hamburg). They are included because FiCo4OMNeT carries the CAN-FD
extension made in this work, and they remain under their original licenses; see the `License`
file in each folder. INET is not included and is
downloaded by the setup script.

## License

Copyright (C) 2026 the MultiGateway authors.

The code written in this project (`MultiProtocol_Gateway/`, `lin/`, the topology folders,
`scripts/` and `setup_macos.sh`) is released under the GNU Lesser General Public License v3.0;
see [`LICENSE`](LICENSE). This matches the license of INET, CoRE4INET and SignalsAndGateways.

The bundled frameworks keep their own licenses:

| Folder | License |
|---|---|
| `FiCo4OMNeT/` (including the CAN-FD extension) | BSD 3-Clause, see `FiCo4OMNeT/License` |
| `CoRE4INET/` | LGPL-3.0, see `CoRE4INET/License` |
| `SignalsAndGateways/` | LGPL-3.0, see `SignalsAndGateways/License` |
