#include "Disthorzion.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"

Disthorzion::Disthorzion(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kGain)->InitGain("Gain", 0., -12., 6., .5);
  GetParam(kQ)->InitDouble("Q (Work Point)", -0.2, -.9, -0.01, .01, "");
  GetParam(kDist)->InitDouble("Distortion Shape", 10, .1, 20, .1, "");
  GetParam(kDrive)->InitGain("Input Drive", .0, -3, 12, .1);
  GetParam(kCascade)->InitBool("Cascade", false);
  GetParam(kDCBlockFreq)->InitDouble("DC Block Freq.", 10., 2., 30., .1, "Hz");

  // Factory Presets
  // Params in order: kGain, kQ, kDist, kDrive, kCascade, kDCBlockFreq
  MakePreset("Warm Bass", .0, -.75, 2.3, .0, false, 10.);
  MakePreset("Bass Overdrive", -10., -.55, 9.3, 6.0, true, 10.);
  MakePreset("Bass Fuzz", -7., -.01, 20., 12.0, true, 10.);

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
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    const IRECT fullGUI = pGraphics->GetBounds();

    const IRECT headerPanel = fullGUI.FracRectVertical(.15f, true);
    const IRECT footerPanel = fullGUI.FracRectVertical(.15f, false);

    const IRECT controlsPanel = IRECT(fullGUI.L, headerPanel.B, fullGUI.R, footerPanel.T);

    auto rows = 5;


    // Input Drive Control
    const IRECT driveColumn = controlsPanel.GetGridCell(0, 0, 1, rows);

    // Drive Slider
    pGraphics->AttachControl(new IBSliderControl(driveColumn, sliderHandleBitmap, sliderTrackBitmap, kDrive));
    pGraphics->AttachControl(new ICaptionControl(driveColumn
      .GetFromBottom(50.f)
      .GetMidVPadded(10.f)
      .GetMidHPadded(12.f)
      , kDrive, IText(13.f), COLOR_TRANSPARENT, false), kNoTag);

    pGraphics->AttachControl(new IBSliderControl(controlsPanel.GetGridCell(0, 1, 1, rows), sliderHandleBitmap, sliderTrackBitmap, kQ));
    pGraphics->AttachControl(new IBSliderControl(controlsPanel.GetGridCell(0, 2, 1, rows), sliderHandleBitmap, sliderTrackBitmap, kDist));
    pGraphics->AttachControl(new IBSwitchControl(controlsPanel.GetGridCell(0, 3, 1, rows), switchBitmap, kCascade));

    pGraphics->AttachControl(new IBSliderControl(controlsPanel.GetGridCell(0, 4, 1, rows), sliderHandleBitmap, sliderTrackBitmap, kGain));
    pGraphics->AttachControl(new ICaptionControl(controlsPanel.GetGridCell(0, 4, 1, rows)
      .GetFromBottom(50.f)
      .GetMidVPadded(10.f)
      .GetMidHPadded(12.f)
      , kGain, IText(14.f), COLOR_TRANSPARENT, false), kNoTag);


    //pGraphics->AttachControl(new IVKnobControl(b.GetGridCell(0, 5, 1, rows), kDCBlockFreq));
  };
#endif
}

#if IPLUG_DSP
void Disthorzion::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  //const double outputGain = GetParam(kGain)->Value() / 100.;
  const int nChans = NOutChansConnected();

  const double Q = GetParam(kQ)->Value();
  const double dist = GetParam(kDist)->Value();
  //const double inputDrive = GetParam(kDrive)->Value();
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
      blockers.push_back(dsptk::DCBlocker(dcBlockerFreq, sampleRate));
      cascadeDcBlockers.push_back(dsptk::DCBlocker(dcBlockerFreq, sampleRate));
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
      sample x = channelIn[s] * linearDrive * 2;

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

      channelOut[s] = y * linearGain;
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
      break;
    }
    case kDrive:
    {
      linearDrive = DBToAmp(GetParam(kDrive)->Value());
      break;
    }
    case kGain:
    {
      linearGain = DBToAmp(GetParam(kGain)->Value());
      break;
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
