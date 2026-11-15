//
// EdgeTrust Highway Scenario Network Definition
// 4 RSUs along highway corridor + 2 Regional Fog Districts + 1 Central SDN Controller
//

import org.car2x.veins.nodes.RSU;
import org.car2x.veins.nodes.Scenario;
import org.car2x.veins.nodes.TrafficLight;
import org.car2x.veins.modules.application.edgetrust.EdgeTrustFogApp;
import org.car2x.veins.modules.application.edgetrust.EdgeTrustSDNController;

network HighwayScenario extends Scenario
{
    submodules:
        rsu[4]: RSU {
            @display("p=150,140;i=veins/sign/yellowdiamond;is=vs");
        }

        fog[2]: EdgeTrustFogApp {
            @display("p=118,15;i=device/server;is=s");
        }

        sdn: EdgeTrustSDNController {
            @display("p=118,8;i=device/server2;is=s");
        }

        tls[0]: TrafficLight {
        }
}
