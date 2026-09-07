// EdgeTrustRSUApp.ned
package org.car2x.veins.modules.application.edgetrust;

import org.car2x.veins.modules.application.ieee80211p.DemoBaseApplLayer;

simple EdgeTrustRSUApp extends DemoBaseApplLayer
{
    parameters:
        @class(veins::EdgeTrustRSUApp);
        @display("i=block/app2");
        int rsuId = default(0);
        string mlModel = default("random_forest"); // "random_forest" or "lightgbm"
        string csvOutputPath = default("results/live_extracted_features.csv");
        string mlDataCsvPath = default("../../../../../edgetrust-ml/data/live_extracted_features.csv");
        double maxCommunicationRange @unit(m) = default(85m);
        bool saveCheckpoint = default(true);
        bool loadCheckpoint = default(false);
        string checkpointFile = default("results/checkpoint_state.json");
        double checkpointInterval @unit(s) = default(25s);
}
