"""
run_multitopo_omnet_suite.py
============================
Executes multi-topology physical OMNeT++ simulations across distinct topologies:
1. SignalsAndGateways smallNetwork (CAN-Ethernet-CAN Gateway)
2. ZonalHybridNetwork (Multi-Zone LIN/CAN/Ethernet)
3. ZonalTopology (Zonal Backbone)
4. RingTopology (Multi-Gateway Ring)

Saves each run to an isolated result-dir: results/run_<id>/
Extracts real physical end-to-end delays and saves to CSV.
"""

import os
import sys
import time
import subprocess
from multiprocessing import Pool, cpu_count
import pandas as pd
from parse_omnet_delays import parse_sca_file

_CLEAN_PROJ = os.environ.get("PROJECT_ROOT", os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
_OMNET_BIN = os.path.join(os.environ.get("OMNETPP_DIR", os.path.expanduser("~/omnetpp-6.0.3")), "bin", "opp_run")

LD_PATHS = [
    os.path.join(os.environ.get("OMNETPP_DIR", os.path.expanduser("~/omnetpp-6.0.3")), "lib"),
    f"{_CLEAN_PROJ}/SignalsAndGateways/out/clang-release/src",
    f"{_CLEAN_PROJ}/FiCo4OMNeT/out/clang-release",
    f"{_CLEAN_PROJ}/inet/out/clang-release/src",
    f"{_CLEAN_PROJ}/CoRE4INET/out/clang-release/src",
    f"{_CLEAN_PROJ}/MultiProtocol_Gateway",
    f"{_CLEAN_PROJ}/lin",
]
ENV = os.environ.copy()
ENV["LD_LIBRARY_PATH"] = ":".join(LD_PATHS) + ":" + ENV.get("LD_LIBRARY_PATH", "")

LIBS = [
    f"{_CLEAN_PROJ}/SignalsAndGateways/out/clang-release/src/libSignalsAndGateways.so",
    f"{_CLEAN_PROJ}/FiCo4OMNeT/out/clang-release/libFiCo4OMNeT.so",
    f"{_CLEAN_PROJ}/inet/out/clang-release/src/libINET.so",
    f"{_CLEAN_PROJ}/CoRE4INET/out/clang-release/src/libCoRE4INET.so",
    f"{_CLEAN_PROJ}/MultiProtocol_Gateway/libMultiProtocol_Gateway.so",
    f"{_CLEAN_PROJ}/lin/libLIN.so",
]

NED_PATHS = [
    f"{_CLEAN_PROJ}/SignalsAndGateways/src",
    f"{_CLEAN_PROJ}/SignalsAndGateways/examples",
    f"{_CLEAN_PROJ}/FiCo4OMNeT/src",
    f"{_CLEAN_PROJ}/CoRE4INET/src",
    f"{_CLEAN_PROJ}/inet/src",
    f"{_CLEAN_PROJ}/MultiProtocol_Gateway",
    f"{_CLEAN_PROJ}/lin",
    f"{_CLEAN_PROJ}/RingTopology/ring_topology",
    f"{_CLEAN_PROJ}/RingTopology/examples",
    f"{_CLEAN_PROJ}/ZonalHybridNetwork/zonal_hybrid",
    f"{_CLEAN_PROJ}/ZonalHybridNetwork/examples",
    f"{_CLEAN_PROJ}/ZonalTopology/zonal_topology",
    f"{_CLEAN_PROJ}/ZonalTopology/examples",
]
NED_ARG = ":".join(NED_PATHS)

NETWORKS = [
    ("smallNetwork",       f"{_CLEAN_PROJ}/SignalsAndGateways/examples/smallNetwork"),
    ("ZonalHybridNetwork", f"{_CLEAN_PROJ}/ZonalHybridNetwork/examples"),
    ("ZonalTopology",      f"{_CLEAN_PROJ}/ZonalTopology/examples"),
    ("RingTopology",       f"{_CLEAN_PROJ}/RingTopology/examples"),
]

DELAYS = ["10us", "20us", "30us", "40us", "50us", "60us", "80us", "100us", "150us", "200us", "300us", "500us"]

def run_single_simulation(task):
    run_id, net_idx, delay_str, sim_time_s = task
    topo_name, net_dir = NETWORKS[net_idx]
    res_subdir = f"results/run_{run_id}"
    res_full_path = os.path.join(net_dir, res_subdir)
    os.makedirs(res_full_path, exist_ok=True)

    cmd = [_OMNET_BIN, "-u", "Cmdenv"]
    for lib in LIBS:
        if os.path.exists(lib):
            cmd.extend(["-l", lib])
    cmd.extend(["-n", NED_ARG])
    cmd.extend([
        f"--sim-time-limit={sim_time_s}s",
        f"--seed-set={run_id}",
        f"--result-dir={res_subdir}",
        "omnetpp.ini"
    ])
    if topo_name == "smallNetwork":
        cmd.append(f"--**.gateway.transformDelay={delay_str}")

    t0 = time.time()
    res = subprocess.run(cmd, cwd=net_dir, env=ENV, capture_output=True, text=True)
    wall_s = time.time() - t0

    # Parse .sca file for real measured delays
    delays_found = []
    if res.returncode == 0:
        for f in os.listdir(res_full_path):
            if f.endswith(".sca"):
                sca_records = parse_sca_file(os.path.join(res_full_path, f))
                for r in sca_records:
                    delays_found.append(r["mean_delay_s"])

    mean_delay = float(pd.Series(delays_found).mean()) if delays_found else None

    return {
        "run_id": run_id,
        "topology": topo_name,
        "gateway_delay": delay_str,
        "sim_time_s": sim_time_s,
        "wallclock_s": wall_s,
        "success": (res.returncode == 0),
        "mean_delay_s": mean_delay,
        "n_delay_stats": len(delays_found),
    }

if __name__ == "__main__":
    n_runs = int(sys.argv[1]) if len(sys.argv) > 1 else 100
    sim_t = float(sys.argv[2]) if len(sys.argv) > 2 else 10.0
    n_workers = min(cpu_count(), 22)

    tasks = []
    for i in range(n_runs):
        net_idx = i % len(NETWORKS)
        d_idx = i % len(DELAYS)
        tasks.append((i, net_idx, DELAYS[d_idx], sim_t))

    print(f"Launching {n_runs} multi-topology physical OMNeT++ runs across {n_workers} CPU cores...")
    t_start = time.time()
    with Pool(n_workers) as pool:
        results = pool.map(run_single_simulation, tasks)
    total_wall = time.time() - t_start

    df = pd.DataFrame(results)
    out_csv = f"{_CLEAN_PROJ}/omnet_multitopo_benchmark_{n_runs}runs.csv"
    df.to_csv(out_csv, index=False)

    n_ok = df["success"].sum()
    print(f"\nExecution Complete in {total_wall:.2f}s ({total_wall/60:.2f} mins):")
    print(f"  Successful Runs: {n_ok}/{n_runs} ({n_ok/n_runs*100:.1f}%)")
    print(f"  Mean per-run wallclock: {df['wallclock_s'].mean():.3f}s")
    print(f"  Valid delay stats captured in {df['n_delay_stats'].gt(0).sum()} runs")
    print(f"  Results saved to: {out_csv}")
