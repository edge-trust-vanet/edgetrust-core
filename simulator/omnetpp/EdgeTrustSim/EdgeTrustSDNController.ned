// EdgeTrustSDNController.ned
package org.car2x.veins.modules.application.edgetrust;

simple EdgeTrustSDNController
{
    parameters:
        @class(veins::EdgeTrustSDNController);
        @display("i=device/server2;is=l");
        int numRSUs = default(4);
        int numFogs = default(2);
        double flowRuleTimeout @unit(s) = default(10.0s);
        double linkTrustThreshold = default(0.60);
        double controllerTickInterval @unit(s) = default(1.0s);
        bool enableAdaptiveRerouting = default(true);
        bool enableQoSPriority = default(true);
        double posX = default(118);
        double posY = default(5);
        double posZ = default(15);
}
