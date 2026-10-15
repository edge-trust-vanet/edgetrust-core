#ifndef __EDGETRUSTFOGAPP_H_
#define __EDGETRUSTFOGAPP_H_

#include <omnetpp.h>
#include <omnetpp/ccanvas.h>
#include "veins/base/utils/Coord.h"
#include <map>
#include <set>
#include <vector>
#include <string>

namespace veins {

// Forward declarations
struct InterRSUDigestEntry;
class EdgeTrustRSUApp;

/**
 * @brief Telemetry observation recorded by a specific RSU for a vehicle.
 */
struct RSUObservation {
    double directTrust = 0.95;
    double histTrust = 0.95;
    double neighborTrustAvg = 0.95;
    bool isMalicious = false;
    int attackType = 0;
    std::string verdict = "ACCEPT";
    std::string modelName = "RandomForest";
    int mlPrediction = 0;
    double mlConfidence = 0.0;
    Coord lastPos;
    double lastSpeed = 0.0;
    double packetDropRatio = 0.0;
    int falsePacketInjection = 0;
    int blackholeAttempts = 0;
    int sybilAttempts = 0;
    int denialOfService = 0;
    simtime_t lastReportTime = SIMTIME_ZERO;
};

/**
 * @brief Master record maintained in the Regional Fog Ledger for each vehicle.
 */
struct FogVehicleRecord {
    int nodeId = 0;
    simtime_t firstSeen = SIMTIME_ZERO;
    simtime_t lastSeen = SIMTIME_ZERO;
    Coord lastPos;
    double lastSpeed = 0.0;

    // Per-RSU reported observations: rsuId -> RSUObservation
    std::map<int, RSUObservation> rsuObservations;

    // Consolidated Regional Trust Metrics
    double regionalTrustScore = 0.95;
    double regionalHistoricalTrust = 0.95;
    bool isRevoked = false;
    int primaryAttackType = 0;
    std::string revocationReason = "";
    simtime_t revokedAt = SIMTIME_ZERO;

    // Conflict Tracking & Resolution
    bool hasConflict = false;
    int conflictCount = 0;
    std::string lastConflictDetails = "";

    // Cumulative Cross-RSU Indicators (Multi-Intersection Detection)
    int cumulativeFalseInjections = 0;
    int cumulativeBlackholeDrops = 0;
    int cumulativeSybilAttempts = 0;
    int cumulativeDosAttempts = 0;
    int totalReportsReceived = 0;
};

/**
 * @brief Consolidated master digest entry pushed from Fog down to all RSUs.
 */
struct FogMasterDigestEntry {
    int nodeId = 0;
    double regionalTrustScore = 0.95;
    double regionalHistoricalTrust = 0.95;
    bool isRevoked = false;
    int attackType = 0;
    Coord lastPos;
    double lastSpeed = 0.0;
    simtime_t lastSeen = SIMTIME_ZERO;
    std::string verdict = "ACCEPT";
    int reportingRSUs = 0;
    bool hadConflict = false;
};

/**
 * @brief Tier-3 Regional Fog Computing Layer in EdgeTrust-VANET.
 * 
 * Aggregates multi-RSU trust digests, resolves duplicate/stale/conflicting trust updates,
 * correlates multi-intersection distributed attacks, maintains the persistent regional ledger,
 * and distributes Certificate Revocation List (CRL) blacklist orders to all intersection RSUs.
 */
class VEINS_API EdgeTrustFogApp : public omnetpp::cSimpleModule {
  public:
    EdgeTrustFogApp() = default;
    virtual ~EdgeTrustFogApp() override = default;

    // Interface called by RSUs
    virtual void receiveRSUDigest(int sourceRsuId, const std::vector<InterRSUDigestEntry>& digest);
    virtual void receiveRSUThreatAlert(int sourceRsuId, int offendingNodeId, int attackType, double confidence, const std::string& reason);

    // Monitoring getters
    int getTrackedVehicleCount() const { return static_cast<int>(regionalLedger.size()); }
    int getRevokedCount() const { return static_cast<int>(regionalRevocationList.size()); }
    int getConflictsResolvedCount() const { return totalConflictsResolved; }

  protected:
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return 2; }
    virtual void handleMessage(omnetpp::cMessage* msg) override;
    virtual void finish() override;

    // Regional aggregation & conflict resolution core
    virtual void processPeriodicRegionalAggregation();
    virtual void reconcileConflictsForNode(FogVehicleRecord& rec, int newRsuId, const InterRSUDigestEntry& entry);
    virtual void triggerRegionalRevocation(int nodeId, int attackType, double confidence, const std::string& reason);
    virtual void pushRegionalMasterDigest();

    // Checkpointing
    virtual void saveCheckpoint(const std::string& filepath);
    virtual bool loadCheckpoint(const std::string& filepath);

    // Canvas Visualization
    virtual void updateFogCanvasVisual(const std::string& type, const std::string& color, const std::string& infoText);
    virtual void clearFogCanvasVisual();
    virtual Coord getModuleVisualPos(omnetpp::cModule* mod, const Coord& fallbackPos);

    // Configuration parameters
    int fogId = 0;
    double fogSyncInterval = 2.0;       // seconds
    double conflictTolerance = 0.20;    // trust difference threshold for conflict detection
    bool saveCheckpointEnabled = true;
    bool loadCheckpointEnabled = false;
    std::string checkpointFilePath = "results/checkpoint_fog_ledger.json";
    double checkpointInterval = 20.0;   // seconds
    double posX = 118.0;
    double posY = 15.0;
    double posZ = 10.0;

    // Timers
    omnetpp::cMessage* fogSyncTimerMsg = nullptr;
    omnetpp::cMessage* checkpointTimerMsg = nullptr;

    // Master Regional State
    std::map<int, FogVehicleRecord> regionalLedger;
    std::set<int> regionalRevocationList;

    // Metrics & Statistics
    int totalRSUDigestsIngested = 0;
    int totalStaleUpdatesDropped = 0;
    int totalConflictsResolved = 0;
    int totalRevocationsIssued = 0;
    int totalMasterSyncsDispatched = 0;
    int totalCrossIntersectionAttacksDetected = 0;
};

} // namespace veins

#endif // __EDGETRUSTFOGAPP_H_
