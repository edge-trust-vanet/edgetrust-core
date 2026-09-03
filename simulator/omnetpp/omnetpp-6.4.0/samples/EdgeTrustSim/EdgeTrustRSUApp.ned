// EdgeTrustRSUApp.ned
package org.car2x.veins.modules.application.edgetrust;

import org.car2x.veins.modules.application.ieee80211p.DemoBaseApplLayer;

simple EdgeTrustRSUApp extends DemoBaseApplLayer
{
    parameters:
        @class(veins::EdgeTrustRSUApp);
        @display("i=block/app2");
        int rsuId = default(0);
        string csvOutputPath = default("results/live_extracted_features.csv");
        string mlDataCsvPath = default("../../../../../edgetrust-ml/data/live_extracted_features.csv");
}
