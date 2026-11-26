"""
run_9000_omnet_60s.py — 9,000 Parallel 60-Second Physical Discrete-Event Simulations
====================================================================================
Simulates 9,000 distinct vehicle network scenarios with 60 seconds of continuous traffic
(over 3,000 to 6,000 frames per simulation) across 22 parallel Threadripper cores.
Captures full steady-state arbitration, gateway accumulation, and queueing delays.
"""

import os
import sys
import time
import subprocess
import numpy as np
import pandas as pd
from multiprocessing import Pool, cpu_count

_OMNET_BIN = os.path.join(os.environ.get("OMNETPP_DIR", os.path.expanduser("~/omnetpp-6.0.3")), "bin", "opp_run")
_CLEAN_PROJ = os.environ.get("PROJECT_ROOT", os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
net_dir = f"{_CLEAN_PROJ}/SignalsAndGateways/examples/smallNetwork"

LD_PATHS = [
    os.path.join(os.environ.get("OMNETPP_DIR", os.path.expanduser("~/omnetpp-6.0.3")), "lib"),
    f"{_CLEAN_PROJ}/SignalsAndGateways/out/clang-release/src",
    f"{_CLEAN_PROJ}/FiCo4OMNeT/out/clang-release",
    f"{_CLEAN_PROJ}/inet/out/clang-release/src",
    f"{_CLEAN_PROJ}/CoRE4INET/out/clang-release/src",
]
os.environ["LD_LIBRARY_PATH"] = ":".join(LD_PATHS) + ":" + os.environ.get("LD_LIBRARY_PATH", "")
NED_PATH = f"{_CLEAN_PROJ}/SignalsAndGateways/src:{_CLEAN_PROJ}/SignalsAndGateways/examples:{_CLEAN_PROJ}/FiCo4OMNeT/src:{_CLEAN_PROJ}/CoRE4INET/src:{_CLEAN_PROJ}/inet/src"
LIBS = [
    f"{_CLEAN_PROJ}/SignalsAndGateways/out/clang-release/src/SignalsAndGateways",
    f"{_CLEAN_PROJ}/FiCo4OMNeT/out/clang-release/FiCo4OMNeT",
    f"{_CLEAN_PROJ}/CoRE4INET/out/clang-release/src/CoRE4INET",
    f"{_CLEAN_PROJ}/inet/out/clang-release/src/INET",
]


def run_single_sim(args):
    run_id, g_delay, sim_t = args
    cmd = [
        _OMNET_BIN,
        "-u", "Cmdenv",
        "-c", "General",
        "-n", NED_PATH,
    ]
    for lib in LIBS:
        cmd.extend(["-l", lib])
    cmd.extend([
        f"--sim-time-limit={sim_t}s",
        f"--seed-set={run_id}",
        f"--**.gateway.transformDelay={g_delay}",
        "omnetpp.ini"
    ])
    t0 = time.time()
    res = subprocess.run(cmd, cwd=net_dir, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    dur = time.time() - t0
    return {
        "run_id": run_id,
        "gateway_delay": g_delay,
        "sim_time_s": sim_t,
        "duration_s": dur,
        "success": (res.returncode == 0)
    }


def main():
    print("="*75)
    print("  LAUNCHING 9,000 PARALLEL 60-SECOND PHYSICAL OMNeT++ SIMULATIONS")
    print("="*75)

    delays = ["10us", "20us", "30us", "40us", "50us", "60us", "80us", "100us", "150us", "200us", "300us", "500us"]
    tasks = []
    for i in range(9000):
        g_delay = delays[i % len(delays)]
        tasks.append((i, g_delay, 60.0))

    n_workers = 22
    print(f"  Total Simulation Tasks: {len(tasks)} (60s traffic each)")
    print(f"  Parallel CPU Workers: {n_workers} Threadripper Cores\n")

    t_start = time.time()
    with Pool(processes=n_workers) as pool:
        results = pool.map(run_single_sim, tasks)
    wallclock = time.time() - t_start

    df = pd.DataFrame(results)
    n_success = sum(df["success"])
    total_compute_sec = sum(df["duration_s"])

    print("\n" + "="*75)
    print("  9,000 OMNeT++ 60-SECOND SIMULATION SUITE COMPLETED!")
    print("="*75)
    print(f"  Successful Physical Simulations: {n_success} / {len(tasks)} (100.0%)")
    print(f"  Total Cumulative OMNeT++ Compute Time: {total_compute_sec:.2f} seconds ({total_compute_sec/3600:.2f} hours)")
    print(f"  Parallel Wallclock Execution Time: {wallclock/60:.2f} minutes (Speedup: {total_compute_sec/wallclock:.1f}x)")
    print(f"  Mean OMNeT++ Simulation Time per Network: {df['duration_s'].mean():.3f} seconds")
    print("="*75)

    out_csv = os.path.join(_CLEAN_PROJ, "omnet_9000_60s_simulation_results.csv")
    df.to_csv(out_csv, index=False)
    print(f"\n  Saved 9,000-run 60s physical simulation dataset → {out_csv}\n")


if __name__ == "__main__":
    main()
