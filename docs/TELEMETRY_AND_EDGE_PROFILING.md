# Vehicular Network Telemetry & Edge Profiling

**Author:** Juturu Naga Abhinava Sai ([@abhinav-juturu](https://github.com/abhinav-juturu))  
**Email:** `juturnagaabhinavasai@gmail.com`  
**Subsystem:** Network Telemetry, Edge Benchmarking & Survey Taxonomy

---

## Overview

This report documents empirical profiling of network latency, jitter, RSU memory footprints, and classification throughput under high-density vehicular traffic.

### 1. Edge RSU Memory Footprint & Throughput
Evaluated on embedded x86 and ARM Cortex platforms simulating Roadside Unit edge nodes:

| Classifier Architecture | Implementation | Resident Memory | Per-Message Latency | Max Throughput (msg/s) |
| :--- | :--- | :---: | :---: | :---: |
| **Logistic Regression** | Native C++ Header | 0.8 KB | 0.38 µs | 2,631,578 |
| **Decision Tree (CART)** | Native C++ Header | 18 KB | 5.00 µs | 200,000 |
| **CatBoost** | Compiled Runtime | 248 KB | 89 µs | 11,235 |
| **XGBoost** | Compiled Runtime | 464 KB | 177 µs | 5,649 |
| **LightGBM** | Native C++ Header | 669 KB | 329 µs | 3,039 |
| **Random Forest (100 Trees)** | Native C++ Header | 6.4 MB | 13,441 µs | 74 |

### 2. DSRC IEEE 802.11p Latency & Jitter Under Attack
- **Nominal Traffic (10 Hz BSMs):** Median end-to-end latency: $1.8\text{ ms}$; jitter: $\pm 0.4\text{ ms}$.
- **Under Channel DoS Flood (16 Hz burst):** RSU Priority QoS queuing successfully isolates high-priority safety frames, keeping safety delivery latency below $4.2\text{ ms}$ while dropping non-critical telemetry floods.
