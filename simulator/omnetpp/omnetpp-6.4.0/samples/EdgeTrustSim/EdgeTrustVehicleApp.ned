// EdgeTrustVehicleApp.ned
package org.car2x.veins.modules.application.edgetrust;

import org.car2x.veins.modules.application.ieee80211p.DemoBaseApplLayer;

simple EdgeTrustVehicleApp extends DemoBaseApplLayer
{
    parameters:
        @class(veins::EdgeTrustVehicleApp);
        @display("i=block/app2");
        double maliciousRatio = default(0.25);
        int attackType = default(-1);
        bool isMalicious = default(false);
}
