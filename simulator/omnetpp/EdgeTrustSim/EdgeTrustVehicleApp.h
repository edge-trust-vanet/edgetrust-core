#ifndef __EDGETRUSTVEHICLEAPP_H_
#define __EDGETRUSTVEHICLEAPP_H_

#include "veins/modules/application/ieee80211p/DemoBaseApplLayer.h"
#include "veins/modules/application/edgetrust/EdgeTrustSafetyMessage_m.h"
#include <omnetpp/ccanvas.h>
#include <map>
#include <deque>
#include <string>

namespace veins {

// VeReMi-NextGen & VANET Complete Attack Types
enum VeReMiAttackType {
    ATTACK_NONE = 0,
    ATTACK_CONST_POS_OFFSET = 1,       // 1. constantPositionOffset (+28m)
    ATTACK_RANDOM_POS_OFFSET = 2,      // 2. randomPositionOffset (+-20m to +-70m)
    ATTACK_POS_MIRRORING = 3,          // 3. positionMirroring (reflected across road)
    ATTACK_CONST_SPEED_OFFSET = 4,     // 4. constantSpeedOffset (+12 m/s)
    ATTACK_RANDOM_SPEED_OFFSET = 5,    // 5. randomSpeedOffset (+-6 m/s)
    ATTACK_ZERO_SPEED_REPORT = 6,      // 6. zeroSpeedReport (0 m/s while moving)
    ATTACK_SUDDEN_STOP = 7,            // 7. suddenStop (frozen position & 0 m/s)
    ATTACK_SUDDEN_CONST_SPEED = 8,     // 8. suddenConstantSpeed (speed frozen at trigger value)
    ATTACK_REVERSED_HEADING = 9,       // 9. reversedHeading (heading + 180 deg)
    ATTACK_FEIGNED_BRAKING = 10,       // 10. feignedBraking (negative accel reported while accelerating)
    ATTACK_ACCEL_MULT = 11,            // 11. accelerationMultiplication (accel * 3)
    ATTACK_DOS_FLOOD = 12,             // 12. dosAttack (burst flooding at 16 Hz)
    ATTACK_SYBIL_PHANTOM = 13,         // 13. trafficCongestionSybil (phantom IDs with jitter)
    ATTACK_DATA_REPLAY = 14,           // 14. dataReplay (replaying stale packet from history)
    ATTACK_TIME_DELAY = 15,            // 15. timeDelayAttack (+3.0s latency offset)
    ATTACK_BLACKHOLE = 16              // 16. blackhole (selective 70% packet dropping)
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

    // VeReMi suddenConstantSpeed state
    bool constSpeedInitialized = false;
    double frozenSpeed = 0.0;

    // VeReMi timeDelay / dataReplay state
    std::deque<StaleBeaconData> staleHistory;

    std::map<int, bool> suspiciousNodes;
};

} // namespace veins

#endif // __EDGETRUSTVEHICLEAPP_H_
