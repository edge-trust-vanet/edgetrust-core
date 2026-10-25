# EdgeTrust-VANET Core Simulation and Edge Engine

**Decentralized Four-Tier Hierarchical Trust Management and Kinematic Filtering Framework for Vehicular Ad-Hoc Networks**

[![Build Status](https://img.shields.io/badge/Build-Passing-brightgreen.svg)]()
[![C++ Standard](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![Network Simulator](https://img.shields.io/badge/OMNeT%2B%2B-6.4.0-orange.svg)](https://omnetpp.org/)
[![Mobility Simulator](https://img.shields.io/badge/SUMO-1.20%2B-yellow.svg)](https://eclipse.dev/sumo/)
[![Vehicular Framework](https://img.shields.io/badge/Veins-5.3.1-red.svg)](https://veins.car2x.org/)
[![Wireless Standard](https://img.shields.io/badge/IEEE-802.11p%20%2F%201609.2-lightgrey.svg)](https://standards.ieee.org/)
[![License](https://img.shields.io/badge/License-Academic%20Research-blue.svg)]()

---

## Executive Overview

Vehicular Ad-hoc Networks (VANETs) serve as the communication foundation for Cooperative Intelligent Transportation Systems (C-ITS), facilitating time-critical safety applications such as Forward Collision Warning (FCW), Emergency Electronic Brake Lights (EEBL), and Blind Spot Assistance over Dedicated Short-Range Communications (DSRC) and IEEE 802.11p protocols.

Existing vehicular security mechanisms rely almost exclusively on Public Key Infrastructure (PKI) and Elliptic Curve Digital Signature Algorithms (ECDSA) standardized under IEEE 1609.2 and ETSI TS 103 097. While cryptographic authentication guarantees certificate authenticity and message non-repudiation, it exhibits a fundamental semantic flaw: **it verifies the identity of the sender, not the physical truthfulness of the transmitted telemetry**. An authenticated vehicle with compromised on-board sensors, tampered firmware, or an adversarial insider driver can broadcast cryptographically valid signatures over completely fabricated kinematic states (e.g., falsified coordinates, fabricated emergency braking, ghost vehicle Sybil attacks, and channel denial of service).

`edgetrust-core` contains the reference C++ simulation engine, edge intelligence runtime, and four-tier architectural implementation of **EdgeTrust-VANET**. The system integrates client-side kinematic heuristic firewalls, sub-microsecond edge machine learning inference directly inside Roadside Units (RSUs), observation-weighted Beta-Bayesian consensus across Regional Fog Nodes, and trust-penalized Dijkstra routing within a centralized Software-Defined Networking (SDN) controller.

---

## System Architecture

EdgeTrust-VANET partitions security evaluation across four discrete functional tiers, eliminating single points of failure, preventing broadcast storms on the 802.11p Control Channel (CCH), and guaranteeing end-to-end decision latencies well below the 100 ms C-ITS deadline.

```
+===================================================================================+
|                                    TIER 4                                         |
|                   CENTRALIZED SOFTWARE-DEFINED NETWORKING (SDN)                  |
|  - Global Topology Graph Maintenance & Flow Rule Enforcement                     |
|  - Trust-Penalized Multi-Hop Dijkstra Shortest Path Routing                      |
|  - Network-Wide Revocation Broadcast for Confirmed Attackers (T < 0.40)          |
+===================================================================================+
                                         ^
                                         | Secure Optical Backhaul (OpenFlow)
                                         v
+===================================================================================+
|                                    TIER 3                                         |
|                        REGIONAL FOG COMPUTING LEDGER                             |
|  - Cross-RSU Handover Tracking & Boundary Discrepancy Resolution (|delta| > 0.20)|
|  - Observation-Weighted Beta-Bayesian Reputation Consensus                       |
|  - Deterministic Safety Overrides & Regional Vehicle Dossiers                    |
+===================================================================================+
                                         ^
                                         | Low-Latency I2I Fiber / 5G Xn Interface
                                         v
+===================================================================================+
|                                    TIER 2                                         |
|                    EDGE ROADSIDE UNIT (RSU) TRUST ENGINE                          |
|  - 14-Dimensional Spatiotemporal Feature Extraction (x in R^14)                  |
|  - Native C++ Machine Learning Inference (0.38 us LR, 5.00 us AdaBoost)          |
|  - Exponential Moving Average (EMA) Trust Update (alpha = 0.70)                  |
|  - 3-Class Priority QoS Queue & Decision Fusion (ACCEPT / WARN / QUARANTINE)     |
+===================================================================================+
                                         ^
                                         | Dedicated Short-Range Communications (DSRC)
                                         | IEEE 802.11p / 1609.4 Control Channel (CCH)
                                         v
+===================================================================================+
|                                    TIER 1                                         |
|                    ON-BOARD UNIT (OBU) KINEMATIC FIREWALL                         |
|  - Client-Side Kinematic Plausibility Checks (v <= 45 m/s, |a| <= 8 m/s^2)        |
|  - Dead Reckoning Telemetry Consistency & Heading Drift Verification              |
|  - Local Beacon Filtering (Drops 34.2% Malicious Frames Before Multi-Hop Retx)   |
+===================================================================================+
```

### Architectural Tier Specifications

#### Tier 1: On-Board Unit (OBU) Kinematic Firewall
Executed locally on receiving vehicle OBUs within the Veins application layer (`EdgeTrustVehicleApp.cc`). Incoming Basic Safety Messages (BSMs) are subjected to instantaneous kinematic sanity checks prior to cryptographic verification or multi-hop retransmission:
- **Velocity Sanity:** $v_{\text{msg}} \le v_{\text{max}} = 45.0\text{ m/s}$ (162 km/h).
- **Acceleration Envelope:** $|a_{\text{msg}}| \le a_{\text{max}} = 8.0\text{ m/s}^2$.
- **Kinematic Dead Reckoning Consistency:**
  $$\Delta d = \|\mathbf{p}_{\text{msg}}(t) - \mathbf{p}_{\text{est}}(t)\| \le v \Delta t + \frac{1}{2} a (\Delta t)^2 + \epsilon_{\text{GPS}}$$
- **Heading Continuity:** Angular deviation $|\theta(t) - \theta(t - \Delta t)| \le \omega_{\text{max}} \Delta t$.
- **Impact:** Eliminates 34.2% of raw malicious packets locally at the receiver, mitigating broadcast storm collapse and preventing control channel saturation.

#### Tier 2: Edge Roadside Unit (RSU) Trust Engine & C++ ML Predictor
Implemented in `EdgeTrustRSUApp.cc`. When vehicles enter an RSU communication zone (300 m radius), the RSU monitors BSM streams and extracts a normalized 14-dimensional feature vector $\mathbf{x} \in \mathbb{R}^{14}$.
- **Hardware Integration:** Employs natively compiled, zero-dependency C++ inference engines (`LogisticRegressionPredictor.h`, `RandomForestPredictor.h`, `LightGBMPredictor.h`) without inter-process communication (IPC) or Python socket overhead.
- **Inference Latency:** $0.38\,\mu\text{s}$ per message for Linear/Logistic Classifiers; $5.00\,\mu\text{s}$ for AdaBoost; $329\,\mu\text{s}$ for LightGBM.
- **Total RSU Decision Cycle:** $0.38\text{ ms}$, operating well within the 100 ms vehicular safety threshold.
- **Priority QoS Queuing:** Segregates incoming packets into a 3-class priority queue (Safety-Critical BSMs, Verification Diagnostics, Low-Priority Telemetry) to ensure deterministic scheduling.

#### Tier 3: Regional Fog Computing Ledger
Implemented in `EdgeTrustFogApp.cc`. Regional Fog nodes aggregate telemetry reports and trust evaluations from adjacent RSUs:
- **Handover Continuity:** Maintains persistent cross-RSU tracking across road segment boundaries.
- **Discrepancy Resolution:** Triggers when the absolute trust score variance across neighboring RSUs exceeds $\epsilon_{\text{tol}} = 0.20$.
- **Beta-Bayesian Consensus:** Weighs individual RSU reports by the number of independent physical observations $N_{\text{obs}}$:
  $$T_{\text{fog}} = \frac{\sum_{k=1}^{M} N_{k} \cdot T_{\text{RSU}, k}}{\sum_{k=1}^{M} N_{k}}$$
- **Safety Override:** If any RSU detects critical kinematic violations (e.g., sudden deceleration illusions or collision trajectory spoofing) with confidence $> 0.90$, the Fog node immediately forces a global quarantine.

#### Tier 4: Centralized Software-Defined Networking (SDN) Controller
Implemented in `EdgeTrustSDNController.cc`. Operates over the regional transport graph $G = (V, E)$ where vertices represent RSUs and edges represent vehicular multi-hop forwarding paths:
- **Dynamic Topology Graphing:** Ingests live link delivery ratios, channel congestion metrics, and node reputation vectors.
- **Trust-Penalized Shortest Path Routing:** Modifies standard link distance $d(u, v)$ with a non-linear trust degradation penalty:
  $$w(u, v) = \frac{d(u, v)}{\max(T_v, \epsilon)} \cdot \left[ 1 + \beta \cdot \text{Congestion}(v) \right]$$
- **Dynamic Flow Rule Invalidation:** When a vehicle's trust score drops below $0.70$, traffic is steered around the degraded node. If $T_v < 0.40$, the controller issues network-wide flow table drop rules to isolate the attacker.

---

## Mathematical Formulation of Trust

### 1. Multi-Factor Evidence Fusion
At each observation interval $t$, the RSU computes an empirical evidence score $E_i(t) \in [0, 1]$ for vehicle $i$ by fusing four orthogonal physical and network dimensions:

$$E_i(t) = w_1 \cdot C_{\text{kin}}(i, t) + w_2 \cdot P_{\text{plaus}}(i, t) + w_3 \cdot S_{\text{comm}}(i, t) + w_4 \cdot N_{\text{neigh}}(i, t)$$

Where:
- $C_{\text{kin}} \in [0, 1]$ measures internal kinematic consistency between reported speed, position delta, and reported acceleration.
- $P_{\text{plaus}} \in [0, 1]$ evaluates physical feasibility against road network geometry and kinematic limits.
- $S_{\text{comm}} \in [0, 1]$ represents communication channel integrity, penalized by packet drop ratio and retransmissions.
- $N_{\text{neigh}} \in [0, 1]$ computes spatial multi-angle consensus from vehicles within a 300 m radius.
- Weights satisfy $\sum_{j=1}^{4} w_j = 1.0$ (Nominal: $w_1 = 0.35, w_2 = 0.25, w_3 = 0.20, w_4 = 0.20$).

### 2. Exponential Moving Average (EMA) Trust Update
Historical reputation is updated dynamically using an Exponential Moving Average:

$$T_i(t) = \alpha \cdot T_i(t - \Delta t) + (1 - \alpha) \cdot E_i(t)$$

- **Pareto-Optimal Forgetting Factor:** Comprehensive sensitivity sweeps determine that $\alpha = 0.70$ provides the optimal trade-off between transient sensor noise rejection and rapid reaction against acute insider attacks.

### 3. Temporal Staleness Decay and Delay Compensation
To prevent dormant or disconnected vehicles from maintaining artificially high reputation scores, an exponential decay function is applied during periods without received beacons:

$$T_{\text{decayed}}(t) = T_i(t_{\text{last}}) \cdot \exp\left(-\lambda \cdot (t - t_{\text{last}})\right)$$

Where $\lambda = 0.05\text{ s}^{-1}$ represents the temporal decay coefficient. If communication latency $\tau$ exceeds 100 ms, kinematic dead reckoning extrapolates the expected vehicle state to verify if delayed packets are legitimate or replays.

### 4. Hybrid Decision Fusion Matrix
The final forwarding verdict is determined by combining the dynamic trust score $T_i(t)$ and the machine learning classification outcome $y_{\text{ML}} \in \{0, 1\}$:

| Trust Score State | ML Classifier Output | System Action | Network Consequence |
| :--- | :--- | :--- | :--- |
| **High Trust** ($T \ge 0.70$) | **Benign** ($y = 0$) | **ACCEPT** | Packets forwarded; full network participation. |
| **Medium Trust** ($0.40 \le T < 0.70$) | **Benign** ($y = 0$) | **WARN** | High-rate safety messages throttled; monitoring rate increased. |
| **High / Medium Trust** ($T \ge 0.40$) | **Malicious** ($y = 1$) | **WARN / PROBATION** | Packet quarantined; neighbor verification challenge issued. |
| **Low Trust** ($T < 0.40$) | **Malicious** ($y = 1$) | **QUARANTINE / BLOCK** | Node evicted; SDN flow drop rules enforced network-wide. |

---

## 16 Attack Vectors Handled

EdgeTrust-VANET comprehensively evaluates and mitigates 16 standardized attack behaviors derived from the VeReMi, VeReMi-NextGen, and V-RADD benchmarks:

| Attack ID | Threat Classification | Attack Mechanism | Primary Detection Tier |
| :---: | :--- | :--- | :---: |
| **1** | `constantPositionOffset` | Transmits static position shift (+28 m). | Tier 1 (Dead Reckoning) / Tier 2 (ML) |
| **2** | `randomPositionOffset` | Random position perturbation ($\pm 20\text{ m}$ to $\pm 70\text{ m}$). | Tier 1 (Kinematic Firewall) |
| **3** | `positionMirroring` | Coordinates reflected across the road axis. | Tier 2 (Spatial Neighbor Consensus) |
| **4** | `constantSpeedOffset` | Injects constant velocity bias (+12 m/s). | Tier 1 (Dead Reckoning Delta) |
| **5** | `randomSpeedOffset` | Velocity perturbed randomly ($\pm 6\text{ m/s}$). | Tier 2 (EMA Trust Derivative) |
| **6** | `zeroSpeedReport` | Reports 0 m/s while moving at arterial speed. | Tier 1 (Kinematic Discrepancy) |
| **7** | `suddenStop` | Telemetry freezes position while claiming 0 m/s. | Tier 1 / Tier 2 (Acceleration Bound) |
| **8** | `suddenConstantSpeed` | Speed locked at trigger value indefinitely. | Tier 2 (Longitudinal Variance) |
| **9** | `reversedHeading` | Transmits inverted orientation ($\theta + 180^{\circ}$). | Tier 1 (Heading Continuity Delta) |
| **10** | `feignedBraking` | Broadcasts $-6\text{ m/s}^2$ while accelerating. | Tier 1 / Tier 2 (Kinematic Filter) |
| **11** | `accelMultiplication` | Acceleration amplified by factor of 3. | Tier 1 (Physical Accel Cap $> 8\text{ m/s}^2$) |
| **12** | `dosAttack` | Flood bursts at 16 Hz to saturate channel. | Tier 2 (Rate Limiter & Priority Queue) |
| **13** | `trafficCongestionSybil`| Synthesizes phantom vehicles with jitter. | Tier 2 (Signal Strength & RSU Cross-Check)|
| **14** | `dataReplay` | Broadcasts historical valid beacons out of order. | Tier 1 / Tier 2 (Timestamp Staleness Check) |
| **15** | `timeDelayAttack` | Adds artificial +3.0 s latency offset to alerts. | Tier 1 (Dead Reckoning Extrapolation) |
| **16** | `blackholeAttack` | Selectively drops 70% of transit safety packets. | Tier 2 (PDR Tracking) / Tier 4 (SDN Routing) |

---

## Repository Directory Structure

```
edgetrust-core/
├── README.md                            # Comprehensive system documentation
├── .gitignore                           # Build artifact exclusions
├── datasets/                            # Simulation datasets and trace archives
├── docs/                                # Technical specifications and progress reports
│   └── EdgeTrust_Project_Progress_Report.pdf
├── reports/                             # Academic paper drafts, TeX sources, figures
│   ├── edge_trust_vanet_final_draft.tex # IEEE Conference publication manuscript
│   ├── architecture.png                 # System architecture diagram
│   ├── figures/                         # Empirical plots and evaluation diagrams
│   │   ├── ablation_study_analysis.png
│   │   ├── delayed_messages_simulation.png
│   │   ├── scalability_urban_vs_rural.png
│   │   ├── trust_alpha_sensitivity.png
│   │   ├── unified_feature_importance.png
│   │   └── unified_model_comparison.png
│   ├── certificates/                    # Team professional certification credentials
│   └── *_certification_summary.tex     # 2-Page Technical Enrichment Reports (All 5 Authors)
├── scripts/                             # Simulation execution and inspection utilities
│   ├── benchmark_live_models.py         # Sub-microsecond live benchmark script
│   ├── generate_progress_report_pdf.py  # PDF compilation utility
│   └── inspect_checkpoint.py           # Simulation ledger and state verification tool
└── simulator/                           # Integrated network and mobility simulator
    ├── omnetpp/                         # OMNeT++ 6.4.0 environment
    │   └── EdgeTrustSim/                # Core C++ Discrete-Event Simulation Models
    │       ├── EdgeTrustFogApp.cc/.h    # Tier 3 Fog Ledger & Consensus
    │       ├── EdgeTrustRSUApp.cc/.h    # Tier 2 RSU Trust Engine & QoS Queue
    │       ├── EdgeTrustVehicleApp.cc/.h# Tier 1 OBU Kinematic Firewall
    │       ├── EdgeTrustSDNController.cc/.h # Tier 4 SDN Routing & Flow Enforcement
    │       ├── EdgeTrustSafetyMessage_m.msg # DSRC BSM Message Definition
    │       ├── LogisticRegressionPredictor.h # Embedded C++ LR Predictor (0.38 us)
    │       ├── RandomForestPredictor.h  # Embedded C++ RF Predictor (6.4 MB)
    │       ├── LightGBMPredictor.h      # Embedded C++ GBDT Predictor (329 us)
    │       ├── omnetpp.ini              # Simulation parameters, topology, and configs
    │       └── edgetrust.launchd.xml    # TraCI / SUMO synchronization config
    ├── sumo/                            # Microscopic road traffic simulation
    │   ├── edgetrust_scenario/          # Multi-lane arterial scenario with RSUs
    │   └── grid_demo/                   # Urban Manhattan grid scenario
    ├── veins/                           # Veins 5.3.1 vehicular network library
    └── results/                         # Live simulation run outputs and ledgers
        ├── checkpoint_state.json        # Serialized node trust states
        ├── checkpoint_fog_ledger.json   # Regional Fog master ledger
        ├── live_simulation_benchmark_dataset.csv
        └── live_model_comparison_metrics.json
```

---

## Build and Execution Guide

### 1. Prerequisites and Environment Setup
Ensure the following tools and compilers are installed on your Linux or Windows (WSL2/MinGW) host:
- **C++ Compiler:** GCC / G++ 11+ or Clang 13+ with C++17 support.
- **OMNeT++:** Version 6.4.0 (configured with canvas support).
- **SUMO:** Version 1.20.0 or higher.
- **Python:** Python 3.8+ (for telemetry analysis and visualization scripts).

### 2. Building EdgeTrustSim in OMNeT++
Navigate to the `EdgeTrustSim` directory and compile the native C++ modules:

```bash
cd simulator/omnetpp/EdgeTrustSim

# Generate Makefiles using opp_makemake
opp_makemake -f --deep -I../../veins/src -L../../veins/src -lveins

# Compile with optimization flags
make -j$(nproc) MODE=release
```

### 3. Launching the SUMO Mobility Scenario
Start the TraCI synchronization daemon in a dedicated shell:

```bash
# Start SUMO with TraCI listening on port 9999
sumo -c simulator/sumo/edgetrust_scenario/scenario.sumocfg --remote-port 9999
```

### 4. Running the Discrete-Event Simulation
Execute `EdgeTrustSim` using the pre-configured simulation configuration:

```bash
cd simulator/omnetpp/EdgeTrustSim

# Run command-line execution (Cmdenv)
./EdgeTrustSim -u Cmdenv -c EdgeTrustScenario -r 0 omnetpp.ini

# Or launch with graphical canvas (Qtenv)
./EdgeTrustSim -u Qtenv -c EdgeTrustScenario omnetpp.ini
```

### 5. Inspecting Results and Checkpoints
Evaluate runtime state, trust convergence, and detection statistics from simulation logs:

```bash
# Inspect generated Fog ledger and vehicle trust scores
python scripts/inspect_checkpoint.py simulator/results/checkpoint_fog_ledger.json

# Execute real-time inference latency benchmark across C++ models
python scripts/benchmark_live_models.py
```

---

## Empirical Evaluation and Research Results

### 1. Real-Time Inference Latency Benchmark
Evaluated on an Intel Core i7-13700H @ 2.40 GHz across 3,539 test evaluations:

| Model Architecture | Implementation | Inference Latency | Memory Footprint | Edge RSU Suitability |
| :--- | :--- | :---: | :---: | :--- |
| **Logistic Regression** | **Native C++** | **0.38 us** | **0.8 KB** | **Optimal for microsecond pre-filtering** |
| **Decision Tree (CART)** | **Native C++** | **5.00 us** | **18 KB** | **Optimal for deterministic edge rules** |
| **CatBoost** | Compiled C++ Runtime | 89 us | 248 KB | High-throughput edge server |
| **XGBoost** | Compiled C++ Runtime | 177 us | 464 KB | Standard RSU deployment |
| **LightGBM** | Native C++ Predictor | 329 us | 669 KB | High-accuracy balanced GBDT |
| **Random Forest (100 Trees)**| Native C++ Predictor | 13,441 us | 6.4 MB | Heavy ensemble baseline |

### 2. Detection Performance on Unified Benchmark
Evaluated across 23,970 records (72.88% VeReMi, 20.86% EdgeTrust, 6.26% V-RADD) partitioned by Vehicle ID via Stratified GroupKFold (zero data leakage):

- **Overall Accuracy:** 97.45%
- **Weighted F1-Score:** 97.28%
- **Precision:** 98.15%
- **False Positive Rate (FPR):** 1.85% (an 85.1% reduction from the 12.42% legacy baseline).
- **Control Channel Overhead Reduction:** 34.2% drop in retransmitted malicious frames due to Tier 1 OBU filtering.

### 3. Architectural Ablation Analysis

| Configuration | Accuracy | F1-Score | FPR | Channel Load (BSM/s) |
| :--- | :---: | :---: | :---: | :---: |
| **Tier 1 Only (Kinematic Firewall)** | 82.15% | 80.40% | 8.92% | 1,240 |
| **Tier 2 Only (Edge RSU ML)** | 91.60% | 90.85% | 6.45% | 1,885 |
| **Tiers 1 + 2 (OBU + RSU)** | 95.80% | 95.50% | 2.80% | 1,240 |
| **Full 4-Tier Hierarchy (EdgeTrust-VANET)** | **97.45%** | **97.28%** | **1.85%** | **1,120** |

---

## Academic Citation

If you utilize this simulation framework, architecture, or benchmark dataset in your research, please cite our conference paper:

```bibtex
@inproceedings{edgetrust_vanet_2026,
  author    = {Neethu, M. R. and Mounik Sai, M. and Aashiq, E. and Vivek Vardhan, C. and Nishanth, K. and Abhinav, J.},
  title     = {EdgeTrust-VANET: A Decentralized Four-Tier Hierarchical Trust Management and Kinematic Filtering Framework for Vehicular Ad-Hoc Networks},
  booktitle = {Proceedings of the IEEE International Conference on Communications (ICC) / VTC},
  year      = {2026},
  pages     = {1--8},
  address   = {Coimbatore, India}
}
```

---

## Research Team and Institutional Affiliation

This project was developed under the supervision of **Dr. Neethu M R** at **Amrita Vishwa Vidyapeetham, Amrita School of Computing, Coimbatore, India**:

- **Dr. Neethu M R** - *Project Guide & Assistant Professor, Department of Computer Science and Engineering* (`mr_neethu@cb.amrita.edu`)
- **Madisetty Mounik Sai** - *Undergraduate Researcher* (`cb.sc.u4cse23561@cb.students.amrita.edu`)
- **Edavalapati Aashiq** - *Undergraduate Researcher* (`cb.sc.u4cse23560@cb.students.amrita.edu`)
- **Chitturi Vivek Vardhan** - *Undergraduate Researcher* (`cb.sc.u4cse23360@cb.students.amrita.edu`)
- **Nishanth K** - *Undergraduate Researcher* (`cb.sc.u4cse23523@cb.students.amrita.edu`)
- **Juturu Naga Abhinava Sai** - *Undergraduate Researcher* (`cb.sc.u4cse23563@cb.students.amrita.edu`)