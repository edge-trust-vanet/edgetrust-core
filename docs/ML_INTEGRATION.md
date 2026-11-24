# EdgeTrust Machine Learning Subsystem Integration

**Author:** Madisetty Mounik Sai ([@mouniksai](https://github.com/mouniksai))  
**Email:** `mouniksai9@gmail.com`  
**Subsystem:** [`edgetrust-ml`](https://github.com/edge-trust-vanet/edgetrust-ml)

---

## Overview

This document outlines how the machine learning pipeline from the `edgetrust-ml` submodule interfaces directly with the C++ discrete-event simulation engine in `edgetrust-core`.

### 1. Leakage-Free 14-Feature Space
The machine learning models deployed in `edgetrust-core` RSUs evaluate an empirical feature vector $\mathbf{x} \in \mathbb{R}^{14}$:
- **Kinematics (5):** `position_x`, `position_y`, `speed`, `direction`, `acceleration`
- **Radio & Network (6):** `packet_sent`, `packet_received`, `packet_drop_ratio`, `latency`, `retransmission_count`, `signal_strength`
- **Dynamic Reputation (3):** `trust_score`, `neighbor_trust_score_avg`, `historical_trust_score`

All retrospective attack counters (`sybil_attack_attempts`, `blackhole_attack_attempts`, `false_packet_injection`) are strictly removed to eliminate target leakage and guarantee real-world generalization on unseen vehicles.

### 2. Multi-Source Harmonized Dataset
- **VeReMi (72.88%, 17,470 samples):** Official Luxembourg (LuST) vehicular traces covering position offset, speed offset, feigned braking, and sudden stop attacks.
- **EdgeTrust-VANET (20.86%, 5,000 samples):** Baseline normal and physical edge telemetry.
- **V-RADD (6.26%, 1,500 samples):** Grayhole selective forwarding and message replay.
- **Total:** 23,970 records partitioned by Vehicle ID via Stratified GroupKFold (zero vehicle overlap between train and test).

### 3. Sub-Microsecond Native C++ Inference
Trained models are exported into zero-dependency C++ header files (`LogisticRegressionPredictor.h`, `RandomForestPredictor.h`, `LightGBMPredictor.h`) and included directly by `EdgeTrustRSUApp.cc`. This achieves wire-speed inference ($0.38\,\mu\text{s}$ for Logistic Regression, $5.00\,\mu\text{s}$ for Decision Trees, $329\,\mu\text{s}$ for LightGBM) without inter-process communication overhead.
