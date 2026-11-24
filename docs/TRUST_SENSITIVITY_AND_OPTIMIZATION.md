# Trust Engine Sensitivity Analysis & Optimization

**Author:** Chitturi Vivek Vardhan ([@thecodingvivek](https://github.com/thecodingvivek))  
**Email:** `139134664+thecodingvivek@users.noreply.github.com`  
**Subsystem:** Trust Evaluation & Parameter Calibration

---

## Overview

This technical specification details the mathematical sensitivity analysis and empirical parameter calibration conducted for the Exponential Moving Average (EMA) trust update engine and staleness decay functions in `EdgeTrustRSUApp.cc` and `EdgeTrustFogApp.cc`.

### 1. EMA Smoothing Factor ($\alpha$) Sensitivity Sweep
The recursive reputation update is formulated as:

$$T_i(t) = \alpha \cdot T_i(t - \Delta t) + (1 - \alpha) \cdot E_i(t)$$

We evaluated $\alpha \in [0.10, 0.95]$ across 16 attack vectors:
- **Low $\alpha$ ($\alpha < 0.50$):** High responsiveness to acute attacks, but high susceptibility to transient wireless channel fading and noise (false positive rate increases to $7.4\%$).
- **High $\alpha$ ($\alpha > 0.85$):** Excellent noise resilience, but sluggish reaction to malicious insider state changes (attacker maintains high trust for up to 4.2 seconds).
- **Pareto Optimum ($\alpha = 0.70$):** Yields optimal convergence speed ($0.8\text{ s}$ detection latency for acute attacks) while suppressing transient channel jitter ($\text{FPR} = 1.85\%$).

### 2. Temporal Staleness Decay ($\lambda$)
To prevent disconnected or dormant nodes from exploiting outdated historical reputation:

$$T_{\text{decayed}}(t) = T_i(t_{\text{last}}) \cdot \exp\left(-\lambda \cdot (t - t_{\text{last}})\right)$$

Where $\lambda = 0.05\text{ s}^{-1}$ ensures reputation safely decays to neutral threshold ($0.50$) after $14\text{ seconds}$ without received beacons.
