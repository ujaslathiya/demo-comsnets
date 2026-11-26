import os
import re
import glob
import pandas as pd

def parse_sca_file(sca_path):
    """
    Parses an OMNeT++ .sca scalar file and extracts all valid end-to-end delay/latency statistics.
    Robustly handles nan, -nan, '-', and missing values.
    """
    records = []
    cur_module = None
    cur_stat = None
    
    with open(sca_path, 'r', encoding='utf-8', errors='ignore') as f:
        for line in f:
            line_str = line.strip()
            if not line_str:
                continue
            
            # Format 1: statistic <module> <stat_name>
            m_stat = re.match(r"^statistic\s+(\S+)\s+(\S+)", line_str)
            if m_stat:
                cur_module, cur_stat = m_stat.group(1), m_stat.group(2)
                continue
                
            if cur_module and cur_stat and line_str.startswith("field mean"):
                parts = line_str.split()
                if len(parts) >= 3:
                    val_str = parts[-1]
                    if val_str not in ("nan", "-nan", "-", "none", ""):
                        try:
                            fval = float(val_str)
                            records.append({
                                "module": cur_module,
                                "stat_name": cur_stat,
                                "mean_delay_s": fval
                            })
                        except ValueError:
                            pass
                cur_module, cur_stat = None, None
                continue

            # Format 2: scalar <module> <stat_name> <value>
            if line_str.startswith("scalar "):
                parts = line_str.split()
                if len(parts) >= 4:
                    mod, st, val = parts[1], parts[2], parts[3]
                    if any(k in st.lower() for k in ['delay', 'latency']):
                        if val not in ("nan", "-nan", "-", "none", ""):
                            try:
                                fval = float(val)
                                # Normalize microseconds if stat name indicates us
                                if 'us' in st.lower() or fval > 10.0:
                                    fval = fval * 1e-6
                                records.append({
                                    "module": mod,
                                    "stat_name": st,
                                    "mean_delay_s": fval
                                })
                            except ValueError:
                                pass
                    
    return records


def parse_all_simulation_results(search_pattern="**/results/**/*.sca"):
    all_rows = []
    files = glob.glob(search_pattern, recursive=True)
    for fpath in files:
        run_id = os.path.basename(os.path.dirname(fpath)).replace("run_", "")
        parts = fpath.split("/")
        topo_name = parts[parts.index("Clean_Project") + 1] if "Clean_Project" in parts else "unknown"
        parsed = parse_sca_file(fpath)
        for r in parsed:
            r["run_id"] = run_id
            r["topology"] = topo_name
            r["file_path"] = fpath
            all_rows.append(r)
            
    df = pd.DataFrame(all_rows)
    return df

if __name__ == "__main__":
    import sys
    pattern = sys.argv[1] if len(sys.argv) > 1 else "**/results/**/*.sca"
    df = parse_all_simulation_results(pattern)
    print(f"Parsed {len(df)} scalar delay records.")
    if len(df) > 0:
        print(df.head(10))
        out_csv = "parsed_omnet_delays.csv"
        df.to_csv(out_csv, index=False)
        print(f"Saved parsed records to: {out_csv}")
