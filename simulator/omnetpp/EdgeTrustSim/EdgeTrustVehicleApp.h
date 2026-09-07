#ifndef __EDGETRUSTVEHICLEAPP_H_
#define __EDGETRUSTVEHICLEAPP_H_

#include "veins/modules/application/ieee80211p/DemoBaseApplLayer.h"
#include "veins/modules/application/edgetrust/EdgeTrustSafetyMessage_m.h"
#include <omnetpp/ccanvas.h>
#include <map>
#include <deque>
#include <string>

namespace veins {

// VeReMi & VANET Attack Types
enum VeReMiAttackType {
    ATTACK_NONE = 0,
    ATTACK_FDI_POS_OFFSET = 1,       // VeReMi constant/random position offset (+28m)
    ATTACK_BLACKHOLE = 2,            // VeReMi / VANET blackhole packet drop (drops 70% of beacons)
    ATTACK_SYBIL_PHANTOM = 3,        // VeReMi traffic congestion sybil (phantom vehicle IDs)
    ATTACK_DOS_FLOOD = 4,            // VeReMi dosAttack (high-frequency burst flooding at 16 Hz)
    ATTACK_FDI_SUDDEN_STOP = 5,      // VeReMi suddenStop / zeroSpeedReport (freezes position & reports 0 speed while moving)
    ATTACK_TIME_DELAY_REPLAY = 6     // VeReMi timeDelayAttack / dataReplay (artificial 3s latency & stale beacons)
};

struct StaleBeaconData {
    simtime_t creationTime;
    Coord pos;
    Coord speed;
    double heading;
    double accel;
    int seq;
};

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
    int attackType = 0;
    std::string scenarioMode = "mixed";
    double maliciousRatio = 0.25;
    int attackStartMessage = 3; // Attack triggers after 2 to 3 message exchanges
    double maxCommunicationRange = 85.0; // Realistic 802.11p urban DSRC range (meters)

    int sequenceNumber = 0;
    int totalPacketsSent = 0;
    int totalPacketsDropped = 0;

    Coord lastPos;
    double lastSpeed = 0.0;
    simtime_t lastTime = SIMTIME_ZERO;

    // VeReMi suddenStop state
    bool suddenStopInitialized = false;
    Coord suddenStopPos;

    // VeReMi timeDelay / dataReplay state
    std::deque<StaleBeaconData> staleHistory;

    std::map<int, bool> suspiciousNodes;
};

} // namespace veins

#endif // __EDGETRUSTVEHICLEAPP_H_
