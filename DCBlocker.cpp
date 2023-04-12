#include "DCBlocker.h"

const double DOUBLE_PI = 6.283185307179586476925286766559;

// Solution reference: https://www.musicdsp.org/en/latest/Filters/135-dc-filter.html
DCBlocker::DCBlocker(double freq, double sRate)
  : frequency{freq}
  , samplerate{sRate}
{
  SetR();
}

double DCBlocker::ProcessSample(double input)
{
  double output = input - lastInput + R * lastOutput;

  lastInput = input;
  lastOutput = output;

  return output;
}

void DCBlocker::UpdateSamplerate(double sRate) {
  samplerate = sRate;
  SetR();
}

void DCBlocker::UpdateFrequency(double freq) {
  frequency = freq;
  SetR();
}

inline void DCBlocker::SetR()
{
  R = 1 - (DOUBLE_PI * frequency / samplerate);
}
