#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include <DCBlocker.h>

const int kNumPresets = 1;

enum EParams
{
  kGain = 0,      // Overall gain
  kQ,             // Work point, should be less than zero
  kDist,          // Distortion's character > 0
  kDrive,         // Input drive > 0
  kCascade,       // Whether we should simulate a single tube or two in cascade
  kNumParams
};

using namespace iplug;
using namespace igraphics;

class Disthorzion final : public Plugin
{
public:
  Disthorzion(const InstanceInfo& info);

#if IPLUG_DSP // http://bit.ly/2S64BDd
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  sample AsymetricalClipping(const double& x, const double& Q, const double& dist);

 private:
   std::vector<DCBlocker> blockers;
   std::vector<DCBlocker> cascadeDcBlockers;
#endif
};
