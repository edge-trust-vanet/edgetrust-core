// EdgeTrustRSUApp.cc
#include "EdgeTrustRSUApp.h"

Define_Module(EdgeTrustRSUApp);

void EdgeTrustRSUApp::initialize(int stage) {
    veins::DemoBaseApplLayer::initialize(stage);
    if (stage == 0) {
        rsuId = par("rsuId").intValue();
    }
}

void EdgeTrustRSUApp::onBSM(veins::DemoSafetyMessage *bsm) {
    EV << "RSU " << rsuId << " received beacon at " << simTime() << endl;
}
