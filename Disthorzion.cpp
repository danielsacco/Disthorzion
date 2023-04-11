#include "Disthorzion.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"

Disthorzion::Disthorzion(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kGain)->InitDouble("Gain", 0., 0., 100.0, 0.01, "%");
  GetParam(kQ)->InitDouble("Q (Work Point)", -0.2, -.9, -0.01, .01, "");
  GetParam(kDist)->InitDouble("Distortion Shape", 10, .1, 20, .1, "");
  GetParam(kDrive)->InitDouble("Input Drive", 2, .8, 10, .1, "");
  GetParam(kCascade)->InitBool("Cascade", false);

 #if IPLUG_DSP

  int nChans = NOutChansConnected();
  double sampleRate = GetSampleRate();

  // TODO Make this frequency configurable
  double dcBlockerFreq = 20.;

  for (int i = 0; i < nChans; i++)
  {
    blockers.push_back(DCBlocker(dcBlockerFreq, sampleRate, i));
    cascadeDcBlockers.push_back(DCBlocker(dcBlockerFreq, sampleRate, i));
  }

#endif

#if IPLUG_EDITOR // http://bit.ly/2S64BDd
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    const IRECT b = pGraphics->GetBounds();
    //pGraphics->AttachControl(new ITextControl(b.GetMidVPadded(50), "Disthorzion 2", IText(50)));
    //pGraphics->AttachControl(new IVKnobControl(b.GetCentredInside(100).GetVShifted(-100), kGain));

    const IBitmap switchBitmap = pGraphics->LoadBitmap((PNGSWITCH_FN), 2, true);
    
    auto rows = 5;

    pGraphics->AttachControl(new IVKnobControl(b.GetGridCell(0, 0, 1, rows), kGain));
    pGraphics->AttachControl(new IVKnobControl(b.GetGridCell(0, 1, 1, rows), kQ));
    pGraphics->AttachControl(new IVKnobControl(b.GetGridCell(0, 2, 1, rows), kDist));
    pGraphics->AttachControl(new IVKnobControl(b.GetGridCell(0, 3, 1, rows), kDrive));
    // TODO Add label
    pGraphics->AttachControl(new IBSwitchControl(b.GetGridCell(0, 4, 1, rows), switchBitmap, kCascade));

  };
#endif
}

#if IPLUG_DSP
void Disthorzion::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double outputGain = GetParam(kGain)->Value() / 100.;
  const int nChans = NOutChansConnected();

  const double Q = GetParam(kQ)->Value();
  const double dist = GetParam(kDist)->Value();
  const double inputDrive = GetParam(kDrive)->Value();
  const bool cascade = GetParam(kCascade)->Bool();


  // TODO Blockers should be managed here:
  // The first time they should be created
  // On each ProcessBlock we should check if freq or sample rate have changed and update the DC Blockers


  
  // For each channel process the samples
  for (int ch = 0; ch < nChans; ch++)
  {
    sample* channelIn  = inputs[ch];
    sample* channelOut = outputs[ch];

    auto dcBlocker = &(blockers[ch]);
    auto cascadeDcBlocker = &(cascadeDcBlockers[ch]);

    for (int s = 0; s < nFrames; s++)
    {
      // Process a sample
      sample x = channelIn[s] * inputDrive * 2;

      // Process via first tube
      sample y = AsymetricalClipping(x, Q, dist);

      // DC block
      y = dcBlocker->ProcessSample(y);

      if (cascade)
      {
        // Process result via second tube.
        // Switch phase before and after processing so it clips the
        // "almost linear" part of the signal
        y = -AsymetricalClipping(-y * 2, Q, dist);

        // DC Block
        y = cascadeDcBlocker->ProcessSample(y);
      }

      // TODO LP Filter

      channelOut[s] = y * outputGain;
    }
  }
}

sample Disthorzion::AsymetricalClipping(const double& x, const double& Q, const double& dist)
{
  // Udo Zolzer DAFX 2nd Ed. Page 122
  if (x == Q)
  {
    return 1. / dist + Q / (1. - std::exp(dist * Q));
  }
  else
  {
    return (x - Q) / (1. - std::exp(-dist * (x - Q))) + Q / (1. - std::exp(dist * Q));
  }
}
#endif
