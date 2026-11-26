# CANFD_Omnet

**Protocol-Accurate CAN/CAN-FD Simulation Framework for OMNeT++**

A high-fidelity, ISO 11898-1:2015 compliant simulation model for Controller Area Network (CAN) and CAN with Flexible Data-rate (CAN-FD) protocols. Built on top of the FiCo4OMNeT framework for the [OMNeT++](https://omnetpp.org/) discrete event simulator.

---

## ✨ Features

### CAN-FD Protocol Support (ISO 11898-1:2015)

| Feature | Status | Description |
|---------|--------|-------------|
| **Dual-Bitrate Operation** | ✅ | Arbitration phase at nominal rate, data phase at higher rate |
| **64-Byte Payloads** | ✅ | Full support for extended payloads (vs 8-byte Classic CAN) |
| **BRS Flag** | ✅ | Bit Rate Switch enables data phase acceleration |
| **ESI Flag** | ✅ | Error State Indicator reflects node health |
| **ISO CRC Engine** | ✅ | CRC-15, CRC-17, and CRC-21 polynomial support |
| **DLC Encoding** | ✅ | Non-linear mapping (9→12, 13→32, 15→64 bytes) |
| **Bit-Stuffing** | ✅ | Timing-accurate overhead calculation |
| **Fault Confinement** | ✅ | TEC/REC counters, Error Active/Passive/Bus-Off states |

### Additional Capabilities
- **Mixed CAN + CAN-FD Networks**: Simulate heterogeneous bus environments
- **Priority-Based Arbitration**: Lower ID = Higher Priority (bitwise arbitration)
- **Visual Animation**: Real-time frame visualization in OMNeT++ Qtenv
- **Statistics Collection**: Built-in signals for throughput, latency, error rates

---

## 🚀 Quick Start

### Prerequisites
See [REQUIREMENTS.md](REQUIREMENTS.md) for detailed system requirements.

**Minimum:**
- OMNeT++ 6.0.2 or later
- C++17 compatible compiler (GCC 9+, Clang 10+)
- Linux (Ubuntu 20.04+), macOS, or Windows with WSL2

### Installation

```bash
# Clone the repository
# FiCo4OMNeT (with the CAN-FD extension) is included in this repository
cd FiCo4OMNeT

# Generate makefiles
make makefiles

# Build (debug mode)
make

# Or release mode
make MODE=release
```

### Run an Example

```bash
cd examples/can/basic
./run
```

Or use the OMNeT++ IDE:
1. Import project: `File → Import → Existing Projects`
2. Select the `FiCo4OMNeT` directory
3. Build: `Ctrl+B`
4. Run: Right-click any `.ini` file → `Run As → OMNeT++ Simulation`

---

## 📁 Project Structure

```
CANFD_Omnet/
├── src/fico4omnet/           # Core simulation modules
│   ├── linklayer/can/        # CAN/CAN-FD link layer (PortInput, PortOutput)
│   ├── bus/can/              # Bus logic and arbitration
│   ├── buffer/can/           # TX/RX buffers
│   ├── applications/can/     # Traffic generators, sinks, gateways
│   └── utilities/            # CRC engine, DLC mapping, helpers
├── examples/                 # Ready-to-run simulation scenarios
│   ├── can/                  # Classic CAN examples
│   └── heterogeneous/        # Mixed CAN + CAN-FD examples
├── doc/                      # Documentation
└── tests/                    # Test cases
```

---

## 📖 Documentation

- [INSTALL](INSTALL) - Detailed build instructions
- [REQUIREMENTS.md](REQUIREMENTS.md) - System requirements
- [doc/](doc/) - API documentation (run `make doc` to generate)

---

## 🔬 Technical Details

### CRC Implementation
The framework implements all three ISO 11898-1 CRC polynomials:

| CRC Type | Polynomial | Usage |
|----------|------------|-------|
| CRC-15 | `0x4599` | Classic CAN |
| CRC-17 | `0x3685B` | CAN-FD (≤16 bytes) |
| CRC-21 | `0x302899` | CAN-FD (>16 bytes) |

### Timing Model
- **Nominal Bitrate**: Configurable (typically 500 kbps)
- **Data Bitrate**: Configurable (typically 2-8 Mbps)
- **Bit-Stuffing**: Dynamic calculation per frame for accurate timing

---

## 📚 Acknowledgments & Citation

This project is based on [CoRE-RG/FiCo4OMNeT](https://github.com/CoRE-RG/FiCo4OMNeT), enhanced with ISO 11898-1:2015 compliant CAN-FD features.

If you use this framework in your research, please cite the original work:

```bibtex
@incollection{meyer2019simulation,
  title={Simulation of Mixed Critical In-vehicular Networks},
  author={Meyer, Philipp and Korf, Franz and Steinbach, Till and Schmidt, Thomas C.},
  booktitle={Recent Advances in Network Simulation},
  year={2019},
  publisher={Springer}
}
```

**Original Repository:** [github.com/CoRE-RG/FiCo4OMNeT](https://github.com/CoRE-RG/FiCo4OMNeT)

---

## 📄 License

This project is licensed under the LGPL License - see the [License](License) file for details.

---

## ⚠️ Disclaimer

This simulation model is under continuous development. While we strive for accuracy, **you are responsible for validating that the models work correctly for your specific use case**.
