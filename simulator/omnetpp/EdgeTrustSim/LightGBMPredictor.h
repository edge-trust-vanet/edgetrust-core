#ifndef __LIGHTGBM_PREDICTOR_H_
#define __LIGHTGBM_PREDICTOR_H_

/**
 * LightGBMPredictor.h
 * ─────────────────────────────────────────────────────────────
 * Auto-generated C++ classifier exported from the trained
 * LightGBM model (unified_LightGBM.pkl) and scaler (unified_scaler.pkl)
 * from edgetrust-ml/models/.
 *
 * Provides sub-microsecond edge inference directly inside RSU.
 *
 * Features (14 inputs):
 *   0: position_x (m)
 *   1: position_y (m)
 *   2: speed (m/s)
 *   3: direction (degrees)
 *   4: acceleration (m/s^2)
 *   5: packet_sent
 *   6: packet_received
 *   7: packet_drop_ratio (0.0 - 1.0)
 *   8: latency (ms)
 *   9: retransmission_count
 *  10: signal_strength (dBm)
 *  11: trust_score (0.0 - 1.0)
 *  12: neighbor_trust_score_avg (0.0 - 1.0)
 *  13: historical_trust_score (0.0 - 1.0)
 * ─────────────────────────────────────────────────────────────
 */

#include "veins/veins.h"

namespace veins {

class VEINS_API LightGBMPredictor {
  public:
    static constexpr int N_FEATURES = 14;
    static constexpr int N_TREES = 180;

    static int predict(const double raw[N_FEATURES]);
    static double predictProba(const double raw[N_FEATURES]);
};

} // namespace veins

#endif // __LIGHTGBM_PREDICTOR_H_
