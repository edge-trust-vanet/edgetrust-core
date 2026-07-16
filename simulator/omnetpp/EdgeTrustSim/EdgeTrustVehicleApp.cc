// EdgeTrustVehicleApp.cc
#include "EdgeTrustVehicleApp.h"
#include "veins/modules/mobility/traci/TraCIMobility.h"
#include "veins/base/utils/Coord.h"
#include "veins/modules/application/ieee80211p/BaseWaveApplLayer_m.h"

Define_Module(EdgeTrustVehicleApp);

void EdgeTrustVehicleApp::initialize(int stage) {
    BaseWaveApplLayer::initialize(stage);
    if (stage == 0) {
        vehicleId = par("vehicleId").intValue();
        beaconTimer = new cMessage("beaconTimer");
        scheduleAt(simTime() + uniform(0,1), beaconTimer); // stagger start
    }
}

void EdgeTrustVehicleApp::handleSelfMsg(cMessage *msg) {
    if (msg == beaconTimer) {
        sendBeacon();
        scheduleAt(simTime() + 1, beaconTimer);
    } else {
        BaseWaveApplLayer::handleSelfMsg(msg);
    }
}

void EdgeTrustVehicleApp::onWSM(veins::BaseFrame1609_4 *wsm) {
    // For now just log receipt – later forward to ML pipeline
    EV << "Vehicle " << vehicleId << " received beacon from " << wsm->getSenderAddress() << endl;
}

void EdgeTrustVehicleApp::sendBeacon() {
    auto wsm = new veins::BaseFrame1609_4("EdgeTrustBeacon");
    wsm->setSenderAddress(myId);
    wsm->addTag<VeinsTags::VehicleId>()->setVehicleId(vehicleId);
    sendDown(wsm);
    EV << "Vehicle " << vehicleId << " sent beacon" << endl;
}
