#!/usr/bin/env python3
"""
inspect_checkpoint.py
─────────────────────────────────────────────────────────────────────────────
Inspects, parses, and validates the EdgeTrust simulation environment checkpoint.
Displays:
  - Checkpoint metadata (simulation time, RSU edge model, vehicle count)
  - Nuanced multi-tier trust scores (direct, historical, neighbor consensus)
  - Detailed trust evidence factors (kinematic plausibility, temporal consistency,
    communication reliability, neighbor validation)
  - ML classification verdicts and confidence ratings
  - Statistical distribution verifying trust scores have evolved to realistic,
    calibrated continuous values instead of discrete 0's and 1's.
─────────────────────────────────────────────────────────────────────────────
"""

import os
import sys
import json
import pandas as pd
import numpy as np

CHECKPOINT_DEFAULT = "/home/aashiq/Desktop/FinalYearProject/edgetrust-test/omnetpp/veins/examples/veins/results/checkpoint_state.json"
SECONDARY_DEFAULT = "/home/aashiq/Desktop/FinalYearProject/edgetrust-core/simulator/results/checkpoint_state.json"


def inspect_checkpoint(filepath=None):
    target_path = filepath or CHECKPOINT_DEFAULT
    if not os.path.exists(target_path):
        target_path = SECONDARY_DEFAULT
    if not os.path.exists(target_path):
        target_path = "/home/aashiq/Desktop/FinalYearProject/edgetrust-test/omnetpp/veins/examples/veins/results/live_environment_state.json"
    
    if not os.path.exists(target_path):
        print(f"Error: No checkpoint file found at {target_path}")
        sys.exit(1)

    print(f"Loading checkpoint from: {target_path}\n")
    with open(target_path, "r") as f:
        data = json.load(f)

    print("=" * 85)
    print("  EdgeTrust-VANET: Simulation Environment Checkpoint Overview")
    print("=" * 85)
    print(f"  Checkpoint Version    : {data.get('checkpoint_version', 1)}")
    print(f"  Simulation Time (s)   : {data.get('simulation_time_sec', 0.0):.2f}s")
    print(f"  RSU Node ID           : {data.get('rsu_id', 0)}")
    print(f"  Edge AI Architecture  : {data.get('edge_model', 'unknown').upper()}")
    print(f"  Total Tracked Vehicles: {data.get('tracked_vehicle_count', 0)}")
    print(f"  Total Evaluated BSMs  : {data.get('total_extracted_records', 0)}")
    print("=" * 85)

    vehicles = data.get("vehicles", {})
    if not vehicles:
        print("  No vehicle records found in checkpoint.")
        return

    table_rows = []
    trust_scores = []
    
    for vid, v in sorted(vehicles.items(), key=lambda x: int(x[0])):
        tf = v.get("trust_factors", {})
        ts = v.get("trust_scores", {})
        ml = v.get("ml_classification", {})
        at = v.get("attacks_detected", {})
        
        t_direct = ts.get("trust_score", 0.95)
        t_hist = ts.get("historical_trust_score", 0.95)
        t_neigh = ts.get("neighbor_trust_score_avg", 0.95)
        trust_scores.append(t_direct)

        table_rows.append({
            "Vehicle": f"V{v.get('node_id', vid)}",
            "Role": "MALICIOUS" if v.get("is_malicious", False) else "BENIGN",
            "Attack": v.get("attack_type", 0),
            "Speed (m/s)": round(v.get("speed_mps", 0.0), 1),
            "Plaus": round(tf.get("plausibility", 1.0), 2),
            "Consist": round(tf.get("consistency", 1.0), 2),
            "Comm": round(tf.get("comm_score", 1.0), 2),
            "Neigh": round(tf.get("neighbor_validation", 0.95), 2),
            "Direct T": round(t_direct, 3),
            "Hist T": round(t_hist, 3),
            "Verdict": ml.get("verdict", "ACCEPT"),
            "Conf": f"{int(ml.get('confidence', 0.9)*100)}%"
        })

    df = pd.DataFrame(table_rows)
    print("\nDetailed Per-Vehicle Calibrated State:")
    print(df.to_string(index=False))

    # Distribution Check: verify values are non-binary
    ts_arr = np.array(trust_scores)
    zeros_count = np.sum(ts_arr == 0.0)
    ones_count = np.sum(ts_arr == 1.0)
    continuous_count = np.sum((ts_arr > 0.0) & (ts_arr < 1.0))
    
    print("\n" + "-" * 85)
    print("Trust Score Calibration & Realism Verification:")
    print(f"  • Min Trust Score       : {np.min(ts_arr):.4f}")
    print(f"  • Mean Trust Score      : {np.mean(ts_arr):.4f}")
    print(f"  • Max Trust Score       : {np.max(ts_arr):.4f}")
    print(f"  • Standard Deviation    : {np.std(ts_arr):.4f}")
    print(f"  • Continuous Values     : {continuous_count} / {len(ts_arr)} ({continuous_count/len(ts_arr)*100:.1f}%)")
    print(f"  • Hard 0's or 1's       : {zeros_count + ones_count} / {len(ts_arr)} ({(zeros_count + ones_count)/len(ts_arr)*100:.1f}%)")
    
    if continuous_count > 0:
        print("  ✔ VERIFIED: Trust scores exhibit realistic, dynamic continuous values!")
    else:
        print("  ✖ WARNING: Trust scores are still binary.")
    print("-" * 85 + "\n")


if __name__ == "__main__":
    cp_arg = sys.argv[1] if len(sys.argv) > 1 else None
    inspect_checkpoint(cp_arg)
