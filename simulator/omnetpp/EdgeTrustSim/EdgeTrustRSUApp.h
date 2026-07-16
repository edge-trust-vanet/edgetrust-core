// EdgeTrustRSUApp.h
#ifndef __EDGETRUSTRSUAPP_H_
#define __EDGETRUSTRSUAPP_H_

#include "veins/base/modules/BaseWaveApplLayer.h"

/**
 * Minimal RSU (RoadSide Unit) Application for EdgeTrust simulation.
 * Receives beacons from vehicles and logs them – later can forward to ML.
 */
class EdgeTrustRSUApp : public veins::BaseWaveApplLayer {
  protected:
    virtual void initialize(int stage) override;
    virtual void onWSM(veins::BaseFrame1609_4 *wsm) override;
    int rsuId = 0;
};

#endif // __EDGETRUSTRSUAPP_H_
