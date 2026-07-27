// EdgeTrustRSUApp.h
#ifndef __EDGETRUSTRSUAPP_H_
#define __EDGETRUSTRSUAPP_H_

#include "veins/modules/application/ieee80211p/DemoBaseApplLayer.h"
#include "veins/modules/messages/DemoSafetyMessage_m.h"
#include <map>
#include <fstream>

struct VehicleHistory {
    veins::Coord lastPos;
    simtime_t lastTime;
    double trustScore = 1.0;
};

class EdgeTrustRSUApp : public veins::DemoBaseApplLayer {
  protected:
    virtual void initialize(int stage) override;
    virtual void onBSM(veins::DemoSafetyMessage *bsm) override;
    virtual void finish() override;

    int rsuId = 0;
    std::map<int, VehicleHistory> vehicleRecords;
    std::ofstream trustLogFile;
};

#endif // __EDGETRUSTRSUAPP_H_
