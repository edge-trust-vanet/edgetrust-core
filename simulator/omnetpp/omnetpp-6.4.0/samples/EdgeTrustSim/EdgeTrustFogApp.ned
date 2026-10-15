package org.car2x.veins.modules.application.edgetrust;

simple EdgeTrustFogApp
{
    parameters:
        @class(veins::EdgeTrustFogApp);
        @display("i=device/server;is=m");
        int fogId = default(0);
        double fogSyncInterval @unit(s) = default(2.0s);
        double conflictTolerance = default(0.20);
        bool saveCheckpoint = default(true);
        bool loadCheckpoint = default(false);
        string checkpointFile = default("results/checkpoint_fog_ledger.json");
        double checkpointInterval @unit(s) = default(20s);
        double posX = default(118);
        double posY = default(15);
        double posZ = default(10);
}
