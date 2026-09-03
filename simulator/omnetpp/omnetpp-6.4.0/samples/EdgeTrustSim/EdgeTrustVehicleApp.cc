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
        // Vehicle ID 1-indexed based on OMNeT++ node index
        vehicleId = findHost()->getIndex() + 1;

        maliciousRatio = hasPar("maliciousRatio") ? par("maliciousRatio").doubleValue() : 0.25;

        // Dynamic random assignment using hash to distribute exactly at maliciousRatio
        unsigned int hashVal = ((unsigned int)vehicleId * 2654435761u) % 100;
        if (maliciousRatio > 0.0 && hashVal < (unsigned int)(maliciousRatio * 100)) {
            isMalicious = true;
        }

        if (hasPar("isMalicious") && par("isMalicious").boolValue()) {
            isMalicious = true;
        }

        if (isMalicious) {
            // Distribute across attack types: 1=FDI, 2=Blackhole, 3=Sybil, 4=DoS
            if (hasPar("attackType") && par("attackType").intValue() > 0) {
                attackType = par("attackType").intValue();
            } else {
                attackType = (vehicleId % 4) + 1;
            }
            findHost()->getDisplayString().setTagArg("i", 1, "red");
            EV_INFO << "EdgeTrust Vehicle " << vehicleId
                    << " initialized as MALICIOUS (Attack: " << attackType << ")" << endl;
        } else {
            attackType = 0;
            findHost()->getDisplayString().setTagArg("i", 1, "green");
            EV_INFO << "EdgeTrust Vehicle " << vehicleId
                    << " initialized as LEGITIMATE." << endl;
        }

        lastTime = simTime();
    }
}

void EdgeTrustVehicleApp::handleSelfMsg(cMessage* msg)
{
    if (msg->getKind() == SEND_BEACON_EVT) {
        EdgeTrustSafetyMessage* bsm = new EdgeTrustSafetyMessage();
        populateWSM(bsm);
        populateEdgeTrustMessage(bsm);

        // Blackhole attack: intentionally drop 70% of outgoing communications
        bool dropThis = (isMalicious && attackType == 2 && ((rand() % 10) < 7));
        if (dropThis) {
            totalPacketsDropped++;
            delete bsm;
            findHost()->bubble("Blackhole: Dropping BSM!");
            EV_DEBUG << "Vehicle " << vehicleId << " [Blackhole] dropped its own beacon." << endl;
        } else {
            sendDown(bsm);
            totalPacketsSent++;
        }

        // DoS flooding: schedule much faster beacon rate (10-20 Hz instead of 1 Hz)
        simtime_t nextInterval = (isMalicious && attackType == 4) ? simtime_t(0.08) : beaconInterval;
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

    int effectiveId = vehicleId;
    int retx = (isMalicious && attackType == 2) ? (4 + (rand() % 6)) : (rand() % 3);

    // ── Simulate Random Application Messages & Attacks ────────
    std::string bsmPayload;
    if (isMalicious) {
        if (attackType == 1) {
            // False Data Injection: Falsify position coordinates and speed
            currentPos.x += ((vehicleId % 2 == 0) ? 140.0 : -140.0);
            currentPos.y += ((vehicleId % 3 == 0) ? 90.0 : -90.0);
            currentSpd = Coord(0, 0, 0); // Fake stopped vehicle / false accident
            bsmPayload = "FDI: Crash on Intersection! Stop!";
            findHost()->bubble(bsmPayload.c_str());
        } else if (attackType == 3) {
            // Sybil Attack: Use alternate virtual IDs from same physical location
            effectiveId = (vehicleId * 100) + (sequenceNumber % 4);
            bsmPayload = "Sybil: Ghost Vehicle Broadcasting";
            findHost()->bubble(bsmPayload.c_str());
        } else if (attackType == 4) {
            // DoS Flooding
            bsmPayload = "DoS: High-Rate Flood";
            findHost()->bubble("DoS Flood!");
        } else if (attackType == 2) {
            bsmPayload = "Blackhole Dropping";
        }
    } else {
        // Legitimate vehicle: random real-world VANET application messages
        if (accel < -2.5) {
            bsmPayload = "V2V: Hard Braking Warning!";
            findHost()->bubble("V2V: Braking Alert!");
        } else if (spdMag < 2.0) {
            bsmPayload = "V2V: Queued at Traffic Light";
            if (sequenceNumber % 4 == 0) findHost()->bubble("V2V: Intersection Wait");
        } else {
            bsmPayload = "V2V: Normal Flow (Speed " + std::to_string((int)spdMag) + " m/s)";
            if (sequenceNumber % 5 == 0) findHost()->bubble("V2V: Clear Transit");
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
    bsm->setIsMalicious(isMalicious);
    bsm->setAttackType(attackType);
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
    std::string msgName = bsm->getName();

    // Check if message is an RSU Safety Advisory
    if (msgName.rfind("RSU-ADVISORY:", 0) == 0) {
        if (msgName.find("Blocked") != std::string::npos) {
            findHost()->bubble("OBU: Received RSU Advisory - Hazard Blocked!");
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

void EdgeTrustVehicleApp::finish()
{
    DemoBaseApplLayer::finish();
    recordScalar("edgeTrust_totalPacketsSent", totalPacketsSent);
    recordScalar("edgeTrust_totalPacketsDropped", totalPacketsDropped);
}

} // namespace veins
