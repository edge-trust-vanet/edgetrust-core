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
        maxCommunicationRange = hasPar("maxCommunicationRange") ? par("maxCommunicationRange").doubleValue() : 85.0;

        // Guaranteed early attack demonstration:
        // Node 1: Legitimate baseline
        // Node 2: Attacking after 2 normal messages (FDI False Data Injection)
        // Node 3: Attacking after 2 normal messages (Blackhole Packet Dropping)
        if (vehicleId == 1) {
            isMalicious = false;
            attackType = 0;
            attackStartMessage = 9999;
        } else if (vehicleId == 2) {
            isMalicious = true;
            attackType = 1; // FDI (False Data Injection)
            attackStartMessage = 3; // Trigger attack right after 2 normal message exchanges!
        } else if (vehicleId == 3) {
            isMalicious = true;
            attackType = 2; // Blackhole Attack
            attackStartMessage = 3;
        } else {
            unsigned int hashVal = ((unsigned int)vehicleId * 2654435761u) % 100;
            if (maliciousRatio > 0.0 && hashVal < (unsigned int)(maliciousRatio * 100)) {
                isMalicious = true;
                attackType = (vehicleId % 4) + 1;
                attackStartMessage = 3;
            } else {
                isMalicious = false;
                attackType = 0;
                attackStartMessage = 9999;
            }
        }

        if (hasPar("isMalicious") && par("isMalicious").boolValue()) {
            isMalicious = true;
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
        bool dropThis = (attackActive && attackType == 2 && ((rand() % 10) < 7));
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
            sendDown(bsm);
            totalPacketsSent++;
        }

        // DoS flooding: schedule much faster beacon rate (10-20 Hz instead of 1 Hz)
        simtime_t nextInterval = (attackActive && attackType == 4) ? simtime_t(0.08) : beaconInterval;
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
    bool attackActive = (isMalicious && sequenceNumber >= attackStartMessage);

    // ── Simulate Random Application Messages & Early Attacks ──
    std::string bsmPayload;
    if (attackActive) {
        if (attackType == 1) {
            // False Data Injection: Falsify position coordinates by 25m (different lane/branch) and speed to 0
            currentPos.x += ((vehicleId % 2 == 0) ? 28.0 : -28.0);
            currentPos.y += ((vehicleId % 3 == 0) ? 22.0 : -22.0);
            currentSpd = Coord(0, 0, 0); // Fake stopped vehicle / false accident
            bsmPayload = "ATTACK: FDI Fake Crash (+28m)";
            findHost()->bubble(bsmPayload.c_str());
            char tagStr[96];
            snprintf(tagStr, sizeof(tagStr), "V%d: FDI ATTACK (Spoofed +28m)", vehicleId);
            findHost()->getDisplayString().setTagArg("t", 0, tagStr);
            findHost()->getDisplayString().setTagArg("t", 1, "t");
            findHost()->getDisplayString().setTagArg("t", 2, "red");
            findHost()->getDisplayString().setTagArg("i", 1, "red");
        } else if (attackType == 3) {
            // Sybil Attack: Use alternate virtual IDs from same physical location
            effectiveId = (vehicleId * 100) + (sequenceNumber % 4);
            bsmPayload = "ATTACK: Sybil Ghost ID " + std::to_string(effectiveId);
            findHost()->bubble(bsmPayload.c_str());
            findHost()->getDisplayString().setTagArg("t", 0, bsmPayload.c_str());
            findHost()->getDisplayString().setTagArg("t", 1, "t");
            findHost()->getDisplayString().setTagArg("t", 2, "red");
            findHost()->getDisplayString().setTagArg("i", 1, "red");
        } else if (attackType == 4) {
            // DoS Flooding
            bsmPayload = "ATTACK: DoS Rapid Flood";
            findHost()->bubble(bsmPayload.c_str());
            findHost()->getDisplayString().setTagArg("t", 0, "ATTACK: DoS Flooding");
            findHost()->getDisplayString().setTagArg("t", 1, "t");
            findHost()->getDisplayString().setTagArg("t", 2, "red");
            findHost()->getDisplayString().setTagArg("i", 1, "red");
        } else if (attackType == 2) {
            findHost()->getDisplayString().setTagArg("t", 0, "V3: BLACKHOLE ATTACK (Dropping)");
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
    cModule* parent = findHost()->getParentModule();
    if (!parent) return;
    cCanvas* canvas = parent->getCanvas();
    if (!canvas) return;

    cLineFigure* arrow = dynamic_cast<cLineFigure*>(canvas->getFigure(arrowId.c_str()));
    if (!arrow) {
        arrow = new cLineFigure(arrowId.c_str());
        arrow->setEndArrowhead(cFigure::ARROW_SIMPLE);
        arrow->setZoomLineWidth(true);
        canvas->addFigure(arrow);
    }
    arrow->setStart(cFigure::Point(from.x, from.y));
    arrow->setEnd(cFigure::Point(to.x, to.y));
    arrow->setLineWidth(2.5);
    arrow->setLineColor(cFigure::Color(color.c_str()));
    arrow->setVisible(true);
}

void EdgeTrustVehicleApp::finish()
{
    DemoBaseApplLayer::finish();
    recordScalar("edgeTrust_totalPacketsSent", totalPacketsSent);
    recordScalar("edgeTrust_totalPacketsDropped", totalPacketsDropped);
}

} // namespace veins
