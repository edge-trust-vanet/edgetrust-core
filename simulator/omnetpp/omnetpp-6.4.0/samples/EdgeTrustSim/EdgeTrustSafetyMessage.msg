// EdgeTrustSafetyMessage.msg
import veins.base.utils.Coord;
import veins.modules.messages.DemoSafetyMessage;

namespace veins;

packet EdgeTrustSafetyMessage extends DemoSafetyMessage
{
    int senderId = 0;
    double heading = 0.0;
    double acceleration = 0.0;
    int sequenceNumber = 0;
    int retransmissionCount = 0;
    bool isMalicious = false;
    int attackType = 0; // 0=None, 1=FDI, 2=Blackhole, 3=Sybil, 4=DoS
}

packet EdgeTrustSDNPacket extends cPacket
{
    int sourceRsuId = 0;
    int destRsuId = 0;
    int currentHop = 0;
    int trafficClass = 0; // 0=Safety Critical (High), 1=Trust/Control (Med), 2=Best Effort (Low)
    int packetId = 0;
    simtime_t generationTime = 0;
    bool isDetour = false;
    string payloadType = "SAFETY_ALERT";
    string routePath = "";
    int originVehicleId = 0;
    double safetyUrgency = 1.0;
}
