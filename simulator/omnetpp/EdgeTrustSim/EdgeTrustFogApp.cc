#include "veins/modules/application/edgetrust/EdgeTrustFogApp.h"
#include "veins/modules/application/edgetrust/EdgeTrustRSUApp.h"
#include "veins/modules/application/edgetrust/EdgeTrustSDNController.h"
#include <omnetpp.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <sys/stat.h>
#include <sys/types.h>

using namespace omnetpp;

namespace veins {

Define_Module(EdgeTrustFogApp);

static void ensureFogDirExists(const std::string& filepath) {
    size_t lastSlash = filepath.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        std::string dir = filepath.substr(0, lastSlash);
        struct stat st;
        if (stat(dir.c_str(), &st) != 0) {
            mkdir(dir.c_str(), 0755);
        }
    }
}

static int extractFogInt(const std::string& block, const std::string& key, int defaultVal = 0) {
    std::string pattern = "\"" + key + "\":";
    size_t pos = block.find(pattern);
    if (pos == std::string::npos) return defaultVal;
    pos += pattern.length();
    while (pos < block.length() && (block[pos] == ' ' || block[pos] == '\t' || block[pos] == '\n' || block[pos] == '\r')) pos++;
    try {
        return std::stoi(block.substr(pos));
    } catch (...) {
        return defaultVal;
    }
}

static double extractFogDouble(const std::string& block, const std::string& key, double defaultVal = 0.0) {
    std::string pattern = "\"" + key + "\":";
    size_t pos = block.find(pattern);
    if (pos == std::string::npos) return defaultVal;
    pos += pattern.length();
    while (pos < block.length() && (block[pos] == ' ' || block[pos] == '\t' || block[pos] == '\n' || block[pos] == '\r')) pos++;
    try {
        return std::stod(block.substr(pos));
    } catch (...) {
        return defaultVal;
    }
}

static bool extractFogBool(const std::string& block, const std::string& key, bool defaultVal = false) {
    std::string pattern = "\"" + key + "\":";
    size_t pos = block.find(pattern);
    if (pos == std::string::npos) return defaultVal;
    pos += pattern.length();
    while (pos < block.length() && (block[pos] == ' ' || block[pos] == '\t' || block[pos] == '\n' || block[pos] == '\r')) pos++;
    if (block.substr(pos, 4) == "true") return true;
    if (block.substr(pos, 5) == "false") return false;
    return defaultVal;
}

static std::string extractFogString(const std::string& block, const std::string& key, const std::string& defaultVal = "") {
    std::string pattern = "\"" + key + "\":";
    size_t pos = block.find(pattern);
    if (pos == std::string::npos) return defaultVal;
    pos = block.find("\"", pos + pattern.length());
    if (pos == std::string::npos) return defaultVal;
    size_t endPos = block.find("\"", pos + 1);
    if (endPos == std::string::npos) return defaultVal;
    return block.substr(pos + 1, endPos - pos - 1);
}

void EdgeTrustFogApp::initialize(int stage)
{
    if (stage == 0) {
        fogId = hasPar("fogId") ? par("fogId").intValue() : 0;
        fogSyncInterval = hasPar("fogSyncInterval") ? par("fogSyncInterval").doubleValue() : 2.0;
        conflictTolerance = hasPar("conflictTolerance") ? par("conflictTolerance").doubleValue() : 0.20;
        saveCheckpointEnabled = hasPar("saveCheckpoint") ? par("saveCheckpoint").boolValue() : true;
        loadCheckpointEnabled = hasPar("loadCheckpoint") ? par("loadCheckpoint").boolValue() : false;
        checkpointFilePath = hasPar("checkpointFile") ? par("checkpointFile").stringValue() : ("results/checkpoint_fog" + std::to_string(fogId) + "_ledger.json");
        checkpointInterval = hasPar("checkpointInterval") ? par("checkpointInterval").doubleValue() : 20.0;
        posX = hasPar("posX") ? par("posX").doubleValue() : (fogId == 0 ? 58.0 : 178.0);
        posY = hasPar("posY") ? par("posY").doubleValue() : 15.0;
        posZ = hasPar("posZ") ? par("posZ").doubleValue() : 10.0;

        getDisplayString().setTagArg("p", 0, std::to_string((int)posX).c_str());
        getDisplayString().setTagArg("p", 1, std::to_string((int)posY).c_str());
        getDisplayString().setTagArg("is", 0, "s");
        getDisplayString().setTagArg("t", 0, ("Fog " + std::to_string(fogId)).c_str());

        managedRsuOffset = hasPar("managedRsuOffset") ? par("managedRsuOffset").intValue() : (fogId * 2);
        managedRsuCount = hasPar("managedRsuCount") ? par("managedRsuCount").intValue() : 2;
        peerFogId = hasPar("peerFogId") ? par("peerFogId").intValue() : (fogId == 0 ? 1 : 0);
        enableInterFog = hasPar("enableInterFog") ? par("enableInterFog").boolValue() : true;
        enableSDNIntegration = hasPar("enableSDNIntegration") ? par("enableSDNIntegration").boolValue() : true;

        fogSyncTimerMsg = new cMessage("fogSyncTimer");
        scheduleAt(simTime() + fogSyncInterval, fogSyncTimerMsg);

        if (saveCheckpointEnabled) {
            checkpointTimerMsg = new cMessage("fogCheckpointTimer");
            scheduleAt(simTime() + checkpointInterval, checkpointTimerMsg);
        }

        if (loadCheckpointEnabled) {
            loadCheckpoint(checkpointFilePath);
        }

        EV_INFO << "============================================================" << endl;
        EV_INFO << " [t=" << simTime() << "s] [FOG LAYER INITIALIZED] Regional Fog Node " << fogId << endl;
        EV_INFO << "   -> Architecture: Tier-3 Regional Trust Aggregation & Consensus Layer" << endl;
        EV_INFO << "   -> Sync Interval: " << fogSyncInterval << "s | Conflict Tolerance: " << conflictTolerance << endl;
        EV_INFO << "   -> Managed Cluster RSUs: Offset " << managedRsuOffset << ", Count " << managedRsuCount << endl;
        EV_INFO << "   -> Inter-Fog Peer: Fog " << peerFogId << " (" << (enableInterFog ? "ENABLED" : "DISABLED") << ")" << endl;
        EV_INFO << "   -> Tier-4 SDN Integration: " << (enableSDNIntegration ? "CONNECTED" : "STANDALONE") << endl;
        EV_INFO << "   -> Regional Checkpoint: " << (saveCheckpointEnabled ? checkpointFilePath : "DISABLED") << endl;
        EV_INFO << "============================================================" << endl;

        updateFogCanvasVisual("init", "dodgerblue", "[Fog " + std::to_string(fogId) + "] Active");
    }
    else if (stage == 1) {
        cModule* parent = getParentModule();
        if (parent && enableSDNIntegration) {
            cModule* sdnMod = parent->getSubmodule("sdn");
            if (sdnMod) {
                EdgeTrustSDNController* sdn = dynamic_cast<EdgeTrustSDNController*>(sdnMod);
                if (sdn) {
                    std::vector<int> managed;
                    for (int i = managedRsuOffset; i < managedRsuOffset + managedRsuCount; ++i) {
                        managed.push_back(i);
                    }
                    sdn->registerFog(fogId, Coord(posX, posY, posZ), managed);
                }
            }
        }
    }
}

void EdgeTrustFogApp::handleMessage(cMessage* msg)
{
    if (msg == fogSyncTimerMsg) {
        processPeriodicRegionalAggregation();
        scheduleAt(simTime() + fogSyncInterval, fogSyncTimerMsg);
    } else if (msg == checkpointTimerMsg) {
        if (saveCheckpointEnabled) {
            saveCheckpoint(checkpointFilePath);
        }
        scheduleAt(simTime() + checkpointInterval, checkpointTimerMsg);
    } else {
        delete msg;
    }
}

void EdgeTrustFogApp::receiveRSUDigest(int sourceRsuId, const std::vector<InterRSUDigestEntry>& digest)
{
    Enter_Method_Silent();
    totalRSUDigestsIngested++;
    int updatedNodes = 0;
    int newNodes = 0;

    for (const auto& entry : digest) {
        auto it = regionalLedger.find(entry.nodeId);
        if (it == regionalLedger.end()) {
            FogVehicleRecord rec;
            rec.nodeId = entry.nodeId;
            rec.firstSeen = simTime();
            rec.lastSeen = entry.lastSeen;
            rec.lastPos = entry.lastPos;
            rec.lastSpeed = entry.lastSpeed;
            rec.regionalTrustScore = entry.trustScore;
            rec.regionalHistoricalTrust = entry.historicalTrustScore;
            rec.isRevoked = entry.isMalicious;
            rec.primaryAttackType = entry.attackType;
            if (entry.isMalicious) {
                rec.revocationReason = "Local RSU " + std::to_string(sourceRsuId) + " Malicious Detection: " + entry.verdict;
                rec.revokedAt = simTime();
                regionalRevocationList.insert(entry.nodeId);
            }

            RSUObservation obs;
            obs.directTrust = entry.trustScore;
            obs.histTrust = entry.historicalTrustScore;
            obs.neighborTrustAvg = entry.neighborTrustScoreAvg;
            obs.isMalicious = entry.isMalicious;
            obs.attackType = entry.attackType;
            obs.verdict = entry.verdict;
            obs.modelName = entry.modelName;
            obs.mlPrediction = entry.mlPrediction;
            obs.mlConfidence = entry.mlConfidence;
            obs.lastPos = entry.lastPos;
            obs.lastSpeed = entry.lastSpeed;
            obs.packetDropRatio = entry.packetDropRatio;
            obs.falsePacketInjection = entry.falsePacketInjection;
            obs.blackholeAttempts = entry.blackholeAttempts;
            obs.sybilAttempts = entry.sybilAttempts;
            obs.denialOfService = entry.denialOfService;
            obs.lastReportTime = entry.lastSeen;

            rec.rsuObservations[sourceRsuId] = obs;
            rec.cumulativeFalseInjections = entry.falsePacketInjection;
            rec.cumulativeBlackholeDrops = entry.blackholeAttempts;
            rec.cumulativeSybilAttempts = entry.sybilAttempts;
            rec.cumulativeDosAttempts = entry.denialOfService;
            rec.totalReportsReceived = 1;

            regionalLedger[entry.nodeId] = rec;
            newNodes++;
        } else {
            FogVehicleRecord& rec = it->second;

            // Stale update mitigation
            if (rec.rsuObservations.find(sourceRsuId) != rec.rsuObservations.end()) {
                if (entry.lastSeen < rec.rsuObservations[sourceRsuId].lastReportTime) {
                    totalStaleUpdatesDropped++;
                    EV_WARN << "[t=" << simTime() << "s] [FOG STALE UPDATE DROPPED] Node " << entry.nodeId
                            << ": Incoming timestamp t=" << entry.lastSeen << "s older than recorded t="
                            << rec.rsuObservations[sourceRsuId].lastReportTime << "s from RSU " << sourceRsuId << endl;
                    continue;
                }
            }

            rec.lastSeen = std::max(rec.lastSeen, entry.lastSeen);
            rec.lastPos = entry.lastPos;
            rec.lastSpeed = entry.lastSpeed;
            rec.totalReportsReceived++;

            // Reconcile multi-RSU conflicts or update single observation
            reconcileConflictsForNode(rec, sourceRsuId, entry);
            updatedNodes++;
        }
    }

    EV_INFO << "============================================================" << endl;
    EV_INFO << " [t=" << simTime() << "s] [FOG REGIONAL INGESTION] Ingested Digest from RSU " << sourceRsuId << endl;
    EV_INFO << "   -> New Corridor Vehicles: " << newNodes << " | Updated Vehicles: " << updatedNodes << endl;
    EV_INFO << "   -> Master Ledger State: " << regionalLedger.size() << " Active Nodes | "
            << regionalRevocationList.size() << " Quarantined Nodes" << endl;
    EV_INFO << "============================================================" << endl;
}

void EdgeTrustFogApp::reconcileConflictsForNode(FogVehicleRecord& rec, int newRsuId, const InterRSUDigestEntry& entry)
{
    RSUObservation newObs;
    newObs.directTrust = entry.trustScore;
    newObs.histTrust = entry.historicalTrustScore;
    newObs.neighborTrustAvg = entry.neighborTrustScoreAvg;
    newObs.isMalicious = entry.isMalicious;
    newObs.attackType = entry.attackType;
    newObs.verdict = entry.verdict;
    newObs.modelName = entry.modelName;
    newObs.mlPrediction = entry.mlPrediction;
    newObs.mlConfidence = entry.mlConfidence;
    newObs.lastPos = entry.lastPos;
    newObs.lastSpeed = entry.lastSpeed;
    newObs.packetDropRatio = entry.packetDropRatio;
    newObs.falsePacketInjection = entry.falsePacketInjection;
    newObs.blackholeAttempts = entry.blackholeAttempts;
    newObs.sybilAttempts = entry.sybilAttempts;
    newObs.denialOfService = entry.denialOfService;
    newObs.lastReportTime = entry.lastSeen;

    rec.rsuObservations[newRsuId] = newObs;

    // Check conflict against existing observations from other RSUs
    int peerRsuId = (newRsuId == 0) ? 1 : 0;
    auto peerIt = rec.rsuObservations.find(peerRsuId);

    if (peerIt != rec.rsuObservations.end()) {
        const RSUObservation& peerObs = peerIt->second;
        double diff = std::fabs(newObs.directTrust - peerObs.directTrust);

        if (diff > conflictTolerance || (newObs.isMalicious != peerObs.isMalicious)) {
            rec.hasConflict = true;
            rec.conflictCount++;
            totalConflictsResolved++;

            // Policy 1: Safety-Critical Pessimism
            // If either RSU flagged malicious, safety overrides optimistic peer reports
            if (newObs.isMalicious || peerObs.isMalicious || newObs.verdict == "BLOCK" || peerObs.verdict == "BLOCK") {
                rec.isRevoked = true;
                rec.regionalTrustScore = std::min(newObs.directTrust, peerObs.directTrust);
                rec.regionalHistoricalTrust = std::min(newObs.histTrust, peerObs.histTrust);
                rec.primaryAttackType = newObs.isMalicious ? newObs.attackType : peerObs.attackType;
                rec.revocationReason = "Cross-RSU Conflict Resolution: Safety-Critical Malicious Override";
                rec.revokedAt = simTime();
                regionalRevocationList.insert(rec.nodeId);

                std::stringstream cs;
                cs << "Safety-Override [RSU " << newRsuId << " (" << std::fixed << std::setprecision(2) << newObs.directTrust
                   << ") vs RSU " << peerRsuId << " (" << peerObs.directTrust << ") -> RECONCILED: " << rec.regionalTrustScore << " BLOCK]";
                rec.lastConflictDetails = cs.str();

                EV_WARN << "============================================================" << endl;
                EV_WARN << " [t=" << simTime() << "s] [FOG CONFLICT RESOLVED] Vehicle " << rec.nodeId << endl;
                EV_WARN << "   -> Divergence Detected: RSU " << newRsuId << " (T=" << newObs.directTrust << ", " << newObs.verdict
                        << ") vs RSU " << peerRsuId << " (T=" << peerObs.directTrust << ", " << peerObs.verdict << ")" << endl;
                EV_WARN << "   -> Resolution Policy: SAFETY-CRITICAL PESSIMISM ENFORCED" << endl;
                EV_WARN << "   -> Master Ledger Decision: Vehicle " << rec.nodeId << " QUARANTINED REGION-WIDE" << endl;
                EV_WARN << "============================================================" << endl;

                // Immediately dispatch authoritative revocation to all RSUs
                triggerRegionalRevocation(rec.nodeId, rec.primaryAttackType, 0.95, rec.revocationReason);
                return;
            } else {
                // Policy 2: Recency-Weighted Bayesian Consensus
                double ageNew = (simTime() - newObs.lastReportTime).dbl();
                double agePeer = (simTime() - peerObs.lastReportTime).dbl();
                double wNew = 1.0 / (1.0 + std::max(0.0, ageNew));
                double wPeer = 1.0 / (1.0 + std::max(0.0, agePeer));
                double consensusTrust = (wNew * newObs.directTrust + wPeer * peerObs.directTrust) / (wNew + wPeer);
                rec.regionalTrustScore = (0.70 * rec.regionalTrustScore) + (0.30 * consensusTrust);
                rec.regionalHistoricalTrust = (0.75 * rec.regionalHistoricalTrust) + (0.25 * consensusTrust);

                std::stringstream cs;
                cs << "Bayesian Consensus [RSU " << newRsuId << " (" << std::fixed << std::setprecision(2) << newObs.directTrust
                   << ") vs RSU " << peerRsuId << " (" << peerObs.directTrust << ") -> RECONCILED: " << rec.regionalTrustScore << "]";
                rec.lastConflictDetails = cs.str();

                EV_INFO << "[t=" << simTime() << "s] [FOG CONFLICT RESOLVED] Node " << rec.nodeId
                        << ": Bayesian Consensus reconciled divergence (" << diff << ") -> Regional Trust = "
                        << rec.regionalTrustScore << endl;
            }
        } else {
            // Normal harmonious convergence
            double avgDirect = (newObs.directTrust + peerObs.directTrust) / 2.0;
            rec.regionalTrustScore = (0.70 * rec.regionalTrustScore) + (0.30 * avgDirect);
            rec.regionalHistoricalTrust = (0.75 * rec.regionalHistoricalTrust) + (0.25 * avgDirect);
        }
    } else {
        // Single RSU observation update
        rec.regionalTrustScore = (0.70 * rec.regionalTrustScore) + (0.30 * newObs.directTrust);
        rec.regionalHistoricalTrust = (0.75 * rec.regionalHistoricalTrust) + (0.25 * newObs.histTrust);
        if (newObs.isMalicious) {
            rec.isRevoked = true;
            rec.primaryAttackType = newObs.attackType;
            rec.revocationReason = "Reported Malicious by Serving RSU " + std::to_string(newRsuId);
            rec.revokedAt = simTime();
            regionalRevocationList.insert(rec.nodeId);
        }
    }

    // Cross-District Multi-Intersection Distributed Threat Correlation
    int cumInj = 0, cumDrop = 0, cumSyb = 0, cumDos = 0;
    for (const auto& kv : rec.rsuObservations) {
        cumInj += kv.second.falsePacketInjection;
        cumDrop += kv.second.blackholeAttempts;
        cumSyb += kv.second.sybilAttempts;
        cumDos += kv.second.denialOfService;
    }
    rec.cumulativeFalseInjections = cumInj;
    rec.cumulativeBlackholeDrops = cumDrop;
    rec.cumulativeSybilAttempts = cumSyb;
    rec.cumulativeDosAttempts = cumDos;

    if (!rec.isRevoked && (cumInj >= 3 || cumDrop >= 5 || cumSyb >= 2 || cumDos >= 10)) {
        totalCrossIntersectionAttacksDetected++;
        rec.isRevoked = true;
        rec.primaryAttackType = 16;
        rec.revocationReason = "Multi-Intersection Distributed Threat Correlated by Fog Ledger";
        rec.revokedAt = simTime();
        regionalRevocationList.insert(rec.nodeId);

        EV_ERROR << "============================================================" << endl;
        EV_ERROR << " [t=" << simTime() << "s] [FOG MULTI-INTERSECTION ATTACK DETECTED] Vehicle " << rec.nodeId << endl;
        EV_ERROR << "   -> Coordinated Threat Across Grid 1 & Grid 2 Detected by Fog!" << endl;
        EV_ERROR << "   -> Cumulative Anomaly Stats: Injections=" << cumInj << " | Drops=" << cumDrop
                 << " | Sybil=" << cumSyb << " | DoS=" << cumDos << endl;
        EV_ERROR << "   -> Action: Network-Wide Certificate Revocation (CRL) Dispatched" << endl;
        EV_ERROR << "============================================================" << endl;

        triggerRegionalRevocation(rec.nodeId, 16, 0.95, rec.revocationReason);
    }
}

void EdgeTrustFogApp::receiveRSUThreatAlert(int sourceRsuId, int offendingNodeId, int attackType, double confidence, const std::string& reason)
{
    Enter_Method_Silent();
    EV_WARN << "============================================================" << endl;
    EV_WARN << " [t=" << simTime() << "s] [FOG REGIONAL INCIDENT INGESTED] Alert from RSU " << sourceRsuId << endl;
    EV_WARN << "   -> Offending Node: " << offendingNodeId << " | Attack Type ID: " << attackType << endl;
    EV_WARN << "   -> Confidence: " << (int)(confidence * 100) << "% | Reason: " << reason << endl;
    EV_WARN << "   -> Immediate Action: Authoritative Regional CRL Broadcast to ALL RSUs" << endl;
    EV_WARN << "============================================================" << endl;

    triggerRegionalRevocation(offendingNodeId, attackType, confidence, reason);
}

void EdgeTrustFogApp::triggerRegionalRevocation(int nodeId, int attackType, double confidence, const std::string& reason)
{
    regionalRevocationList.insert(nodeId);
    totalRevocationsIssued++;

    auto it = regionalLedger.find(nodeId);
    if (it != regionalLedger.end()) {
        it->second.isRevoked = true;
        it->second.regionalTrustScore = 0.05;
        it->second.regionalHistoricalTrust = 0.05;
        it->second.primaryAttackType = attackType;
        it->second.revocationReason = reason;
        it->second.revokedAt = simTime();
    }

    cModule* parent = getParentModule();
    if (parent) {
        for (int i = managedRsuOffset; i < managedRsuOffset + managedRsuCount; ++i) {
            cModule* rsuHost = parent->getSubmodule("rsu", i);
            if (rsuHost) {
                EdgeTrustRSUApp* rsuApp = dynamic_cast<EdgeTrustRSUApp*>(rsuHost->getSubmodule("appl"));
                if (rsuApp) {
                    rsuApp->receiveFogRevocation(nodeId, attackType, confidence, reason);
                }
            }
        }

        // Propagate across clusters to peer Fog
        if (enableInterFog) {
            cModule* peerFogHost = parent->getSubmodule("fog", peerFogId);
            if (peerFogHost) {
                EdgeTrustFogApp* peerFog = dynamic_cast<EdgeTrustFogApp*>(peerFogHost);
                if (peerFog) {
                    totalInterFogRevocationsSent++;
                    peerFog->receiveInterFogRevocation(fogId, nodeId, attackType, confidence, reason);
                }
            }
        }

        // Notify SDN Controller
        if (enableSDNIntegration) {
            cModule* sdnHost = parent->getSubmodule("sdn");
            if (sdnHost) {
                EdgeTrustSDNController* sdn = dynamic_cast<EdgeTrustSDNController*>(sdnHost);
                if (sdn) {
                    sdn->notifyNodeThreat(fogId + 100, nodeId, attackType, confidence, reason);
                }
            }
        }
    }

    updateFogCanvasVisual("revocation", "firebrick", "CRL: Node " + std::to_string(nodeId));
}

void EdgeTrustFogApp::processPeriodicRegionalAggregation()
{
    pushRegionalMasterDigest();

    if (enableInterFog) {
        sendInterFogSync();
    }
    if (enableSDNIntegration) {
        reportToSDN();
    }

    std::stringstream info;
    info << "[Fog " << fogId << "] " << regionalLedger.size() << " Nodes | CRL: " << regionalRevocationList.size();

    updateFogCanvasVisual("sync", "seagreen", info.str());
}

void EdgeTrustFogApp::pushRegionalMasterDigest()
{
    totalMasterSyncsDispatched++;
    std::vector<FogMasterDigestEntry> masterDigest;
    masterDigest.reserve(regionalLedger.size());

    for (const auto& kv : regionalLedger) {
        const FogVehicleRecord& rec = kv.second;
        if (simTime() - rec.lastSeen < 20.0 || rec.isRevoked) {
            FogMasterDigestEntry entry;
            entry.nodeId = rec.nodeId;
            entry.regionalTrustScore = rec.regionalTrustScore;
            entry.regionalHistoricalTrust = rec.regionalHistoricalTrust;
            entry.isRevoked = rec.isRevoked;
            entry.attackType = rec.primaryAttackType;
            entry.lastPos = rec.lastPos;
            entry.lastSpeed = rec.lastSpeed;
            entry.lastSeen = rec.lastSeen;
            entry.verdict = rec.isRevoked ? "BLOCK" : (rec.regionalTrustScore < 0.70 ? "WARN" : "ACCEPT");
            entry.reportingRSUs = static_cast<int>(rec.rsuObservations.size());
            entry.hadConflict = rec.hasConflict;
            masterDigest.push_back(entry);
        }
    }

    cModule* parent = getParentModule();
    if (parent) {
        for (int i = managedRsuOffset; i < managedRsuOffset + managedRsuCount; ++i) {
            cModule* rsuHost = parent->getSubmodule("rsu", i);
            if (rsuHost) {
                EdgeTrustRSUApp* rsuApp = dynamic_cast<EdgeTrustRSUApp*>(rsuHost->getSubmodule("appl"));
                if (rsuApp) {
                    rsuApp->receiveFogMasterDigest(masterDigest);
                }
            }
        }
    }

    EV_INFO << "[t=" << simTime() << "s] [FOG " << fogId << " REGIONAL MASTER SYNC] Dispatched consolidated ledger ("
            << masterDigest.size() << " entries) to managed RSUs (" << managedRsuOffset << ".."
            << (managedRsuOffset + managedRsuCount - 1) << ")." << endl;
}

void EdgeTrustFogApp::sendInterFogSync()
{
    cModule* parent = getParentModule();
    if (!parent) return;
    cModule* peerFogHost = parent->getSubmodule("fog", peerFogId);
    if (!peerFogHost) return;
    EdgeTrustFogApp* peerFog = dynamic_cast<EdgeTrustFogApp*>(peerFogHost);
    if (!peerFog) return;

    totalInterFogSyncsSent++;
    std::vector<FogMasterDigestEntry> clusterDigest;
    clusterDigest.reserve(regionalLedger.size());
    for (const auto& kv : regionalLedger) {
        const FogVehicleRecord& rec = kv.second;
        if (simTime() - rec.lastSeen < 20.0 || rec.isRevoked) {
            FogMasterDigestEntry e;
            e.nodeId = rec.nodeId;
            e.regionalTrustScore = rec.regionalTrustScore;
            e.regionalHistoricalTrust = rec.regionalHistoricalTrust;
            e.isRevoked = rec.isRevoked;
            e.attackType = rec.primaryAttackType;
            e.lastPos = rec.lastPos;
            e.lastSpeed = rec.lastSpeed;
            e.lastSeen = rec.lastSeen;
            e.verdict = rec.isRevoked ? "BLOCK" : (rec.regionalTrustScore < 0.70 ? "WARN" : "ACCEPT");
            e.reportingRSUs = static_cast<int>(rec.rsuObservations.size());
            e.hadConflict = rec.hasConflict;
            clusterDigest.push_back(e);
        }
    }

    peerFog->receiveInterFogDigest(fogId, clusterDigest);
}

void EdgeTrustFogApp::receiveInterFogDigest(int sourceFogId, const std::vector<FogMasterDigestEntry>& digest)
{
    Enter_Method_Silent();
    totalInterFogSyncsReceived++;
    int syncedCount = 0;
    int prewarmedCount = 0;

    for (const auto& entry : digest) {
        if (entry.isRevoked) {
            if (regionalRevocationList.find(entry.nodeId) == regionalRevocationList.end()) {
                regionalRevocationList.insert(entry.nodeId);
                totalRevocationsIssued++;
                auto it = regionalLedger.find(entry.nodeId);
                if (it != regionalLedger.end()) {
                    it->second.isRevoked = true;
                    it->second.regionalTrustScore = 0.05;
                }
            }
        }

        auto it = regionalLedger.find(entry.nodeId);
        if (it != regionalLedger.end()) {
            it->second.regionalHistoricalTrust = (0.75 * it->second.regionalHistoricalTrust) + (0.25 * entry.regionalHistoricalTrust);
            syncedCount++;
        } else {
            // Pre-warm record for incoming cross-cluster handover
            FogVehicleRecord rec;
            rec.nodeId = entry.nodeId;
            rec.firstSeen = simTime();
            rec.lastSeen = entry.lastSeen;
            rec.lastPos = entry.lastPos;
            rec.lastSpeed = entry.lastSpeed;
            rec.regionalTrustScore = entry.regionalTrustScore;
            rec.regionalHistoricalTrust = entry.regionalHistoricalTrust;
            rec.isRevoked = entry.isRevoked;
            rec.primaryAttackType = entry.attackType;
            rec.revocationReason = entry.isRevoked ? "Peer Fog Authoritative Revocation" : "";
            regionalLedger[entry.nodeId] = rec;
            prewarmedCount++;
        }
    }

    EV_INFO << "[t=" << simTime() << "s] [INTER-FOG DIGEST INGESTED] Fog " << fogId
            << " ingested " << digest.size() << " entries from Peer Fog " << sourceFogId
            << " (" << syncedCount << " aligned, " << prewarmedCount << " handover pre-warmed)." << endl;
}

void EdgeTrustFogApp::receiveInterFogRevocation(int sourceFogId, int offendingNodeId, int attackType, double confidence, const std::string& reason)
{
    Enter_Method_Silent();
    totalInterFogRevocationsReceived++;
    regionalRevocationList.insert(offendingNodeId);

    auto it = regionalLedger.find(offendingNodeId);
    if (it != regionalLedger.end()) {
        it->second.isRevoked = true;
        it->second.regionalTrustScore = 0.05;
        it->second.primaryAttackType = attackType;
        it->second.revocationReason = "Global CRL Revocation from Fog " + std::to_string(sourceFogId);
        it->second.revokedAt = simTime();
    }

    // Immediately propagate CRL revocation to local managed RSUs
    cModule* parent = getParentModule();
    if (parent) {
        for (int i = managedRsuOffset; i < managedRsuOffset + managedRsuCount; ++i) {
            cModule* rsuHost = parent->getSubmodule("rsu", i);
            if (rsuHost) {
                EdgeTrustRSUApp* rsuApp = dynamic_cast<EdgeTrustRSUApp*>(rsuHost->getSubmodule("appl"));
                if (rsuApp) {
                    rsuApp->receiveFogRevocation(offendingNodeId, attackType, confidence, reason);
                }
            }
        }
    }

    EV_WARN << "============================================================" << endl;
    EV_WARN << " [t=" << simTime() << "s] [GLOBAL INTER-FOG CRL ORDER APPLIED] Fog " << fogId << endl;
    EV_WARN << "   -> Target Node: " << offendingNodeId << " revoked via Inter-Fog Trunk from Fog " << sourceFogId << endl;
    EV_WARN << "   -> Reason: " << reason << " | Dispatched to Local Cluster RSUs (" << managedRsuOffset
            << ".." << (managedRsuOffset + managedRsuCount - 1) << ")" << endl;
    EV_WARN << "============================================================" << endl;
}

void EdgeTrustFogApp::reportToSDN()
{
    cModule* parent = getParentModule();
    if (!parent) return;
    cModule* sdnMod = parent->getSubmodule("sdn");
    if (!sdnMod) return;
    EdgeTrustSDNController* sdn = dynamic_cast<EdgeTrustSDNController*>(sdnMod);
    if (!sdn) return;

    for (int i = managedRsuOffset; i < managedRsuOffset + managedRsuCount; ++i) {
        double trustSum = 0.0;
        int count = 0;
        for (const auto& kv : regionalLedger) {
            auto obsIt = kv.second.rsuObservations.find(i);
            if (obsIt != kv.second.rsuObservations.end()) {
                trustSum += obsIt->second.directTrust;
                count++;
            }
        }
        double avgTrust = (count > 0) ? (trustSum / count) : 1.0;
        int peerInCluster = (i == managedRsuOffset) ? (managedRsuOffset + 1) : managedRsuOffset;
        sdn->updateLinkMetrics(i, peerInCluster, avgTrust, 1.0, 2.0, 0.05);
    }
}

void EdgeTrustFogApp::saveCheckpoint(const std::string& filepath)
{
    ensureFogDirExists(filepath);
    std::ofstream out(filepath, std::ios::out | std::ios::trunc);
    if (!out.is_open()) {
        EV_ERROR << "EdgeTrust Fog " << fogId << ": Failed to open checkpoint file " << filepath << " for writing." << endl;
        return;
    }

    std::stringstream ss;
    ss << "{\n";
    ss << "  \"checkpoint_layer\": \"Fog_Regional_Ledger\",\n";
    ss << "  \"simulation_time_sec\": " << std::fixed << std::setprecision(4) << simTime().dbl() << ",\n";
    ss << "  \"fog_id\": " << fogId << ",\n";
    ss << "  \"tracked_vehicle_count\": " << regionalLedger.size() << ",\n";
    ss << "  \"revoked_count\": " << regionalRevocationList.size() << ",\n";
    ss << "  \"conflicts_resolved_count\": " << totalConflictsResolved << ",\n";
    ss << "  \"revoked_node_ids\": [";
    size_t rIdx = 0;
    for (int rId : regionalRevocationList) {
        ss << rId << (++rIdx < regionalRevocationList.size() ? ", " : "");
    }
    ss << "],\n";

    ss << "  \"regional_ledger\": {\n";
    size_t vIdx = 0;
    size_t total = regionalLedger.size();
    for (const auto& kv : regionalLedger) {
        int vId = kv.first;
        const FogVehicleRecord& v = kv.second;
        ss << "    \"" << vId << "\": {\n";
        ss << "      \"node_id\": " << vId << ",\n";
        ss << "      \"regional_trust_score\": " << std::fixed << std::setprecision(4) << v.regionalTrustScore << ",\n";
        ss << "      \"regional_historical_trust\": " << std::fixed << std::setprecision(4) << v.regionalHistoricalTrust << ",\n";
        ss << "      \"is_revoked\": " << (v.isRevoked ? "true" : "false") << ",\n";
        ss << "      \"attack_type\": " << v.primaryAttackType << ",\n";
        ss << "      \"revocation_reason\": \"" << v.revocationReason << "\",\n";
        ss << "      \"has_conflict\": " << (v.hasConflict ? "true" : "false") << ",\n";
        ss << "      \"conflict_count\": " << v.conflictCount << ",\n";
        ss << "      \"cumulative_false_injections\": " << v.cumulativeFalseInjections << ",\n";
        ss << "      \"cumulative_blackhole_drops\": " << v.cumulativeBlackholeDrops << ",\n";
        ss << "      \"reporting_rsus\": [";
        size_t oIdx = 0;
        for (const auto& obsPair : v.rsuObservations) {
            ss << obsPair.first << (++oIdx < v.rsuObservations.size() ? ", " : "");
        }
        ss << "]\n";
        ss << "    }" << (++vIdx < total ? "," : "") << "\n";
    }
    ss << "  }\n";
    ss << "}\n";

    out << ss.str();
    out.close();

    EV_INFO << "EdgeTrust Fog " << fogId << ": Regional Ledger checkpoint saved ("
            << regionalLedger.size() << " vehicles, " << regionalRevocationList.size()
            << " revocations) to " << filepath << " at t=" << simTime() << "s." << endl;
}

bool EdgeTrustFogApp::loadCheckpoint(const std::string& filepath)
{
    std::ifstream in(filepath);
    if (!in.is_open()) {
        EV_WARN << "EdgeTrust Fog " << fogId << ": Checkpoint file [" << filepath
                << "] not found. Initializing cold regional ledger." << endl;
        return false;
    }

    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    size_t ledPos = content.find("\"regional_ledger\":");
    if (ledPos == std::string::npos) {
        EV_WARN << "EdgeTrust Fog " << fogId << ": Invalid checkpoint format." << endl;
        return false;
    }

    int loadedCount = 0;
    size_t searchPos = ledPos;
    while (true) {
        size_t idPos = content.find("\"node_id\":", searchPos);
        if (idPos == std::string::npos) break;

        size_t braceStart = content.rfind('{', idPos);
        int depth = 0;
        size_t braceEnd = braceStart;
        for (size_t k = braceStart; k < content.length(); ++k) {
            if (content[k] == '{') depth++;
            else if (content[k] == '}') {
                depth--;
                if (depth == 0) {
                    braceEnd = k;
                    break;
                }
            }
        }

        std::string block = content.substr(braceStart, braceEnd - braceStart + 1);
        int vId = extractFogInt(block, "node_id");
        if (vId > 0) {
            FogVehicleRecord rec;
            rec.nodeId = vId;
            rec.regionalTrustScore = extractFogDouble(block, "regional_trust_score", 0.95);
            rec.regionalHistoricalTrust = extractFogDouble(block, "regional_historical_trust", 0.95);
            rec.isRevoked = extractFogBool(block, "is_revoked", false);
            rec.primaryAttackType = extractFogInt(block, "attack_type", 0);
            rec.revocationReason = extractFogString(block, "revocation_reason", "");
            rec.hasConflict = extractFogBool(block, "has_conflict", false);
            rec.conflictCount = extractFogInt(block, "conflict_count", 0);
            rec.cumulativeFalseInjections = extractFogInt(block, "cumulative_false_injections", 0);
            rec.cumulativeBlackholeDrops = extractFogInt(block, "cumulative_blackhole_drops", 0);
            rec.firstSeen = simTime();
            rec.lastSeen = simTime();

            if (rec.isRevoked) {
                regionalRevocationList.insert(vId);
            }

            regionalLedger[vId] = rec;
            loadedCount++;
        }
        searchPos = braceEnd + 1;
    }

    EV_INFO << "EdgeTrust Fog " << fogId << ": Restored " << loadedCount
            << " vehicle states from regional checkpoint [" << filepath << "]." << endl;
    return (loadedCount > 0);
}

Coord EdgeTrustFogApp::getModuleVisualPos(cModule* mod, const Coord& fallbackPos)
{
    if (mod) {
        const char* pTag = mod->getDisplayString().getTagArg("p", 0);
        const char* yTag = mod->getDisplayString().getTagArg("p", 1);
        if (pTag && yTag && pTag[0] != '\0' && yTag[0] != '\0') {
            try {
                double vx = std::stod(pTag);
                double vy = std::stod(yTag);
                return Coord(vx, vy, 0.0);
            } catch (...) {}
        }
    }
    return fallbackPos;
}

void EdgeTrustFogApp::updateFogCanvasVisual(const std::string& type, const std::string& color, const std::string& infoText)
{
    cModule* parent = getParentModule();
    if (!parent) return;
    cCanvas* canvas = parent->getCanvas();
    if (!canvas) return;
    cFigure* root = canvas->getRootFigure();
    if (!root) return;

    Coord pFog = Coord(posX, posY, 0.0);

    // Connect Fog to its managed RSUs
    for (int i = managedRsuOffset; i < managedRsuOffset + managedRsuCount; ++i) {
        cModule* rsuMod = parent->getSubmodule("rsu", i);
        Coord pRsu = getModuleVisualPos(rsuMod, Coord(i < 2 ? 58.0 : 178.0, (i % 2 == 0) ? 49.0 : 98.0, 0));
        std::string lineName = "fog" + std::to_string(fogId) + "_rsu" + std::to_string(i) + "_line";

        cLineFigure* line = dynamic_cast<cLineFigure*>(root->getFigure(lineName.c_str()));
        if (!line) {
            line = new cLineFigure(lineName.c_str());
            root->addFigure(line);
        }
        line->setStart(cFigure::Point(pRsu.x, pRsu.y));
        line->setEnd(cFigure::Point(pFog.x, pFog.y));
        line->setLineColor(cFigure::Color(color.c_str()));
        line->setLineWidth(type == "revocation" ? 1.6 : 1.0);
        line->setLineStyle(type == "revocation" ? cFigure::LINE_SOLID : cFigure::LINE_DASHED);
    }

    // If Fog 0, draw Inter-Fog trunk link between Fog 0 and Fog 1
    if (fogId == 0 && enableInterFog) {
        cModule* fog1Mod = parent->getSubmodule("fog", 1);
        if (fog1Mod) {
            Coord pFog1 = Coord(178.0, 15.0, 0.0);
            cLineFigure* trunk = dynamic_cast<cLineFigure*>(root->getFigure("fog_trunk_line"));
            if (!trunk) {
                trunk = new cLineFigure("fog_trunk_line");
                root->addFigure(trunk);
            }
            trunk->setStart(cFigure::Point(pFog.x, pFog.y));
            trunk->setEnd(cFigure::Point(pFog1.x, pFog1.y));
            trunk->setLineColor(cFigure::Color("mediumpurple"));
            trunk->setLineWidth(1.2);
            trunk->setLineStyle(cFigure::LINE_SOLID);
        }
    }

    // Label on Fog Node (compact 6pt bold, centered with halo)
    std::string labelName = "fog" + std::to_string(fogId) + "_status_label";
    cTextFigure* label = dynamic_cast<cTextFigure*>(root->getFigure(labelName.c_str()));
    if (!label) {
        label = new cTextFigure(labelName.c_str());
        root->addFigure(label);
    }
    label->setPosition(cFigure::Point(pFog.x, pFog.y - 7.0));
    label->setText(infoText.c_str());
    label->setColor(cFigure::Color(color.c_str()));
    label->setFont(cFigure::Font("", 6, cFigure::FONT_BOLD));
    label->setAnchor(cFigure::ANCHOR_CENTER);
    label->setHalo(true);
}

void EdgeTrustFogApp::clearFogCanvasVisual()
{
    cModule* parent = getParentModule();
    if (!parent) return;
    cCanvas* canvas = parent->getCanvas();
    if (!canvas) return;
    cFigure* root = canvas->getRootFigure();
    if (!root) return;

    for (int i = managedRsuOffset; i < managedRsuOffset + managedRsuCount; ++i) {
        std::string lineName = "fog" + std::to_string(fogId) + "_rsu" + std::to_string(i) + "_line";
        cFigure* l = root->getFigure(lineName.c_str());
        if (l) delete root->removeFigure(l);
    }
    std::string labelName = "fog" + std::to_string(fogId) + "_status_label";
    cFigure* lbl = root->getFigure(labelName.c_str());
    if (lbl) delete root->removeFigure(lbl);

    if (fogId == 0) {
        cFigure* trunk = root->getFigure("fog_trunk_line");
        if (trunk) delete root->removeFigure(trunk);
    }
}

void EdgeTrustFogApp::finish()
{
    clearFogCanvasVisual();

    if (saveCheckpointEnabled) {
        saveCheckpoint(checkpointFilePath);
    }

    if (fogSyncTimerMsg) {
        cancelAndDelete(fogSyncTimerMsg);
        fogSyncTimerMsg = nullptr;
    }
    if (checkpointTimerMsg) {
        cancelAndDelete(checkpointTimerMsg);
        checkpointTimerMsg = nullptr;
    }

    EV_INFO << "============================================================" << endl;
    EV_INFO << " [t=" << simTime() << "s] [FOG LAYER SUMMARY] Regional Fog Node " << fogId << " Finished" << endl;
    EV_INFO << "   -> Total Vehicles Monitored: " << regionalLedger.size() << endl;
    EV_INFO << "   -> RSU Digests Ingested: " << totalRSUDigestsIngested << endl;
    EV_INFO << "   -> Conflicts Reconciled: " << totalConflictsResolved << endl;
    EV_INFO << "   -> Stale Updates Dropped: " << totalStaleUpdatesDropped << endl;
    EV_INFO << "   -> Regional Revocations Dispatched: " << totalRevocationsIssued << endl;
    EV_INFO << "   -> Multi-Intersection Distributed Attacks Detected: " << totalCrossIntersectionAttacksDetected << endl;
    EV_INFO << "   -> Regional Master Syncs Pushed: " << totalMasterSyncsDispatched << endl;
    EV_INFO << "   -> Inter-Fog Syncs [Sent: " << totalInterFogSyncsSent << ", Recv: " << totalInterFogSyncsReceived << "]" << endl;
    EV_INFO << "   -> Inter-Fog Revocations [Sent: " << totalInterFogRevocationsSent << ", Recv: " << totalInterFogRevocationsReceived << "]" << endl;
    EV_INFO << "============================================================" << endl;

    recordScalar("edgeTrust_fogTrackedVehicles", regionalLedger.size());
    recordScalar("edgeTrust_fogDigestsIngested", totalRSUDigestsIngested);
    recordScalar("edgeTrust_fogConflictsResolved", totalConflictsResolved);
    recordScalar("edgeTrust_fogRevocationsIssued", totalRevocationsIssued);
    recordScalar("edgeTrust_fogMasterSyncsDispatched", totalMasterSyncsDispatched);
    recordScalar("edgeTrust_fogInterFogSyncsSent", totalInterFogSyncsSent);
    recordScalar("edgeTrust_fogInterFogSyncsReceived", totalInterFogSyncsReceived);
    recordScalar("edgeTrust_fogInterFogRevocationsSent", totalInterFogRevocationsSent);
    recordScalar("edgeTrust_fogInterFogRevocationsReceived", totalInterFogRevocationsReceived);
}

} // namespace veins
