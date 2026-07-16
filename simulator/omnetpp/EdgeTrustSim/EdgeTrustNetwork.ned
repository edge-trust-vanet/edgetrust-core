// EdgeTrustNetwork.ned
package edgetrustsim;

import veins.*;

// Simple vehicle and RSU modules that extend Veins base application layer
simple EdgeTrustVehicle extends veins::BaseWaveApplLayer {
    parameters:
        @display("i=car");
        int vehicleId = default(0);
}

simple EdgeTrustRSU extends veins::BaseWaveApplLayer {
    parameters:
        @display("i=router");
        int rsuId = default(0);
}

network EdgeTrustNetwork {
    types:
        channel defaultChannel extends ned.DatarateChannel {
            datarate = 2Mbps;
            delay = 100us;
        }
    submodules:
        // Vehicles – you can tune the amount later via the ini file
        vehicle[10]: EdgeTrustVehicle {
            @display("p=100,100,i=car");
            parameters:
                vehicleId = index;
        }
        // RSUs – fixed infrastructure nodes
        rsu[2]: EdgeTrustRSU {
            @display("p=200,200,i=router");
            parameters:
                rsuId = index;
        }
}
