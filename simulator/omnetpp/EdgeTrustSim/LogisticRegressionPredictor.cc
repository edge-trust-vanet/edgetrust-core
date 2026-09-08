#include "veins/modules/application/edgetrust/LogisticRegressionPredictor.h"
#include <cmath>

namespace veins {

namespace {

constexpr double MEAN[14] = { 3889.476798460172, 4580.897162866449, 19.206167130589, 171.111712170566, 0.041064968907, 224.922238673379, 78.352739117560, 0.734345306485, 22.574598164051, 5.621735267989, -52.768030204323, 0.775753319514, 0.775147995262, 0.788444708321 };
constexpr double SCALE[14] = { 2162.908636649812, 2187.688724544255, 12.362090734282, 106.730166832751, 1.082132736573, 201.017726085868, 126.336743983750, 0.289751310149, 33.326945586863, 2.036606407547, 13.658771903767, 0.093146425155, 0.042145266390, 0.110301779682 };
constexpr double COEF[14] = { 0.331080339328, 0.124608304441, -0.315844760971, 0.013145445419, 0.059816014332, -0.405727171465, 0.135635635971, -2.903580645195, -0.747490805146, -0.077854288962, -0.130156314942, -6.704703351540, -0.170931983415, 3.650700535041 };
constexpr double INTERCEPT = -1.105667374608;

inline void scaleFeatures(const double raw[14], double scaled[14]) {
    for (int i = 0; i < 14; ++i) {
        scaled[i] = (raw[i] - MEAN[i]) / SCALE[i];
    }
}

} // anonymous namespace

int LogisticRegressionPredictor::predict(const double raw[N_FEATURES])
{
    double proba = predictProba(raw);
    return (proba >= 0.5) ? 1 : 0;
}

double LogisticRegressionPredictor::predictProba(const double raw[N_FEATURES])
{
    double scaled[14];
    scaleFeatures(raw, scaled);

    double logit = INTERCEPT;
    for (int i = 0; i < 14; ++i) {
        logit += COEF[i] * scaled[i];
    }

    // Sigmoid activation function
    double proba = 1.0 / (1.0 + std::exp(-logit));
    return proba;
}

} // namespace veins
