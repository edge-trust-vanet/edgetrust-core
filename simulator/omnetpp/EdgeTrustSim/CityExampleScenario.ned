//
// EdgeTrust City-Wide Scenario Network Definition (Erlangen City Map)
// 6 RSUs across 3 Regional Fog Districts + 1 Centralized SDN Controller
//

import org.car2x.veins.nodes.RSU;
import org.car2x.veins.nodes.Scenario;
import org.car2x.veins.nodes.TrafficLight;
import org.car2x.veins.modules.application.edgetrust.EdgeTrustFogApp;
import org.car2x.veins.modules.application.edgetrust.EdgeTrustSDNController;

network CityExampleScenario extends Scenario
{
    submodules:
        rsu[6]: RSU {
            @display("p=150,140;i=veins/sign/yellowdiamond;is=vs");
        }

        fog[3]: EdgeTrustFogApp {
            @display("p=118,15;i=device/server;is=s");
        }

        sdn: EdgeTrustSDNController {
            @display("p=118,8;i=device/server2;is=s");
        }

        tls[0]: TrafficLight {
        }
}
