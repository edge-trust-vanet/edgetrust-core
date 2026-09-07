#ifndef __EDGETRUSTRSUAPP_H_
#define __EDGETRUSTRSUAPP_H_

#include "veins/modules/application/ieee80211p/DemoBaseApplLayer.h"
#include "veins/modules/application/edgetrust/EdgeTrustSafetyMessage_m.h"
#include "veins/modules/application/edgetrust/LightGBMPredictor.h"
#include "veins/modules/application/edgetrust/RandomForestPredictor.h"
#include <omnetpp/ccanvas.h>
#include <map>
#include <fstream>
#include <mutex>
#include <string>

namespace veins {

struct VehicleTelemetry {
    int nodeId = 0;
    Coord lastPos;
    double lastSpeed = 0.0;
    double lastDirection = 0.0;
    double lastAcceleration = 0.0;
    simtime_t lastTime = SIMTIME_ZERO;
    simtime_t firstSeen = SIMTIME_ZERO;

    int packetSent = 0;
    int packetReceived = 0;
    double packetDropRatio = 0.0;
    double lastLatency = 20.0;
    int retransmissionCount = 0;
    double signalStrength = -70.0;

    // Detailed Trust Evidence Factors
    double lastConsistency = 0.95;
    double lastPlausibility = 0.95;
    double lastCommScore = 0.95;
    double lastNeighborValidation = 0.95;
    double lastEvidenceScore = 0.95;

    // Dynamic Multi-Tier Trust Scores
    double trustScore = 0.95;
    double neighborTrustScoreAvg = 0.95;
    double historicalTrustScore = 0.95;

    // Detected Attack Indicators & Anomalies
    int falsePacketInjection = 0;
    int blackholeAttackAttempts = 0;
    int sybilAttackAttempts = 0;
    int denialOfService = 0;
    bool isMalicious = false;
    int attackType = 0;

    // Edge ML Classification State
    int lastMlPrediction = 0;
    double lastMlConfidence = 0.0;
    std::string lastVerdict = "ACCEPT";
    std::string lastModelName = "RandomForest";

    int totalEvaluations = 0;
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
    virtual void handleSelfMsg(cMessage* msg) override;
    virtual void finish() override;

    virtual void broadcastSafetyAdvisory(int targetVehicleId, const std::string& verdict, double confidence);
    virtual void clearTransmissionArrows();
    virtual void addTransmissionArrow(const Coord& from, const Coord& to, const std::string& color);
    virtual Coord getModuleVisualPos(cModule* mod, const Coord& fallbackPos);

    virtual void logVehicleFeatures(int nodeId, double posX, double posY,
                                   double speed, double direction, double acceleration,
                                   int packetSent, int packetReceived, double dropRatio,
                                   double latency, int retxCount, double signalStrength,
                                   double trustScore, double neighborTrustAvg, double histTrust,
                                   int falseInjection, int blackholeAttempts, int sybilAttempts,
                                   int dosAttempts, int isMalicious);

    virtual void saveCheckpoint(const std::string& filepath);
    virtual bool loadCheckpoint(const std::string& filepath);

    int rsuId = 0;
    std::string mlModel = "random_forest"; // "random_forest" or "lightgbm"
    std::string csvOutputPath;
    std::string mlDataCsvPath;
    double maxCommunicationRange = 85.0; // Realistic 802.11p RSU range (meters)

    // Checkpoint configuration
    bool saveCheckpointEnabled = true;
    bool loadCheckpointEnabled = false;
    std::string checkpointFilePath = "results/checkpoint_state.json";
    double checkpointInterval = 25.0; // seconds
    cMessage* checkpointTimerMsg = nullptr;

    std::map<int, VehicleTelemetry> vehicleRecords;

    static std::mutex csvFileMutex;
    static bool headerWritten;
    static int totalExtractedRecords;
};

} // namespace veins

#endif // __EDGETRUSTRSUAPP_H_
