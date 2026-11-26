#pragma once
#include "oscillator.hpp"

enum LFOTarget { Pitch, Volume, Cutoff, PWM };

class LFO {
  private:
    LFOTarget target;
    Oscillator oscillator;
    float sampleRate;
    float depth;
    float phase;
    float value;

  public:
    LFO(float sampleRate);
    void setRate(float rate);
    void setDepth(float depth);
    void setWave(Waveform waveform);
    float getValue();
    void process();
};