#ifndef __EDGETRUSTRSUAPP_H_
#define __EDGETRUSTRSUAPP_H_

#include "veins/modules/application/ieee80211p/DemoBaseApplLayer.h"
#include "veins/modules/messages/DemoSafetyMessage_m.h"

class EdgeTrustRSUApp : public veins::DemoBaseApplLayer {
  protected:
    virtual void initialize(int stage) override;
    virtual void onBSM(veins::DemoSafetyMessage *bsm) override;
    int rsuId = 0;
};

#endif // __EDGETRUSTRSUAPP_H_
