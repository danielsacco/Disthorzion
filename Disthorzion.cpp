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
  GetParam(kDCBlockFreq)->InitDouble("DC Block Freq.", 10., 2., 30., .1, "Hz");


#if IPLUG_EDITOR // http://bit.ly/2S64BDd
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    // External resosurces
    const IBitmap switchBitmap = pGraphics->LoadBitmap((PNGSWITCH_FN), 2, true);
    const IBitmap sliderHandleBitmap = pGraphics->LoadBitmap(PNGSLIDERHANDLE_FN);
    const IBitmap sliderTrackBitmap = pGraphics->LoadBitmap(PNGSLIDERTRACK_FN);
    const IBitmap backgroundBitmap = pGraphics->LoadBitmap(PNGGUIBACKGRND_FN);

    pGraphics->AttachBackground(PNGGUIBACKGRND_FN);
    //pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    const IRECT fullGUI = pGraphics->GetBounds();

    const IRECT headerPanel = fullGUI.FracRectVertical(.15, true);
    const IRECT footerPanel = fullGUI.FracRectVertical(.15, false);
    // Como armo el centro ???

    const IRECT controlsPanel = IRECT(fullGUI.L, headerPanel.B, fullGUI.R, footerPanel.T);

    auto rows = 5;

    {
      // Input Drive Control
      const IRECT driveColumn = controlsPanel.GetGridCell(0, 0, 1, rows);
      const IRECT driveSliderColumn = driveColumn.SubRectHorizontal(2, 0);
      const IRECT driveLabelsColumn = driveColumn.SubRectHorizontal(2, 1)
        .GetMidVPadded(sliderTrackBitmap.FH()/2 - sliderHandleBitmap.FH()/4);

      const IRECT driveSliderCell = driveSliderColumn;

      // Drive Slider
      pGraphics->AttachControl(new IBSliderControl(driveColumn, sliderHandleBitmap, sliderTrackBitmap, kDrive));
    }

    pGraphics->AttachControl(new IBSliderControl(controlsPanel.GetGridCell(0, 1, 1, rows), sliderHandleBitmap, sliderTrackBitmap, kQ));
    pGraphics->AttachControl(new IBSliderControl(controlsPanel.GetGridCell(0, 2, 1, rows), sliderHandleBitmap, sliderTrackBitmap, kDist));
    pGraphics->AttachControl(new IBSwitchControl(controlsPanel.GetGridCell(0, 3, 1, rows), switchBitmap, kCascade));
    pGraphics->AttachControl(new IBSliderControl(controlsPanel.GetGridCell(0, 4, 1, rows), sliderHandleBitmap, sliderTrackBitmap, kGain));

    //pGraphics->AttachControl(new IVKnobControl(b.GetGridCell(0, 5, 1, rows), kDCBlockFreq));
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
 
  // DC Blockers should be created here the first time.
  if (blockers.size() < nChans)
  {
    // Discard previous blockers (may be the case if we are switching from mono to stereo)
    blockers.clear();
    cascadeDcBlockers.clear();

    const double dcBlockerFreq = GetParam(kDCBlockFreq)->Value();
    const double sampleRate = GetSampleRate();
    for (int i = 0; i < nChans; i++)
    {
      blockers.push_back(DCBlocker(dcBlockerFreq, sampleRate));
      cascadeDcBlockers.push_back(DCBlocker(dcBlockerFreq, sampleRate));
    }
  }
  
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

      channelOut[s] = y * outputGain;
    }
  }
}

void Disthorzion::OnParamChange(int paramIdx)
{
  switch (paramIdx)
  {
    case kDCBlockFreq:
    {
      double freq = GetParam(kDCBlockFreq)->Value();
      for (auto blocker : blockers)
      {
        blocker.UpdateFrequency(freq);
      }
      for (auto cascadeBlocker : cascadeDcBlockers)
      {
        cascadeBlocker.UpdateFrequency(freq);
      }
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
