#pragma once


class DCBlocker final
{
public:
  DCBlocker(double frequency, double samplerate);

  double ProcessSample(double input);

  void UpdateSamplerate(double samplerate);

  void UpdateFrequency(double frequency);

private:
  double frequency;
  double samplerate;
  double lastInput = .0;
  double lastOutput = .0;
  double R;

  inline void SetR();
};
