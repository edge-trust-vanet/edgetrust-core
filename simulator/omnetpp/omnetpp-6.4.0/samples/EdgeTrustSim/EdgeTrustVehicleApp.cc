// EdgeTrustVehicleApp.cc
#include "EdgeTrustVehicleApp.h"

Define_Module(EdgeTrustVehicleApp);

void EdgeTrustVehicleApp::initialize(int stage) {
    veins::DemoBaseApplLayer::initialize(stage);
    if (stage == 0) {
        vehicleId = par("vehicleId").intValue();
    }
}

void EdgeTrustVehicleApp::onBSM(veins::DemoSafetyMessage *bsm) {
    EV << "Vehicle " << vehicleId << " received beacon at " << simTime() << endl;
}
