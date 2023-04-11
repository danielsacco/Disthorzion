#pragma once


class DCBlocker final
{
public:
  DCBlocker(double frequency, double samplerate, int channel);

  // Copy constructor and assignment
  //DCBlocker(const DCBlocker&);
  //DCBlocker& operator=(const DCBlocker&);

  //DCBlocker& operator==(const DCBlocker&);

  double ProcessSample(double input);

  void UpdateSamplerate(double samplerate);

  void UpdateFrequency(double frequency);

private:
  double frequency;
  double samplerate;
  double lastInput = .0;
  double lastOutput = .0;
  double R;

  int channel;  // Just for testing purposes !!!  TODO: Delete this member

  inline void SetR();

};
