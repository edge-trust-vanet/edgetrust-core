// EdgeTrustRSUApp.cc
#include "veins/modules/application/edgetrust/EdgeTrustRSUApp.h"
#include <cmath>
#include <iomanip>
#include <iostream>
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
        csvOutputPath = hasPar("csvOutputPath") ? par("csvOutputPath").stringValue() : "results/live_extracted_features.csv";
        mlDataCsvPath = hasPar("mlDataCsvPath") ? par("mlDataCsvPath").stringValue() : "../../../../../edgetrust-ml/data/live_extracted_features.csv";

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

        EV_INFO << "EdgeTrust RSU " << rsuId << " ready to extract live VANET telemetry." << endl;
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
    Coord reportedSpeedCoord = bsm->getSenderSpeed();
    double reportedSpeed = reportedSpeedCoord.length();
    simtime_t currentTime = simTime();

    // Direction / heading angle (degrees)
    double headingDeg = reportedHeading;
    if (headingDeg == 0.0 && reportedSpeed > 0.05) {
        headingDeg = std::fmod(std::atan2(reportedSpeedCoord.y, reportedSpeedCoord.x) * 180.0 / M_PI + 360.0, 360.0);
    }

    // Physical distance and signal strength (RSSI in dBm)
    double dist = curPosition.distance(reportedPos);
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
        newRec.trustScore = isMal ? 0.40 : 0.92;
        newRec.neighborTrustScoreAvg = isMal ? 0.38 : 0.90;
        newRec.historicalTrustScore = isMal ? 0.42 : 0.94;
        vehicleRecords[senderId] = newRec;
    }

    VehicleTelemetry& rec = vehicleRecords[senderId];

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

    // Plausibility Check (kinematics comparison)
    double distanceMoved = reportedPos.distance(rec.lastPos);
    double calculatedSpeed = (dtSec > 0.0) ? distanceMoved / dtSec : reportedSpeed;
    double speedDiff = std::abs(reportedSpeed - calculatedSpeed);

    double plausibility = 1.0;
    if (speedDiff > 5.0) {
        plausibility = std::max(0.0, 1.0 - ((speedDiff - 5.0) / 25.0));
    }
    if (calculatedSpeed > 45.0 || distanceMoved > 60.0 || speedDiff > 12.0) {
        plausibility = 0.0;
        rec.falsePacketInjection++;
    }

    // Consistency Check
    double consistency = 1.0;
    if (std::abs(calcAccel) > 5.5) consistency -= 0.35;
    if (calculatedSpeed > 40.0) consistency -= 0.40;
    consistency = std::max(0.0, consistency);

    // Rate / Denial of Service Check
    if ((currentTime - rec.secondWindowStart).dbl() >= 1.0) {
        rec.secondWindowStart = currentTime;
        rec.bsmCountInLastSecond = 1;
    } else {
        rec.bsmCountInLastSecond++;
        if (rec.bsmCountInLastSecond > 6) {
            rec.denialOfService++;
        }
    }

    // Blackhole Attack Detection
    if (rec.packetDropRatio > 0.45) {
        rec.blackholeAttackAttempts++;
    }

    // Sybil Attack Detection
    if (senderId > 100 || (senderId % 100 != 0 && (senderId % 10 == 0))) {
        rec.sybilAttackAttempts++;
    }

    // Trust Score (Direct, Neighbor, Historical)
    double commScore = std::max(0.0, 1.0 - rec.packetDropRatio);
    double directTrust = (0.50 * plausibility) + (0.30 * consistency) + (0.20 * commScore);
    rec.trustScore = (0.70 * rec.trustScore) + (0.30 * directTrust);

    double noise = ((rand() % 100) - 50) / 1000.0;
    rec.neighborTrustScoreAvg = std::max(0.05, std::min(1.0, rec.trustScore + noise));
    rec.historicalTrustScore = (0.70 * rec.historicalTrustScore) + (0.30 * rec.trustScore);

    rec.lastSpeed = reportedSpeed;
    rec.lastPos = reportedPos;
    rec.lastTime = currentTime;
    rec.lastLatency = latency;
    rec.signalStrength = rssi;
    rec.retransmissionCount = retx;

    // Log to CSV
    logVehicleFeatures(
        senderId, reportedPos.x, reportedPos.y, reportedSpeed, headingDeg, calcAccel,
        rec.packetSent, rec.packetReceived, rec.packetDropRatio, latency, retx, rssi,
        rec.trustScore, rec.neighborTrustScoreAvg, rec.historicalTrustScore,
        rec.falsePacketInjection, rec.blackholeAttackAttempts, rec.sybilAttackAttempts,
        rec.denialOfService, rec.isMalicious ? 1 : 0
    );

    // Visual indicators in OMNeT++ GUI
    if (rec.trustScore < 0.40) {
        findHost()->bubble("Decision: BLOCK");
        EV_WARN << "RSU " << rsuId << ": Node " << senderId
                << " flagged as MALICIOUS (Trust: " << rec.trustScore << "). Action: BLOCK" << endl;
    } else if (rec.trustScore < 0.70) {
        findHost()->bubble("Decision: WARN");
    } else {
        findHost()->bubble("Decision: ACCEPT");
    }
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
    DemoBaseApplLayer::finish();
    EV_INFO << "EdgeTrust RSU " << rsuId << " finished. Total extracted records: "
            << totalExtractedRecords << endl;
    recordScalar("edgeTrust_extractedRecords", totalExtractedRecords);
}

} // namespace veins
