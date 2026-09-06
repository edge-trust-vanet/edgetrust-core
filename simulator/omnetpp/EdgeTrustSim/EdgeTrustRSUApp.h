#ifndef __EDGETRUSTRSUAPP_H_
#define __EDGETRUSTRSUAPP_H_

#include "veins/modules/application/ieee80211p/DemoBaseApplLayer.h"
#include "veins/modules/application/edgetrust/EdgeTrustSafetyMessage_m.h"
#include "veins/modules/application/edgetrust/AdaBoostPredictor.h"
#include "veins/modules/application/edgetrust/RandomForestPredictor.h"
#include <omnetpp/ccanvas.h>
#include <map>
#include <fstream>
#include <mutex>
#include <string>

namespace veins {

struct VehicleTelemetry {
    Coord lastPos;
    double lastSpeed = 0.0;
    simtime_t lastTime = SIMTIME_ZERO;
    simtime_t firstSeen = SIMTIME_ZERO;

    int packetSent = 0;
    int packetReceived = 0;
    double packetDropRatio = 0.0;
    double lastLatency = 20.0;
    int retransmissionCount = 0;
    double signalStrength = -70.0;

    double trustScore = 0.85;
    double neighborTrustScoreAvg = 0.85;
    double historicalTrustScore = 0.85;

    int falsePacketInjection = 0;
    int blackholeAttackAttempts = 0;
    int sybilAttackAttempts = 0;
    int denialOfService = 0;
    bool isMalicious = false;

    // AdaBoost ML state
    int lastMlPrediction = 0;
    double lastMlConfidence = 0.0;
    std::string lastVerdict = "ACCEPT";

    int bsmCountInLastSecond = 0;
    simtime_t secondWindowStart = SIMTIME_ZERO;
};

class VEINS_API EdgeTrustRSUApp : public DemoBaseApplLayer {
  public:
    EdgeTrustRSUApp() = default;
    virtual ~EdgeTrustRSUApp() override = default;

  protected:
    virtual void initialize(int stage) override;
    virtual void onBSM(DemoSafetyMessage* bsm) override;
    virtual void finish() override;

    virtual void broadcastSafetyAdvisory(int targetVehicleId, const std::string& verdict, double confidence);
    virtual void drawArrow(const Coord& from, const Coord& to, const std::string& color, const std::string& arrowId);

    virtual void logVehicleFeatures(int nodeId, double posX, double posY,
                                   double speed, double direction, double acceleration,
                                   int packetSent, int packetReceived, double dropRatio,
                                   double latency, int retxCount, double signalStrength,
                                   double trustScore, double neighborTrustAvg, double histTrust,
                                   int falseInjection, int blackholeAttempts, int sybilAttempts,
                                   int dosAttempts, int isMalicious);

    int rsuId = 0;
    std::string mlModel = "adaboost";
    std::string csvOutputPath;
    std::string mlDataCsvPath;
    double maxCommunicationRange = 85.0; // Realistic 802.11p RSU range (meters)

    std::map<int, VehicleTelemetry> vehicleRecords;

    static std::mutex csvFileMutex;
    static bool headerWritten;
    static int totalExtractedRecords;
};

} // namespace veins

#endif // __EDGETRUSTRSUAPP_H_
