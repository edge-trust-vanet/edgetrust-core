// EdgeTrustSDNController.cc
#include "veins/modules/application/edgetrust/EdgeTrustSDNController.h"
#include "veins/modules/application/edgetrust/EdgeTrustRSUApp.h"
#include "veins/modules/application/edgetrust/EdgeTrustFogApp.h"
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <queue>
#include <algorithm>

using namespace omnetpp;

namespace veins {

Define_Module(veins::EdgeTrustSDNController);

void EdgeTrustSDNController::initialize(int stage)
{
    if (stage == 0) {
        numRSUs = hasPar("numRSUs") ? par("numRSUs").intValue() : 4;
        numFogs = hasPar("numFogs") ? par("numFogs").intValue() : 2;
        flowRuleTimeout = hasPar("flowRuleTimeout") ? par("flowRuleTimeout").doubleValue() : 10.0;
        linkTrustThreshold = hasPar("linkTrustThreshold") ? par("linkTrustThreshold").doubleValue() : 0.60;
        controllerTickInterval = hasPar("controllerTickInterval") ? par("controllerTickInterval").doubleValue() : 1.0;
        enableAdaptiveRerouting = hasPar("enableAdaptiveRerouting") ? par("enableAdaptiveRerouting").boolValue() : true;
        enableQoSPriority = hasPar("enableQoSPriority") ? par("enableQoSPriority").boolValue() : true;
        posX = hasPar("posX") ? par("posX").doubleValue() : 118.0;
        posY = hasPar("posY") ? par("posY").doubleValue() : 5.0;
        posZ = hasPar("posZ") ? par("posZ").doubleValue() : 15.0;

        initializeDefaultTopology();

        sdnTickTimerMsg = new cMessage("sdnTickTimer");
        scheduleAt(simTime() + controllerTickInterval, sdnTickTimerMsg);

        EV_INFO << "============================================================" << endl;
        EV_INFO << " [SDN CONTROLLER INITIALIZED] Tier-4 Centralized Control Plane" << endl;
        EV_INFO << "   -> Managed RSUs: " << numRSUs << " | Regional Fog Nodes: " << numFogs << endl;
        EV_INFO << "   -> Link Trust Threshold: " << linkTrustThreshold << endl;
        EV_INFO << "   -> Adaptive Rerouting: " << (enableAdaptiveRerouting ? "ENABLED" : "DISABLED") << endl;
        EV_INFO << "   -> Dynamic QoS Prioritization: " << (enableQoSPriority ? "ENABLED" : "DISABLED") << endl;
        EV_INFO << "============================================================" << endl;
    }
    else if (stage == 1) {
        // Stage 1: Auto-discover RSUs and Fog nodes in the network
        cModule* parent = getParentModule();
        if (parent) {
            for (int i = 0; i < numRSUs; ++i) {
                cModule* rsuMod = parent->getSubmodule("rsu", i);
                if (rsuMod) {
                    Coord rsuPos;
                    if (rsuMod->findSubmodule("mobility") != -1) {
                        cModule* mob = rsuMod->getSubmodule("mobility");
                        if (mob) {
                            rsuPos.x = mob->hasPar("x") ? mob->par("x").doubleValue() : (i < 2 ? 58.0 : 178.0);
                            rsuPos.y = mob->hasPar("y") ? mob->par("y").doubleValue() : ((i % 2 == 0) ? 49.0 : 98.0);
                            rsuPos.z = mob->hasPar("z") ? mob->par("z").doubleValue() : 3.0;
                        }
                    }
                    int assignedFog = (i < 2) ? 0 : 1;
                    registerRSU(i, rsuPos, assignedFog);
                }
            }

            for (int f = 0; f < numFogs; ++f) {
                cModule* fogMod = parent->getSubmodule("fog", f);
                if (fogMod) {
                    Coord fogPos(f == 0 ? 58.0 : 178.0, 15.0, 10.0);
                    std::vector<int> managed;
                    if (f == 0) { managed.push_back(0); managed.push_back(1); }
                    else { managed.push_back(2); managed.push_back(3); }
                    registerFog(f, fogPos, managed);
                }
            }
        }

        // Install baseline flow rules
        triggerGlobalRecomputation("Initial Baseline Network Formation");
        updateSDNCanvasVisual();
    }
}

void EdgeTrustSDNController::initializeDefaultTopology()
{
    // Define 4-RSU grid topology:
    // Cluster 0 (Grid 1): RSU 0 (58, 49) [South], RSU 1 (58, 98) [North]
    // Cluster 1 (Grid 2): RSU 2 (178, 49) [South], RSU 3 (178, 98) [North]

    auto addBidirectionalLink = [this](int u, int v, double dist) {
        SDNLink l1;
        l1.fromRsu = u;
        l1.toRsu = v;
        l1.distance = dist;
        l1.trustScore = 1.0;
        l1.pdr = 1.0;
        l1.latencyMs = 2.0;
        l1.congestion = 0.0;
        l1.isCompromised = false;
        l1.lastUpdated = simTime();
        linkTopology[{u, v}] = l1;

        SDNLink l2 = l1;
        l2.fromRsu = v;
        l2.toRsu = u;
        linkTopology[{v, u}] = l2;
    };

    // Intra-Cluster Links (vertical corridors)
    addBidirectionalLink(0, 1, 49.0);
    addBidirectionalLink(2, 3, 49.0);

    // Inter-Cluster Links (horizontal cross-grid corridors)
    addBidirectionalLink(0, 2, 120.0); // South corridor (primary)
    addBidirectionalLink(1, 3, 120.0); // North corridor (alternate detour)

    // Diagonal Cross-Links (backhaul mesh redundancy)
    addBidirectionalLink(0, 3, 130.0);
    addBidirectionalLink(1, 2, 130.0);

    // Initialize node profiles
    for (int i = 0; i < numRSUs; ++i) {
        SDNNode node;
        node.rsuId = i;
        node.pos = Coord((i < 2 ? 58.0 : 178.0), ((i % 2 == 0) ? 49.0 : 98.0), 3.0);
        node.isQuarantined = false;
        node.assignedFogId = (i < 2) ? 0 : 1;
        node.averageTrust = 1.0;
        rsuNodes[i] = node;
    }
}

void EdgeTrustSDNController::registerRSU(int rsuId, const Coord& pos, int fogId)
{
    Enter_Method_Silent();
    SDNNode& node = rsuNodes[rsuId];
    node.rsuId = rsuId;
    node.pos = pos;
    node.assignedFogId = fogId;
    EV_INFO << "[SDN] Registered RSU " << rsuId << " at (" << pos.x << ", " << pos.y
            << ") assigned to Fog Cluster " << fogId << endl;
}

void EdgeTrustSDNController::registerFog(int fogId, const Coord& pos, const std::vector<int>& managedRsus)
{
    Enter_Method_Silent();
    SDNFogNode fn;
    fn.fogId = fogId;
    fn.pos = pos;
    fn.managedRsus = managedRsus;
    fn.lastSync = simTime();
    fogNodes[fogId] = fn;
    EV_INFO << "[SDN] Registered Fog Cluster " << fogId << " managing " << managedRsus.size() << " RSUs." << endl;
}

void EdgeTrustSDNController::handleMessage(cMessage* msg)
{
    if (msg == sdnTickTimerMsg) {
        // Periodic Controller Tick: inspect network state, flush expired rules, update visual
        simtime_t now = simTime();

        // Evict expired flow rules
        for (auto it = globalFlowTable.begin(); it != globalFlowTable.end(); ) {
            if (it->second.expiresAt > SIMTIME_ZERO && it->second.expiresAt < now) {
                it = globalFlowTable.erase(it);
            } else {
                ++it;
            }
        }

        updateSDNCanvasVisual();
        scheduleAt(simTime() + controllerTickInterval, sdnTickTimerMsg);
    } else {
        delete msg;
    }
}

void EdgeTrustSDNController::updateLinkMetrics(int fromRsu, int toRsu, double trustScore, double pdr, double latencyMs, double congestion)
{
    Enter_Method_Silent();
    auto it = linkTopology.find({fromRsu, toRsu});
    if (it != linkTopology.end()) {
        SDNLink& link = it->second;
        double oldTrust = link.trustScore;
        link.trustScore = trustScore;
        link.pdr = pdr;
        link.latencyMs = latencyMs;
        link.congestion = congestion;
        link.lastUpdated = simTime();

        // Detect severe degradation or compromise
        if (trustScore < linkTrustThreshold && oldTrust >= linkTrustThreshold) {
            link.isCompromised = true;
            EV_WARN << "============================================================" << endl;
            EV_WARN << " [SDN TOPOLOGY ALERT] Link (" << fromRsu << " -> " << toRsu
                    << ") Trust DEGRADED below threshold (" << trustScore << " < " << linkTrustThreshold << ")!" << endl;
            EV_WARN << "   -> Triggering Trust-Adaptive Detour Rerouting across network..." << endl;
            EV_WARN << "============================================================" << endl;
            triggerGlobalRecomputation("Link Trust Degradation (" + std::to_string(fromRsu) + "->" + std::to_string(toRsu) + ")");
        }
    }
}

void EdgeTrustSDNController::notifyNodeThreat(int reporterId, int offendingId, int attackType, double confidence, const std::string& reason)
{
    Enter_Method_Silent();
    EV_WARN << "============================================================" << endl;
    EV_WARN << " [SDN CONTROL PLANE THREAT INGESTION] Incident reported by Node " << reporterId << endl;
    EV_WARN << "   -> Offending Node / Target: " << offendingId << " | Attack Type: " << attackType << endl;
    EV_WARN << "   -> Confidence: " << confidence << " | Reason: " << reason << endl;

    if (offendingId >= 0 && offendingId < numRSUs) {
        // An entire RSU is compromised / quarantined
        quarantinedNodes.insert(offendingId);
        rsuNodes[offendingId].isQuarantined = true;
        for (auto& kv : linkTopology) {
            if (kv.first.first == offendingId || kv.first.second == offendingId) {
                kv.second.isCompromised = true;
                kv.second.trustScore = 0.05;
            }
        }
        EV_WARN << "   -> SDN ACTION: RSU " << offendingId << " QUARANTINED! All incident links isolated." << endl;
    } else {
        // Vehicle-level threat in reporterId's sector
        if (reporterId >= 0 && reporterId < numRSUs) {
            // Penalize links passing through reporterId if attack is severe (e.g. DoS / Blackhole)
            if (attackType == 2 || attackType == 4 || attackType == 12 || attackType == 16) {
                for (auto& kv : linkTopology) {
                    if (kv.first.first == reporterId || kv.first.second == reporterId) {
                        kv.second.trustScore = std::max(0.10, kv.second.trustScore - 0.35);
                        if (kv.second.trustScore < linkTrustThreshold) {
                            kv.second.isCompromised = true;
                        }
                    }
                }
            }
        }
    }
    EV_WARN << "============================================================" << endl;

    triggerGlobalRecomputation("Threat Incident / Node Quarantine");
    updateSDNCanvasVisual();
}

void EdgeTrustSDNController::reportDeliveryOutcome(int srcRsu, int dstRsu, int trafficClass, bool delivered, double latencySec, bool wasDetour)
{
    Enter_Method_Silent();
    totalPacketsRouted++;
    if (trafficClass == 0) {
        safetyPacketsRouted++;
        if (delivered) {
            safetyPacketsDelivered++;
        }
    } else if (trafficClass == 1) {
        controlPacketsRouted++;
    } else {
        bestEffortPacketsRouted++;
    }

    if (delivered) {
        deliveredPacketCount++;
        totalLatencyMsSum += (latencySec * 1000.0);
    }
}

int EdgeTrustSDNController::resolveNextHop(int currentRsu, int dstRsu, int trafficClass, bool& outIsDetour, std::vector<int>& outFullPath)
{
    Enter_Method_Silent();
    if (currentRsu == dstRsu) {
        outIsDetour = false;
        outFullPath = {currentRsu};
        return currentRsu;
    }

    FlowKey key{currentRsu, dstRsu, trafficClass};
    auto it = globalFlowTable.find(key);
    if (it != globalFlowTable.end() && it->second.expiresAt > simTime()) {
        it->second.packetsMatched++;
        outIsDetour = it->second.isDetour;
        outFullPath = it->second.fullPath;
        return it->second.nextHop;
    }

    // Compute fresh adaptive path
    double cost = 0.0;
    outFullPath = computeAdaptivePath(currentRsu, dstRsu, trafficClass, cost, outIsDetour);
    int nextHop = (outFullPath.size() > 1) ? outFullPath[1] : dstRsu;

    // Install Flow Rule
    FlowRule rule;
    rule.ruleId = ++flowRulesInstalled;
    rule.srcRsu = currentRsu;
    rule.dstRsu = dstRsu;
    rule.trafficClass = trafficClass;
    rule.nextHop = nextHop;
    rule.fullPath = outFullPath;
    rule.isDetour = outIsDetour;
    rule.computedCost = cost;
    rule.installedAt = simTime();
    rule.expiresAt = simTime() + flowRuleTimeout;
    rule.packetsMatched = 1;
    rule.reason = outIsDetour ? "TRUST_ADAPTIVE_DETOUR" : "SHORTEST_PATH";

    globalFlowTable[key] = rule;
    pushFlowModToRSUs(rule);

    return nextHop;
}

std::vector<int> EdgeTrustSDNController::computeAdaptivePath(int srcRsu, int dstRsu, int trafficClass, double& outCost, bool& outIsDetour)
{
    Enter_Method_Silent();
    outIsDetour = false;
    outCost = 0.0;

    if (srcRsu == dstRsu) {
        return {srcRsu};
    }

    // Dijkstra's Algorithm over Trust-Cost Matrix
    const double INF = 1e9;
    std::vector<double> dist(numRSUs, INF);
    std::vector<int> prev(numRSUs, -1);
    std::vector<bool> visited(numRSUs, false);

    dist[srcRsu] = 0.0;

    for (int iter = 0; iter < numRSUs; ++iter) {
        int u = -1;
        double minDist = INF;
        for (int i = 0; i < numRSUs; ++i) {
            if (!visited[i] && dist[i] < minDist) {
                minDist = dist[i];
                u = i;
            }
        }

        if (u == -1 || u == dstRsu) break;
        visited[u] = true;

        for (int v = 0; v < numRSUs; ++v) {
            if (u == v || visited[v]) continue;

            auto linkIt = linkTopology.find({u, v});
            if (linkIt == linkTopology.end()) continue;

            const SDNLink& link = linkIt->second;

            // Check if node v or link (u, v) is quarantined or compromised
            bool isNodeBad = (quarantinedNodes.find(v) != quarantinedNodes.end());
            bool isLinkBad = link.isCompromised || (link.trustScore < linkTrustThreshold);

            double edgeCost = link.distance / 10.0; // Base hop distance

            if (isNodeBad) {
                edgeCost = INF; // Pruned
            } else if (isLinkBad) {
                untrustedLinksBypassed++;
                if (trafficClass == 0) {
                    // Safety-critical: zero tolerance for untrusted links
                    edgeCost += 10000.0;
                } else if (trafficClass == 1) {
                    edgeCost += 3000.0;
                } else {
                    edgeCost += 1000.0;
                }
            } else {
                // Trust penalty (lower trust = higher routing cost)
                double trustPenalty = (1.0 - link.trustScore) * 50.0;
                edgeCost += trustPenalty;

                // Dynamic QoS queue & latency adjustments
                if (trafficClass == 0) {
                    edgeCost += (link.latencyMs * 2.0) + (link.congestion * 30.0);
                } else if (trafficClass == 1) {
                    edgeCost += (link.latencyMs * 1.0) + (link.congestion * 15.0);
                } else {
                    edgeCost += (link.congestion * 50.0);
                }
            }

            if (dist[u] + edgeCost < dist[v]) {
                dist[v] = dist[u] + edgeCost;
                prev[v] = u;
            }
        }
    }

    if (dist[dstRsu] >= INF) {
        // Fallback: direct hop if unreachable
        EV_WARN << "[SDN] Path calculation from " << srcRsu << " to " << dstRsu
                << " could not find fully trusted path. Falling back to direct hop." << endl;
        return {srcRsu, dstRsu};
    }

    // Reconstruct path
    std::vector<int> path;
    for (int curr = dstRsu; curr != -1; curr = prev[curr]) {
        path.push_back(curr);
    }
    std::reverse(path.begin(), path.end());

    outCost = dist[dstRsu];

    // Check if detour was selected
    // If a direct 1-hop link exists between src and dst, but path has length > 2
    auto directIt = linkTopology.find({srcRsu, dstRsu});
    if (directIt != linkTopology.end() && path.size() > 2) {
        outIsDetour = true;
        detoursActivated++;
    }

    return path;
}

void EdgeTrustSDNController::triggerGlobalRecomputation(const std::string& triggerReason)
{
    EV_INFO << "============================================================" << endl;
    EV_INFO << " [t=" << simTime() << "s] [SDN GLOBAL RECOMPUTATION] Trigger: " << triggerReason << endl;

    globalFlowTable.clear();

    // Pre-calculate optimal paths for critical traffic pairs
    int activeDetours = 0;
    for (int s = 0; s < numRSUs; ++s) {
        for (int d = 0; d < numRSUs; ++d) {
            if (s == d) continue;
            for (int tc = 0; tc <= 2; ++tc) {
                double cost = 0.0;
                bool isDetour = false;
                std::vector<int> path = computeAdaptivePath(s, d, tc, cost, isDetour);
                if (isDetour && tc == 0) activeDetours++;

                FlowRule rule;
                rule.ruleId = ++flowRulesInstalled;
                rule.srcRsu = s;
                rule.dstRsu = d;
                rule.trafficClass = tc;
                rule.nextHop = (path.size() > 1) ? path[1] : d;
                rule.fullPath = path;
                rule.isDetour = isDetour;
                rule.computedCost = cost;
                rule.installedAt = simTime();
                rule.expiresAt = simTime() + flowRuleTimeout;
                rule.packetsMatched = 0;
                rule.reason = isDetour ? "ADAPTIVE_DETOUR_ACTIVE" : "SHORTEST_PATH";

                globalFlowTable[{s, d, tc}] = rule;
            }
        }
    }

    EV_INFO << "   -> Recomputed " << globalFlowTable.size() << " Active Flow Rules across 4 RSUs." << endl;
    EV_INFO << "   -> Active Safety Detours Enforced: " << activeDetours << endl;
    EV_INFO << "============================================================" << endl;
}

void EdgeTrustSDNController::pushFlowModToRSUs(const FlowRule& rule)
{
    // Notify ingress RSU of installed flow modification
    cModule* parent = getParentModule();
    if (!parent) return;

    cModule* rsuMod = parent->getSubmodule("rsu", rule.srcRsu);
    if (!rsuMod) return;

    EdgeTrustRSUApp* rsuApp = dynamic_cast<EdgeTrustRSUApp*>(rsuMod->getSubmodule("appl"));
    if (rsuApp) {
        std::stringstream pathStr;
        for (size_t i = 0; i < rule.fullPath.size(); ++i) {
            pathStr << rule.fullPath[i];
            if (i + 1 < rule.fullPath.size()) pathStr << " -> ";
        }

        EV_INFO << "[SDN FLOW-MOD PUSH] Ingress RSU " << rule.srcRsu << " [Dest: " << rule.dstRsu
                << " | QoS: Class " << rule.trafficClass << "] ==> NextHop: " << rule.nextHop
                << " (" << (rule.isDetour ? "DETOUR PATH: " : "DIRECT PATH: ") << pathStr.str() << ")" << endl;
    }
}

double EdgeTrustSDNController::getLinkTrust(int u, int v) const
{
    auto it = linkTopology.find({u, v});
    return (it != linkTopology.end()) ? it->second.trustScore : 1.0;
}

bool EdgeTrustSDNController::isNodeQuarantined(int rsuId) const
{
    return quarantinedNodes.find(rsuId) != quarantinedNodes.end();
}

void EdgeTrustSDNController::clearSDNCanvasVisual()
{
    cCanvas* canvas = getParentModule()->getCanvas();
    if (!canvas) return;

    cFigure* root = canvas->getRootFigure();
    for (int i = root->getNumFigures() - 1; i >= 0; --i) {
        cFigure* fig = root->getFigure(i);
        if (fig && std::string(fig->getName()).rfind("sdn_", 0) == 0) {
            delete root->removeFigure(i);
        }
    }
}

void EdgeTrustSDNController::updateSDNCanvasVisual()
{
    cCanvas* canvas = getParentModule()->getCanvas();
    if (!canvas) return;

    clearSDNCanvasVisual();
    cFigure* root = canvas->getRootFigure();

    // 1. Draw Controller Hub Node
    cGroupFigure* group = new cGroupFigure("sdn_topology_overlay");

    cOvalFigure* hub = new cOvalFigure("sdn_hub_circle");
    hub->setBounds(cFigure::Rectangle(posX - 12.0, posY - 8.0, 24.0, 16.0));
    hub->setFillColor(cFigure::Color("midnightblue"));
    hub->setFilled(true);
    hub->setLineColor(cFigure::Color("deepskyblue"));
    hub->setLineWidth(2.0);
    group->addFigure(hub);

    cTextFigure* hubLabel = new cTextFigure("sdn_hub_label");
    hubLabel->setPosition(cFigure::Point(posX, posY - 12.0));
    hubLabel->setText("SDN Controller [Tier-4 OpenFlow]");
    hubLabel->setColor(cFigure::Color("white"));
    hubLabel->setFont(cFigure::Font("Arial", 8, cFigure::FONT_BOLD));
    group->addFigure(hubLabel);

    // 2. Draw Active Flow Links between RSUs
    for (const auto& kv : linkTopology) {
        int u = kv.first.first;
        int v = kv.first.second;
        if (u >= v) continue; // Draw each undirected edge once

        const SDNLink& link = kv.second;
        Coord posU = rsuNodes[u].pos;
        Coord posV = rsuNodes[v].pos;

        cLineFigure* line = new cLineFigure(("sdn_link_" + std::to_string(u) + "_" + std::to_string(v)).c_str());
        line->setStart(cFigure::Point(posU.x, posU.y));
        line->setEnd(cFigure::Point(posV.x, posV.y));

        if (link.isCompromised || link.trustScore < linkTrustThreshold) {
            line->setLineColor(cFigure::Color("red"));
            line->setLineWidth(3.0);
            line->setLineStyle(cFigure::LINE_DASHED);
        } else if (link.trustScore < 0.85) {
            line->setLineColor(cFigure::Color("darkorange"));
            line->setLineWidth(2.5);
            line->setLineStyle(cFigure::LINE_DASHED);
        } else {
            line->setLineColor(cFigure::Color("dodgerblue"));
            line->setLineWidth(1.8);
            line->setLineStyle(cFigure::LINE_SOLID);
        }
        group->addFigure(line);
    }

    // 3. Draw Top Live Status Banner
    double pdr = (safetyPacketsRouted > 0) ? (100.0 * safetyPacketsDelivered / safetyPacketsRouted) : 100.0;
    std::stringstream banner;
    banner << "SDN TOPOLOGY: 4 RSUs | 2 Fog Clusters | Flow Rules: " << globalFlowTable.size()
           << " | Detours Active: " << detoursActivated
           << " | Bypassed: " << untrustedLinksBypassed
           << " | Safety PDR: " << std::fixed << std::setprecision(1) << pdr << "%";

    cTextFigure* bannerText = new cTextFigure("sdn_status_banner");
    bannerText->setPosition(cFigure::Point(118.0, 1.0));
    bannerText->setText(banner.str().c_str());
    bannerText->setColor(cFigure::Color("black"));
    bannerText->setFont(cFigure::Font("Arial", 9, cFigure::FONT_BOLD));
    group->addFigure(bannerText);

    root->addFigure(group);
}

void EdgeTrustSDNController::finish()
{
    clearSDNCanvasVisual();

    double pdr = (safetyPacketsRouted > 0) ? (100.0 * safetyPacketsDelivered / safetyPacketsRouted) : 100.0;
    double avgLatency = (deliveredPacketCount > 0) ? (totalLatencyMsSum / deliveredPacketCount) : 0.0;

    EV_INFO << "============================================================" << endl;
    EV_INFO << " [SDN CONTROLLER FINISHED] Final Summary & Telemetry:" << endl;
    EV_INFO << "   -> Total SDN Packets Routed: " << totalPacketsRouted << endl;
    EV_INFO << "   -> Safety-Critical Packets: " << safetyPacketsRouted
            << " | Delivered: " << safetyPacketsDelivered
            << " | Safety PDR: " << pdr << "%" << endl;
    EV_INFO << "   -> Control / Trust Packets Routed: " << controlPacketsRouted << endl;
    EV_INFO << "   -> Best-Effort Packets Routed: " << bestEffortPacketsRouted << endl;
    EV_INFO << "   -> Trust-Adaptive Detours Activated: " << detoursActivated << endl;
    EV_INFO << "   -> Flow Modification Rules Installed: " << flowRulesInstalled << endl;
    EV_INFO << "   -> Compromised / Untrusted Links Bypassed: " << untrustedLinksBypassed << endl;
    EV_INFO << "   -> Average End-to-End Latency: " << avgLatency << " ms" << endl;
    EV_INFO << "============================================================" << endl;

    recordScalar("sdn_totalPacketsRouted", totalPacketsRouted);
    recordScalar("sdn_safetyPacketsRouted", safetyPacketsRouted);
    recordScalar("sdn_safetyPacketsDelivered", safetyPacketsDelivered);
    recordScalar("sdn_safetyPDR", pdr);
    recordScalar("sdn_detoursActivated", detoursActivated);
    recordScalar("sdn_flowRulesInstalled", flowRulesInstalled);
    recordScalar("sdn_untrustedLinksBypassed", untrustedLinksBypassed);
    recordScalar("sdn_avgLatencyMs", avgLatency);
}

} // namespace veins
