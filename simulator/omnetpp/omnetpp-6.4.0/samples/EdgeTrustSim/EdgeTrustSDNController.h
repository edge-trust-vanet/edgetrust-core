#ifndef __EDGETRUSTSDNCONTROLLER_H_
#define __EDGETRUSTSDNCONTROLLER_H_

#include <omnetpp.h>
#include <omnetpp/ccanvas.h>
#include "veins/base/utils/Coord.h"
#include <map>
#include <set>
#include <vector>
#include <string>
#include <tuple>

namespace veins {

class EdgeTrustRSUApp;
class EdgeTrustFogApp;

/**
 * @brief Directed link state maintained by the SDN Controller.
 */
struct SDNLink {
    int fromRsu = -1;
    int toRsu = -1;
    double distance = 50.0;           // meters
    double trustScore = 1.0;          // [0.0 - 1.0]
    double pdr = 1.0;                 // Packet Delivery Ratio [0.0 - 1.0]
    double latencyMs = 2.0;           // ms
    double congestion = 0.0;          // Queue / channel utilization [0.0 - 1.0]
    bool isCompromised = false;       // Marked if attacked / blackholed
    simtime_t lastUpdated = SIMTIME_ZERO;
};

/**
 * @brief RSU node profile in the SDN topology.
 */
struct SDNNode {
    int rsuId = -1;
    Coord pos;
    bool isQuarantined = false;
    int assignedFogId = 0;
    double averageTrust = 0.95;
    int packetsForwarded = 0;
    int packetsDropped = 0;
};

/**
 * @brief Regional Fog node registered with the SDN Controller.
 */
struct SDNFogNode {
    int fogId = -1;
    Coord pos;
    std::vector<int> managedRsus;
    int activeVehicles = 0;
    int activeRevocations = 0;
    simtime_t lastSync = SIMTIME_ZERO;
};

/**
 * @brief OpenFlow-style match key for RSU packet forwarding.
 */
struct FlowKey {
    int srcRsu = 0;
    int dstRsu = 0;
    int trafficClass = 0; // 0=Safety Critical, 1=Trust/Control, 2=Best Effort

    bool operator<(const FlowKey& o) const {
        return std::tie(srcRsu, dstRsu, trafficClass) < std::tie(o.srcRsu, o.dstRsu, o.trafficClass);
    }
};

/**
 * @brief Installed forwarding rule computed by SDN Controller.
 */
struct FlowRule {
    int ruleId = 0;
    int srcRsu = 0;
    int dstRsu = 0;
    int trafficClass = 0;
    int nextHop = -1;
    std::vector<int> fullPath;
    bool isDetour = false;
    double computedCost = 0.0;
    simtime_t installedAt = SIMTIME_ZERO;
    simtime_t expiresAt = SIMTIME_ZERO;
    int packetsMatched = 0;
    std::string reason = "SHORTEST_TRUSTED";
};

/**
 * @brief Software-Defined Networking (SDN) Controller for EdgeTrust-VANET.
 * 
 * Centralized control plane that maintains global network topology across all
 * Fog clusters and RSUs, dynamically computes optimal routing paths, executes
 * trust-adaptive detour rerouting around compromised or degraded nodes,
 * and enforces QoS priority queuing.
 */
class VEINS_API EdgeTrustSDNController : public omnetpp::cSimpleModule {
  public:
    EdgeTrustSDNController() = default;
    virtual ~EdgeTrustSDNController() override = default;

    // Node & Fog Registration Interface
    virtual void registerRSU(int rsuId, const Coord& pos, int fogId);
    virtual void registerFog(int fogId, const Coord& pos, const std::vector<int>& managedRsus);

    // Dynamic Telemetry & Threat Reporting
    virtual void updateLinkMetrics(int fromRsu, int toRsu, double trustScore, double pdr, double latencyMs, double congestion);
    virtual void notifyNodeThreat(int reporterId, int offendingId, int attackType, double confidence, const std::string& reason);
    virtual void reportDeliveryOutcome(int srcRsu, int dstRsu, int trafficClass, bool delivered, double latencySec, bool wasDetour);

    // Forwarding & Flow Table Resolution
    virtual int resolveNextHop(int currentRsu, int dstRsu, int trafficClass, bool& outIsDetour, std::vector<int>& outFullPath);
    virtual std::vector<int> computeAdaptivePath(int srcRsu, int dstRsu, int trafficClass, double& outCost, bool& outIsDetour);

    // Inspection
    double getLinkTrust(int u, int v) const;
    bool isNodeQuarantined(int rsuId) const;

  protected:
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return 2; }
    virtual void handleMessage(omnetpp::cMessage* msg) override;
    virtual void finish() override;

    // Core Topology & Routing Logic
    virtual void initializeDefaultTopology();
    virtual void triggerGlobalRecomputation(const std::string& triggerReason);
    virtual void pushFlowModToRSUs(const FlowRule& rule);

    // Canvas Visualization
    virtual void updateSDNCanvasVisual();
    virtual void clearSDNCanvasVisual();

    // Module Parameters
    int numRSUs = 4;
    int numFogs = 2;
    double flowRuleTimeout = 10.0;       // seconds
    double linkTrustThreshold = 0.60;     // threshold under which detour is mandatory
    double controllerTickInterval = 1.0; // seconds
    bool enableAdaptiveRerouting = true;
    bool enableQoSPriority = true;
    double posX = 118.0;
    double posY = 5.0;
    double posZ = 15.0;

    // Timers
    omnetpp::cMessage* sdnTickTimerMsg = nullptr;

    // Global State
    std::map<std::pair<int, int>, SDNLink> linkTopology;
    std::map<int, SDNNode> rsuNodes;
    std::map<int, SDNFogNode> fogNodes;
    std::map<FlowKey, FlowRule> globalFlowTable;
    std::set<int> quarantinedNodes;

    // Statistics
    int totalPacketsRouted = 0;
    int safetyPacketsRouted = 0;
    int safetyPacketsDelivered = 0;
    int controlPacketsRouted = 0;
    int bestEffortPacketsRouted = 0;
    int detoursActivated = 0;
    int flowRulesInstalled = 0;
    int untrustedLinksBypassed = 0;
    double totalLatencyMsSum = 0.0;
    int deliveredPacketCount = 0;
};

} // namespace veins

#endif // __EDGETRUSTSDNCONTROLLER_H_
