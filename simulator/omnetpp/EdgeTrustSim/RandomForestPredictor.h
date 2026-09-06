#ifndef __RANDOM_FOREST_PREDICTOR_H_
#define __RANDOM_FOREST_PREDICTOR_H_

/**
 * RandomForestPredictor.h
 * ─────────────────────────────────────────────────────────────
 * Auto-generated C++ classifier exported from the trained
 * scikit-learn Random Forest model (Random_Forest_GridSearch.pkl)
 * and scaler (scaler.pkl) from edgetrust-ml/models/.
 *
 * Provides sub-microsecond edge inference directly inside RSU.
 *
 * Features (8 inputs):
 *   0: speed (m/s)
 *   1: acceleration (m/s^2)
 *   2: position_x (m)
 *   3: position_y (m)
 *   4: direction (degrees)
 *   5: packet_drop_ratio (0.0 - 1.0)
 *   6: latency (ms)
 *   7: signal_strength (dBm)
 * ─────────────────────────────────────────────────────────────
 */

#include "veins/veins.h"

namespace veins {

class VEINS_API RandomForestPredictor {
  public:
    static constexpr int N_FEATURES = 8;
    static constexpr int N_ESTIMATORS = 50;

    static int predict(const double raw[N_FEATURES]);
    static double predictProba(const double raw[N_FEATURES]);
};

} // namespace veins

#endif // __RANDOM_FOREST_PREDICTOR_H_
