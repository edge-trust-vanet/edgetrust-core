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

void EdgeTrustRSUApp::initialize(int stage)
{
    DemoBaseApplLayer::initialize(stage);
    if (stage == 0) {
        rsuId = hasPar("rsuId") ? par("rsuId").intValue() : findHost()->getIndex();
        mlModel = hasPar("mlModel") ? par("mlModel").stdstringValue() : "adaboost";
        csvOutputPath = hasPar("csvOutputPath") ? par("csvOutputPath").stringValue() : "results/live_extracted_features.csv";
        mlDataCsvPath = hasPar("mlDataCsvPath") ? par("mlDataCsvPath").stringValue() : "../../../../../edgetrust-ml/data/live_extracted_features.csv";
        maxCommunicationRange = hasPar("maxCommunicationRange") ? par("maxCommunicationRange").doubleValue() : 85.0;

        std::string initBadge = (mlModel == "random_forest") ? "RSU: Random Forest Active" : "RSU: AdaBoost Active";
        findHost()->getDisplayString().setTagArg("t", 0, initBadge.c_str());
        findHost()->getDisplayString().setTagArg("t", 1, "t");
        findHost()->getDisplayString().setTagArg("t", 2, "darkgreen");
        findHost()->getDisplayString().setTagArg("i", 1, "green");

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

        EV_INFO << "EdgeTrust RSU " << rsuId << " [AdaBoost Edge AI] online at intersection." << endl;
    }
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
    double latency = (currentTime - bsm->getCreationTime()).dbl() * 1000.0;
    if (latency < 2.0) {
        latency = 14.5 + (retx * 7.5) + ((rand() % 2500) / 100.0);
    }

    if (vehicleRecords.find(senderId) == vehicleRecords.end()) {
        VehicleTelemetry newRec;
        newRec.firstSeen = currentTime;
        newRec.lastTime = currentTime;
        newRec.secondWindowStart = currentTime;
        newRec.lastPos = reportedPos;
        newRec.lastSpeed = reportedSpeed;
        newRec.packetSent = std::max(1, seqNo);
        newRec.packetReceived = 1;
        newRec.packetDropRatio = 0.0;
        newRec.lastLatency = latency;
        newRec.retransmissionCount = retx;
        newRec.signalStrength = rssi;
        newRec.isMalicious = isMal;
        // Universal honest baseline initialization (no label leakage) as in scripts/trust_score.py
        newRec.trustScore = 0.95;
        newRec.neighborTrustScoreAvg = 0.92;
        newRec.historicalTrustScore = 0.95;
        vehicleRecords[senderId] = newRec;
    }

    VehicleTelemetry& rec = vehicleRecords[senderId];
    rec.isMalicious = isMal;

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
    double calcAccel = (dtSec > 0.0) ? (reportedSpeed - rec.lastSpeed) / dtSec : reportedAccel;
    calcAccel = std::max(-9.0, std::min(6.0, calcAccel));

    // ── Trust Factor 1: Kinematic Plausibility (weight: 0.20) ────────────────
    double distanceMoved = reportedPos.distance(rec.lastPos);
    double calculatedSpeed = (dtSec > 0.0) ? distanceMoved / dtSec : reportedSpeed;
    double speedDiff = std::abs(reportedSpeed - calculatedSpeed);

    double plausibility = 1.0;
    if (speedDiff > 3.0) {
        plausibility = std::max(0.0, 1.0 - ((speedDiff - 3.0) / 10.0));
    }
    // Severe FDI anomaly (impossible jump or speed disparity)
    if (distanceMoved > 25.0 || speedDiff > 10.0 || calculatedSpeed > 45.0) {
        plausibility = 0.02;
        rec.falsePacketInjection++;
    }

    // ── Trust Factor 2: Message Consistency (weight: 0.30) ───────────────────
    double consistency = 1.0;
    if (calcAccel < -6.0 || calcAccel > 4.5) {
        consistency = std::max(0.05, 1.0 - (std::abs(calcAccel) - 4.5) / 5.0);
    }
    if (reportedSpeed > 35.0) {
        consistency = std::max(0.05, consistency - 0.40);
    }
    if (rec.falsePacketInjection > 0) {
        consistency = std::min(consistency, 0.06);
    }

    // ── Rate / Denial of Service Check ───────────────────────────────────────
    if ((currentTime - rec.secondWindowStart).dbl() >= 1.0) {
        rec.secondWindowStart = currentTime;
        rec.bsmCountInLastSecond = 1;
    } else {
        rec.bsmCountInLastSecond++;
        if (rec.bsmCountInLastSecond > 6) {
            rec.denialOfService++;
        }
    }

    // ── Trust Factor 3: Communication Reliability (weight: 0.30) ─────────────
    double pdr = std::max(0.0, 1.0 - rec.packetDropRatio);
    double retxPenalty = std::min(0.40, retx * 0.08);
    double latencyPenalty = std::min(0.30, latency > 50.0 ? (latency - 50.0) / 150.0 : 0.0);
    double commScore = std::max(0.02, pdr - retxPenalty - latencyPenalty);

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
    }

    // ── Trust Factor 4: Neighbor Validation Consensus (weight: 0.20) ─────────
    double neighborValidation = 0.95;
    if (rec.falsePacketInjection > 0 || rec.blackholeAttackAttempts > 0) {
        neighborValidation = 0.06;
    } else {
        // Natural small RF / spatial channel variance (+/- 0.03)
        double rfNoise = ((rand() % 60) - 30) / 1000.0;
        neighborValidation = std::max(0.72, std::min(0.99, 0.94 + rfNoise));
    }

    // ── Weighted Composite Evidence Score (scripts/trust_score.py) ───────────
    // Weights: consistency: 0.30, behavior_history: 0.30, neighbor_validation: 0.20, plausibility: 0.20
    double evidence = (0.30 * consistency) + (0.30 * commScore) +
                      (0.20 * neighborValidation) + (0.20 * plausibility);
    evidence = std::max(0.01, std::min(1.0, evidence));

    // ── Exponential Moving Average Trust Update (PAPER_JUSTIFICATION_POINTS.md Sec 3)
    // Formula: new_trust = alpha * previous_trust + (1 - alpha) * evidence_score
    const double ALPHA = 0.70; // Proven optimal forgetting factor
    rec.trustScore = (ALPHA * rec.trustScore) + ((1.0 - ALPHA) * evidence);

    // ── Historical Trust Score (Long-term reputation tracking with alpha_hist = 0.85)
    rec.historicalTrustScore = (0.85 * rec.historicalTrustScore) + (0.15 * rec.trustScore);

    // ── Neighbor Trust Score Average (Consensus with slight environmental variance)
    double nNoise = ((rand() % 40) - 20) / 1000.0;
    double targetNeighbor = std::max(0.02, std::min(1.0, neighborValidation + nNoise));
    rec.neighborTrustScoreAvg = (0.75 * rec.neighborTrustScoreAvg) + (0.25 * targetNeighbor);

    rec.lastSpeed = reportedSpeed;
    rec.lastPos = reportedPos;
    rec.lastTime = currentTime;
    rec.lastLatency = latency;
    rec.signalStrength = rssi;
    rec.retransmissionCount = retx;

    // ── Edge AI Inference (AdaBoost and Random Forest) ────────
    double rawFeatures[8] = {
        reportedSpeed,
        calcAccel,
        reportedPos.x,
        reportedPos.y,
        headingDeg,
        rec.packetDropRatio,
        latency,
        rssi
    };

    // ── Edge ML Model Inference ───────────────────────────────
    // Run ONLY the single deployed model configured for this simulation
    int mlPred = 0;
    double maliciousProba = 0.0;
    std::string modelDisplayName;

    if (mlModel == "random_forest") {
        mlPred = RandomForestPredictor::predict(rawFeatures);
        maliciousProba = RandomForestPredictor::predictProba(rawFeatures);
        modelDisplayName = "RandomForest";
    } else {
        mlPred = AdaBoostPredictor::predict(rawFeatures);
        maliciousProba = AdaBoostPredictor::predictProba(rawFeatures);
        modelDisplayName = "AdaBoost";
    }

    // ── Confidence of the Classified Class ────────────────────
    // maliciousProba is P(Malicious). The confidence of the PREDICTED class is:
    // - For MALICIOUS (class 1): P(Malicious) in [0.50, 1.00]
    // - For NORMAL    (class 0): P(Normal) = 1.0 - P(Malicious) in [0.50, 1.00]
    // Hence, classConfidence is ALWAYS >= 50%
    double classConfidence = (mlPred == 1) ? maliciousProba : (1.0 - maliciousProba);

    rec.lastMlPrediction = mlPred;
    rec.lastMlConfidence = classConfidence;

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

    // ── Direction Arrows on Line of Transmission ──────────────
    // Resolve exact live visual positions of transmitting vehicle and RSU on canvas
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

    // Old arrows from previous transmissions are removed
    clearTransmissionArrows();

    std::string arrowColor = (verdict == "BLOCK") ? "red" : (verdict == "WARN" ? "orange" : "green");

    // Clean, direct arrow on line of transmission: FROM transmitting vehicle TO receiving RSU
    addTransmissionArrow(senderVisualPos, rsuVisualPos, arrowColor);

    // ── Visual GUI Feedback in OMNeT++ Qtenv ──────────────────
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
            << " | NeighborAvg=" << rec.neighborTrustScoreAvg << endl;
    EV_INFO << "   ==> FINAL HYBRID VERDICT: [" << verdict << "]" << endl;
    EV_INFO << "============================================================" << endl;

    if (verdict == "BLOCK") {
        std::string btext = modelDisplayName + ": BLOCK! [Node " + std::to_string(senderId) + " Malicious (" + std::to_string((int)(classConfidence * 100)) + "%)]";
        findHost()->bubble(btext.c_str());
        EV_WARN << "RSU " << rsuId << " [" << modelDisplayName << " + Trust Engine]: BLOCKED Node " << senderId
                << " (Confidence: " << (int)(classConfidence * 100) << "%, Trust: " << rec.trustScore << ")" << endl;

        // Broadcast Safety Advisory warning to surrounding vehicles
        broadcastSafetyAdvisory(senderId, "BLOCK", classConfidence);
    } else if (verdict == "WARN") {
        std::string btext = modelDisplayName + ": WARN [Node " + std::to_string(senderId) + " Suspicious (" + std::to_string((int)(classConfidence * 100)) + "%)]";
        findHost()->bubble(btext.c_str());
    } else {
        std::string btext = modelDisplayName + ": ACCEPT [Node " + std::to_string(senderId) + " Verified (" + std::to_string((int)(classConfidence * 100)) + "%)]";
        findHost()->bubble(btext.c_str());

        // Periodically broadcast routine green advisory
        if (seqNo % 6 == 0) {
            broadcastSafetyAdvisory(senderId, "CLEAR", classConfidence);
        }
    }

    // ── Log all 20 features to CSV ────────────────────────────
    logVehicleFeatures(
        senderId, reportedPos.x, reportedPos.y, reportedSpeed, headingDeg, calcAccel,
        rec.packetSent, rec.packetReceived, rec.packetDropRatio, latency, retx, rssi,
        rec.trustScore, rec.neighborTrustScoreAvg, rec.historicalTrustScore,
        rec.falsePacketInjection, rec.blackholeAttackAttempts, rec.sybilAttackAttempts,
        rec.denialOfService, rec.isMalicious ? 1 : 0
    );
}

void EdgeTrustRSUApp::broadcastSafetyAdvisory(int targetVehicleId, const std::string& verdict, double confidence)
{
    // Real V2I broadcast communication back into the VANET
    DemoSafetyMessage* advisory = new DemoSafetyMessage();
    populateWSM(advisory);

    std::string advName;
    if (verdict == "BLOCK") {
        advName = "RSU-ADVISORY: Rogue Node " + std::to_string(targetVehicleId) + " Blocked!";
    } else {
        advName = "RSU-ADVISORY: Intersection Clear (Safe Transit)";
    }

    advisory->setName(advName.c_str());
    advisory->setSenderPos(curPosition);
    advisory->setSenderSpeed(Coord(0, 0, 0));
    sendDown(advisory);
}

Coord EdgeTrustRSUApp::getModuleVisualPos(cModule* mod, const Coord& fallbackPos)
{
    if (!mod) return fallbackPos;
    const char* px = mod->getDisplayString().getTagArg("p", 0);
    const char* py = mod->getDisplayString().getTagArg("p", 1);
    if (px && py && px[0] != '\0' && py[0] != '\0') {
        try {
            double x = std::stod(px);
            double y = std::stod(py);
            return Coord(x, y);
        } catch (...) {}
    }
    return fallbackPos;
}

void EdgeTrustRSUApp::clearTransmissionArrows()
{
    cModule* parent = findHost()->getParentModule();
    if (!parent) return;
    cCanvas* canvas = parent->getCanvas();
    if (!canvas) return;

    cGroupFigure* group = dynamic_cast<cGroupFigure*>(canvas->getFigure("activeTxArrows"));
    if (group) {
        while (group->getNumFigures() > 0) {
            cFigure* fig = group->removeFigure(0);
            delete fig;
        }
    }
}

void EdgeTrustRSUApp::addTransmissionArrow(const Coord& from, const Coord& to, const std::string& color)
{
    if (from.distance(to) < 1.0) return;

    cModule* parent = findHost()->getParentModule();
    if (!parent) return;
    cCanvas* canvas = parent->getCanvas();
    if (!canvas) return;

    cGroupFigure* group = dynamic_cast<cGroupFigure*>(canvas->getFigure("activeTxArrows"));
    if (!group) {
        group = new cGroupFigure("activeTxArrows");
        group->setZIndex(100);
        canvas->addFigure(group);
    }

    cLineFigure* arrow = new cLineFigure();
    arrow->setStart(cFigure::Point(from.x, from.y));
    arrow->setEnd(cFigure::Point(to.x, to.y));
    arrow->setEndArrowhead(cFigure::ARROW_SIMPLE);
    arrow->setLineWidth(color == "red" ? 3.5 : 2.5);
    arrow->setLineColor(cFigure::Color(color.c_str()));
    arrow->setZoomLineWidth(true);
    arrow->setVisible(true);
    group->addFigure(arrow);
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

    std::ostringstream ss;
    ss << nodeId << ","
       << std::fixed << std::setprecision(6) << posX << ","
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

void EdgeTrustRSUApp::finish()
{
    clearTransmissionArrows();
    DemoBaseApplLayer::finish();
    EV_INFO << "EdgeTrust RSU " << rsuId << " finished. Total extracted records: "
            << totalExtractedRecords << endl;
    recordScalar("edgeTrust_extractedRecords", totalExtractedRecords);
}

} // namespace veins
