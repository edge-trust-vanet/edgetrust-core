# EdgeTrust-VANET Core Simulation and Edge Engine

**Decentralized Four-Tier Hierarchical Trust Management and Kinematic Filtering Framework for Vehicular Ad-Hoc Networks**

[![Build Status](https://img.shields.io/badge/Build-Passing-brightgreen.svg)]()
[![C++ Standard](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![Network Simulator](https://img.shields.io/badge/OMNeT%2B%2B-6.4.0-orange.svg)](https://omnetpp.org/)
[![Mobility Simulator](https://img.shields.io/badge/SUMO-1.20%2B-yellow.svg)](https://eclipse.dev/sumo/)
[![Vehicular Framework](https://img.shields.io/badge/Veins-5.3.1-red.svg)](https://veins.car2x.org/)
[![Wireless Standard](https://img.shields.io/badge/IEEE-802.11p%20%2F%201609.4-lightgrey.svg)](https://standards.ieee.org/)
[![License](https://img.shields.io/badge/License-Academic%20Research-blue.svg)]()

---

## Executive Overview

Vehicular Ad-hoc Networks (VANETs) form the communication backbone of Cooperative Intelligent Transportation Systems (C-ITS), enabling time-critical safety applications like Forward Collision Warning (FCW), Emergency Electronic Brake Lights (EEBL), and Intersection Collision Avoidance over Dedicated Short-Range Communications (DSRC / IEEE 802.11p) and C-V2X.

Current vehicular security architectures rely almost exclusively on Public Key Infrastructure (PKI) and Elliptic Curve Digital Signature Algorithms (ECDSA) standardized under IEEE 1609.2 and ETSI TS 103 097. While cryptographic authentication guarantees message integrity and non-repudiation, it exhibits a fundamental vulnerability: **it authenticates the sender's identity, but cannot verify the physical truthfulness of the transmitted telemetry**. 

An authenticated insider vehicle—whether compromised by malware, faulty sensors, or an adversarial driver—can sign and broadcast mathematically valid signatures over completely falsified kinematics (e.g., ghost coordinates, phantom emergency braking, fabricated Sybil jams, and selective blackhole packet drops).

**EdgeTrust-VANET** addresses this challenge through a defense-in-depth, four-tier architecture combining:
1. **Client-side kinematic heuristic filtering** on On-Board Units (OBUs) to drop obviously invalid packets locally;
2. **Sub-microsecond native C++ machine learning inference** embedded directly inside edge Roadside Units (RSUs) without Python runtime or IPC overhead;
3. **Cross-RSU handover tracking and observation-weighted Beta-Bayesian consensus** across Regional Fog nodes;
4. **Dynamic, trust-penalized shortest-path routing** within a centralized Software-Defined Networking (SDN) controller that isolates compromised nodes and routes safety traffic around degraded links.

---

## Repository Ecosystem & Git Submodules

`edgetrust-core` serves as the central orchestration and simulation repository. It directly integrates two specialized submodules that form the complete EdgeTrust-VANET ecosystem:

```
edgetrust-core/ (Main Engine: OMNeT++, SUMO, Veins, C++ Four-Tier Runtime)
│
├── edgetrust-ml/  [Git Submodule]
│   └── Leakage-free 14-feature pipeline, 17-model benchmark,
│       embedded C++ model exporters, and interactive Flask edge dashboard.
│
└── edgetrust-web/ [Git Submodule]
    └── React 18 + Vite + Tailwind interactive web portal, microscopic
        mobility canvas simulator, literature survey reader, and research showcase.
```

### Cloning with Submodules

To clone `edgetrust-core` along with all nested submodules in a single step:

```bash
git clone --recurse-submodules https://github.com/edge-trust-vanet/edgetrust-core.git
cd edgetrust-core
```

If you already cloned without the `--recurse-submodules` flag, initialize and fetch the submodules at any time:

```bash
git submodule update --init --recursive
```

---

## System Architecture

EdgeTrust-VANET partitions trust evaluation across four tiers to eliminate single points of failure, avoid broadcast storms on the 802.11p Control Channel (CCH), and guarantee decision latencies well within the 100 ms C-ITS safety deadline.

```
+===================================================================================+
|                                    TIER 4                                         |
|                   CENTRALIZED SOFTWARE-DEFINED NETWORKING (SDN)                  |
|  - Global Topology Graph Maintenance & Dynamic Link Weight Calculation            |
|  - Trust-Penalized Multi-Hop Dijkstra Shortest Path Routing                      |
|  - Automated OpenFlow Drop Rules & Rogue Node Isolation (T < 0.40)                |
+===================================================================================+
                                         ^
                                         | Secure Optical Backhaul (OpenFlow)
                                         v
+===================================================================================+
|                                    TIER 3                                         |
|                        REGIONAL FOG COMPUTING LEDGER                             |
|  - Cross-RSU Handover Tracking & Boundary Discrepancy Resolution (|delta| > 0.20)|
|  - Observation-Weighted Beta-Bayesian Reputation Consensus                       |
|  - Deterministic Safety Overrides & Persistent Regional Vehicle Ledgers          |
+===================================================================================+
                                         ^
                                         | Low-Latency I2I Fiber / 5G Xn Interface
                                         v
+===================================================================================+
|                                    TIER 2                                         |
|                    EDGE ROADSIDE UNIT (RSU) TRUST ENGINE                          |
|  - 14-Dimensional Spatiotemporal Feature Extraction (x in R^14)                  |
|  - Native C++ ML Predictors (0.38 us LR, 5.00 us CART, 329 us LightGBM)          |
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
|  - Drops 34.2% Malicious Frames Locally Before Multi-Hop Retransmission           |
+===================================================================================+
```

### Tier Specifications

#### Tier 1: On-Board Unit (OBU) Kinematic Firewall
Implemented in `EdgeTrustVehicleApp.cc`. Before forwarding an incoming Basic Safety Message (BSM) or committing it to local safety logic, the vehicle's OBU performs instantaneous kinematic sanity checks:
- **Velocity Envelope:** $v_{\text{msg}} \le 45.0\text{ m/s}$ (162 km/h).
- **Acceleration Bound:** $|a_{\text{msg}}| \le 8.0\text{ m/s}^2$.
- **Dead Reckoning Continuity:**
  $$\Delta d = \|\mathbf{p}_{\text{msg}}(t) - \mathbf{p}_{\text{est}}(t)\| \le v \Delta t + \frac{1}{2} a (\Delta t)^2 + \epsilon_{\text{GPS}}$$
- **Heading Consistency:** Angular deviation $|\theta(t) - \theta(t - \Delta t)| \le \omega_{\text{max}} \Delta t$.
- **Impact:** Eliminates **34.2%** of raw spoofed messages locally, preventing wireless channel congestion and broadcast storm collapse.

#### Tier 2: Edge Roadside Unit (RSU) Trust Engine & Native C++ Predictors
Implemented in `EdgeTrustRSUApp.cc`. Operating over a 300 m communication radius, each RSU computes an empirical evidence score from 14 observable features and runs embedded C++ classifiers:
- **Zero-IPC C++ Inference:** Employs header-only C++ predictors (`LogisticRegressionPredictor.h`, `RandomForestPredictor.h`, `LightGBMPredictor.h`) without Python runtime or socket latency.
- **Inference Speed:** $0.38\,\mu\text{s}$ (Logistic Regression), $5.00\,\mu\text{s}$ (Decision Tree / CART), $329\,\mu\text{s}$ (LightGBM).
- **RSU Decision Latency:** $0.38\text{ ms}$ total, comfortably beneath the 100 ms vehicular deadline.
- **Priority QoS Queue:** Prioritizes safety-critical BSMs ahead of diagnostic telemetry to maintain deterministic delivery.

#### Tier 3: Regional Fog Computing Ledger
Implemented in `EdgeTrustFogApp.cc`. Aggregates trust assessments from neighboring RSUs to maintain continuous coverage as vehicles cross cell boundaries:
- **Discrepancy Resolution:** Automatically triggers when trust score variance across adjacent RSUs exceeds $\epsilon_{\text{tol}} = 0.20$.
- **Beta-Bayesian Consensus:** Weighs individual RSU reports by the number of independent physical observations $N_{\text{obs}}$:
  $$T_{\text{fog}} = \frac{\sum_{k=1}^{M} N_{k} \cdot T_{\text{RSU}, k}}{\sum_{k=1}^{M} N_{k}}$$
- **Safety Overrides:** If any RSU detects critical kinematic violations with confidence $> 0.90$, the Fog node immediately broadcasts a regional quarantine.

#### Tier 4: Centralized Software-Defined Networking (SDN) Controller
Implemented in `EdgeTrustSDNController.cc`. Operates over the regional transport graph $G = (V, E)$:
- **Dynamic Topology Graph:** Tracks link packet delivery ratios, latency, and node trust vectors.
- **Trust-Penalized Shortest Path Routing:** Adjusts link weights with a non-linear trust penalty:
  $$w(u, v) = \frac{d(u, v)}{\max(T_v, \epsilon)} \cdot \left[ 1 + \beta \cdot \text{Congestion}(v) \right]$$
- **Adaptive Rerouting & Isolation:** Diverts transit traffic away from suspect nodes ($T < 0.70$) and pushes OpenFlow drop rules when $T < 0.40$.

---

## Mathematical Trust Formulation

### 1. Multi-Factor Evidence Fusion
At each observation interval $t$, the RSU calculates an empirical evidence score $E_i(t) \in [0, 1]$ across four orthogonal dimensions:

$$E_i(t) = w_1 \cdot C_{\text{kin}}(i, t) + w_2 \cdot P_{\text{plaus}}(i, t) + w_3 \cdot S_{\text{comm}}(i, t) + w_4 \cdot N_{\text{neigh}}(i, t)$$

Where:
- $C_{\text{kin}}$ evaluates kinematic consistency between reported speed, delta position, and acceleration.
- $P_{\text{plaus}}$ verifies physical road geometry constraints.
- $S_{\text{comm}}$ tracks wireless channel health (packet delivery ratio, retransmission count).
- $N_{\text{neigh}}$ computes spatial neighbor multi-angle consensus within 300 m.
- Standard weights: $w_1 = 0.35, w_2 = 0.25, w_3 = 0.20, w_4 = 0.20$.

### 2. Exponential Moving Average (EMA) Update
Reputation evolves smoothly over time:

$$T_i(t) = \alpha \cdot T_i(t - \Delta t) + (1 - \alpha) \cdot E_i(t)$$

Empirical sensitivity sweeps confirm that $\alpha = 0.70$ offers the optimal balance between filtering transient noise and rapidly reacting to acute insider attacks.

### 3. Temporal Staleness Decay
To prevent disconnected or dormant vehicles from exploiting outdated good reputation:

$$T_{\text{decayed}}(t) = T_i(t_{\text{last}}) \cdot \exp\left(-\lambda \cdot (t - t_{\text{last}})\right)$$

Where $\lambda = 0.05\text{ s}^{-1}$.

### 4. Hybrid Decision Fusion Matrix

| Trust Score | ML Classification | Verdict | Network Action |
| :--- | :--- | :--- | :--- |
| **High** ($T \ge 0.70$) | **Benign** ($y = 0$) | **ACCEPT** | Unrestricted forwarding; full network participation. |
| **Medium** ($0.40 \le T < 0.70$) | **Benign** ($y = 0$) | **WARN** | High-rate messages throttled; observation frequency doubled. |
| **High / Medium** ($T \ge 0.40$) | **Malicious** ($y = 1$) | **PROBATION** | Message quarantined; neighbor verification challenged. |
| **Low** ($T < 0.40$) | **Malicious** ($y = 1$) | **QUARANTINE / BLOCK** | Node evicted; SDN pushes network-wide drop rules. |

---

## 16 Evaluated Attack Vectors

EdgeTrust-VANET mitigates 16 standardized attack behaviors from the VeReMi, VeReMi-NextGen, and V-RADD benchmarks:

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

## Simulation Scenarios & Turnkey Execution

### 1. Highway Corridor Deployment (`HighwayScenario.ned`, `highway.ini`)
- **Corridor Geometry:** 2,000-meter multi-lane divided carriageway with on-ramp acceleration lanes, off-ramp deceleration lanes, and an overpass interchange at $X = 1000\text{ m}$.
- **Infrastructure:** 4 RSUs spaced along the corridor, 2 Regional Fog nodes, and a centralized SDN controller.
- **Traffic:** Heterogeneous mix of sedans, SUVs, heavy freight tractor-trailers, and priority emergency vehicles.

### 2. City-Wide Urban Deployment (`CityExampleScenario.ned`, `city.ini`)
- **Urban Map:** Full-scale OpenStreetMap network of Erlangen, Germany ($2.7\text{ km} \times 3.1\text{ km}$).
- **Multi-District Infrastructure:** 6 RSUs partitioned across 3 Regional Fog Districts:
  - **District 0 (East Campus / Tech Park):** RSUs 0 & 1, Fog 0
  - **District 1 (Central / Downtown Core):** RSUs 2 & 3, Fog 1
  - **District 2 (West Commercial Arterial):** RSUs 4 & 5, Fog 2
  - **SDN Controller:** Centralized plane overseeing cross-district inter-fog trunks and backhaul links.

### How to Run the Simulations

#### A. Fast Terminal / Headless Mode
Execute the automated bash runner to simulate and inspect real-time metrics in the terminal:

```bash
# Run city scenario for 30 simulation seconds
./scripts/run_city_headless.sh City_Full_Erlangen 30s

# Test SDN dynamic rerouting under backhaul link failure
./scripts/run_city_headless.sh City_SDN_Detour_Downtown_Reroute 30s

# Test high-density adversarial storm (40% malicious nodes)
./scripts/run_city_headless.sh City_Adversarial_Storm 30s
```

#### B. Interactive Graphical Mode (OMNeT++ Qtenv + SUMO-GUI)
Launch the visual co-simulation to see moving vehicles, RSU coverage zones, and live packet routing:

```bash
./scripts/run_city_gui.sh City_Full_Erlangen
```
*In the OMNeT++ window, click the green **Run** or **Fast** button to start.*

#### C. Comprehensive Scaling & Failure Suite
Run all 14 benchmark scenarios (urban vs. rural, varying vehicle densities, link severance, packet loss):

```bash
python3 scripts/run_experiments_scaling_and_failures.py
```

---

## Directory Structure

```text
edgetrust-core/
├── README.md                            # Primary architectural and operational documentation
├── .gitmodules                          # Submodule references (edgetrust-ml, edgetrust-web)
├── edgetrust-ml/                        # [Submodule] ML training pipeline, 17 models, dashboard
├── edgetrust-web/                       # [Submodule] Interactive React web portal & survey viewer
├── docs/                                # Technical specifications and milestone reports
├── experiments/                         # Scenario definitions and configuration suites
│   ├── city_erlangen/                   # Erlangen city SUMO and OMNeT++ configs
│   │   ├── erlangen_city.sumo.cfg
│   │   ├── erlangen_city.rou.xml
│   │   ├── erlangen_city.add.xml
│   │   └── city.ini
│   ├── highway/                         # 2 km visual highway corridor configs
│   │   ├── highway.net.xml
│   │   ├── highway.sumocfg
│   │   └── highway.ini
│   └── scaling_and_failures/            # Empirical benchmark scripts and JSON outputs
├── reports/                             # IEEE publication drafts, TeX sources, figures
│   └── figures/                         # System diagrams and empirical plots
├── scripts/                             # Turnkey simulation launchers and evaluation tools
│   ├── run_city_headless.sh             # Fast headless city runner
│   ├── run_city_gui.sh                  # Interactive Qtenv + SUMO-GUI launcher
│   └── run_experiments_scaling_and_failures.py
└── simulator/                           # Core C++ OMNeT++ and Veins modules
    └── omnetpp/EdgeTrustSim/
        ├── EdgeTrustVehicleApp.cc/.h    # Tier 1 OBU Kinematic Firewall
        ├── EdgeTrustRSUApp.cc/.h        # Tier 2 RSU Trust Engine & ML Predictors
        ├── EdgeTrustFogApp.cc/.h        # Tier 3 Fog Ledger & Handover Consensus
        ├── EdgeTrustSDNController.cc/.h # Tier 4 SDN Routing & Flow Enforcement
        ├── LogisticRegressionPredictor.h# 0.38 us embedded C++ predictor
        ├── RandomForestPredictor.h      # Embedded C++ Random Forest predictor
        └── LightGBMPredictor.h          # 329 us embedded C++ GBDT predictor
```

---

## Empirical Evaluation Summary

### 1. Native C++ Inference Latency
Benchmarked on an Intel Core i7-13700H @ 2.40 GHz across 3,539 test evaluations:

| Model Architecture | Implementation | Latency | Memory | Operational Role |
| :--- | :--- | :---: | :---: | :--- |
| **Logistic Regression** | **Native C++** | **0.38 µs** | **0.8 KB** | **Wire-speed line-rate pre-filter** |
| **Decision Tree (CART)** | **Native C++** | **5.00 µs** | **18 KB** | **Deterministic edge rule validation** |
| **CatBoost** | Compiled C++ | 89 µs | 248 KB | High-throughput edge server |
| **XGBoost** | Compiled C++ | 177 µs | 464 KB | Standard RSU deployment |
| **LightGBM** | Native C++ | 329 µs | 669 KB | High-accuracy balanced GBDT |
| **Random Forest (100 Trees)**| Native C++ | 13,441 µs | 6.4 MB | Heavy ensemble baseline |

### 2. Detection Performance (Zero Data Leakage)
Evaluated across 23,970 records partitioned by Vehicle ID via Stratified GroupKFold:
- **Overall Accuracy:** 97.45%
- **Weighted F1-Score:** 97.28%
- **Precision:** 98.15%
- **False Positive Rate (FPR):** 1.85% (an 85.1% reduction from legacy 12.42% baseline).
- **Control Channel Bandwidth Savings:** 34.2% reduction in retransmitted malicious frames.
- **SDN Safety Packet Delivery Ratio (PDR):** Maintained at **100%** even under backhaul link severances.

---

## Academic Citation

If you use EdgeTrust-VANET in your research, please cite our work:

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

## Project Contributors and Research Team

EdgeTrust-VANET is developed by **Team 43** under the academic supervision of **Dr. Neethu M R** at **Amrita Vishwa Vidyapeetham, Amrita School of Computing, Coimbatore, India**.

All contributors across the core simulation engine and its nested submodules ([`edgetrust-ml`](https://github.com/edge-trust-vanet/edgetrust-ml) and [`edgetrust-web`](https://github.com/edge-trust-vanet/edgetrust-web)) are recognized below:

| Contributor | GitHub Profile | Primary Focus & Subsystem Ownership |
| :--- | :--- | :--- |
| **Edavalapati Aashiq** | [@Aashiq-Edavalapati](https://github.com/Aashiq-Edavalapati) | **Core Systems Architect:** OMNeT++ / Veins simulation engine, SDN adaptive forwarding controller, multi-district Fog consensus, visual Highway & Erlangen City deployments. |
| **Madisetty Mounik Sai** | [@mouniksai](https://github.com/mouniksai) | **Machine Learning Lead:** 14-feature observable vector pipeline, 17-model benchmark suite, data leakage verification audit, native C++ model exporter, and real-time Flask dashboard. |
| **Nishanth K** | [@NISHANTH-KONCHADA](https://github.com/NISHANTH-KONCHADA) | **Web Platform & Visualization Lead:** React 18 / Tailwind web portal, 60 FPS microscopic mobility canvas (`VanetCanvas.tsx`), LaTeX publication sources, and technical devlogs. |
| **Chitturi Vivek Vardhan** | [@thecodingvivek](https://github.com/thecodingvivek) | **Trust Engine & Sensitivity Optimization:** Exponential Moving Average ($\alpha = 0.70$) sensitivity sweeps, model inference speed vs. memory trade-off profiling, and trust decay calibration. |
| **Juturu Naga Abhinava Sai** | [@abhinav-juturu](https://github.com/abhinav-juturu) | **Telemetry & Edge Profiling:** Vehicular network latency and jitter profiling, edge RSU memory footprint analysis, decentralized trust taxonomy, and systematic survey synthesis. |

> [!NOTE]
> Canonical contributor identities and legacy Git alias deduplication (e.g. mapping `NISHANTH-KONCHADA` and `nishanth k` to `Nishanth K`) are managed via [`.mailmap`](.mailmap). For detailed attribution, see [CONTRIBUTORS.md](CONTRIBUTORS.md).

### Project Supervision

- **Dr. Neethu M R**  
  *Project Supervisor & Assistant Professor*  
  Department of Computer Science and Engineering  
  Amrita School of Computing, Amrita Vishwa Vidyapeetham, Coimbatore, India  
  Email: `mr_neethu@cb.amrita.edu`
