#ifndef __ADABOOST_PREDICTOR_H_
#define __ADABOOST_PREDICTOR_H_

/**
 * AdaBoostPredictor.h
 * ─────────────────────────────────────────────────────────────
 * Auto-generated C++ classifier exported from the trained
 * scikit-learn AdaBoost model (AdaBoost.pkl) and scaler (scaler.pkl).
 * Provides sub-microsecond inference directly on RSU edge hardware.
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

#include <cmath>

namespace veins {

struct AdaBoostStump {
    int featureIdx;
    double threshold;
    double leftVal;   // +1.0 for Malicious, -1.0 for Normal
    double rightVal;
    double weight;
};

class AdaBoostPredictor {
  public:
    static constexpr int N_FEATURES = 8;
    static constexpr int N_ESTIMATORS = 50;

    // Scaler parameters
    static constexpr double MEAN[8] = { 14.98027914, 0.07207817, 495.09850765, 495.82071204, 181.74889784, 0.82623006, 50.23847608, -64.74960221 };
    static constexpr double SCALE[8] = { 8.69264835, 1.73079119, 289.48651441, 285.78021799, 103.92795267, 1.05117603, 28.64705802, 20.40901196 };
    static constexpr double TOTAL_WEIGHT = 1.88443755;

    /**
     * Scale raw features to mean=0, std=1 using trained scaler parameters.
     */
    static inline void scaleFeatures(const double raw[N_FEATURES], double scaled[N_FEATURES]) {
        for (int i = 0; i < N_FEATURES; ++i) {
            scaled[i] = (raw[i] - MEAN[i]) / SCALE[i];
        }
    }

    /**
     * Evaluate AdaBoost ensemble on scaled features.
     * Returns: raw score sum(weight_m * pred_m)
     */
    static inline double evaluateRawScore(const double scaled[N_FEATURES]) {
        static const AdaBoostStump STUMPS[N_ESTIMATORS] = {
    { 2, -0.71438053, -1.0, -1.0, 1.08532306 },
    { 2, -0.71438053, 1.0, -1.0, 0.09946269 },
    { 4, 1.68421054, 1.0, -1.0, 0.05663338 },
    { 4, 1.68421054, -1.0, -1.0, 0.01794886 },
    { 4, 1.68421054, 1.0, -1.0, 0.01764252 },
    { 4, -0.75398850, -1.0, 1.0, 0.07642799 },
    { 4, 1.68421054, -1.0, -1.0, 0.05308217 },
    { 4, 1.68421054, 1.0, -1.0, 0.01775984 },
    { 4, 1.68421054, -1.0, -1.0, 0.01745730 },
    { 4, 1.68421054, 1.0, -1.0, 0.01716124 },
    { 4, 1.68421054, -1.0, -1.0, 0.01687143 },
    { 4, 1.68421054, 1.0, -1.0, 0.01658770 },
    { 4, 1.68421054, -1.0, -1.0, 0.01630986 },
    { 4, 1.68421054, 1.0, -1.0, 0.01603775 },
    { 3, 1.74624097, -1.0, -1.0, 0.01577119 },
    { 3, 1.74624097, 1.0, -1.0, 0.01057841 },
    { 5, 5.47016096, -1.0, -1.0, 0.01050155 },
    { 5, 5.47016096, 1.0, -1.0, 0.00755460 },
    { 3, 1.74624097, -1.0, -1.0, 0.00752617 },
    { 3, 1.74624097, 1.0, -1.0, 0.01042041 },
    { 5, 5.47016096, -1.0, -1.0, 0.01034528 },
    { 5, 5.47016096, 1.0, -1.0, 0.00749096 },
    { 5, 5.47016096, -1.0, -1.0, 0.00746301 },
    { 5, 5.47016096, 1.0, -1.0, 0.00743527 },
    { 5, 5.47016096, -1.0, -1.0, 0.00740773 },
    { 5, 5.47016096, 1.0, -1.0, 0.00738039 },
    { 3, 1.74624097, -1.0, -1.0, 0.00735326 },
    { 3, 1.74624097, 1.0, -1.0, 0.01025625 },
    { 4, 1.68421054, -1.0, -1.0, 0.01018292 },
    { 5, 5.47016096, 1.0, -1.0, 0.00731966 },
    { 4, 1.68421054, 1.0, -1.0, 0.00834595 },
    { 5, 5.47016096, -1.0, -1.0, 0.01550095 },
    { 5, 5.47016096, 1.0, -1.0, 0.00726720 },
    { 5, 5.47016096, -1.0, -1.0, 0.00724089 },
    { 5, 5.47016096, 1.0, -1.0, 0.00721477 },
    { 3, 1.74624097, -1.0, -1.0, 0.00718884 },
    { 3, 1.74624097, 1.0, -1.0, 0.01009774 },
    { 5, 5.47016096, -1.0, -1.0, 0.01002613 },
    { 5, 5.47016096, 1.0, -1.0, 0.00715674 },
    { 5, 5.47016096, -1.0, -1.0, 0.00713122 },
    { 5, 5.47016096, 1.0, -1.0, 0.00710588 },
    { 4, 1.68421054, 1.0, -1.0, 0.00828513 },
    { 5, 5.47016096, -1.0, -1.0, 0.01522939 },
    { 5, 5.47016096, 1.0, -1.0, 0.00705645 },
    { 3, 1.74624097, -1.0, -1.0, 0.00703164 },
    { 3, 1.74624097, 1.0, -1.0, 0.00994334 },
    { 5, 5.47016096, -1.0, -1.0, 0.00987337 },
    { 5, 5.47016096, 1.0, -1.0, 0.00700093 },
    { 6, -1.67716998, 1.0, -1.0, 0.02856796 },
    { 5, 5.47016096, 1.0, -1.0, 0.02748018 }
        };

        double score = 0.0;
        for (int i = 0; i < N_ESTIMATORS; ++i) {
            const AdaBoostStump& s = STUMPS[i];
            double pred = (scaled[s.featureIdx] <= s.threshold) ? s.leftVal : s.rightVal;
            score += s.weight * pred;
        }
        return score;
    }

    /**
     * Predict class from raw features:
     * Returns: 1 if Malicious, 0 if Normal
     */
    static inline int predict(const double raw[N_FEATURES]) {
        double scaled[N_FEATURES];
        scaleFeatures(raw, scaled);
        double score = evaluateRawScore(scaled);
        return (score > 0.0) ? 1 : 0;
    }

    /**
     * Predict probability of being Malicious (Class 1) [0.0 - 1.0]
     */
    static inline double predictProba(const double raw[N_FEATURES]) {
        double scaled[N_FEATURES];
        scaleFeatures(raw, scaled);
        double score = evaluateRawScore(scaled);
        double normScore = score / TOTAL_WEIGHT;
        // Logistic sigmoid mapping of normalized margin
        return 1.0 / (1.0 + std::exp(-2.0 * normScore));
    }
};

} // namespace veins

#endif // __ADABOOST_PREDICTOR_H_
