// EdgeTrustVehicleApp.cc
#include "EdgeTrustVehicleApp.h"

Define_Module(EdgeTrustVehicleApp);

void EdgeTrustVehicleApp::initialize(int stage) {
    veins::DemoBaseApplLayer::initialize(stage);
    if (stage == 0) {
        vehicleId = par("vehicleId").intValue();
        // Set 25% of vehicles as malicious (e.g., node 0, 4, 8, etc.)
        if (vehicleId % 4 == 0) {
            isMalicious = true;
            EV << "Vehicle " << vehicleId << " initialized as a MALICIOUS node." << endl;
        } else {
            EV << "Vehicle " << vehicleId << " initialized as a LEGITIMATE node." << endl;
        }
    }
}

void EdgeTrustVehicleApp::sendDown(cMessage *msg) {
    veins::DemoSafetyMessage *bsm = dynamic_cast<veins::DemoSafetyMessage*>(msg);
    if (bsm) {
        // Embed the vehicle ID in the message name to pass it to the RSU without altering Veins .msg files
        std::string newName = "BSM-" + std::to_string(vehicleId);
        bsm->setName(newName.c_str());

        if (isMalicious) {
            // False Data Injection (FDI): Report falsified coordinates and speed (e.g. stopped)
            veins::Coord fakePos = bsm->getSenderPos();
            fakePos.x += 150; // Falsify position by 150m
            bsm->setSenderPos(fakePos);
            
            veins::Coord fakeSpeed(0, 0, 0); // Pretend to be completely stopped
            bsm->setSenderSpeed(fakeSpeed);
            
            EV << "Malicious Vehicle " << vehicleId << " falsifying BSM data!" << endl;
        }
    }
    veins::DemoBaseApplLayer::sendDown(msg);
}

void EdgeTrustVehicleApp::onBSM(veins::DemoSafetyMessage *bsm) {
    EV << "Vehicle " << vehicleId << " received beacon at " << simTime() << endl;
}
