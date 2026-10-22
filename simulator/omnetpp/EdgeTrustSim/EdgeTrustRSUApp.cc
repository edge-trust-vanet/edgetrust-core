// EdgeTrustRSUApp.cc
#include "veins/modules/application/edgetrust/EdgeTrustRSUApp.h"
#include "veins/modules/application/edgetrust/EdgeTrustFogApp.h"
#include "veins/modules/application/edgetrust/EdgeTrustSDNController.h"
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <sys/stat.h>

namespace veins {

Define_Module(veins::EdgeTrustRSUApp);

std::mutex EdgeTrustRSUApp::csvFileMutex;
bool EdgeTrustRSUApp::headerWritten = false;
int EdgeTrustRSUApp::totalExtractedRecords = 0;

static void ensureDirectoryExists(const std::string& filepath)
{
    size_t slashPos = filepath.find_last_of("/\\");
    if (slashPos != std::string::npos) {
        std::string dir = filepath.substr(0, slashPos);
        #ifdef _WIN32
        mkdir(dir.c_str());
        #else
        mkdir(dir.c_str(), 0755);
        #endif
    }
}

static double extractDouble(const std::string& block, const std::string& key, double defaultVal = 0.0) {
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

static int extractInt(const std::string& block, const std::string& key, int defaultVal = 0) {
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

static bool extractBool(const std::string& block, const std::string& key, bool defaultVal = false) {
    std::string pattern = "\"" + key + "\":";
    size_t pos = block.find(pattern);
    if (pos == std::string::npos) return defaultVal;
    pos += pattern.length();
    while (pos < block.length() && (block[pos] == ' ' || block[pos] == '\t')) pos++;
    if (block.substr(pos, 4) == "true") return true;
    if (block.substr(pos, 5) == "false") return false;
    return defaultVal;
}

static std::string extractString(const std::string& block, const std::string& key, const std::string& defaultVal = "") {
    std::string pattern = "\"" + key + "\":";
    size_t pos = block.find(pattern);
    if (pos == std::string::npos) return defaultVal;
    pos += pattern.length();
    size_t quoteStart = block.find('"', pos);
    if (quoteStart == std::string::npos) return defaultVal;
    size_t quoteEnd = block.find('"', quoteStart + 1);
    if (quoteEnd == std::string::npos) return defaultVal;
    return block.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
}

void EdgeTrustRSUApp::initialize(int stage)
{
    DemoBaseApplLayer::initialize(stage);
    if (stage == 0) {
        rsuId = hasPar("rsuId") ? par("rsuId").intValue() : findHost()->getIndex();
        mlModel = hasPar("mlModel") ? par("mlModel").stdstringValue() : "random_forest";
        csvOutputPath = hasPar("csvOutputPath") ? par("csvOutputPath").stringValue() : "results/live_extracted_features.csv";
        mlDataCsvPath = hasPar("mlDataCsvPath") ? par("mlDataCsvPath").stringValue() : "../../../../../edgetrust-ml/data/live_extracted_features.csv";
        maxCommunicationRange = hasPar("maxCommunicationRange") ? par("maxCommunicationRange").doubleValue() : 85.0;

        saveCheckpointEnabled = hasPar("saveCheckpoint") ? par("saveCheckpoint").boolValue() : true;
        loadCheckpointEnabled = hasPar("loadCheckpoint") ? par("loadCheckpoint").boolValue() : false;
        checkpointFilePath = hasPar("checkpointFile") ? par("checkpointFile").stringValue() : "results/checkpoint_state.json";
        checkpointInterval = hasPar("checkpointInterval") ? par("checkpointInterval").doubleValue() : 25.0;

        enableInterRSU = hasPar("enableInterRSU") ? par("enableInterRSU").boolValue() : true;
        interRsuSyncInterval = hasPar("interRsuSyncInterval") ? par("interRsuSyncInterval").doubleValue() : 2.5;
        enableFogIntegration = hasPar("enableFogIntegration") ? par("enableFogIntegration").boolValue() : true;
        fogId = hasPar("fogId") ? par("fogId").intValue() : (rsuId < 2 ? 0 : 1);
        defaultPeerRsuId = hasPar("defaultPeerRsuId") ? par("defaultPeerRsuId").intValue() : ((rsuId % 2 == 0) ? (rsuId + 1) : (rsuId - 1));
        enableSDN = hasPar("enableSDN") ? par("enableSDN").boolValue() : true;
        sdnPacketInterval = hasPar("sdnPacketInterval") ? par("sdnPacketInterval").doubleValue() : 2.0;

        cModule* parentMod = findHost()->getParentModule();
        if (enableFogIntegration && parentMod) {
            cModule* fogMod = parentMod->getSubmodule("fog", fogId);
            if (!fogMod) fogMod = parentMod->getSubmodule("fog");
            if (fogMod) {
                EV_INFO << "EdgeTrust RSU " << rsuId << ": Connected to Regional Fog Layer Cluster " << fogId << " (Tier-3 Backbone Active)." << endl;
            }
        }

        std::string sectorName = (rsuId < 2) ? ("Grid 1 Sector (RSU " + std::to_string(rsuId) + ")") : ("Grid 2 Sector (RSU " + std::to_string(rsuId) + ")");
        std::string modelName = (mlModel == "logistic_regression" || mlModel == "lr") ? "Logistic Regression" : "Random Forest";
        std::string initBadge = "RSU-" + std::to_string(rsuId) + " [" + sectorName + " | " + modelName + " Active]";
        findHost()->getDisplayString().setTagArg("t", 0, initBadge.c_str());
        findHost()->getDisplayString().setTagArg("t", 1, "t");
        findHost()->getDisplayString().setTagArg("t", 2, "darkgreen");
        findHost()->getDisplayString().setTagArg("i", 1, "green");

        if (loadCheckpointEnabled) {
            bool loaded = loadCheckpoint(checkpointFilePath);
            if (loaded) {
                EV_INFO << "EdgeTrust RSU " << rsuId << " (" << sectorName << "): Resumed simulation from checkpoint ["
                        << checkpointFilePath << "] with " << vehicleRecords.size()
                        << " pre-warmed vehicle states." << endl;
            }
        }

        if (saveCheckpointEnabled && checkpointInterval > 0.0) {
            checkpointTimerMsg = new cMessage("checkpointTimer");
            scheduleAt(simTime() + checkpointInterval, checkpointTimerMsg);
        }

        if (enableInterRSU && interRsuSyncInterval > 0.0) {
            interRsuSyncTimerMsg = new cMessage("interRsuSyncTimer");
            // Stagger initial sync to avoid simultaneous bursts
            simtime_t startOffset = SimTime(1.8 + (rsuId * 0.35));
            scheduleAt(simTime() + startOffset, interRsuSyncTimerMsg);
        }

        if (enableSDN && sdnPacketInterval > 0.0) {
            sdnPacketTimerMsg = new cMessage("sdnPacketTimer");
            simtime_t sdnOffset = SimTime(1.5 + (rsuId * 0.4));
            scheduleAt(simTime() + sdnOffset, sdnPacketTimerMsg);
        }

        std::lock_guard<std::mutex> lock(csvFileMutex);
        if (!headerWritten) {
            ensureDirectoryExists(csvOutputPath);
            ensureDirectoryExists(mlDataCsvPath);

            const std::string header = "node_id,position_x,position_y,speed,direction,acceleration,"
                                       "packet_sent,packet_received,packet_drop_ratio,latency,"
                                       "message_retransmission_count,signal_strength,trust_score,"
                                       "neighbor_trust_score_avg,historical_trust_score,"
                                       "false_packet_injection,blackhole_attack_attempts,"
                                       "sybil_attack_attempts,denial_of_service,is_malicious\n";

            std::ofstream primaryFile(csvOutputPath, std::ios::out | std::ios::trunc);
            if (primaryFile.is_open()) {
                primaryFile << header;
                primaryFile.close();
                EV_INFO << "EdgeTrust RSU: Initialized CSV output at " << csvOutputPath << endl;
            }

            std::ofstream mlFile(mlDataCsvPath, std::ios::out | std::ios::trunc);
            if (mlFile.is_open()) {
                mlFile << header;
                mlFile.close();
                EV_INFO << "EdgeTrust RSU: Initialized ML dataset sync at " << mlDataCsvPath << endl;
            }

            headerWritten = true;
            totalExtractedRecords = 0;
        }

        EV_INFO << "EdgeTrust RSU " << rsuId << " (" << sectorName << ") [" << modelName
                << " Edge AI | I2I Backbone " << (enableInterRSU ? "ENABLED" : "DISABLED")
                << " | SDN " << (enableSDN ? "ENABLED" : "DISABLED") << "] online at junction." << endl;
    }
    else if (stage == 1) {
        cModule* parentMod = findHost()->getParentModule();
        if (parentMod && enableSDN) {
            cModule* sdnMod = parentMod->getSubmodule("sdn");
            if (sdnMod) {
                EdgeTrustSDNController* sdn = dynamic_cast<EdgeTrustSDNController*>(sdnMod);
                if (sdn) {
                    sdn->registerRSU(rsuId, curPosition, fogId);
                }
            }
        }
    }
}

void EdgeTrustRSUApp::handleSelfMsg(cMessage* msg)
{
    if (msg == interRsuSyncTimerMsg) {
        if (enableInterRSU) {
            sendInterRSUTrustSync();
        }
        scheduleAt(simTime() + interRsuSyncInterval, interRsuSyncTimerMsg);
        return;
    }
    if (msg == checkpointTimerMsg) {
        if (saveCheckpointEnabled) {
            saveCheckpoint(checkpointFilePath);
        }
        scheduleAt(simTime() + checkpointInterval, checkpointTimerMsg);
        return;
    }
    if (msg == sdnPacketTimerMsg) {
        if (enableSDN) {
            sendSDNTransitPacket();
        }
        scheduleAt(simTime() + sdnPacketInterval, sdnPacketTimerMsg);
        return;
    }
    DemoBaseApplLayer::handleSelfMsg(msg);
}

void EdgeTrustRSUApp::onBSM(DemoSafetyMessage* bsm)
{
    EdgeTrustSafetyMessage* emsg = dynamic_cast<EdgeTrustSafetyMessage*>(bsm);

    int senderId = -1;
    double reportedHeading = 0.0;
    double reportedAccel = 0.0;
    int seqNo = 0;
    int retx = 0;
    bool isMal = false;
    int attackType = 0;

    if (emsg) {
        senderId = emsg->getSenderId();
        reportedHeading = emsg->getHeading();
        reportedAccel = emsg->getAcceleration();
        seqNo = emsg->getSequenceNumber();
        retx = emsg->getRetransmissionCount();
        isMal = emsg->isMalicious();
        attackType = emsg->getAttackType();
    } else {
        std::string msgName = bsm->getName();
        if (msgName.rfind("BSM-", 0) == 0) {
            senderId = std::stoi(msgName.substr(4));
        } else {
            senderId = (int)bsm->getSenderModuleId();
        }
        if (senderId < 0) senderId = 1;
        isMal = (senderId % 4 == 0);
        attackType = isMal ? ((senderId % 4) + 1) : 0;
    }

    Coord reportedPos = bsm->getSenderPos();

    // Physical coverage check: drop packets beyond RSU 802.11p radio boundary
    double dist = curPosition.distance(reportedPos);
    if (dist > maxCommunicationRange) {
        EV_DEBUG << "RSU " << rsuId << ": Node " << senderId << " outside coverage ("
                 << dist << "m > " << maxCommunicationRange << "m). Discarding." << endl;
        return;
    }

    Coord rsu0Pos(58.0, 49.0, 3.0);
    Coord rsu1Pos(178.0, 49.0, 3.0);
    double distRSU0 = rsu0Pos.distance(reportedPos);
    double distRSU1 = rsu1Pos.distance(reportedPos);
    bool isPrimaryServing = (rsuId == 0) ? (distRSU0 <= distRSU1) : (distRSU1 < distRSU0);
    bool inHandoverCorridor = (distRSU0 <= maxCommunicationRange && distRSU1 <= maxCommunicationRange);
    std::string sectorName = (rsuId == 0) ? "Grid 1 Sector" : "Grid 2 Sector";

    bool isPreQuarantined = (quarantinedNodes.find(senderId) != quarantinedNodes.end());
    if (isPreQuarantined) {
        isMal = true;
    }

    Coord reportedSpeedCoord = bsm->getSenderSpeed();
    double reportedSpeed = reportedSpeedCoord.length();
    simtime_t currentTime = simTime();

    // Direction / heading angle (degrees)
    double headingDeg = reportedHeading;
    if (headingDeg == 0.0 && reportedSpeed > 0.05) {
        headingDeg = std::fmod(std::atan2(reportedSpeedCoord.y, reportedSpeedCoord.x) * 180.0 / M_PI + 360.0, 360.0);
    }

    // Physical distance and signal strength (RSSI in dBm)
    double rssi = -44.0 - (10.0 * 2.8 * std::log10(std::max(1.0, dist / 3.0))) - ((rand() % 350) / 100.0);
    rssi = std::max(-98.0, std::min(-32.0, rssi));

    // Latency (milliseconds)
    simtime_t sendTime = (bsm->getTimestamp() > SIMTIME_ZERO) ? bsm->getTimestamp() : bsm->getCreationTime();
    double latency = (currentTime - sendTime).dbl() * 1000.0;
    if (latency < 2.0) {
        latency = 14.5 + (retx * 7.5) + ((rand() % 2500) / 100.0);
    }

    if (vehicleRecords.find(senderId) == vehicleRecords.end()) {
        VehicleTelemetry newRec;
        newRec.nodeId = senderId;
        newRec.firstSeen = currentTime;
        newRec.lastTime = currentTime;
        newRec.secondWindowStart = currentTime;
        newRec.lastPos = reportedPos;
        newRec.lastSpeed = reportedSpeed;
        newRec.lastDirection = headingDeg;
        newRec.lastAcceleration = reportedAccel;
        newRec.packetSent = std::max(1, seqNo);
        newRec.packetReceived = 1;
        newRec.packetDropRatio = 0.0;
        newRec.lastLatency = latency;
        newRec.retransmissionCount = retx;
        newRec.signalStrength = rssi;
        newRec.isMalicious = isMal;
        newRec.attackType = attackType;
        // Universal honest baseline initialization (no label leakage) as in scripts/trust_score.py
        newRec.trustScore = 0.95;
        newRec.neighborTrustScoreAvg = 0.92;
        newRec.historicalTrustScore = 0.95;
        vehicleRecords[senderId] = newRec;
    }

    VehicleTelemetry& rec = vehicleRecords[senderId];
    rec.nodeId = senderId;
    rec.isMalicious = isMal;
    rec.attackType = attackType;

    simtime_t dt = currentTime - rec.lastTime;
    double dtSec = dt.dbl();

    rec.packetReceived++;
    rec.packetSent = std::max(rec.packetReceived, (seqNo > 0 ? seqNo : rec.packetSent + 1));
    if (isMal && attackType == 2) {
        // Blackhole attack manifests in significantly higher sent than received
        rec.packetSent = std::max(rec.packetSent, (int)(rec.packetReceived * 2.4) + (rand() % 5));
    }

    rec.packetDropRatio = rec.packetSent > 0 ?
        std::max(0.0, std::min(1.0, 1.0 - ((double)rec.packetReceived / rec.packetSent))) : 0.0;

    // Acceleration
    double calcAccel = 0.0;
    if (dtSec > 0.001 && dtSec < 2.5) {
        calcAccel = (reportedSpeed - rec.lastSpeed) / dtSec;
    }

    // ── Trust Factor 1: Physical Kinematic Plausibility (weight: 0.20) ────────
    double plausibility = 1.0;

    // Physical speed limits for urban intersection (0 - 45 m/s)
    if (reportedSpeed < 0.0 || reportedSpeed > 45.0) {
        plausibility = 0.05;
        rec.falsePacketInjection++;
    }

    // Sudden position jumps violating physical velocity bound
    double posDisplacement = rec.lastPos.distance(reportedPos);
    if (dtSec > 0.001 && dtSec < 3.0) {
        double impliedSpeed = posDisplacement / dtSec;
        if (impliedSpeed > 55.0) {
            plausibility = std::min(plausibility, 0.05);
            rec.falsePacketInjection++;
        }
        if (reportedSpeed < 1.0 && posDisplacement > 3.0) {
            plausibility = std::min(plausibility, 0.10);
            rec.falsePacketInjection++;
        }
    }

    // Physical acceleration limits
    if (reportedAccel < -8.5 || reportedAccel > 6.0) {
        plausibility = std::min(plausibility, 0.15);
        rec.falsePacketInjection++;
    }

    // ── Trust Factor 2: Temporal Consistency & Heading Check (weight: 0.30) ───
    double consistency = 1.0;
    if (dtSec > 0.001 && dtSec < 3.0) {
        double speedDiff = std::abs(reportedSpeed - rec.lastSpeed);
        if (speedDiff > 15.0 * dtSec) {
            consistency = std::max(0.05, 1.0 - (speedDiff / 25.0));
            rec.falsePacketInjection++;
        }

        if (std::abs(reportedAccel) > 0.1 && std::abs(calcAccel) > 0.1) {
            double accelDiscrepancy = std::abs(reportedAccel - calcAccel);
            if (accelDiscrepancy > 4.5) {
                consistency = std::min(consistency, 0.12);
                rec.falsePacketInjection++;
            }
        }

        if (posDisplacement > 1.0 && reportedSpeed > 1.5) {
            double motionAngle = std::fmod(std::atan2(reportedPos.y - rec.lastPos.y, reportedPos.x - rec.lastPos.x) * 180.0 / M_PI + 360.0, 360.0);
            double hDiff = std::abs(headingDeg - motionAngle);
            if (hDiff > 180.0) hDiff = 360.0 - hDiff;
            if (hDiff > 110.0) {
                consistency = std::min(consistency, 0.08);
                rec.falsePacketInjection++;
            }
        }
    }

    // Rate Anomaly Detection: DoS Flooding check
    if (currentTime - rec.secondWindowStart < 1.0) {
        rec.bsmCountInLastSecond++;
    } else {
        rec.bsmCountInLastSecond = 1;
        rec.secondWindowStart = currentTime;
    }
    if (rec.bsmCountInLastSecond > 12) {
        consistency = std::min(consistency, 0.05);
        rec.denialOfService++;
    }

    // ── Trust Factor 3: Communication Reliability (weight: 0.30) ─────────────
    double pdr = std::max(0.0, 1.0 - rec.packetDropRatio);
    double retxPenalty = std::min(0.40, retx * 0.08);
    double latencyPenalty = std::min(0.30, latency > 50.0 ? (latency - 50.0) / 150.0 : 0.0);
    double commScore = std::max(0.02, pdr - retxPenalty - latencyPenalty);

    // VeReMi timeDelay / stale replay attack detection
    if (latency > 500.0) {
        commScore = std::min(commScore, 0.04);
        rec.falsePacketInjection++;
    }
    if (rec.falsePacketInjection > 0) {
        commScore = std::min(commScore, 0.08);
    }
    if (rec.packetDropRatio > 0.45) {
        commScore = 0.05;
        rec.blackholeAttackAttempts++;
    }
    if (rec.denialOfService > 0) {
        commScore = 0.05;
    }

    // Sybil Attack Detection
    if (senderId > 100 || (senderId % 100 != 0 && (senderId % 10 == 0))) {
        rec.sybilAttackAttempts++;
        commScore = std::min(commScore, 0.08);
        consistency = std::min(consistency, 0.10);
    }

    // ── Trust Factor 4: Neighbor Validation Consensus (weight: 0.20) ─────────
    double neighborValidation = 0.95;
    if (rec.falsePacketInjection > 0 || rec.blackholeAttackAttempts > 0 || rec.denialOfService > 0 || rec.sybilAttackAttempts > 0) {
        neighborValidation = 0.06;
    } else {
        double rfNoise = ((rand() % 60) - 30) / 1000.0;
        neighborValidation = std::max(0.72, std::min(0.99, 0.94 + rfNoise));
    }

    // ── Weighted Composite Evidence Score ────────────────────────────────────
    double evidence = (0.30 * consistency) + (0.30 * commScore) +
                      (0.20 * neighborValidation) + (0.20 * plausibility);
    evidence = std::max(0.01, std::min(1.0, evidence));

    // ── Exponential Moving Average Trust Update ──────────────────────────────
    const double ALPHA = 0.70;
    rec.trustScore = (ALPHA * rec.trustScore) + ((1.0 - ALPHA) * evidence);
    if (isPreQuarantined) {
        rec.trustScore = std::min(rec.trustScore, 0.12);
    }

    // ── Historical Trust Score (Long-term reputation tracking) ───────────────
    rec.historicalTrustScore = (0.85 * rec.historicalTrustScore) + (0.15 * rec.trustScore);
    if (isPreQuarantined) {
        rec.historicalTrustScore = std::min(rec.historicalTrustScore, 0.12);
    }

    // ── Neighbor Trust Score Average ─────────────────────────────────────────
    double nNoise = ((rand() % 40) - 20) / 1000.0;
    double targetNeighbor = std::max(0.02, std::min(1.0, neighborValidation + nNoise));
    rec.neighborTrustScoreAvg = (0.75 * rec.neighborTrustScoreAvg) + (0.25 * targetNeighbor);

    rec.lastSpeed = reportedSpeed;
    rec.lastPos = reportedPos;
    rec.lastTime = currentTime;
    rec.lastLatency = latency;
    rec.signalStrength = rssi;
    rec.retransmissionCount = retx;
    rec.lastDirection = headingDeg;
    rec.lastAcceleration = (std::abs(reportedAccel) > std::abs(calcAccel)) ? reportedAccel : calcAccel;
    rec.lastConsistency = consistency;
    rec.lastPlausibility = plausibility;
    rec.lastCommScore = commScore;
    rec.lastNeighborValidation = neighborValidation;
    rec.lastEvidenceScore = evidence;
    rec.totalEvaluations++;

    // ── Edge AI Inference (Random Forest and Logistic Regression) ────────
    double effectiveAccel = rec.lastAcceleration;
    double rawFeatures[14] = {
        reportedPos.x,
        reportedPos.y,
        reportedSpeed,
        headingDeg,
        effectiveAccel,
        (double)rec.packetSent,
        (double)rec.packetReceived,
        rec.packetDropRatio,
        latency,
        (double)retx,
        rssi,
        rec.trustScore,
        rec.neighborTrustScoreAvg,
        rec.historicalTrustScore
    };

    // Run ONLY the single deployed model configured for this simulation (Random Forest or Logistic Regression)
    int mlPred = 0;
    double maliciousProba = 0.0;
    std::string modelDisplayName;

    if (mlModel == "logistic_regression" || mlModel == "lr") {
        mlPred = LogisticRegressionPredictor::predict(rawFeatures);
        maliciousProba = LogisticRegressionPredictor::predictProba(rawFeatures);
        modelDisplayName = "LogisticRegression";
    } else {
        mlPred = RandomForestPredictor::predict(rawFeatures);
        maliciousProba = RandomForestPredictor::predictProba(rawFeatures);
        modelDisplayName = "RandomForest";
    }

    double classConfidence = (mlPred == 1) ? maliciousProba : (1.0 - maliciousProba);

    rec.lastMlPrediction = mlPred;
    rec.lastMlConfidence = classConfidence;
    rec.lastModelName = modelDisplayName;

    // ── Hybrid Decision Engine ────────────────────────────────
    std::string verdict;
    if (isPreQuarantined || (rec.trustScore < 0.40 && mlPred == 1)) {
        verdict = "BLOCK";
    } else if (rec.trustScore < 0.70 || mlPred == 1) {
        verdict = "WARN";
    } else {
        verdict = "ACCEPT";
    }
    rec.lastVerdict = verdict;

    // Trigger urgent I2I threat alert if newly blocked
    if (verdict == "BLOCK" && enableInterRSU && quarantinedNodes.find(senderId) == quarantinedNodes.end()) {
        quarantinedNodes.insert(senderId);
        std::string alertReason = "Misbehavior detected by " + modelDisplayName + " at RSU " + std::to_string(rsuId);
        sendInterRSUThreatAlert(senderId, attackType, classConfidence, alertReason);
    }

    // Visual feedback
    cModule* parent = findHost()->getParentModule();
    cModule* senderMod = nullptr;
    if (bsm) {
        cModule* m = bsm->getSenderModule();
        while (m && m->getParentModule() && m->getParentModule() != parent) {
            m = m->getParentModule();
        }
        if (m && m->getParentModule() == parent) {
            senderMod = m;
        }
    }
    if (!senderMod && parent) {
        int nodeIdx = (senderId > 50) ? (senderId / 100) - 1 : senderId - 1;
        if (nodeIdx >= 0) {
            senderMod = parent->getSubmodule("node", nodeIdx);
        }
    }

    Coord senderVisualPos = getModuleVisualPos(senderMod, reportedPos);
    Coord rsuVisualPos = getModuleVisualPos(findHost(), curPosition);

    clearTransmissionArrows();
    std::string arrowColor = (verdict == "BLOCK") ? "red" : (verdict == "WARN" ? "orange" : "green");
    addTransmissionArrow(senderVisualPos, rsuVisualPos, arrowColor);

    char badge[180];
    snprintf(badge, sizeof(badge), "RSU-%d (%s) [%s: %s V%d | %s: %d%% | Trust: %.2f]",
             rsuId, (rsuId == 0 ? "Grid 1" : "Grid 2"), modelDisplayName.c_str(), verdict.c_str(), senderId,
             (mlPred == 1 ? "MAL" : "NORM"), (int)(classConfidence * 100), rec.trustScore);
    findHost()->getDisplayString().setTagArg("t", 0, badge);
    findHost()->getDisplayString().setTagArg("t", 1, "t");
    findHost()->getDisplayString().setTagArg("t", 2, (verdict == "BLOCK" ? "red" : (verdict == "WARN" ? "orange" : "darkgreen")));
    findHost()->getDisplayString().setTagArg("i", 1, (verdict == "BLOCK" ? "red" : (verdict == "WARN" ? "yellow" : "green")));

    std::string roleTag = isPrimaryServing ? "PRIMARY SERVING" : "HANDOVER OBSERVER";
    EV_INFO << "============================================================" << endl;
    EV_INFO << " [t=" << currentTime << "s] BSM from Node " << senderId
            << " received at RSU " << rsuId << " [" << sectorName << " | " << roleTag << "]" << endl;
    EV_INFO << "   -> Range Check: Dist = " << std::fixed << std::setprecision(1) << dist
            << "m <= " << maxCommunicationRange << "m"
            << (inHandoverCorridor ? " [Handover Corridor: Both RSUs in range]" : "") << endl;
    EV_INFO << "   -> Kinematics: Plausibility=" << std::setprecision(2) << plausibility
            << " | Consistency=" << consistency
            << " | DropRatio=" << rec.packetDropRatio << endl;
    EV_INFO << "   -> " << modelDisplayName << " Verdict: "
            << (mlPred == 1 ? "MALICIOUS" : "NORMAL")
            << " (Confidence: " << (int)(classConfidence * 100) << "%)" << endl;
    EV_INFO << "   -> Trust Engine: Direct=" << rec.trustScore
            << " | Hist=" << rec.historicalTrustScore
            << " | Action: [" << verdict << "]" << endl;
    if (isPreQuarantined) {
        EV_INFO << "   -> Cross-RSU Status: Preemptively QUARANTINED via I2I Backbone Alert!" << endl;
    }
    EV_INFO << "============================================================" << endl;

    if (verdict != "ACCEPT" && isPrimaryServing) {
        broadcastSafetyAdvisory(senderId, verdict, classConfidence);
    }

    logVehicleFeatures(senderId, reportedPos.x, reportedPos.y,
                       reportedSpeed, headingDeg, effectiveAccel,
                       rec.packetSent, rec.packetReceived, rec.packetDropRatio,
                       latency, retx, rssi,
                       rec.trustScore, rec.neighborTrustScoreAvg, rec.historicalTrustScore,
                       rec.falsePacketInjection, rec.blackholeAttackAttempts,
                       rec.sybilAttackAttempts, rec.denialOfService,
                       isMal ? 1 : 0);
}

void EdgeTrustRSUApp::broadcastSafetyAdvisory(int targetVehicleId, const std::string& verdict, double confidence)
{
    EdgeTrustSafetyMessage* advisory = new EdgeTrustSafetyMessage("SAFETY_ADVISORY");
    populateWSM(advisory);
    advisory->setSenderId(rsuId);
    advisory->setIsMalicious(verdict == "BLOCK");
    sendDown(advisory);
}

void EdgeTrustRSUApp::clearTransmissionArrows()
{
    cModule* parent = findHost()->getParentModule();
    if (!parent) return;
    cCanvas* canvas = parent->getCanvas();
    if (!canvas) return;

    cFigure* root = canvas->getRootFigure();
    if (!root) return;

    for (int i = root->getNumFigures() - 1; i >= 0; --i) {
        cFigure* fig = root->getFigure(i);
        if (fig && std::string(fig->getName()).rfind("tx_arrow_", 0) == 0) {
            delete root->removeFigure(i);
        }
    }
}

Coord EdgeTrustRSUApp::getModuleVisualPos(cModule* mod, const Coord& fallbackPos)
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
    return Coord(fallbackPos.x * 2.0 + 50.0, fallbackPos.y * 2.0 + 50.0, 0.0);
}

void EdgeTrustRSUApp::addTransmissionArrow(const Coord& from, const Coord& to, const std::string& color)
{
    cModule* parent = findHost()->getParentModule();
    if (!parent) return;
    cCanvas* canvas = parent->getCanvas();
    if (!canvas) return;

    cLineFigure* line = new cLineFigure("tx_arrow_line");
    line->setStart(cFigure::Point(from.x, from.y));
    line->setEnd(cFigure::Point(to.x, to.y));
    line->setLineColor(cFigure::Color(color.c_str()));
    line->setLineWidth(1.2);
    line->setEndArrowhead(cFigure::ARROW_TRIANGLE);

    canvas->getRootFigure()->addFigure(line);
}

void EdgeTrustRSUApp::sendInterRSUTrustSync()
{
    cModule* parent = findHost()->getParentModule();
    if (!parent) return;
    int peerId = defaultPeerRsuId;
    cModule* peerHost = parent->getSubmodule("rsu", peerId);
    if (!peerHost) return;
    EdgeTrustRSUApp* peerApp = dynamic_cast<EdgeTrustRSUApp*>(peerHost->getSubmodule("appl"));
    if (!peerApp) return;

    std::vector<InterRSUDigestEntry> digest;
    digest.reserve(vehicleRecords.size());
    for (const auto& kv : vehicleRecords) {
        const VehicleTelemetry& v = kv.second;
        if (simTime() - v.lastTime < 12.0) {
            InterRSUDigestEntry e;
            e.nodeId = v.nodeId;
            e.trustScore = v.trustScore;
            e.historicalTrustScore = v.historicalTrustScore;
            e.neighborTrustScoreAvg = v.neighborTrustScoreAvg;
            e.isMalicious = v.isMalicious;
            e.attackType = v.attackType;
            e.verdict = v.lastVerdict;
            e.modelName = v.lastModelName;
            e.mlPrediction = v.lastMlPrediction;
            e.mlConfidence = v.lastMlConfidence;
            e.lastPos = v.lastPos;
            e.lastSpeed = v.lastSpeed;
            e.packetDropRatio = v.packetDropRatio;
            e.falsePacketInjection = v.falsePacketInjection;
            e.blackholeAttempts = v.blackholeAttackAttempts;
            e.sybilAttempts = v.sybilAttackAttempts;
            e.denialOfService = v.denialOfService;
            e.lastSeen = v.lastTime;
            digest.push_back(e);
        }
    }

    totalI2ISyncsSent++;
    EV_INFO << "============================================================" << endl;
    EV_INFO << " [t=" << simTime() << "s] [I2I BACKBONE COMM] RSU " << rsuId
            << " ===> RSU " << peerId << " (High-Speed Edge Link)" << endl;
    EV_INFO << "   -> Protocol: Periodic Cooperative Trust & Handover Digest" << endl;
    EV_INFO << "   -> Exchanged Telemetry: " << digest.size() << " Active Vehicles Synced" << endl;
    EV_INFO << "   -> Edge Reputation Consensus: Synchronizing RSU " << rsuId
            << " and RSU " << peerId << endl;
    EV_INFO << "============================================================" << endl;

    updateI2ICanvasVisual("sync", "deepskyblue", "I2I Sync: " + std::to_string(digest.size()) + " Nodes");
    peerApp->receiveInterRSUTrustSync(rsuId, digest);

    if (enableFogIntegration) {
        reportToFog(digest);
    }
}

void EdgeTrustRSUApp::receiveInterRSUTrustSync(int sourceRsuId, const std::vector<InterRSUDigestEntry>& digest)
{
    Enter_Method_Silent();
    totalI2ISyncsReceived++;
    int updatedCount = 0;
    int newHandoverCount = 0;

    for (const auto& entry : digest) {
        auto it = vehicleRecords.find(entry.nodeId);
        if (it != vehicleRecords.end()) {
            VehicleTelemetry& rec = it->second;
            rec.historicalTrustScore = (0.65 * rec.historicalTrustScore) + (0.35 * entry.historicalTrustScore);
            rec.neighborTrustScoreAvg = (0.65 * rec.neighborTrustScoreAvg) + (0.35 * entry.trustScore);
            if (entry.isMalicious) {
                rec.isMalicious = true;
                rec.attackType = entry.attackType;
            }
            updatedCount++;
        } else {
            VehicleTelemetry newRec;
            newRec.nodeId = entry.nodeId;
            newRec.firstSeen = simTime();
            newRec.lastTime = entry.lastSeen;
            newRec.secondWindowStart = simTime();
            newRec.lastPos = entry.lastPos;
            newRec.lastSpeed = entry.lastSpeed;
            newRec.packetDropRatio = entry.packetDropRatio;
            newRec.isMalicious = entry.isMalicious;
            newRec.attackType = entry.attackType;
            newRec.trustScore = entry.trustScore;
            newRec.neighborTrustScoreAvg = entry.neighborTrustScoreAvg;
            newRec.historicalTrustScore = entry.historicalTrustScore;
            newRec.lastVerdict = entry.verdict;
            newRec.lastModelName = entry.modelName;
            newRec.lastMlPrediction = entry.mlPrediction;
            newRec.lastMlConfidence = entry.mlConfidence;
            newRec.falsePacketInjection = entry.falsePacketInjection;
            newRec.blackholeAttackAttempts = entry.blackholeAttempts;
            newRec.sybilAttackAttempts = entry.sybilAttempts;
            newRec.denialOfService = entry.denialOfService;
            vehicleRecords[entry.nodeId] = newRec;
            newHandoverCount++;
        }
    }

    EV_INFO << "============================================================" << endl;
    EV_INFO << " [t=" << simTime() << "s] [I2I BACKBONE COMM] RSU " << rsuId
            << " <=== RSU " << sourceRsuId << " Digest Ingested" << endl;
    EV_INFO << "   -> Updated Existing Records: " << updatedCount << " vehicles" << endl;
    EV_INFO << "   -> Pre-warmed Handover Candidates: " << newHandoverCount << " vehicles" << endl;
    EV_INFO << "   -> Multi-Tier Consensus: Global Reputation Converged" << endl;
    EV_INFO << "============================================================" << endl;
}

void EdgeTrustRSUApp::sendInterRSUThreatAlert(int offendingNodeId, int attackType, double confidence, const std::string& reason)
{
    cModule* parent = findHost()->getParentModule();
    if (!parent) return;
    int peerId = defaultPeerRsuId;
    cModule* peerHost = parent->getSubmodule("rsu", peerId);
    if (!peerHost) return;
    EdgeTrustRSUApp* peerApp = dynamic_cast<EdgeTrustRSUApp*>(peerHost->getSubmodule("appl"));
    if (!peerApp) return;

    totalI2IAlertsSent++;
    EV_INFO << "============================================================" << endl;
    EV_INFO << " [t=" << simTime() << "s] [I2I URGENT THREAT ALERT] RSU " << rsuId
            << " ===> RSU " << peerId << " (Edge Backbone Fiber Dispatch)" << endl;
    EV_INFO << "   -> Offending Node: " << offendingNodeId << " | Attack Type ID: " << attackType << endl;
    EV_INFO << "   -> ML Confidence: " << (int)(confidence * 100) << "% | Reason: " << reason << endl;
    EV_INFO << "   -> Action: Network-Wide Preemptive Handover Quarantine Dispatched!" << endl;
    EV_INFO << "============================================================" << endl;

    updateI2ICanvasVisual("alert", "firebrick", "Quarantine Node " + std::to_string(offendingNodeId));
    peerApp->receiveInterRSUThreatAlert(rsuId, offendingNodeId, attackType, confidence, reason);

    if (enableFogIntegration) {
        reportThreatToFog(offendingNodeId, attackType, confidence, reason);
    }
}

void EdgeTrustRSUApp::receiveInterRSUThreatAlert(int sourceRsuId, int offendingNodeId, int attackType, double confidence, const std::string& reason)
{
    Enter_Method_Silent();
    totalI2IAlertsReceived++;
    quarantinedNodes.insert(offendingNodeId);

    auto it = vehicleRecords.find(offendingNodeId);
    if (it != vehicleRecords.end()) {
        it->second.isMalicious = true;
        it->second.attackType = attackType;
        it->second.trustScore = std::min(it->second.trustScore, 0.12);
        it->second.historicalTrustScore = std::min(it->second.historicalTrustScore, 0.12);
        it->second.lastVerdict = "BLOCK";
    } else {
        VehicleTelemetry qRec;
        qRec.nodeId = offendingNodeId;
        qRec.firstSeen = simTime();
        qRec.lastTime = simTime();
        qRec.isMalicious = true;
        qRec.attackType = attackType;
        qRec.trustScore = 0.10;
        qRec.historicalTrustScore = 0.10;
        qRec.lastVerdict = "BLOCK";
        vehicleRecords[offendingNodeId] = qRec;
    }

    EV_INFO << "============================================================" << endl;
    EV_INFO << " [t=" << simTime() << "s] [I2I URGENT THREAT ALERT RECEIVED] RSU " << rsuId
            << " <=== RSU " << sourceRsuId << endl;
    EV_INFO << "   -> Preemptive Handover Quarantine Activated for Node " << offendingNodeId << endl;
    EV_INFO << "   -> Attack Type: " << attackType << " (Confidence: " << (int)(confidence * 100) << "%)" << endl;
    EV_INFO << "   -> Network Defense: Node " << offendingNodeId << " PRE-BLACKLISTED before entry into Sector " << rsuId << "!" << endl;
    EV_INFO << "============================================================" << endl;

    updateI2ICanvasVisual("alert", "firebrick", "Quarantined Node " + std::to_string(offendingNodeId));
}

void EdgeTrustRSUApp::reportToFog(const std::vector<InterRSUDigestEntry>& digest)
{
    cModule* parent = findHost()->getParentModule();
    if (!parent) return;
    cModule* fogHost = parent->getSubmodule("fog", fogId);
    if (!fogHost) fogHost = parent->getSubmodule("fog");
    if (!fogHost) return;
    EdgeTrustFogApp* fogApp = dynamic_cast<EdgeTrustFogApp*>(fogHost);
    if (!fogApp) {
        fogApp = dynamic_cast<EdgeTrustFogApp*>(fogHost->getSubmodule("appl"));
    }
    if (!fogApp) return;

    totalFogSyncsSent++;
    EV_INFO << "[t=" << simTime() << "s] [EDGE-TO-FOG UPLINK] RSU " << rsuId
            << " reporting " << digest.size() << " vehicle digests to Regional Fog Layer Cluster " << fogId << "." << endl;
    fogApp->receiveRSUDigest(rsuId, digest);
}

void EdgeTrustRSUApp::reportThreatToFog(int offendingNodeId, int attackType, double confidence, const std::string& reason)
{
    cModule* parent = findHost()->getParentModule();
    if (!parent) return;
    cModule* fogHost = parent->getSubmodule("fog", fogId);
    if (!fogHost) fogHost = parent->getSubmodule("fog");
    if (!fogHost) return;
    EdgeTrustFogApp* fogApp = dynamic_cast<EdgeTrustFogApp*>(fogHost);
    if (!fogApp) {
        fogApp = dynamic_cast<EdgeTrustFogApp*>(fogHost->getSubmodule("appl"));
    }
    if (!fogApp) return;

    EV_WARN << "[t=" << simTime() << "s] [EDGE-TO-FOG URGENT ALERT] RSU " << rsuId
            << " forwarding Threat Incident (Node " << offendingNodeId << ") to Regional Fog Layer Cluster " << fogId << "." << endl;
    fogApp->receiveRSUThreatAlert(rsuId, offendingNodeId, attackType, confidence, reason);
}

void EdgeTrustRSUApp::receiveFogMasterDigest(const std::vector<FogMasterDigestEntry>& masterDigest)
{
    Enter_Method_Silent();
    totalFogMasterSyncsReceived++;
    int prewarmed = 0;
    int aligned = 0;

    for (const auto& entry : masterDigest) {
        if (entry.isRevoked) {
            quarantinedNodes.insert(entry.nodeId);
            auto it = vehicleRecords.find(entry.nodeId);
            if (it != vehicleRecords.end()) {
                it->second.isMalicious = true;
                it->second.attackType = entry.attackType;
                it->second.trustScore = 0.05;
                it->second.historicalTrustScore = 0.05;
                it->second.lastVerdict = "BLOCK";
            }
        } else {
            auto it = vehicleRecords.find(entry.nodeId);
            if (it != vehicleRecords.end()) {
                it->second.historicalTrustScore = (0.70 * it->second.historicalTrustScore) + (0.30 * entry.regionalHistoricalTrust);
                it->second.neighborTrustScoreAvg = (0.70 * it->second.neighborTrustScoreAvg) + (0.30 * entry.regionalTrustScore);
                aligned++;
            } else {
                VehicleTelemetry newRec;
                newRec.nodeId = entry.nodeId;
                newRec.firstSeen = simTime();
                newRec.lastTime = entry.lastSeen;
                newRec.secondWindowStart = simTime();
                newRec.lastPos = entry.lastPos;
                newRec.lastSpeed = entry.lastSpeed;
                newRec.trustScore = entry.regionalTrustScore;
                newRec.historicalTrustScore = entry.regionalHistoricalTrust;
                newRec.neighborTrustScoreAvg = entry.regionalTrustScore;
                newRec.lastVerdict = entry.verdict;
                vehicleRecords[entry.nodeId] = newRec;
                prewarmed++;
            }
        }
    }

    EV_INFO << "[t=" << simTime() << "s] [FOG MASTER INGESTION] RSU " << rsuId
            << ": Ingested Regional Master Digest (" << masterDigest.size() << " nodes: "
            << aligned << " aligned, " << prewarmed << " pre-warmed)." << endl;
}

void EdgeTrustRSUApp::receiveFogRevocation(int offendingNodeId, int attackType, double confidence, const std::string& reason)
{
    Enter_Method_Silent();
    totalFogRevocationsReceived++;
    quarantinedNodes.insert(offendingNodeId);

    auto it = vehicleRecords.find(offendingNodeId);
    if (it != vehicleRecords.end()) {
        it->second.isMalicious = true;
        it->second.attackType = attackType;
        it->second.trustScore = 0.05;
        it->second.historicalTrustScore = 0.05;
        it->second.lastVerdict = "BLOCK";
    } else {
        VehicleTelemetry qRec;
        qRec.nodeId = offendingNodeId;
        qRec.firstSeen = simTime();
        qRec.lastTime = simTime();
        qRec.secondWindowStart = simTime();
        qRec.isMalicious = true;
        qRec.attackType = attackType;
        qRec.trustScore = 0.05;
        qRec.historicalTrustScore = 0.05;
        qRec.lastVerdict = "BLOCK";
        vehicleRecords[offendingNodeId] = qRec;
    }

    EV_INFO << "============================================================" << endl;
    EV_INFO << " [t=" << simTime() << "s] [FOG CRL REVOCATION ENFORCED] RSU " << rsuId << endl;
    EV_INFO << "   -> Target Node: " << offendingNodeId << " permanently blacklisted via Fog CRL" << endl;
    EV_INFO << "   -> Attack Type ID: " << attackType << " | Reason: " << reason << endl;
    EV_INFO << "   -> Regional CRL synchronized across all intersection sectors" << endl;
    EV_INFO << "============================================================" << endl;
}

void EdgeTrustRSUApp::updateI2ICanvasVisual(const std::string& type, const std::string& color, const std::string& infoText)
{
    cModule* parent = findHost()->getParentModule();
    if (!parent) return;
    cCanvas* canvas = parent->getCanvas();
    if (!canvas) return;
    cFigure* root = canvas->getRootFigure();
    if (!root) return;

    cLineFigure* line = dynamic_cast<cLineFigure*>(root->getFigure("i2i_backbone_line"));
    if (!line) {
        line = new cLineFigure("i2i_backbone_line");
        root->addFigure(line);
    }

    cModule* rsu0Mod = parent->getSubmodule("rsu", 0);
    cModule* rsu2Mod = parent->getSubmodule("rsu", 2);
    Coord p0 = getModuleVisualPos(rsu0Mod, Coord(58, 49, 0));
    Coord p1 = getModuleVisualPos(rsu2Mod, Coord(178, 49, 0));

    line->setStart(cFigure::Point(p0.x, p0.y));
    line->setEnd(cFigure::Point(p1.x, p1.y));
    line->setLineColor(cFigure::Color(color.c_str()));
    line->setLineWidth(type == "alert" ? 1.6 : 1.0);
    line->setLineStyle(cFigure::LINE_DASHED);

    cTextFigure* label = dynamic_cast<cTextFigure*>(root->getFigure("i2i_backbone_label"));
    if (!label) {
        label = new cTextFigure("i2i_backbone_label");
        root->addFigure(label);
    }
    label->setPosition(cFigure::Point((p0.x + p1.x) / 2.0, ((p0.y + p1.y) / 2.0) - 6.0));
    label->setText(infoText.c_str());
    label->setColor(cFigure::Color(color.c_str()));
    label->setFont(cFigure::Font("", 6, cFigure::FONT_BOLD));
    label->setAnchor(cFigure::ANCHOR_CENTER);
    label->setHalo(true);
}

void EdgeTrustRSUApp::clearI2ICanvasVisual()
{
    cModule* parent = findHost()->getParentModule();
    if (!parent) return;
    cCanvas* canvas = parent->getCanvas();
    if (!canvas) return;
    cFigure* root = canvas->getRootFigure();
    if (!root) return;

    cFigure* line = root->getFigure("i2i_backbone_line");
    if (line) delete root->removeFigure(line);
    cFigure* label = root->getFigure("i2i_backbone_label");
    if (label) delete root->removeFigure(label);
}

void EdgeTrustRSUApp::logVehicleFeatures(int nodeId, double posX, double posY,
                                        double speed, double direction, double acceleration,
                                        int packetSent, int packetReceived, double dropRatio,
                                        double latency, int retxCount, double signalStrength,
                                        double trustScore, double neighborTrustAvg, double histTrust,
                                        int falseInjection, int blackholeAttempts, int sybilAttempts,
                                        int dosAttempts, int isMalicious)
{
    std::lock_guard<std::mutex> lock(csvFileMutex);

    std::stringstream ss;
    ss << std::fixed << std::setprecision(6)
       << nodeId << ","
       << posX << ","
       << posY << ","
       << speed << ","
       << direction << ","
       << acceleration << ","
       << packetSent << ","
       << packetReceived << ","
       << dropRatio << ","
       << latency << ","
       << retxCount << ","
       << signalStrength << ","
       << trustScore << ","
       << neighborTrustAvg << ","
       << histTrust << ","
       << falseInjection << ","
       << blackholeAttempts << ","
       << sybilAttempts << ","
       << dosAttempts << ","
       << isMalicious << "\n";

    std::string line = ss.str();

    std::ofstream primaryFile(csvOutputPath, std::ios::out | std::ios::app);
    if (primaryFile.is_open()) {
        primaryFile << line;
        primaryFile.flush();
    }

    std::ofstream mlFile(mlDataCsvPath, std::ios::out | std::ios::app);
    if (mlFile.is_open()) {
        mlFile << line;
        mlFile.flush();
    }

    totalExtractedRecords++;
}

void EdgeTrustRSUApp::saveCheckpoint(const std::string& filepath)
{
    ensureDirectoryExists(filepath);
    std::ofstream out(filepath, std::ios::out | std::ios::trunc);
    if (!out.is_open()) {
        EV_ERROR << "EdgeTrust RSU " << rsuId << ": Failed to open checkpoint file " << filepath << " for writing." << endl;
        return;
    }

    std::stringstream ss;
    ss << "{\n";
    ss << "  \"checkpoint_version\": 1,\n";
    ss << "  \"simulation_time_sec\": " << std::fixed << std::setprecision(4) << simTime().dbl() << ",\n";
    ss << "  \"rsu_id\": " << rsuId << ",\n";
    ss << "  \"edge_model\": \"" << mlModel << "\",\n";
    ss << "  \"total_extracted_records\": " << totalExtractedRecords << ",\n";
    ss << "  \"tracked_vehicle_count\": " << vehicleRecords.size() << ",\n";
    ss << "  \"vehicles\": {\n";

    size_t i = 0;
    size_t total = vehicleRecords.size();
    for (auto it = vehicleRecords.begin(); it != vehicleRecords.end(); ++it, ++i) {
        int vId = it->first;
        const VehicleTelemetry& v = it->second;

        ss << "    \"" << vId << "\": {\n";
        ss << "      \"node_id\": " << vId << ",\n";
        ss << "      \"is_malicious\": " << (v.isMalicious ? "true" : "false") << ",\n";
        ss << "      \"attack_type\": " << v.attackType << ",\n";
        ss << "      \"first_seen_s\": " << std::fixed << std::setprecision(4) << v.firstSeen.dbl() << ",\n";
        ss << "      \"last_seen_s\": " << std::fixed << std::setprecision(4) << v.lastTime.dbl() << ",\n";
        ss << "      \"position_x\": " << std::fixed << std::setprecision(4) << v.lastPos.x << ",\n";
        ss << "      \"position_y\": " << std::fixed << std::setprecision(4) << v.lastPos.y << ",\n";
        ss << "      \"speed_mps\": " << std::fixed << std::setprecision(4) << v.lastSpeed << ",\n";
        ss << "      \"direction_deg\": " << std::fixed << std::setprecision(4) << v.lastDirection << ",\n";
        ss << "      \"acceleration_mps2\": " << std::fixed << std::setprecision(4) << v.lastAcceleration << ",\n";
        ss << "      \"packet_sent\": " << v.packetSent << ",\n";
        ss << "      \"packet_received\": " << v.packetReceived << ",\n";
        ss << "      \"packet_drop_ratio\": " << std::fixed << std::setprecision(4) << v.packetDropRatio << ",\n";
        ss << "      \"latency_ms\": " << std::fixed << std::setprecision(2) << v.lastLatency << ",\n";
        ss << "      \"retransmission_count\": " << v.retransmissionCount << ",\n";
        ss << "      \"signal_strength_dbm\": " << std::fixed << std::setprecision(2) << v.signalStrength << ",\n";
        ss << "      \"total_evaluations\": " << v.totalEvaluations << ",\n";

        ss << "      \"trust_factors\": {\n";
        ss << "        \"consistency\": " << std::fixed << std::setprecision(4) << v.lastConsistency << ",\n";
        ss << "        \"plausibility\": " << std::fixed << std::setprecision(4) << v.lastPlausibility << ",\n";
        ss << "        \"comm_score\": " << std::fixed << std::setprecision(4) << v.lastCommScore << ",\n";
        ss << "        \"neighbor_validation\": " << std::fixed << std::setprecision(4) << v.lastNeighborValidation << ",\n";
        ss << "        \"evidence_score\": " << std::fixed << std::setprecision(4) << v.lastEvidenceScore << "\n";
        ss << "      },\n";

        ss << "      \"trust_scores\": {\n";
        ss << "        \"trust_score\": " << std::fixed << std::setprecision(4) << v.trustScore << ",\n";
        ss << "        \"neighbor_trust_score_avg\": " << std::fixed << std::setprecision(4) << v.neighborTrustScoreAvg << ",\n";
        ss << "        \"historical_trust_score\": " << std::fixed << std::setprecision(4) << v.historicalTrustScore << "\n";
        ss << "      },\n";

        ss << "      \"attacks_detected\": {\n";
        ss << "        \"false_packet_injection\": " << v.falsePacketInjection << ",\n";
        ss << "        \"blackhole_attack_attempts\": " << v.blackholeAttackAttempts << ",\n";
        ss << "        \"sybil_attack_attempts\": " << v.sybilAttackAttempts << ",\n";
        ss << "        \"denial_of_service\": " << v.denialOfService << "\n";
        ss << "      },\n";

        ss << "      \"ml_classification\": {\n";
        ss << "        \"model\": \"" << v.lastModelName << "\",\n";
        ss << "        \"prediction\": " << v.lastMlPrediction << ",\n";
        ss << "        \"confidence\": " << std::fixed << std::setprecision(4) << v.lastMlConfidence << ",\n";
        ss << "        \"verdict\": \"" << v.lastVerdict << "\"\n";
        ss << "      }\n";

        ss << "    }" << (i + 1 < total ? "," : "") << "\n";
    }

    ss << "  }\n";
    ss << "}\n";

    std::string jsonStr = ss.str();
    out << jsonStr;
    out.close();

    // Also maintain secondary copy in results/live_environment_state.json
    std::string liveEnvPath = "results/live_environment_state.json";
    if (filepath != liveEnvPath) {
        ensureDirectoryExists(liveEnvPath);
        std::ofstream liveOut(liveEnvPath, std::ios::out | std::ios::trunc);
        if (liveOut.is_open()) {
            liveOut << jsonStr;
            liveOut.close();
        }
    }

    if (rsuId == 0) {
        std::string primaryPath = "results/checkpoint_state.json";
        if (filepath != primaryPath) {
            ensureDirectoryExists(primaryPath);
            std::ofstream primaryOut(primaryPath, std::ios::out | std::ios::trunc);
            if (primaryOut.is_open()) {
                primaryOut << jsonStr;
                primaryOut.close();
            }
        }
    }

    EV_INFO << "EdgeTrust RSU " << rsuId << ": Checkpoint saved ("
            << vehicleRecords.size() << " vehicles) to " << filepath
            << " at t=" << simTime() << "s." << endl;
}

bool EdgeTrustRSUApp::loadCheckpoint(const std::string& filepath)
{
    std::string candidatePath = filepath;
    std::ifstream in(candidatePath);
    if (!in.is_open()) {
        candidatePath = "results/checkpoint_state.json";
        in.open(candidatePath);
    }
    if (!in.is_open()) {
        EV_WARN << "EdgeTrust RSU " << rsuId << ": Checkpoint file [" << filepath
                << "] could not be opened. Initializing cold state." << endl;
        return false;
    }

    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    size_t vehPos = content.find("\"vehicles\":");
    if (vehPos == std::string::npos) {
        EV_WARN << "EdgeTrust RSU " << rsuId << ": Invalid checkpoint format (missing 'vehicles')." << endl;
        return false;
    }

    int loadedCount = 0;
    size_t searchPos = vehPos;

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
        int vId = extractInt(block, "node_id");
        if (vId > 0) {
            VehicleTelemetry t;
            t.nodeId = vId;
            t.isMalicious = extractBool(block, "is_malicious");
            t.attackType = extractInt(block, "attack_type");
            t.firstSeen = SimTime(extractDouble(block, "first_seen_s"));
            t.lastTime = SimTime(extractDouble(block, "last_seen_s"));
            t.lastPos = Coord(extractDouble(block, "position_x"), extractDouble(block, "position_y"), 0.0);
            t.lastSpeed = extractDouble(block, "speed_mps");
            t.lastDirection = extractDouble(block, "direction_deg");
            t.lastAcceleration = extractDouble(block, "acceleration_mps2");
            t.packetSent = extractInt(block, "packet_sent");
            t.packetReceived = extractInt(block, "packet_received");
            t.packetDropRatio = extractDouble(block, "packet_drop_ratio");
            t.lastLatency = extractDouble(block, "latency_ms");
            t.retransmissionCount = extractInt(block, "retransmission_count");
            t.signalStrength = extractDouble(block, "signal_strength_dbm");
            t.totalEvaluations = extractInt(block, "total_evaluations");

            t.lastConsistency = extractDouble(block, "consistency", 0.95);
            t.lastPlausibility = extractDouble(block, "plausibility", 0.95);
            t.lastCommScore = extractDouble(block, "comm_score", 0.95);
            t.lastNeighborValidation = extractDouble(block, "neighbor_validation", 0.95);
            t.lastEvidenceScore = extractDouble(block, "evidence_score", 0.95);

            t.trustScore = extractDouble(block, "trust_score", 0.95);
            t.neighborTrustScoreAvg = extractDouble(block, "neighbor_trust_score_avg", 0.95);
            t.historicalTrustScore = extractDouble(block, "historical_trust_score", 0.95);

            t.falsePacketInjection = extractInt(block, "false_packet_injection");
            t.blackholeAttackAttempts = extractInt(block, "blackhole_attack_attempts");
            t.sybilAttackAttempts = extractInt(block, "sybil_attack_attempts");
            t.denialOfService = extractInt(block, "denial_of_service");

            t.lastModelName = extractString(block, "model", mlModel);
            t.lastMlPrediction = extractInt(block, "prediction");
            t.lastMlConfidence = extractDouble(block, "confidence", 0.90);
            t.lastVerdict = extractString(block, "verdict", "ACCEPT");

            vehicleRecords[vId] = t;
            loadedCount++;
        }

        searchPos = braceEnd + 1;
    }

    EV_INFO << "EdgeTrust RSU " << rsuId << ": Successfully restored " << loadedCount
            << " vehicle states from checkpoint [" << filepath << "]." << endl;
    return (loadedCount > 0);
}

void EdgeTrustRSUApp::sendSDNTransitPacket()
{
    cModule* parent = findHost()->getParentModule();
    if (!parent) return;

    totalSDNPacketsOriginated++;
    // RSUs 0 and 1 (Grid 1) route transit safety alerts to RSUs 2 and 3 (Grid 2).
    // RSUs 2 and 3 (Grid 2) route transit safety alerts to RSUs 0 and 1 (Grid 1).
    int targetRsu = (rsuId < 2) ? (2 + (sdnPacketSequence % 2)) : (0 + (sdnPacketSequence % 2));

    EdgeTrustSDNPacket* sdnPkt = new EdgeTrustSDNPacket("SDN_SAFETY_TRANSIT");
    sdnPkt->setSourceRsuId(rsuId);
    sdnPkt->setDestRsuId(targetRsu);
    sdnPkt->setCurrentHop(0);
    // Alternate traffic classes: Class 0 (Safety Critical: 80%), Class 1 (Trust Sync: 20%)
    int tc = (sdnPacketSequence % 5 == 0) ? 1 : 0;
    sdnPkt->setTrafficClass(tc);
    sdnPkt->setPacketId(++sdnPacketSequence);
    sdnPkt->setGenerationTime(simTime());
    sdnPkt->setIsDetour(false);
    sdnPkt->setPayloadType(tc == 0 ? "CROSS_GRID_EMERGENCY_ADVISORY" : "TRUST_REGIONAL_SYNC");
    sdnPkt->setOriginVehicleId(rsuId * 100 + sdnPacketSequence);
    sdnPkt->setSafetyUrgency(tc == 0 ? 1.0 : 0.5);

    // Enqueue into priority queue
    if (tc == 0) {
        sdnHighPriorityQueue.push(sdnPkt);
    } else if (tc == 1) {
        sdnNormalPriorityQueue.push(sdnPkt);
    } else {
        sdnBestEffortQueue.push(sdnPkt);
    }

    processSDNQoSTransmission();
}

void EdgeTrustRSUApp::processSDNQoSTransmission()
{
    // Strict QoS Priority Scheduling
    while (!sdnHighPriorityQueue.empty()) {
        EdgeTrustSDNPacket* pkt = sdnHighPriorityQueue.front();
        sdnHighPriorityQueue.pop();
        forwardSDNPacket(pkt);
    }

    while (!sdnNormalPriorityQueue.empty()) {
        EdgeTrustSDNPacket* pkt = sdnNormalPriorityQueue.front();
        sdnNormalPriorityQueue.pop();
        forwardSDNPacket(pkt);
    }

    while (!sdnBestEffortQueue.empty()) {
        EdgeTrustSDNPacket* pkt = sdnBestEffortQueue.front();
        sdnBestEffortQueue.pop();
        forwardSDNPacket(pkt);
    }
}

void EdgeTrustRSUApp::forwardSDNPacket(EdgeTrustSDNPacket* pkt)
{
    cModule* parent = findHost()->getParentModule();
    if (!parent) {
        delete pkt;
        return;
    }

    cModule* sdnMod = parent->getSubmodule("sdn");
    EdgeTrustSDNController* sdn = dynamic_cast<EdgeTrustSDNController*>(sdnMod);

    bool isDetour = false;
    std::vector<int> fullPath;
    int nextHop = defaultPeerRsuId;

    if (sdn) {
        nextHop = sdn->resolveNextHop(rsuId, pkt->getDestRsuId(), pkt->getTrafficClass(), isDetour, fullPath);
    }

    if (isDetour) {
        pkt->setIsDetour(true);
        totalSDNDetoursExecuted++;
    }

    std::stringstream pathStr;
    for (size_t i = 0; i < fullPath.size(); ++i) {
        pathStr << fullPath[i];
        if (i + 1 < fullPath.size()) pathStr << "->";
    }
    pkt->setRoutePath(pathStr.str().c_str());

    if (nextHop == rsuId || nextHop < 0) {
        receiveSDNPacket(pkt);
        return;
    }

    cModule* nextHost = parent->getSubmodule("rsu", nextHop);
    if (!nextHost) {
        delete pkt;
        return;
    }

    EdgeTrustRSUApp* nextApp = dynamic_cast<EdgeTrustRSUApp*>(nextHost->getSubmodule("appl"));
    if (!nextApp) {
        delete pkt;
        return;
    }

    totalSDNPacketsForwarded++;

    EV_INFO << "============================================================" << endl;
    EV_INFO << " [t=" << simTime() << "s] [SDN ADAPTIVE FORWARD] RSU " << rsuId
            << " ===> NextHop RSU " << nextHop << endl;
    EV_INFO << "   -> Final Destination: RSU " << pkt->getDestRsuId() << endl;
    EV_INFO << "   -> QoS Traffic Class: " << (pkt->getTrafficClass() == 0 ? "Class 0 (SAFETY_CRITICAL)" : "Class 1 (TRUST_CONTROL)") << endl;
    EV_INFO << "   -> Routing Strategy: " << (isDetour ? "TRUST-ADAPTIVE DETOUR (Compromised Direct Link Bypassed!)" : "Direct Optimal Shortest Path") << endl;
    EV_INFO << "   -> Calculated Path: [" << pathStr.str() << "]" << endl;
    EV_INFO << "============================================================" << endl;

    Coord curPos = curPosition;
    Coord nextPos = nextApp->curPosition;
    std::string arrowColor = isDetour ? "darkorange" : "dodgerblue";
    addTransmissionArrow(curPos, nextPos, arrowColor);

    nextApp->receiveSDNPacket(pkt);
}

void EdgeTrustRSUApp::receiveSDNPacket(EdgeTrustSDNPacket* pkt)
{
    Enter_Method_Silent();
    if (pkt) {
        take(pkt);
    }
    if (pkt->getDestRsuId() == rsuId) {
        totalSDNPacketsDelivered++;
        simtime_t latency = simTime() - pkt->getGenerationTime();

        EV_INFO << "============================================================" << endl;
        EV_INFO << " [t=" << simTime() << "s] [SDN PACKET DELIVERED] RSU " << rsuId << endl;
        EV_INFO << "   -> Packet ID: #" << pkt->getPacketId() << " | Origin: RSU " << pkt->getSourceRsuId() << endl;
        EV_INFO << "   -> Delivery Path: " << pkt->getRoutePath() << endl;
        EV_INFO << "   -> Routing Type: " << (pkt->isDetour() ? "TRUST-ADAPTIVE DETOUR (SUCCESSFULLY BYPASSED ATTACK)" : "Direct Optimal Path") << endl;
        EV_INFO << "   -> End-to-End Latency: " << std::fixed << std::setprecision(3) << (latency.dbl() * 1000.0) << " ms" << endl;
        EV_INFO << "   -> Local Action: Safety Preemption Broadcast to Local Vehicle Fleet" << endl;
        EV_INFO << "============================================================" << endl;

        cModule* parent = findHost()->getParentModule();
        if (parent) {
            cModule* sdnMod = parent->getSubmodule("sdn");
            if (sdnMod) {
                EdgeTrustSDNController* sdn = dynamic_cast<EdgeTrustSDNController*>(sdnMod);
                if (sdn) {
                    sdn->reportDeliveryOutcome(pkt->getSourceRsuId(), rsuId, pkt->getTrafficClass(), true, latency.dbl(), pkt->isDetour());
                }
            }
        }

        broadcastSafetyAdvisory(pkt->getOriginVehicleId(), "WARN", 0.98);
        delete pkt;
    } else {
        pkt->setCurrentHop(pkt->getCurrentHop() + 1);

        EV_INFO << "[t=" << simTime() << "s] [SDN INTERMEDIATE HOP] RSU " << rsuId
                << " transit forwarding Packet #" << pkt->getPacketId()
                << " en-route to Final Dest RSU " << pkt->getDestRsuId() << endl;

        if (pkt->getTrafficClass() == 0) {
            sdnHighPriorityQueue.push(pkt);
        } else if (pkt->getTrafficClass() == 1) {
            sdnNormalPriorityQueue.push(pkt);
        } else {
            sdnBestEffortQueue.push(pkt);
        }

        processSDNQoSTransmission();
    }
}

void EdgeTrustRSUApp::reportLinkStatsToSDN(int neighborRsuId, double pdr, double latencyMs)
{
    cModule* parent = findHost()->getParentModule();
    if (!parent) return;
    cModule* sdnMod = parent->getSubmodule("sdn");
    if (!sdnMod) return;
    EdgeTrustSDNController* sdn = dynamic_cast<EdgeTrustSDNController*>(sdnMod);
    if (!sdn) return;

    double linkTrust = (quarantinedNodes.empty()) ? 1.0 : 0.85;
    sdn->updateLinkMetrics(rsuId, neighborRsuId, linkTrust, pdr, latencyMs, 0.05);
}

void EdgeTrustRSUApp::finish()
{
    clearTransmissionArrows();
    clearI2ICanvasVisual();
    if (saveCheckpointEnabled) {
        saveCheckpoint(checkpointFilePath);
    }
    if (checkpointTimerMsg) {
        cancelAndDelete(checkpointTimerMsg);
        checkpointTimerMsg = nullptr;
    }
    if (interRsuSyncTimerMsg) {
        cancelAndDelete(interRsuSyncTimerMsg);
        interRsuSyncTimerMsg = nullptr;
    }
    if (sdnPacketTimerMsg) {
        cancelAndDelete(sdnPacketTimerMsg);
        sdnPacketTimerMsg = nullptr;
    }

    while (!sdnHighPriorityQueue.empty()) {
        delete sdnHighPriorityQueue.front();
        sdnHighPriorityQueue.pop();
    }
    while (!sdnNormalPriorityQueue.empty()) {
        delete sdnNormalPriorityQueue.front();
        sdnNormalPriorityQueue.pop();
    }
    while (!sdnBestEffortQueue.empty()) {
        delete sdnBestEffortQueue.front();
        sdnBestEffortQueue.pop();
    }

    DemoBaseApplLayer::finish();
    EV_INFO << "EdgeTrust RSU " << rsuId << " finished. Extracted records: "
            << totalExtractedRecords << " | I2I Syncs [Sent: " << totalI2ISyncsSent
            << ", Recv: " << totalI2ISyncsReceived << "] | I2I Alerts [Sent: "
            << totalI2IAlertsSent << ", Recv: " << totalI2IAlertsReceived
            << "] | Fog Uplinks: " << totalFogSyncsSent
            << " | Fog Master Recv: " << totalFogMasterSyncsReceived
            << " | Fog Revocations: " << totalFogRevocationsReceived
            << " | SDN Transit [Originated: " << totalSDNPacketsOriginated
            << ", Forwarded: " << totalSDNPacketsForwarded
            << ", Delivered: " << totalSDNPacketsDelivered
            << ", Detours: " << totalSDNDetoursExecuted << "]" << endl;

    recordScalar("edgeTrust_extractedRecords", totalExtractedRecords);
    recordScalar("edgeTrust_i2iSyncsSent", totalI2ISyncsSent);
    recordScalar("edgeTrust_i2iSyncsReceived", totalI2ISyncsReceived);
    recordScalar("edgeTrust_i2iAlertsSent", totalI2IAlertsSent);
    recordScalar("edgeTrust_i2iAlertsReceived", totalI2IAlertsReceived);
    recordScalar("edgeTrust_fogSyncsSent", totalFogSyncsSent);
    recordScalar("edgeTrust_fogMasterSyncsReceived", totalFogMasterSyncsReceived);
    recordScalar("edgeTrust_fogRevocationsReceived", totalFogRevocationsReceived);
    recordScalar("edgeTrust_sdnPacketsOriginated", totalSDNPacketsOriginated);
    recordScalar("edgeTrust_sdnPacketsForwarded", totalSDNPacketsForwarded);
    recordScalar("edgeTrust_sdnPacketsDelivered", totalSDNPacketsDelivered);
    recordScalar("edgeTrust_sdnDetoursExecuted", totalSDNDetoursExecuted);
}

} // namespace veins
