# Requirements

System requirements for building and running CANFD_Omnet.

---

## Required Software

### OMNeT++ Simulation Framework
| Component | Version | Notes |
|-----------|---------|-------|
| **OMNeT++** | 6.0.2+ | [Download](https://omnetpp.org/download/) |

### Build Tools
| Tool | Minimum Version | Check Command |
|------|-----------------|---------------|
| **GCC** | 9.0+ | `gcc --version` |
| **Clang** (alternative) | 10.0+ | `clang --version` |
| **Make** | 4.0+ | `make --version` |

---

## Supported Platforms

| Platform | Status | Notes |
|----------|--------|-------|
| Ubuntu 20.04+ | ✅ Tested | Recommended |
| Ubuntu 22.04 (WSL2) | ✅ Tested | Windows users |
| macOS 12+ | ✅ Supported | Xcode CLT required |
| Windows 11 | ✅ Supported | Via WSL2 or MinGW |

---

## Optional Tools

| Tool | Purpose |
|------|---------|
| **Doxygen** | Generate API documentation (`make doc`) |
| **Graphviz** | Dependency graphs in documentation |

---

## Quick Verification

After installing OMNeT++, verify your setup:

```bash
# Check OMNeT++ is in PATH
which opp_run

# Expected output: /path/to/omnetpp/bin/opp_run

# Check nedtool
which nedtool

# Expected output: /path/to/omnetpp/bin/nedtool
```

If these commands fail, ensure OMNeT++ is properly installed and `setenv` script has been sourced:

```bash
source /path/to/omnetpp/setenv
```
