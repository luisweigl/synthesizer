#pragma once
#include "envelope.hpp"
#include "oscillator.hpp"
#include <array>

#define OSCILLATORS 2

enum VoiceState { Attack, Decay, Sustain, Release, Disabled };

class Voice {
  private:
    VoiceState state;
    int samples; // number in samples in current phase
    float amplitude;
    float targetAmp;
    int note;

    float getPhase() const;
    float getSample(std::array<Oscillator, OSCILLATORS> oscillators, Envelope &envelope);
};