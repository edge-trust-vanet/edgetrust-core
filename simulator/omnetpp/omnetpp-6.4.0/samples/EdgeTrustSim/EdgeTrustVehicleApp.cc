// EdgeTrustVehicleApp.cc
#include "EdgeTrustVehicleApp.h"

Define_Module(EdgeTrustVehicleApp);

void EdgeTrustVehicleApp::initialize(int stage) {
    veins::DemoBaseApplLayer::initialize(stage);
    if (stage == 0) {
        vehicleId = par("vehicleId").intValue();
        // Set 25% of vehicles as malicious (e.g., node 0, 4, 8, etc.)
        if (vehicleId % 4 == 0) {
            isMalicious = true;
            EV << "Vehicle " << vehicleId << " initialized as a MALICIOUS node." << endl;
        } else {
            EV << "Vehicle " << vehicleId << " initialized as a LEGITIMATE node." << endl;
        }
    }
}

void EdgeTrustVehicleApp::sendDown(cMessage *msg) {
    veins::DemoSafetyMessage *bsm = dynamic_cast<veins::DemoSafetyMessage*>(msg);
    if (bsm) {
        // Embed the vehicle ID in the message name to pass it to the RSU without altering Veins .msg files
        std::string newName = "BSM-" + std::to_string(vehicleId);
        bsm->setName(newName.c_str());

        if (isMalicious) {
            // False Data Injection (FDI): Report falsified coordinates and speed (e.g. stopped)
            veins::Coord fakePos = bsm->getSenderPos();
            fakePos.x += 150; // Falsify position by 150m
            bsm->setSenderPos(fakePos);
            
            veins::Coord fakeSpeed(0, 0, 0); // Pretend to be completely stopped
            bsm->setSenderSpeed(fakeSpeed);
            
            std::string fakePayload = "CRITICAL: Major accident identified near traffic lights! Road blocked!";
            EV << "Malicious Vehicle " << vehicleId << " falsifying BSM data! Payload: [" << fakePayload << "]" << endl;
            
            // Pop up a visual speech bubble over the vehicle in the OMNeT++ GUI!
            findHost()->bubble("Broadcasting Fake Accident!");
        } else {
            std::string routinePayload = "Routine Telemetry. Clear roads.";
            EV << "Vehicle " << vehicleId << " broadcasting valid BSM. Payload: [" << routinePayload << "]" << endl;
        }
    }
    veins::DemoBaseApplLayer::sendDown(msg);
}

bool EdgeTrustVehicleApp::lightweightTrustFilter(veins::DemoSafetyMessage *bsm) {
    // Parse sender ID from the message name (e.g., "BSM-4")
    std::string msgName = bsm->getName();
    int senderId = -1;
    if (msgName.find("BSM-") == 0) {
        senderId = std::stoi(msgName.substr(4));
    }

    // If sender is already known to be malicious, drop immediately
    if (senderId != -1 && suspiciousNodes.count(senderId) > 0 && suspiciousNodes[senderId]) {
        return false;
    }

    // Lightweight Kinematics Check: Is the speed physically impossible?
    // Anything above 50 m/s (~111 mph) in an urban grid is likely a False Data Injection
    veins::Coord senderSpeed = bsm->getSenderSpeed();
    double speedMag = senderSpeed.length();

    if (speedMag > 50.0) {
        if (senderId != -1) {
            suspiciousNodes[senderId] = true; // Add to local blacklist
        }
        return false; // Fails trust check
    }

    return true; // Passes trust check
}

void EdgeTrustVehicleApp::onBSM(veins::DemoSafetyMessage *bsm) {
    // 1. OBU-Level Trust Validation (Lightweight Filter)
    bool isTrusted = lightweightTrustFilter(bsm);

    if (!isTrusted) {
        EV << "OBU-Level Trust Filter: Malicious data detected from " << bsm->getName() 
           << "! Dropping message to minimize communication overhead." << endl;
           
        // Visual indicator in GUI that the car recognized a fake message!
        findHost()->bubble("Dropped Fake Accident Alert!");
        return; // Prevent further processing or forwarding
    }

    EV << "OBU-Level Trust Filter: Message from " << bsm->getName() 
       << " verified locally. Authorized for processing/forwarding." << endl;
       
    // Visual indicator that routine traffic is flowing normally
    findHost()->bubble("Verified Routine Data.");
}
