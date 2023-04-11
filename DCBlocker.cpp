#include "DCBlocker.h"

const double DOUBLE_PI = 6.283185307179586476925286766559;

// Solution reference: https://www.musicdsp.org/en/latest/Filters/135-dc-filter.html
DCBlocker::DCBlocker(double freq, double sRate, int ch)
  : frequency{freq}
  , samplerate{sRate}
  , channel{ch}
{
  SetR();
}

//DCBlocker::DCBlocker(const DCBlocker& source)
//  : frequency{source.frequency}
//  , samplerate{source.samplerate}
//  , R{source.R}
//  , lastInput{source.lastInput}
//  , lastOutput{source.lastOutput}
//  , channel{source.channel}
//{
//}

//DCBlocker& DCBlocker::operator=(const DCBlocker& source)
//{
//  if (this == &source)
//    return *this;
//
//  frequency = source.frequency;
//  samplerate = source.samplerate;
//  R = source.R;
//  lastInput = source.lastInput;
//  lastOutput = source.lastOutput;
//
//  return *this;
//}
//
//DCBlocker& DCBlocker::operator==(const DCBlocker& other)
//{
//  return *this == other;
//}

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
