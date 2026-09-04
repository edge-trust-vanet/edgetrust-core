#ifndef __EDGETRUSTVEHICLEAPP_H_
#define __EDGETRUSTVEHICLEAPP_H_

#include "veins/modules/application/ieee80211p/DemoBaseApplLayer.h"
#include "veins/modules/application/edgetrust/EdgeTrustSafetyMessage_m.h"
#include <omnetpp/ccanvas.h>
#include <map>
#include <string>

namespace veins {

class VEINS_API EdgeTrustVehicleApp : public DemoBaseApplLayer {
  public:
    EdgeTrustVehicleApp() = default;
    virtual ~EdgeTrustVehicleApp() override = default;

  protected:
    virtual void initialize(int stage) override;
    virtual void handleSelfMsg(cMessage* msg) override;
    virtual void onBSM(DemoSafetyMessage* bsm) override;
    virtual void finish() override;

    virtual void populateEdgeTrustMessage(EdgeTrustSafetyMessage* bsm);
    virtual bool lightweightTrustFilter(DemoSafetyMessage* bsm);
    virtual void drawArrow(const Coord& from, const Coord& to, const std::string& color, const std::string& arrowId);

    int vehicleId = 0;
    bool isMalicious = false;
    int attackType = 0; // 0=None, 1=FDI, 2=Blackhole, 3=Sybil, 4=DoS
    double maliciousRatio = 0.25;
    int attackStartMessage = 3; // Attack triggers after 2 to 3 message exchanges
    double maxCommunicationRange = 85.0; // Realistic 802.11p urban DSRC range (meters)

    int sequenceNumber = 0;
    int totalPacketsSent = 0;
    int totalPacketsDropped = 0;

    Coord lastPos;
    double lastSpeed = 0.0;
    simtime_t lastTime = SIMTIME_ZERO;

    std::map<int, bool> suspiciousNodes;
};

} // namespace veins

#endif // __EDGETRUSTVEHICLEAPP_H_
