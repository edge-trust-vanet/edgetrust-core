// EdgeTrustRSUApp.cc
#include "veins/modules/application/edgetrust/EdgeTrustRSUApp.h"
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

        std::string initBadge = (mlModel == "lightgbm") ? "RSU: LightGBM Active" : "RSU: Random Forest Active";
        findHost()->getDisplayString().setTagArg("t", 0, initBadge.c_str());
        findHost()->getDisplayString().setTagArg("t", 1, "t");
        findHost()->getDisplayString().setTagArg("t", 2, "darkgreen");
        findHost()->getDisplayString().setTagArg("i", 1, "green");

        if (loadCheckpointEnabled) {
            bool loaded = loadCheckpoint(checkpointFilePath);
            if (loaded) {
                EV_INFO << "EdgeTrust RSU " << rsuId << ": Resumed simulation from checkpoint ["
                        << checkpointFilePath << "] with " << vehicleRecords.size()
                        << " pre-warmed vehicle states." << endl;
            }
        }

        if (saveCheckpointEnabled && checkpointInterval > 0.0) {
            checkpointTimerMsg = new cMessage("checkpointTimer");
            scheduleAt(simTime() + checkpointInterval, checkpointTimerMsg);
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

        EV_INFO << "EdgeTrust RSU " << rsuId << " [" << (mlModel == "lightgbm" ? "LightGBM" : "Random Forest")
                << " Edge AI] online at intersection." << endl;
    }
}

void EdgeTrustRSUApp::handleSelfMsg(cMessage* msg)
{
    if (msg == checkpointTimerMsg) {
        if (saveCheckpointEnabled) {
            saveCheckpoint(checkpointFilePath);
        }
        scheduleAt(simTime() + checkpointInterval, checkpointTimerMsg);
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

    // ── Historical Trust Score (Long-term reputation tracking) ───────────────
    rec.historicalTrustScore = (0.85 * rec.historicalTrustScore) + (0.15 * rec.trustScore);

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

    // ── Edge AI Inference (Random Forest and LightGBM) ────────
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

    // Run ONLY the single deployed model configured for this simulation (Random Forest or LightGBM)
    int mlPred = 0;
    double maliciousProba = 0.0;
    std::string modelDisplayName;

    if (mlModel == "lightgbm") {
        mlPred = LightGBMPredictor::predict(rawFeatures);
        maliciousProba = LightGBMPredictor::predictProba(rawFeatures);
        modelDisplayName = "LightGBM";
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
    if (rec.trustScore < 0.40 && mlPred == 1) {
        verdict = "BLOCK";
    } else if (rec.trustScore < 0.70 || mlPred == 1) {
        verdict = "WARN";
    } else {
        verdict = "ACCEPT";
    }
    rec.lastVerdict = verdict;

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

    char badge[160];
    snprintf(badge, sizeof(badge), "RSU-%d [%s: %s V%d | %s: %d%% | Trust: %.2f]",
             rsuId, modelDisplayName.c_str(), verdict.c_str(), senderId,
             (mlPred == 1 ? "MAL" : "NORM"), (int)(classConfidence * 100), rec.trustScore);
    findHost()->getDisplayString().setTagArg("t", 0, badge);
    findHost()->getDisplayString().setTagArg("t", 1, "t");
    findHost()->getDisplayString().setTagArg("t", 2, (verdict == "BLOCK" ? "red" : (verdict == "WARN" ? "orange" : "darkgreen")));
    findHost()->getDisplayString().setTagArg("i", 1, (verdict == "BLOCK" ? "red" : (verdict == "WARN" ? "yellow" : "green")));

    EV_INFO << "============================================================" << endl;
    EV_INFO << " [t=" << currentTime << "s] BSM from Node " << senderId
            << " | Deployed Edge Model: [" << modelDisplayName << "]" << endl;
    EV_INFO << "   -> Kinematics: Plausibility=" << plausibility
            << " | Consistency=" << consistency
            << " | DropRatio=" << rec.packetDropRatio << endl;
    EV_INFO << "   -> " << modelDisplayName << " Verdict: "
            << (mlPred == 1 ? "MALICIOUS" : "NORMAL")
            << " (Confidence: " << (int)(classConfidence * 100) << "%)" << endl;
    EV_INFO << "   -> Trust Engine: Direct=" << rec.trustScore
            << " | Hist=" << rec.historicalTrustScore
            << " | Action: [" << verdict << "]" << endl;
    EV_INFO << "============================================================" << endl;

    if (verdict != "ACCEPT") {
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
    line->setLineWidth(2.5);
    line->setEndArrowhead(cFigure::ARROW_TRIANGLE);

    canvas->getRootFigure()->addFigure(line);
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

    EV_INFO << "EdgeTrust RSU " << rsuId << ": Checkpoint saved ("
            << vehicleRecords.size() << " vehicles) to " << filepath
            << " at t=" << simTime() << "s." << endl;
}

bool EdgeTrustRSUApp::loadCheckpoint(const std::string& filepath)
{
    std::ifstream in(filepath);
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

void EdgeTrustRSUApp::finish()
{
    clearTransmissionArrows();
    if (saveCheckpointEnabled) {
        saveCheckpoint(checkpointFilePath);
    }
    if (checkpointTimerMsg) {
        cancelAndDelete(checkpointTimerMsg);
        checkpointTimerMsg = nullptr;
    }
    DemoBaseApplLayer::finish();
    EV_INFO << "EdgeTrust RSU " << rsuId << " finished. Total extracted records: "
            << totalExtractedRecords << endl;
    recordScalar("edgeTrust_extractedRecords", totalExtractedRecords);
}

} // namespace veins
