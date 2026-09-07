// EdgeTrustVehicleApp.cc
#include "veins/modules/application/edgetrust/EdgeTrustVehicleApp.h"
#include <cmath>
#include <sstream>

namespace veins {

Define_Module(veins::EdgeTrustVehicleApp);

void EdgeTrustVehicleApp::initialize(int stage)
{
    DemoBaseApplLayer::initialize(stage);
    if (stage == 0) {
        vehicleId = findHost()->getIndex() + 1;

        scenarioMode = hasPar("scenarioMode") ? par("scenarioMode").stdstringValue() : "mixed";
        maliciousRatio = hasPar("maliciousRatio") ? par("maliciousRatio").doubleValue() : 0.25;
        maxCommunicationRange = hasPar("maxCommunicationRange") ? par("maxCommunicationRange").doubleValue() : 85.0;

        // Configure attack profiles based on scenarioMode
        if (scenarioMode == "baseline") {
            // All vehicles legitimate
            isMalicious = false;
            attackType = ATTACK_NONE;
            attackStartMessage = 9999;
        } else if (scenarioMode == "fdi_position") {
            // VeReMi constant/random position offset scenario
            if (vehicleId == 1) {
                isMalicious = false;
                attackType = ATTACK_NONE;
                attackStartMessage = 9999;
            } else if (vehicleId == 2) {
                isMalicious = true;
                attackType = ATTACK_FDI_POS_OFFSET;
                attackStartMessage = 3;
            } else {
                isMalicious = ((vehicleId % 3) == 0);
                attackType = isMalicious ? ATTACK_FDI_POS_OFFSET : ATTACK_NONE;
                attackStartMessage = 3;
            }
        } else if (scenarioMode == "fdi_suddenstop") {
            // VeReMi sudden stop & zero speed report scenario
            if (vehicleId == 1) {
                isMalicious = false;
                attackType = ATTACK_NONE;
                attackStartMessage = 9999;
            } else if (vehicleId == 2) {
                isMalicious = true;
                attackType = ATTACK_FDI_SUDDEN_STOP;
                attackStartMessage = 3;
            } else {
                isMalicious = ((vehicleId % 3) == 0);
                attackType = isMalicious ? ATTACK_FDI_SUDDEN_STOP : ATTACK_NONE;
                attackStartMessage = 3;
            }
        } else if (scenarioMode == "dos") {
            // VeReMi DoS high-rate flooding scenario
            if (vehicleId == 1) {
                isMalicious = false;
                attackType = ATTACK_NONE;
                attackStartMessage = 9999;
            } else if (vehicleId == 2) {
                isMalicious = true;
                attackType = ATTACK_DOS_FLOOD;
                attackStartMessage = 3;
            } else {
                isMalicious = ((vehicleId % 3) == 0);
                attackType = isMalicious ? ATTACK_DOS_FLOOD : ATTACK_NONE;
                attackStartMessage = 3;
            }
        } else if (scenarioMode == "sybil") {
            // VeReMi Sybil phantom congestion scenario
            if (vehicleId == 1) {
                isMalicious = false;
                attackType = ATTACK_NONE;
                attackStartMessage = 9999;
            } else if (vehicleId == 2) {
                isMalicious = true;
                attackType = ATTACK_SYBIL_PHANTOM;
                attackStartMessage = 3;
            } else {
                isMalicious = ((vehicleId % 3) == 0);
                attackType = isMalicious ? ATTACK_SYBIL_PHANTOM : ATTACK_NONE;
                attackStartMessage = 3;
            }
        } else if (scenarioMode == "blackhole") {
            // Blackhole selective beacon dropping scenario
            if (vehicleId == 1) {
                isMalicious = false;
                attackType = ATTACK_NONE;
                attackStartMessage = 9999;
            } else if (vehicleId == 2) {
                isMalicious = true;
                attackType = ATTACK_BLACKHOLE;
                attackStartMessage = 3;
            } else {
                isMalicious = ((vehicleId % 3) == 0);
                attackType = isMalicious ? ATTACK_BLACKHOLE : ATTACK_NONE;
                attackStartMessage = 3;
            }
        } else if (scenarioMode == "timedelay") {
            // VeReMi time delay & stale message replay scenario
            if (vehicleId == 1) {
                isMalicious = false;
                attackType = ATTACK_NONE;
                attackStartMessage = 9999;
            } else if (vehicleId == 2) {
                isMalicious = true;
                attackType = ATTACK_TIME_DELAY_REPLAY;
                attackStartMessage = 3;
            } else {
                isMalicious = ((vehicleId % 3) == 0);
                attackType = isMalicious ? ATTACK_TIME_DELAY_REPLAY : ATTACK_NONE;
                attackStartMessage = 3;
            }
        } else {
            // "mixed" mode (VeReMi mixAll / mixThree): heterogeneous attacks across vehicles
            if (vehicleId == 1) {
                isMalicious = false;
                attackType = ATTACK_NONE;
                attackStartMessage = 9999;
            } else if (vehicleId == 2) {
                isMalicious = true;
                attackType = ATTACK_FDI_POS_OFFSET;
                attackStartMessage = 3;
            } else if (vehicleId == 3) {
                isMalicious = true;
                attackType = ATTACK_BLACKHOLE;
                attackStartMessage = 3;
            } else if (vehicleId == 4) {
                isMalicious = true;
                attackType = ATTACK_DOS_FLOOD;
                attackStartMessage = 3;
            } else if (vehicleId == 5) {
                isMalicious = true;
                attackType = ATTACK_SYBIL_PHANTOM;
                attackStartMessage = 3;
            } else if (vehicleId == 6) {
                isMalicious = true;
                attackType = ATTACK_FDI_SUDDEN_STOP;
                attackStartMessage = 3;
            } else if (vehicleId == 7) {
                isMalicious = true;
                attackType = ATTACK_TIME_DELAY_REPLAY;
                attackStartMessage = 3;
            } else {
                unsigned int hashVal = ((unsigned int)vehicleId * 2654435761u) % 100;
                if (maliciousRatio > 0.0 && hashVal < (unsigned int)(maliciousRatio * 100)) {
                    isMalicious = true;
                    attackType = (vehicleId % 6) + 1;
                    attackStartMessage = 3;
                } else {
                    isMalicious = false;
                    attackType = ATTACK_NONE;
                    attackStartMessage = 9999;
                }
            }
        }

        // Explicit overrides if set in ini/ned
        if (hasPar("isMalicious") && par("isMalicious").boolValue()) {
            isMalicious = true;
        }
        if (hasPar("attackType") && par("attackType").intValue() >= 0) {
            attackType = par("attackType").intValue();
            if (attackType > 0) isMalicious = true;
        }

        findHost()->getDisplayString().setTagArg("t", 0, "Initializing OBU...");
        findHost()->getDisplayString().setTagArg("t", 1, "t");
        findHost()->getDisplayString().setTagArg("t", 2, "darkgreen");
        findHost()->getDisplayString().setTagArg("i", 1, "green");

        lastTime = simTime();
    }
}

void EdgeTrustVehicleApp::handleSelfMsg(cMessage* msg)
{
    if (msg->getKind() == SEND_BEACON_EVT) {
        EdgeTrustSafetyMessage* bsm = new EdgeTrustSafetyMessage();
        populateWSM(bsm);
        populateEdgeTrustMessage(bsm);

        bool attackActive = (isMalicious && sequenceNumber >= attackStartMessage);

        // Blackhole attack: intentionally drop 70% of outgoing communications
        bool dropThis = (attackActive && attackType == ATTACK_BLACKHOLE && ((rand() % 10) < 7));
        if (dropThis) {
            totalPacketsDropped++;
            delete bsm;
            findHost()->bubble("Blackhole: Dropping BSM!");
            findHost()->getDisplayString().setTagArg("t", 0, "ATTACK: Blackhole Dropping!");
            findHost()->getDisplayString().setTagArg("t", 1, "t");
            findHost()->getDisplayString().setTagArg("t", 2, "red");
            findHost()->getDisplayString().setTagArg("i", 1, "red");
            EV_DEBUG << "Vehicle " << vehicleId << " [Blackhole] dropped its own beacon." << endl;
        } else {
            // VeReMi Time Delay / Replay: artificially delay timestamp
            if (attackActive && attackType == ATTACK_TIME_DELAY_REPLAY) {
                bsm->setTimestamp(simTime() - simtime_t(3.0));
            }
            sendDown(bsm);
            totalPacketsSent++;
        }

        // DoS flooding: schedule much faster beacon rate (16 Hz instead of 1 Hz)
        simtime_t nextInterval = (attackActive && attackType == ATTACK_DOS_FLOOD) ? simtime_t(0.06) : beaconInterval;
        scheduleAt(simTime() + nextInterval, sendBeaconEvt);
    } else {
        DemoBaseApplLayer::handleSelfMsg(msg);
    }
}

void EdgeTrustVehicleApp::populateEdgeTrustMessage(EdgeTrustSafetyMessage* bsm)
{
    Coord currentPos = curPosition;
    double spdMag = 0.0;
    if (mobility) {
        try {
            spdMag = mobility->getSpeed();
            if (spdMag < 0.0) spdMag = curSpeed.length();
        } catch (...) {
            spdMag = curSpeed.length();
        }
    } else {
        spdMag = curSpeed.length();
    }

    // Direction / heading angle in degrees [0, 360)
    double headingDeg = 0.0;
    Coord headingVec(1, 0, 0);
    if (mobility) {
        try {
            headingDeg = std::fmod(mobility->getHeading().getRad() * 180.0 / M_PI + 360.0, 360.0);
            headingVec = mobility->getHeading().toCoord();
        } catch (...) {
            if (curSpeed.length() > 0.05) {
                headingDeg = std::fmod(std::atan2(curSpeed.y, curSpeed.x) * 180.0 / M_PI + 360.0, 360.0);
                headingVec = Coord(std::cos(headingDeg * M_PI / 180.0), std::sin(headingDeg * M_PI / 180.0), 0);
            }
        }
    }
    Coord currentSpd(headingVec.x * spdMag, headingVec.y * spdMag, 0);

    // Acceleration dv/dt in m/s^2
    simtime_t dt = simTime() - lastTime;
    double accel = 0.0;
    if (dt > 0.0 && lastTime > SIMTIME_ZERO) {
        accel = (spdMag - lastSpeed) / dt.dbl();
        accel = std::max(-8.5, std::min(5.5, accel));
    }

    lastSpeed = spdMag;
    lastPos = currentPos;
    lastTime = simTime();
    sequenceNumber++;

    // Cache beacon data into history buffer for time delay / replay attack simulation
    StaleBeaconData curEntry;
    curEntry.creationTime = simTime();
    curEntry.pos = currentPos;
    curEntry.speed = currentSpd;
    curEntry.heading = headingDeg;
    curEntry.accel = accel;
    curEntry.seq = sequenceNumber;
    staleHistory.push_back(curEntry);
    if (staleHistory.size() > 20) {
        staleHistory.pop_front();
    }

    int effectiveId = vehicleId;
    int retx = (isMalicious && attackType == ATTACK_BLACKHOLE) ? (4 + (rand() % 6)) : (rand() % 3);
    bool attackActive = (isMalicious && sequenceNumber >= attackStartMessage);

    // ── Simulate VeReMi-NextGen Attack Scenarios ──
    std::string bsmPayload;
    if (attackActive) {
        if (attackType == ATTACK_FDI_POS_OFFSET) {
            // VeReMi constant/random position offset: spoof position by ~28m
            currentPos.x += ((vehicleId % 2 == 0) ? 28.0 : -28.0);
            currentPos.y += ((vehicleId % 3 == 0) ? 22.0 : -22.0);
            currentSpd = Coord(0, 0, 0); // Fake collision / stopped vehicle
            bsmPayload = "ATTACK: FDI Fake Crash (+28m)";
            findHost()->bubble(bsmPayload.c_str());
            char tagStr[96];
            snprintf(tagStr, sizeof(tagStr), "V%d: FDI ATTACK (Spoofed +28m)", vehicleId);
            findHost()->getDisplayString().setTagArg("t", 0, tagStr);
            findHost()->getDisplayString().setTagArg("t", 1, "t");
            findHost()->getDisplayString().setTagArg("t", 2, "red");
            findHost()->getDisplayString().setTagArg("i", 1, "red");
        } else if (attackType == ATTACK_FDI_SUDDEN_STOP) {
            // VeReMi suddenStop / zeroSpeedReport: freeze position & report 0 speed while moving with feigned emergency braking
            if (!suddenStopInitialized) {
                suddenStopPos = curPosition;
                suddenStopInitialized = true;
            }
            currentPos = suddenStopPos;
            currentSpd = Coord(0, 0, 0);
            spdMag = 0.0;
            accel = -9.2; // VeReMi feigned hard braking / zero speed report
            bsmPayload = "ATTACK: SuddenStop / ZeroSpeed";
            findHost()->bubble(bsmPayload.c_str());
            char tagStr[96];
            snprintf(tagStr, sizeof(tagStr), "V%d: FDI SuddenStop (Ghost at 0m/s)", vehicleId);
            findHost()->getDisplayString().setTagArg("t", 0, tagStr);
            findHost()->getDisplayString().setTagArg("t", 1, "t");
            findHost()->getDisplayString().setTagArg("t", 2, "red");
            findHost()->getDisplayString().setTagArg("i", 1, "red");
        } else if (attackType == ATTACK_DOS_FLOOD) {
            // VeReMi dosAttack: high-frequency beacon flood (16 Hz)
            bsmPayload = "ATTACK: DoS Rapid Flood";
            findHost()->bubble(bsmPayload.c_str());
            char tagStr[96];
            snprintf(tagStr, sizeof(tagStr), "V%d: DoS Flooding (16 Hz)", vehicleId);
            findHost()->getDisplayString().setTagArg("t", 0, tagStr);
            findHost()->getDisplayString().setTagArg("t", 1, "t");
            findHost()->getDisplayString().setTagArg("t", 2, "red");
            findHost()->getDisplayString().setTagArg("i", 1, "red");
        } else if (attackType == ATTACK_SYBIL_PHANTOM) {
            // VeReMi trafficCongestionSybil: broadcast fake phantom vehicle identities
            effectiveId = (vehicleId * 100) + (sequenceNumber % 4);
            currentPos.x += ((sequenceNumber % 2 == 0) ? 3.0 : -3.0);
            currentPos.y += ((sequenceNumber % 3 == 0) ? 5.5 : -5.5);
            bsmPayload = "ATTACK: Sybil Phantom ID " + std::to_string(effectiveId);
            findHost()->bubble(bsmPayload.c_str());
            char tagStr[96];
            snprintf(tagStr, sizeof(tagStr), "V%d: Sybil Phantom (ID %d)", vehicleId, effectiveId);
            findHost()->getDisplayString().setTagArg("t", 0, tagStr);
            findHost()->getDisplayString().setTagArg("t", 1, "t");
            findHost()->getDisplayString().setTagArg("t", 2, "red");
            findHost()->getDisplayString().setTagArg("i", 1, "red");
        } else if (attackType == ATTACK_BLACKHOLE) {
            // VeReMi / VANET blackhole packet drop
            char tagStr[96];
            snprintf(tagStr, sizeof(tagStr), "V%d: BLACKHOLE ATTACK (Dropping)", vehicleId);
            findHost()->getDisplayString().setTagArg("t", 0, tagStr);
            findHost()->getDisplayString().setTagArg("t", 1, "t");
            findHost()->getDisplayString().setTagArg("t", 2, "red");
            findHost()->getDisplayString().setTagArg("i", 1, "red");
        } else if (attackType == ATTACK_TIME_DELAY_REPLAY) {
            // VeReMi timeDelayAttack / dataReplay: replay stale beacon from 3 seconds ago
            if (staleHistory.size() >= 3) {
                const StaleBeaconData& stale = staleHistory[staleHistory.size() - 3];
                currentPos = stale.pos;
                currentSpd = stale.speed;
                headingDeg = stale.heading;
                accel = stale.accel;
            }
            retx = 5;
            bsmPayload = "ATTACK: Replay Stale Packet (+3s)";
            findHost()->bubble(bsmPayload.c_str());
            char tagStr[96];
            snprintf(tagStr, sizeof(tagStr), "V%d: TimeDelay/Replay (+3.0s)", vehicleId);
            findHost()->getDisplayString().setTagArg("t", 0, tagStr);
            findHost()->getDisplayString().setTagArg("t", 1, "t");
            findHost()->getDisplayString().setTagArg("t", 2, "red");
            findHost()->getDisplayString().setTagArg("i", 1, "red");
        }
    } else {
        // Legitimate vehicle or pre-attack stage: persistent status badge + bubble
        char tagStr[96];
        snprintf(tagStr, sizeof(tagStr), "V%d: Legitimate (%.1f m/s)", vehicleId, spdMag);
        findHost()->getDisplayString().setTagArg("t", 0, tagStr);
        findHost()->getDisplayString().setTagArg("t", 1, "t");
        findHost()->getDisplayString().setTagArg("t", 2, "darkgreen");
        findHost()->getDisplayString().setTagArg("i", 1, "green");

        if (accel < -2.5) {
            findHost()->bubble("V2V: Hard Braking Warning!");
        } else if (spdMag < 2.0) {
            if (sequenceNumber % 3 == 0) findHost()->bubble("V2V: Queued at Intersection");
        } else {
            if (sequenceNumber % 4 == 0) findHost()->bubble("V2V: Normal Transit");
        }
    }

    std::string msgName = "BSM-" + std::to_string(effectiveId);
    bsm->setName(msgName.c_str());
    bsm->setSenderId(effectiveId);
    bsm->setSenderPos(currentPos);
    bsm->setSenderSpeed(currentSpd);
    bsm->setHeading(headingDeg);
    bsm->setAcceleration(accel);
    bsm->setSequenceNumber(sequenceNumber);
    bsm->setRetransmissionCount(retx);
    bsm->setIsMalicious(attackActive);
    bsm->setAttackType(attackActive ? attackType : 0);
}

bool EdgeTrustVehicleApp::lightweightTrustFilter(DemoSafetyMessage* bsm)
{
    EdgeTrustSafetyMessage* emsg = dynamic_cast<EdgeTrustSafetyMessage*>(bsm);
    int senderId = emsg ? emsg->getSenderId() : -1;
    if (senderId == -1) {
        std::string msgName = bsm->getName();
        if (msgName.rfind("BSM-", 0) == 0) {
            senderId = std::stoi(msgName.substr(4));
        }
    }

    if (senderId != -1 && suspiciousNodes.count(senderId) > 0 && suspiciousNodes[senderId]) {
        return false;
    }

    double speedMag = bsm->getSenderSpeed().length();
    if (speedMag > 50.0) { // Kinematics anomaly: impossible urban speed
        if (senderId != -1) suspiciousNodes[senderId] = true;
        return false;
    }

    return true;
}

void EdgeTrustVehicleApp::onBSM(DemoSafetyMessage* bsm)
{
    // Physical coverage check: drop packets beyond maxCommunicationRange
    double dist = curPosition.distance(bsm->getSenderPos());
    if (dist > maxCommunicationRange) {
        return; // Beyond 802.11p radio reception boundary
    }

    std::string msgName = bsm->getName();

    // Check if message is an RSU Safety Advisory
    if (msgName.rfind("RSU-ADVISORY:", 0) == 0) {
        if (msgName.find("Blocked") != std::string::npos) {
            findHost()->bubble("OBU: Warning - Rogue Node Blocked!");
            findHost()->getDisplayString().setTagArg("t", 0, "OBU: Alert Acknowledged");
            findHost()->getDisplayString().setTagArg("t", 1, "t");
            findHost()->getDisplayString().setTagArg("t", 2, "blue");
        } else {
            if (rand() % 3 == 0) findHost()->bubble("OBU: RSU Signal Verified");
        }
        return;
    }

    // Peer vehicle message: run lightweight trust filter
    if (!lightweightTrustFilter(bsm)) {
        findHost()->bubble("OBU: Filter Dropped Malicious BSM!");
        return;
    }
}

void EdgeTrustVehicleApp::drawArrow(const Coord& from, const Coord& to, const std::string& color, const std::string& arrowId)
{
    // Direct arrows disabled for clean, clutter-free GUI presentation
}

void EdgeTrustVehicleApp::finish()
{
    DemoBaseApplLayer::finish();
    recordScalar("edgeTrust_totalPacketsSent", totalPacketsSent);
    recordScalar("edgeTrust_totalPacketsDropped", totalPacketsDropped);
}

} // namespace veins
