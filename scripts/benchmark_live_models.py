#!/usr/bin/env python3
"""
benchmark_live_models.py
─────────────────────────────────────────────────────────────────────────────
Comprehensive Edge ML Benchmark on Live VANET Simulation Environment.
Evaluates all 17 trained models from edgetrust-ml/models/ on live telemetry
extracted by RSUs in OMNeT++ / Veins simulation.

Calculates:
  - Accuracy, Precision (Macro/Weighted), Recall (Macro/Weighted)
  - F1-Score (Macro & Weighted), ROC-AUC, PR-AUC
  - False Alarm Rate (FAR), Missed Attack Rate (MAR)
  - Confusion Matrix (TP, TN, FP, FN)
  - Microsecond Edge Inference Latency (µs/sample) & Throughput (samples/s)
  - Model Storage Footprint (KB)
  - Training vs. Live Environment Generalization Comparison
─────────────────────────────────────────────────────────────────────────────
"""

import os
import sys
import time
import json
import joblib
import numpy as np
import pandas as pd
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

from sklearn.metrics import (
    accuracy_score, f1_score, precision_score, recall_score,
    roc_auc_score, average_precision_score, confusion_matrix
)

# Directories
CORE_DIR = "/home/aashiq/Desktop/FinalYearProject/edgetrust-core"
ML_DIR = "/home/aashiq/Desktop/FinalYearProject/edgetrust-ml"
MODELS_DIR = os.path.join(ML_DIR, "models")
RESULTS_DIR_CORE = os.path.join(CORE_DIR, "simulator", "results")
RESULTS_DIR_ML = os.path.join(ML_DIR, "results")
os.makedirs(RESULTS_DIR_CORE, exist_ok=True)
os.makedirs(RESULTS_DIR_ML, exist_ok=True)

LIVE_DATA_FILE = os.path.join(RESULTS_DIR_CORE, "live_simulation_benchmark_dataset.csv")
TRAIN_METRICS_FILE = os.path.join(RESULTS_DIR_ML, "unified_model_metrics.json")

FEATURES_14 = [
    'position_x', 'position_y', 'speed', 'direction', 'acceleration',
    'packet_sent', 'packet_received', 'packet_drop_ratio', 'latency',
    'message_retransmission_count', 'signal_strength',
    'trust_score', 'neighbor_trust_score_avg', 'historical_trust_score'
]
TARGET = 'is_malicious'

# All 17 Model Names & Filenames
MODELS_CATALOG = [
    ("Random Forest", "unified_Random_Forest.pkl"),
    ("LightGBM", "unified_LightGBM.pkl"),
    ("XGBoost", "unified_XGBoost.pkl"),
    ("CatBoost", "unified_CatBoost.pkl"),
    ("Extra Trees", "unified_Extra_Trees.pkl"),
    ("Hist Gradient Boosting", "unified_Hist_Gradient_Boosting.pkl"),
    ("Gradient Boosting", "unified_Gradient_Boosting.pkl"),
    ("AdaBoost", "unified_AdaBoost.pkl"),
    ("Decision Tree", "unified_Decision_Tree.pkl"),
    ("Stacking Ensemble", "unified_Stacking_Ensemble.pkl"),
    ("Voting Ensemble", "unified_Voting_Ensemble.pkl"),
    ("MLP Neural Network", "unified_MLP_Neural_Network.pkl"),
    ("SVM (RBF)", "unified_SVM_RBF.pkl"),
    ("K-Nearest Neighbors", "unified_K-Nearest_Neighbors.pkl"),
    ("Logistic Regression", "unified_Logistic_Regression.pkl"),
    ("Gaussian Naive Bayes", "unified_Gaussian_Naive_Bayes.pkl"),
    ("Linear Discriminant Analysis", "unified_Linear_Discriminant_Analysis.pkl"),
]


def load_live_data():
    if not os.path.exists(LIVE_DATA_FILE):
        raise FileNotFoundError(f"Live dataset not found at {LIVE_DATA_FILE}. Run collect_live_simulation_data.py first.")
    
    df = pd.read_csv(LIVE_DATA_FILE)
    print(f"Loaded Live Simulation Telemetry: {len(df)} samples")
    print(f"  - Malicious (Attacks): {sum(df[TARGET] == 1)} ({sum(df[TARGET] == 1)/len(df)*100:.1f}%)")
    print(f"  - Benign (Normal):     {sum(df[TARGET] == 0)} ({sum(df[TARGET] == 0)/len(df)*100:.1f}%)")
    
    # Handle column aliases if present
    if 'retransmission_count' in df.columns and 'message_retransmission_count' not in df.columns:
        df['message_retransmission_count'] = df['retransmission_count']
    elif 'message_retransmission_count' in df.columns and 'retransmission_count' not in df.columns:
        df['retransmission_count'] = df['message_retransmission_count']

    X_raw = df[FEATURES_14].values
    y_true = df[TARGET].values.astype(int)
    return df, X_raw, y_true


def measure_inference_latency(model, X_scaled):
    """Measures precise edge inference latency in microseconds per sample."""
    # Warmup
    _ = model.predict(X_scaled[:min(50, len(X_scaled))])
    
    latencies = []
    # Benchmark 200 single-sample inferences to simulate streaming RSU packet arrivals
    indices = np.random.choice(len(X_scaled), size=min(200, len(X_scaled)), replace=False)
    
    for idx in indices:
        sample = X_scaled[idx:idx+1]
        t0 = time.perf_counter_ns()
        _ = model.predict(sample)
        t1 = time.perf_counter_ns()
        latencies.append((t1 - t0) / 1000.0) # convert to microseconds
    
    mean_lat_us = float(np.median(latencies))
    return round(mean_lat_us, 2)


def main():
    print("=" * 80)
    print("  EdgeTrust-VANET: Live Simulation Edge ML Benchmark & Model Comparison")
    print("=" * 80)
    
    df, X_raw, y_true = load_live_data()
    
    scaler_path = os.path.join(MODELS_DIR, "unified_scaler.pkl")
    if not os.path.exists(scaler_path):
        raise FileNotFoundError(f"Scaler missing at {scaler_path}")
    scaler = joblib.load(scaler_path)
    X_scaled = scaler.transform(X_raw)
    
    # Load training metrics for comparison if available
    train_metrics = {}
    if os.path.exists(TRAIN_METRICS_FILE):
        try:
            with open(TRAIN_METRICS_FILE, "r") as f:
                raw_tm = json.load(f)
                train_metrics = raw_tm.get("models", {})
        except Exception as e:
            print(f"Warning: Could not load training metrics: {e}")

    live_results = {}
    comparison_rows = []

    for name, filename in MODELS_CATALOG:
        filepath = os.path.join(MODELS_DIR, filename)
        if not os.path.exists(filepath):
            print(f"  [MISSING] {name} ({filename})")
            continue
        
        try:
            model = joblib.load(filepath)
            size_kb = round(os.path.getsize(filepath) / 1024.0, 1)
            
            # Predict
            y_pred = model.predict(X_scaled)
            
            # Probability / ROC-AUC
            y_proba = None
            if hasattr(model, "predict_proba"):
                try:
                    y_proba = model.predict_proba(X_scaled)[:, 1]
                except Exception:
                    pass
            elif hasattr(model, "decision_function"):
                try:
                    dfunc = model.decision_function(X_scaled)
                    y_proba = 1.0 / (1.0 + np.exp(-dfunc))
                except Exception:
                    pass

            acc = accuracy_score(y_true, y_pred) * 100.0
            f1_w = f1_score(y_true, y_pred, average='weighted', zero_division=0) * 100.0
            f1_m = f1_score(y_true, y_pred, average='macro', zero_division=0) * 100.0
            prec_w = precision_score(y_true, y_pred, average='weighted', zero_division=0) * 100.0
            rec_w = recall_score(y_true, y_pred, average='weighted', zero_division=0) * 100.0
            
            if y_proba is not None:
                roc_auc = roc_auc_score(y_true, y_proba) * 100.0
                pr_auc = average_precision_score(y_true, y_proba) * 100.0
            else:
                roc_auc = acc
                pr_auc = acc

            cm = confusion_matrix(y_true, y_pred)
            tn, fp, fn, tp = cm.ravel()
            
            far = (fp / (fp + tn) * 100.0) if (fp + tn) > 0 else 0.0
            mar = (fn / (fn + tp) * 100.0) if (fn + tp) > 0 else 0.0
            
            # Microsecond edge inference latency
            lat_us = measure_inference_latency(model, X_scaled)
            throughput = int(1_000_000.0 / max(1.0, lat_us))
            
            # Comparison with train metrics
            tm = train_metrics.get(name, {})
            train_acc = tm.get("accuracy", None)
            train_f1 = tm.get("f1_score", None)
            train_lat = tm.get("latency_us", None)
            
            live_results[name] = {
                "accuracy": round(acc, 2),
                "f1_weighted": round(f1_w, 2),
                "f1_macro": round(f1_m, 2),
                "precision": round(prec_w, 2),
                "recall": round(rec_w, 2),
                "roc_auc": round(roc_auc, 2),
                "pr_auc": round(pr_auc, 2),
                "false_alarm_rate": round(far, 2),
                "missed_attack_rate": round(mar, 2),
                "confusion_matrix": cm.tolist(),
                "edge_latency_us": lat_us,
                "throughput_samples_sec": throughput,
                "model_size_kb": size_kb,
                "train_accuracy": train_acc,
                "train_f1_score": train_f1,
                "train_latency_us": train_lat
            }
            
            comparison_rows.append({
                "Model": name,
                "Live Acc (%)": round(acc, 2),
                "Live F1 (%)": round(f1_w, 2),
                "Live ROC-AUC (%)": round(roc_auc, 2),
                "FAR (%)": round(far, 2),
                "MAR (%)": round(mar, 2),
                "Latency (µs)": lat_us,
                "Throughput (p/s)": throughput,
                "Size (KB)": size_kb,
                "Train Acc (%)": train_acc,
                "Train F1 (%)": train_f1
            })
            
            print(f"  ✔ {name:28} | Live Acc: {acc:6.2f}% | F1: {f1_w:6.2f}% | Latency: {lat_us:7.1f} µs | FAR: {far:5.2f}% | MAR: {mar:5.2f}%")
            
        except Exception as e:
            print(f"  ✖ {name:28} -> Error: {e}")

    # Convert to DataFrame and sort by Live F1 score
    res_df = pd.DataFrame(comparison_rows).sort_values(by="Live F1 (%)", ascending=False).reset_index(drop=True)
    
    # Save CSV and JSON
    res_df.to_csv(os.path.join(RESULTS_DIR_CORE, "live_model_comparison_metrics.csv"), index=False)
    res_df.to_csv(os.path.join(RESULTS_DIR_ML, "live_model_comparison_metrics.csv"), index=False)
    
    with open(os.path.join(RESULTS_DIR_CORE, "live_model_comparison_metrics.json"), "w") as f:
        json.dump(live_results, f, indent=2)
    with open(os.path.join(RESULTS_DIR_ML, "live_model_comparison_metrics.json"), "w") as f:
        json.dump(live_results, f, indent=2)
        
    print("\n" + "=" * 80)
    print("  Summary Table: Live Simulation Environment Model Performance")
    print("=" * 80)
    print(res_df.to_string(index=False))

    # Generate Publication Comparison Plot
    plot_comparison(res_df)


def plot_comparison(df):
    top_models = df.head(10).copy()
    
    fig, axes = plt.subplots(1, 2, figsize=(15, 6))
    
    # Accuracy vs F1 in Live Environment
    x = np.arange(len(top_models))
    width = 0.35
    axes[0].bar(x - width/2, top_models["Live Acc (%)"], width, label="Live Accuracy (%)", color="#2b5c8f")
    axes[0].bar(x + width/2, top_models["Live F1 (%)"], width, label="Live F1 Score (%)", color="#2ca02c")
    axes[0].set_ylabel("Score (%)")
    axes[0].set_title("Live Simulation: Accuracy & F1 Score (Top 10 Models)")
    axes[0].set_xticks(x)
    axes[0].set_xticklabels(top_models["Model"], rotation=40, ha="right")
    axes[0].set_ylim(80, 102)
    axes[0].grid(axis="y", linestyle="--", alpha=0.5)
    axes[0].legend()
    
    # Edge Inference Latency (µs)
    axes[1].bar(top_models["Model"], top_models["Latency (µs)"], color="#d95f02")
    axes[1].set_ylabel("Inference Latency (µs)")
    axes[1].set_title("Edge Inference Latency (Lower is Better)")
    axes[1].set_xticklabels(top_models["Model"], rotation=40, ha="right")
    axes[1].set_yscale("log")
    axes[1].grid(axis="y", linestyle="--", alpha=0.5)
    
    plt.tight_layout()
    chart_path_core = os.path.join(RESULTS_DIR_CORE, "live_model_comparison.png")
    chart_path_ml = os.path.join(RESULTS_DIR_ML, "live_model_comparison.png")
    plt.savefig(chart_path_core, dpi=300)
    plt.savefig(chart_path_ml, dpi=300)
    plt.close()
    print(f"\nSaved comparison chart to {chart_path_core} and {chart_path_ml}")


if __name__ == "__main__":
    main()
