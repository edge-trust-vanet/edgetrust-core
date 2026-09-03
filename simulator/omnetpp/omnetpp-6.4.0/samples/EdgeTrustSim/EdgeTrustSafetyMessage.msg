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
