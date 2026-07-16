// EdgeTrustVehicleApp.h
#ifndef __EDGETRUSTVEHICLEAPP_H_
#define __EDGETRUSTVEHICLEAPP_H_

#include "veins/base/modules/BaseWaveApplLayer.h"

/**
 * Minimal Vehicle Application for EdgeTrust simulation.
 * Sends a simple beacon every second containing its ID.
 */
class EdgeTrustVehicleApp : public veins::BaseWaveApplLayer {
  protected:
    virtual void initialize(int stage) override;
    virtual void handleSelfMsg(cMessage *msg) override;
    virtual void onWSM(veins::BaseFrame1609_4 *wsm) override;
    void sendBeacon();
    cMessage *beaconTimer = nullptr;
    int vehicleId = 0;
};

#endif // __EDGETRUSTVEHICLEAPP_H_
