// EdgeTrustNetwork.ned

import org.car2x.veins.nodes.Scenario;
import org.car2x.veins.nodes.RSU;
import org.car2x.veins.modules.application.ieee80211p.DemoBaseApplLayer;

simple EdgeTrustVehicleApp extends DemoBaseApplLayer {
    parameters:
        @class(EdgeTrustVehicleApp);
        int vehicleId = default(0);
}

simple EdgeTrustRSUApp extends DemoBaseApplLayer {
    parameters:
        @class(EdgeTrustRSUApp);
        int rsuId = default(0);
}

network EdgeTrustNetwork extends Scenario {
    submodules:
        rsu[2]: RSU {
            @display("p=150,140;i=veins/sign/yellowdiamond;is=vs");
        }
}
