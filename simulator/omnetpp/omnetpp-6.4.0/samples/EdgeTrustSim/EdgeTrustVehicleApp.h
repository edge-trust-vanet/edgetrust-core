// EdgeTrustVehicleApp.h
#ifndef __EDGETRUSTVEHICLEAPP_H_
#define __EDGETRUSTVEHICLEAPP_H_

#include "veins/modules/application/ieee80211p/DemoBaseApplLayer.h"
#include "veins/modules/messages/DemoSafetyMessage_m.h"

class EdgeTrustVehicleApp : public veins::DemoBaseApplLayer {
  protected:
    virtual void initialize(int stage) override;
    virtual void onBSM(veins::DemoSafetyMessage *bsm) override;
    virtual void sendDown(cMessage *msg) override;

    int vehicleId = 0;
    bool isMalicious = false;
};

#endif // __EDGETRUSTVEHICLEAPP_H_
