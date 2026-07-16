// EdgeTrustRSUApp.cc
#include "EdgeTrustRSUApp.h"
#include "veins/base/utils/Coord.h"
#include "veins/modules/application/ieee80211p/BaseWaveApplLayer_m.h"

Define_Module(EdgeTrustRSUApp);

void EdgeTrustRSUApp::initialize(int stage) {
    BaseWaveApplLayer::initialize(stage);
    if (stage == 0) {
        rsuId = par("rsuId").intValue();
    }
}

void EdgeTrustRSUApp::onWSM(veins::BaseFrame1609_4 *wsm) {
    // Log receipt of beacon
    EV << "RSU " << rsuId << " received beacon from vehicle " << wsm->getSenderAddress() << endl;
    // Future: forward data to EdgeTrust ML module.
}
