// EdgeTrustVehicleApp.cc
#include "veins/modules/application/edgetrust/EdgeTrustVehicleApp.h"
#include <cmath>
#include <sstream>

namespace veins {

Define_Module(veins::EdgeTrustVehicleApp);

static int parseScenarioAttackType(const std::string& mode) {
    if (mode == "constant_pos_offset" || mode == "fdi_position") return ATTACK_CONST_POS_OFFSET;
    if (mode == "random_pos_offset") return ATTACK_RANDOM_POS_OFFSET;
    if (mode == "pos_mirroring" || mode == "position_mirroring") return ATTACK_POS_MIRRORING;
    if (mode == "constant_speed_offset") return ATTACK_CONST_SPEED_OFFSET;
    if (mode == "random_speed_offset") return ATTACK_RANDOM_SPEED_OFFSET;
    if (mode == "zero_speed_report") return ATTACK_ZERO_SPEED_REPORT;
    if (mode == "sudden_stop" || mode == "fdi_suddenstop") return ATTACK_SUDDEN_STOP;
    if (mode == "sudden_const_speed" || mode == "sudden_constant_speed") return ATTACK_SUDDEN_CONST_SPEED;
    if (mode == "reversed_heading") return ATTACK_REVERSED_HEADING;
    if (mode == "feigned_braking") return ATTACK_FEIGNED_BRAKING;
    if (mode == "accel_mult" || mode == "acceleration_multiplication") return ATTACK_ACCEL_MULT;
    if (mode == "dos" || mode == "dos_attack") return ATTACK_DOS_FLOOD;
    if (mode == "sybil" || mode == "traffic_congestion_sybil") return ATTACK_SYBIL_PHANTOM;
    if (mode == "data_replay") return ATTACK_DATA_REPLAY;
    if (mode == "time_delay" || mode == "timedelay" || mode == "time_delay_attack") return ATTACK_TIME_DELAY;
    if (mode == "blackhole") return ATTACK_BLACKHOLE;
    return ATTACK_NONE;
}

void EdgeTrustVehicleApp::initialize(int stage)
{
    DemoBaseApplLayer::initialize(stage);
    if (stage == 0) {
        vehicleId = findHost()->getIndex() + 1;

        scenarioMode = hasPar("scenarioMode") ? par("scenarioMode").stdstringValue() : "mixed";
        maliciousRatio = hasPar("maliciousRatio") ? par("maliciousRatio").doubleValue() : 0.25;
        maxCommunicationRange = hasPar("maxCommunicationRange") ? par("maxCommunicationRange").doubleValue() : 85.0;

        int singleAttack = parseScenarioAttackType(scenarioMode);

        // Configure attack assignment based on scenarioMode
        if (scenarioMode == "baseline") {
            // All vehicles legitimate
            isMalicious = false;
            attackType = ATTACK_NONE;
            attackStartMessage = 9999;
        } else if (singleAttack != ATTACK_NONE) {
            // Dedicated single-attack scenario
            if (vehicleId == 1) {
                isMalicious = false;
                attackType = ATTACK_NONE;
                attackStartMessage = 9999;
            } else if (vehicleId == 2) {
                isMalicious = true;
                attackType = singleAttack;
                attackStartMessage = 3;
            } else {
                isMalicious = ((vehicleId % 3) == 0);
                attackType = isMalicious ? singleAttack : ATTACK_NONE;
                attackStartMessage = 3;
            }
        } else if (scenarioMode == "mix_three") {
            // VeReMi mixThree: constantPositionOffset, randomSpeedOffset, suddenStop
            static const int mix3[3] = { ATTACK_CONST_POS_OFFSET, ATTACK_RANDOM_SPEED_OFFSET, ATTACK_SUDDEN_STOP };
            if (vehicleId == 1) {
                isMalicious = false;
                attackType = ATTACK_NONE;
                attackStartMessage = 9999;
            } else {
                unsigned int hashVal = ((unsigned int)vehicleId * 2654435761u) % 100;
                if (maliciousRatio > 0.0 && hashVal < (unsigned int)(maliciousRatio * 100)) {
                    isMalicious = true;
                    attackType = mix3[(vehicleId - 2) % 3];
                    attackStartMessage = 3;
                } else {
                    isMalicious = false;
                    attackType = ATTACK_NONE;
                    attackStartMessage = 9999;
                }
            }
        } else {
            // "mix_all" or "mixed" (default): iterates across ALL 16 attacks!
            static const int allAttacks[16] = {
                ATTACK_CONST_POS_OFFSET,
                ATTACK_RANDOM_POS_OFFSET,
                ATTACK_POS_MIRRORING,
                ATTACK_CONST_SPEED_OFFSET,
                ATTACK_RANDOM_SPEED_OFFSET,
                ATTACK_ZERO_SPEED_REPORT,
                ATTACK_SUDDEN_STOP,
                ATTACK_SUDDEN_CONST_SPEED,
                ATTACK_REVERSED_HEADING,
                ATTACK_FEIGNED_BRAKING,
                ATTACK_ACCEL_MULT,
                ATTACK_DOS_FLOOD,
                ATTACK_SYBIL_PHANTOM,
                ATTACK_DATA_REPLAY,
                ATTACK_TIME_DELAY,
                ATTACK_BLACKHOLE
            };
            if (vehicleId == 1) {
                isMalicious = false;
                attackType = ATTACK_NONE;
                attackStartMessage = 9999;
            } else if (vehicleId >= 2 && vehicleId <= 17) {
                isMalicious = true;
                attackType = allAttacks[vehicleId - 2];
                attackStartMessage = 3;
            } else {
                unsigned int hashVal = ((unsigned int)vehicleId * 2654435761u) % 100;
                if (maliciousRatio > 0.0 && hashVal < (unsigned int)(maliciousRatio * 100)) {
                    isMalicious = true;
                    attackType = allAttacks[(vehicleId - 2) % 16];
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
            // VeReMi Time Delay / Data Replay: artificially delay timestamp
            if (attackActive && (attackType == ATTACK_TIME_DELAY || attackType == ATTACK_DATA_REPLAY)) {
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

    // ── Simulate VeReMi-NextGen Complete Attack Catalog ──
    std::string bsmPayload;
    if (attackActive) {
        switch (attackType) {
            case ATTACK_CONST_POS_OFFSET: {
                currentPos.x += ((vehicleId % 2 == 0) ? 28.0 : -28.0);
                currentPos.y += ((vehicleId % 3 == 0) ? 22.0 : -22.0);
                currentSpd = Coord(0, 0, 0);
                bsmPayload = "ATTACK: ConstPosOffset (+28m)";
                findHost()->bubble(bsmPayload.c_str());
                char tagStr[96];
                snprintf(tagStr, sizeof(tagStr), "V%d: ConstPosOffset (+28m)", vehicleId);
                findHost()->getDisplayString().setTagArg("t", 0, tagStr);
                break;
            }
            case ATTACK_RANDOM_POS_OFFSET: {
                double rX = ((rand() % 2 == 0 ? 1.0 : -1.0) * (20.0 + (rand() % 50)));
                double rY = ((rand() % 2 == 0 ? 1.0 : -1.0) * (20.0 + (rand() % 50)));
                currentPos.x += rX; currentPos.y += rY;
                bsmPayload = "ATTACK: RandomPosOffset (+-50m)";
                findHost()->bubble(bsmPayload.c_str());
                char tagStr[96];
                snprintf(tagStr, sizeof(tagStr), "V%d: RandomPosOffset (+-50m)", vehicleId);
                findHost()->getDisplayString().setTagArg("t", 0, tagStr);
                break;
            }
            case ATTACK_POS_MIRRORING: {
                double perpRad = (headingDeg - 90.0) * M_PI / 180.0;
                currentPos.x += std::cos(perpRad) * 16.0;
                currentPos.y += std::sin(perpRad) * 16.0;
                bsmPayload = "ATTACK: PosMirroring (Opposite Lane)";
                findHost()->bubble(bsmPayload.c_str());
                char tagStr[96];
                snprintf(tagStr, sizeof(tagStr), "V%d: PosMirroring (+16m Perp)", vehicleId);
                findHost()->getDisplayString().setTagArg("t", 0, tagStr);
                break;
            }
            case ATTACK_CONST_SPEED_OFFSET: {
                spdMag += 12.0;
                currentSpd = Coord(headingVec.x * spdMag, headingVec.y * spdMag, 0);
                bsmPayload = "ATTACK: ConstSpeedOffset (+12m/s)";
                findHost()->bubble(bsmPayload.c_str());
                char tagStr[96];
                snprintf(tagStr, sizeof(tagStr), "V%d: ConstSpeedOffset (+12m/s)", vehicleId);
                findHost()->getDisplayString().setTagArg("t", 0, tagStr);
                break;
            }
            case ATTACK_RANDOM_SPEED_OFFSET: {
                double rSpd = (rand() % 2 == 0 ? 1.0 : -1.0) * (4.0 + (rand() % 5));
                spdMag = std::max(0.0, spdMag + rSpd);
                currentSpd = Coord(headingVec.x * spdMag, headingVec.y * spdMag, 0);
                bsmPayload = "ATTACK: RandomSpeedOffset (+-6m/s)";
                findHost()->bubble(bsmPayload.c_str());
                char tagStr[96];
                snprintf(tagStr, sizeof(tagStr), "V%d: RandomSpeedOffset", vehicleId);
                findHost()->getDisplayString().setTagArg("t", 0, tagStr);
                break;
            }
            case ATTACK_ZERO_SPEED_REPORT: {
                currentSpd = Coord(0, 0, 0);
                spdMag = 0.0;
                bsmPayload = "ATTACK: ZeroSpeedReport (Reports 0m/s)";
                findHost()->bubble(bsmPayload.c_str());
                char tagStr[96];
                snprintf(tagStr, sizeof(tagStr), "V%d: ZeroSpeedReport (0m/s)", vehicleId);
                findHost()->getDisplayString().setTagArg("t", 0, tagStr);
                break;
            }
            case ATTACK_SUDDEN_STOP: {
                if (!suddenStopInitialized) {
                    suddenStopPos = curPosition;
                    suddenStopInitialized = true;
                }
                currentPos = suddenStopPos;
                currentSpd = Coord(0, 0, 0);
                spdMag = 0.0;
                accel = -9.2;
                bsmPayload = "ATTACK: SuddenStop (Ghost at 0m/s)";
                findHost()->bubble(bsmPayload.c_str());
                char tagStr[96];
                snprintf(tagStr, sizeof(tagStr), "V%d: SuddenStop (Ghost 0m/s)", vehicleId);
                findHost()->getDisplayString().setTagArg("t", 0, tagStr);
                break;
            }
            case ATTACK_SUDDEN_CONST_SPEED: {
                if (!constSpeedInitialized) {
                    frozenSpeed = spdMag > 1.0 ? spdMag : 14.0;
                    constSpeedInitialized = true;
                }
                spdMag = frozenSpeed;
                currentSpd = Coord(headingVec.x * spdMag, headingVec.y * spdMag, 0);
                accel = 0.0;
                bsmPayload = "ATTACK: SuddenConstSpeed (Frozen Speed)";
                findHost()->bubble(bsmPayload.c_str());
                char tagStr[96];
                snprintf(tagStr, sizeof(tagStr), "V%d: SuddenConstSpeed (%.1fm/s)", vehicleId, frozenSpeed);
                findHost()->getDisplayString().setTagArg("t", 0, tagStr);
                break;
            }
            case ATTACK_REVERSED_HEADING: {
                headingDeg = std::fmod(headingDeg + 180.0, 360.0);
                bsmPayload = "ATTACK: ReversedHeading (+180 deg)";
                findHost()->bubble(bsmPayload.c_str());
                char tagStr[96];
                snprintf(tagStr, sizeof(tagStr), "V%d: ReversedHeading (Opposite)", vehicleId);
                findHost()->getDisplayString().setTagArg("t", 0, tagStr);
                break;
            }
            case ATTACK_FEIGNED_BRAKING: {
                accel = (accel > 0.0 ? -1.0 : 1.0) * (std::abs(accel) * 2.5 + 4.5);
                accel = std::max(-9.5, std::min(6.0, accel));
                bsmPayload = "ATTACK: FeignedBraking (False Decel)";
                findHost()->bubble(bsmPayload.c_str());
                char tagStr[96];
                snprintf(tagStr, sizeof(tagStr), "V%d: FeignedBraking (%.1fm/s2)", vehicleId, accel);
                findHost()->getDisplayString().setTagArg("t", 0, tagStr);
                break;
            }
            case ATTACK_ACCEL_MULT: {
                accel = (accel >= 0.0 ? 1.0 : -1.0) * (std::max(1.5, std::abs(accel)) * 3.5);
                accel = std::max(-12.0, std::min(10.0, accel));
                bsmPayload = "ATTACK: AccelMultiplication (3.5x)";
                findHost()->bubble(bsmPayload.c_str());
                char tagStr[96];
                snprintf(tagStr, sizeof(tagStr), "V%d: AccelMult (%.1fm/s2)", vehicleId, accel);
                findHost()->getDisplayString().setTagArg("t", 0, tagStr);
                break;
            }
            case ATTACK_DOS_FLOOD: {
                bsmPayload = "ATTACK: DoS Rapid Flood";
                findHost()->bubble(bsmPayload.c_str());
                char tagStr[96];
                snprintf(tagStr, sizeof(tagStr), "V%d: DoS Flooding (16 Hz)", vehicleId);
                findHost()->getDisplayString().setTagArg("t", 0, tagStr);
                break;
            }
            case ATTACK_SYBIL_PHANTOM: {
                effectiveId = (vehicleId * 100) + (sequenceNumber % 4);
                currentPos.x += ((sequenceNumber % 2 == 0) ? 3.0 : -3.0);
                currentPos.y += ((sequenceNumber % 3 == 0) ? 5.5 : -5.5);
                bsmPayload = "ATTACK: Sybil Phantom ID " + std::to_string(effectiveId);
                findHost()->bubble(bsmPayload.c_str());
                char tagStr[96];
                snprintf(tagStr, sizeof(tagStr), "V%d: Sybil Phantom (ID %d)", vehicleId, effectiveId);
                findHost()->getDisplayString().setTagArg("t", 0, tagStr);
                break;
            }
            case ATTACK_DATA_REPLAY: {
                if (staleHistory.size() >= 3) {
                    const StaleBeaconData& stale = staleHistory[staleHistory.size() - 3];
                    currentPos = stale.pos;
                    currentSpd = stale.speed;
                    headingDeg = stale.heading;
                    accel = stale.accel;
                }
                retx = 5;
                bsmPayload = "ATTACK: DataReplay (Stale Packet)";
                findHost()->bubble(bsmPayload.c_str());
                char tagStr[96];
                snprintf(tagStr, sizeof(tagStr), "V%d: DataReplay (+3.0s Delay)", vehicleId);
                findHost()->getDisplayString().setTagArg("t", 0, tagStr);
                break;
            }
            case ATTACK_TIME_DELAY: {
                retx = 4;
                bsmPayload = "ATTACK: TimeDelay (+3.0s Delay)";
                findHost()->bubble(bsmPayload.c_str());
                char tagStr[96];
                snprintf(tagStr, sizeof(tagStr), "V%d: TimeDelay (+3.0s)", vehicleId);
                findHost()->getDisplayString().setTagArg("t", 0, tagStr);
                break;
            }
            case ATTACK_BLACKHOLE: {
                char tagStr[96];
                snprintf(tagStr, sizeof(tagStr), "V%d: BLACKHOLE ATTACK (Dropping)", vehicleId);
                findHost()->getDisplayString().setTagArg("t", 0, tagStr);
                break;
            }
            default:
                break;
        }
        findHost()->getDisplayString().setTagArg("t", 1, "t");
        findHost()->getDisplayString().setTagArg("t", 2, "red");
        findHost()->getDisplayString().setTagArg("i", 1, "red");
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
