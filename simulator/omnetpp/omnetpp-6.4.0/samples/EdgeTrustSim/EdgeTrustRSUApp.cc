// EdgeTrustRSUApp.cc
#include "EdgeTrustRSUApp.h"
#include <cmath>

Define_Module(EdgeTrustRSUApp);

void EdgeTrustRSUApp::initialize(int stage) {
    veins::DemoBaseApplLayer::initialize(stage);
    if (stage == 0) {
        rsuId = par("rsuId").intValue();
        // RSU 0 initializes the header for the shared file
        if (rsuId == 0) {
            trustLogFile.open("results/trust_log.csv", std::ios::out);
            trustLogFile << "Time,RSU_ID,Vehicle_ID,ReportedSpeed,CalculatedSpeed,Plausibility,Consistency,TrustScore,Verdict\n";
        } else {
            // Give RSU 0 a moment to create it, then append
            trustLogFile.open("results/trust_log.csv", std::ios::app);
        }
    }
}

void EdgeTrustRSUApp::onBSM(veins::DemoSafetyMessage *bsm) {
    // Extract senderId from the message name set by EdgeTrustVehicleApp
    std::string msgName = bsm->getName();
    long senderId = -1;
    if (msgName.find("BSM-") == 0) {
        senderId = std::stol(msgName.substr(4));
    } else {
        senderId = bsm->getSenderModuleId(); // Fallback
    }
    
    veins::Coord reportedPos = bsm->getSenderPos();
    veins::Coord reportedSpeed = bsm->getSenderSpeed();
    double reportedSpeedMag = reportedSpeed.length();
    
    simtime_t currentTime = simTime();
    
    double plausibility = 1.0;
    double consistency = 1.0;
    double calculatedSpeedMag = reportedSpeedMag;
    
    if (vehicleRecords.find(senderId) != vehicleRecords.end()) {
        VehicleHistory& hist = vehicleRecords[senderId];
        
        // Physical Plausibility: Calculate speed based on distance / time
        double distance = reportedPos.distance(hist.lastPos);
        double timeDiff = (currentTime - hist.lastTime).dbl();
        
        if (timeDiff > 0) {
            calculatedSpeedMag = distance / timeDiff;
            
            // If reported speed differs from calculated speed by more than 5 m/s, lower plausibility
            double speedDiff = std::abs(reportedSpeedMag - calculatedSpeedMag);
            if (speedDiff > 5.0) {
                plausibility = std::max(0.0, 1.0 - (speedDiff / 30.0)); // Linear penalty
            }
        }
        
        // Message Consistency (simplified): compare reported speed against typical bounds
        // If reported speed is 0 but calculated speed is high, or if calculated speed > 40m/s
        if (calculatedSpeedMag > 40.0) { // Unrealistic speed > 144 km/h
            consistency = 0.0;
        }
        
        // Calculate new trust score
        double currentTrust = 0.6 * plausibility + 0.4 * consistency;
        hist.trustScore = 0.7 * hist.trustScore + 0.3 * currentTrust; // Exponential moving average
        
        std::string verdict = "ACCEPT";
        if (hist.trustScore < 0.4) verdict = "BLOCK";
        else if (hist.trustScore < 0.7) verdict = "WARN";
        
        // Log to file
        if (trustLogFile.is_open()) {
            trustLogFile << currentTime.dbl() << "," << rsuId << "," << senderId << ","
                         << reportedSpeedMag << "," << calculatedSpeedMag << "," 
                         << plausibility << "," << consistency << "," << hist.trustScore << "," 
                         << verdict << "\n";
            trustLogFile.flush();
        }
        
        if (verdict == "BLOCK") {
            EV << "RSU " << rsuId << " detected MALICIOUS behavior from node " << senderId 
               << " (Trust: " << hist.trustScore << "). Action: BLOCK" << endl;
        }
        
        // Update history
        hist.lastPos = reportedPos;
        hist.lastTime = currentTime;
    } else {
        // First time seeing this vehicle
        VehicleHistory newHist;
        newHist.lastPos = reportedPos;
        newHist.lastTime = currentTime;
        newHist.trustScore = 1.0;
        vehicleRecords[senderId] = newHist;
    }
}

void EdgeTrustRSUApp::finish() {
    veins::DemoBaseApplLayer::finish();
    if (trustLogFile.is_open()) {
        trustLogFile.close();
    }
}
